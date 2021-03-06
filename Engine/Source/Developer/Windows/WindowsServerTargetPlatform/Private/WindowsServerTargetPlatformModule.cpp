// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GenericWindowsTargetPlatform.h"
#include "Interfaces/ITargetPlatformModule.h"


/** Holds the target platform singleton. */
static ITargetPlatform* Singleton = nullptr;


/**
 * Module for the Windows target platform as a server.
 */
class FWindowsServerTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FWindowsServerTargetPlatformModule( )
	{
		Singleton = nullptr;
	}

	virtual ITargetPlatform* GetTargetPlatform( )
	{
		if (Singleton == nullptr && TGenericWindowsTargetPlatform<false, true, false>::IsUsable())
		{
			Singleton = new TGenericWindowsTargetPlatform<false, true, false>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FWindowsServerTargetPlatformModule, WindowsServerTargetPlatform);
