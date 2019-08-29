// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayerTimeSynchronizationSource.h"
#include "TimecodeSynchronizerModule.h"

#include "IMediaCache.h"
#include "IMediaPlayer.h"
#include "MediaPlayer.h"
#include "MediaPlayerFacade.h"
#include "MediaTexture.h"
#include "TimeSynchronizableMediaSource.h"


#if WITH_EDITOR
void UMediaPlayerTimeSynchronizationSource::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMediaPlayerTimeSynchronizationSource, MediaSource))
	{
		if (bUseForSynchronization && MediaSource && !MediaSource->bUseTimeSynchronization)
		{
			// Warn the user that the MediaSource he just added isn't set for using time synchronization
			UE_LOG(LogTimecodeSynchronizer, Warning, TEXT("MediaSource %s doesn't have the option to use time synchronization enabled."), *MediaSource->GetName());
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

static FFrameTime TimeSpanToFrameTime(const FTimespan& Timespan, const FFrameRate& FrameRate)
{
	return FFrameTime::FromDecimal(Timespan.GetTotalSeconds() * FrameRate.AsDecimal()).RoundToFrame();
}

FFrameTime UMediaPlayerTimeSynchronizationSource::GetOldestSampleTime() const
{
	TOptional<FTimespan> UseTimespan;

	if (MediaTexture && MediaTexture->GetMediaPlayer())
	{
		if (MediaTexture->GetAvailableSampleCount() > 0)
		{
			// Ideally, the MediaTexture (or more likely, the TMediaSampleQueue) would be able to track
			// the current span of samples available. However, that's already prone to some threading issues
			// and trying to manage more data will only exacerbate that.

			// Therefore, we can only use the next available sample time.
			UseTimespan = MediaTexture->GetNextSampleTime();
		}

		const TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe>& Player = MediaTexture->GetMediaPlayer()->GetPlayerFacade()->GetPlayer();
		if (Player.IsValid())
		{
			IMediaCache& Cache = Player->GetCache();
			if (Cache.GetSampleCount(EMediaCacheState::Loaded) > 0)
			{
				TRangeSet<FTimespan> SampleTimes;
				if (Cache.QueryCacheState(EMediaCacheState::Loaded, SampleTimes))
				{
					const FTimespan MinBound = SampleTimes.GetMinBoundValue();
					UseTimespan = (UseTimespan.IsSet()) ? FMath::Min(MinBound, UseTimespan.GetValue()) : MinBound;
				}
			}
		}
	}

	return UseTimespan.IsSet() ? TimeSpanToFrameTime(UseTimespan.GetValue(), GetFrameRate()) : FFrameTime(0);
}

FFrameTime UMediaPlayerTimeSynchronizationSource::GetNewestSampleTime() const
{
	TOptional<FTimespan> UseTimespan;

	if (MediaTexture && MediaTexture->GetMediaPlayer())
	{
		if (MediaTexture->GetAvailableSampleCount() > 0)
		{
			// Ideally, the MediaTexture (or more likely, the TMediaSampleQueue) would be able to track
			// the current span of samples available. However, that's already prone to some threading issues
			// and trying to manage more data will only exacerbate that.

			// Therefore, we can only use the next available sample time.
			UseTimespan = MediaTexture->GetNextSampleTime();
		}

		const TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe>& Player = MediaTexture->GetMediaPlayer()->GetPlayerFacade()->GetPlayer();
		if (Player.IsValid())
		{
			IMediaCache& Cache = Player->GetCache();
			if (Cache.GetSampleCount(EMediaCacheState::Loaded) > 0)
			{
				TRangeSet<FTimespan> SampleTimes;
				if (Cache.QueryCacheState(EMediaCacheState::Loaded, SampleTimes))
				{
					const FTimespan MaxBound = SampleTimes.GetMaxBoundValue();
					UseTimespan = (UseTimespan.IsSet()) ? FMath::Max(MaxBound, UseTimespan.GetValue()) : MaxBound;
				}
			}
		}
	}

	return UseTimespan.IsSet() ? TimeSpanToFrameTime(UseTimespan.GetValue(), GetFrameRate()) : FFrameTime(0);
}

FFrameRate UMediaPlayerTimeSynchronizationSource::GetFrameRate() const
{
	if (IsReady())
	{
		UMediaPlayer* MediaPlayer = MediaTexture->GetMediaPlayer();
		check(MediaPlayer);
		const TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe>& Player = MediaPlayer->GetPlayerFacade()->GetPlayer();

		if (Player.IsValid())
		{
			//Save the FrameRate of the current track of the media player for future use
			const int32 SelectedTrack = MediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);
			const int32 SelectedFormat = MediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, SelectedTrack);
			const float FrameRate = MediaPlayer->GetVideoTrackFrameRate(SelectedTrack, SelectedFormat);

			//Convert using 1001 for DropFrame FrameRate detection
			const uint32 Precision = 1001;
			return FFrameRate(FMath::RoundToInt(FrameRate * Precision), Precision);
		}
	}
	return FFrameRate();
}

bool UMediaPlayerTimeSynchronizationSource::IsReady() const
{
	return MediaTexture && MediaTexture->GetMediaPlayer() && MediaTexture->GetMediaPlayer()->IsReady() && MediaSource;
}

bool UMediaPlayerTimeSynchronizationSource::Open(const FTimeSynchronizationOpenData& InOpenData)
{
	OpenData = InOpenData;

	bool bResult = false;
	if (MediaSource && MediaTexture)
	{
		UMediaPlayer* MediaPlayer = MediaTexture->GetMediaPlayer();
		if (MediaPlayer)
		{
			if (bUseForSynchronization && !MediaSource->bUseTimeSynchronization)
			{
				UE_LOG(LogTimecodeSynchronizer, Error, TEXT("MediaSource %s doesn't support timecode synchronization"), *MediaSource->GetName());
			}
			else
			{
				bResult = MediaPlayer->OpenSource(MediaSource);
				if (!bResult)
				{
					UE_LOG(LogTimecodeSynchronizer, Error, TEXT("Player could not be started."));
				}
				else
				{
					const auto& Player = MediaPlayer->GetPlayerFacade()->GetPlayer();
					if (!Player.IsValid())
					{
						UE_LOG(LogTimecodeSynchronizer, Error, TEXT("Current player is invalid."));
						MediaPlayer->Close();
					}
				}
			}
		}
	}

	return bResult;
}

void UMediaPlayerTimeSynchronizationSource::Start(const FTimeSynchronizationStartData& InStartData)
{
	StartData = InStartData;

	UMediaPlayer* MediaPlayer = MediaTexture ? MediaTexture->GetMediaPlayer() : nullptr;
	if (MediaPlayer)
	{
		const FFrameRate LocalFrameRate = GetFrameRate();
		const FFrameTime LocalStartFrame = FFrameRate::TransformTime(StartData->StartFrame, OpenData->SynchronizationFrameRate, LocalFrameRate);
		const FTimespan StartTimespan = FTimespan::FromSeconds(LocalFrameRate.AsSeconds(LocalStartFrame));

		// If this source is used for synchronization, then we'll try to seek to the start frame.
		if (bUseForSynchronization)
		{
			if (MediaPlayer->SupportsSeeking())
			{
				MediaPlayer->Seek(StartTimespan);
			}
		}

		// Otherwise, we'll at least try to set a delay so it sort of lines up.
		else
		{
			const FFrameTime MinimumTime = GetOldestSampleTime();

			// TODO: Verify this is the correct order. The comments on SetDelay seem confusing.
			// TODO: Maybe also do this for sync sources that don't support seeking? Need test cases.
			const FFrameTime DelayFrames = LocalStartFrame - MinimumTime;
			const double Delay = LocalFrameRate.AsSeconds(DelayFrames);

			if (Delay > 0)
			{
				MediaPlayer->SetTimeDelay(Delay);
			}
		}

		MediaPlayer->Play();
	}
}

void UMediaPlayerTimeSynchronizationSource::Close()
{
	if (MediaSource && MediaTexture)
	{
		UMediaPlayer* MediaPlayer = MediaTexture->GetMediaPlayer();
		if (MediaPlayer)
		{
			MediaPlayer->Close();
		}
	}

	StartData.Reset();
	OpenData.Reset();
}

FString UMediaPlayerTimeSynchronizationSource::GetDisplayName() const
{
	return MediaTexture && MediaTexture->GetMediaPlayer() ? *MediaTexture->GetMediaPlayer()->GetName() : FString();
}

