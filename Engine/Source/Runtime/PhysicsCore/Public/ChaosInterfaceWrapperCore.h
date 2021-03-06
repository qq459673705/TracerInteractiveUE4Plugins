// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ChaosSQTypes.h"
#include "PhysicsInterfaceWrapperShared.h"
#include "PhysicsInterfaceTypesCore.h"

#if WITH_PHYSX
#include "PhysXPublicCore.h"
#endif

class UPhysicalMaterial;

namespace Chaos
{
	template <class T, int d>
	class TImplicitObject;

	template <class T>
	class TCapsule;
}

namespace ChaosInterface
{
struct FDummyPhysType {};
struct FDummyPhysActor {};

template<typename DummyT>
struct FDummyCallback {};

#if WITH_PHYSX
using FQueryFilterData = PxQueryFilterData;
#else
using FQueryFilterData = FDummyPhysType;
#endif

/** We use this struct so that if no conversion is needed in another API, we can avoid the copy (if we think that's critical) */
struct FPhysicsRaycastInputAdapater
{
	FPhysicsRaycastInputAdapater(const FVector& InStart, const FVector& InDir, const EHitFlags InFlags)
		: Start(InStart)
		, Dir(InDir)
		, OutputFlags(InFlags)
	{

	}
	FVector Start;
	FVector Dir;
	EHitFlags OutputFlags;
};

/** We use this struct so that if no conversion is needed in another API, we can avoid the copy (if we think that's critical) */
struct FPhysicsSweepInputAdapater
{
	FPhysicsSweepInputAdapater(const FTransform& InStartTM, const FVector& InDir, const EHitFlags InFlags)
		: StartTM(InStartTM)
		, Dir(InDir)
		, OutputFlags(InFlags)
	{

	}
	FTransform StartTM;
	FVector Dir;
	EHitFlags OutputFlags;
};

/** We use this struct so that if no conversion is needed in another API, we can avoid the copy (if we think that's critical) */
struct FPhysicsOverlapInputAdapater
{
	FPhysicsOverlapInputAdapater(const FTransform& InPose)
		: GeomPose(InPose)
	{

	}
	FTransform GeomPose;
};

#if WITH_CHAOS
/** This is used to add debug data to scene query visitors in non-shipping builds */
struct FQueryDebugParams
{
#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING) 
	FQueryDebugParams()
		: bDebugQuery(false) { }
	bool bDebugQuery;
	bool IsDebugQuery() const { return bDebugQuery; }
#else
	// In test or shipping builds, this struct must be left empty
	FQueryDebugParams() { }
	constexpr bool IsDebugQuery() const { return false; }
#endif
};
#endif

extern PHYSICSCORE_API FCollisionFilterData GetQueryFilterData(const Chaos::TPerShapeData<float, 3>& Shape);
extern PHYSICSCORE_API FCollisionFilterData GetSimulationFilterData(const Chaos::TPerShapeData<float, 3>& Shape);


PHYSICSCORE_API ECollisionShapeType GetImplicitType(const Chaos::TImplicitObject<float, 3>& InGeometry);

FORCEINLINE ECollisionShapeType GetType(const Chaos::TImplicitObject<float, 3>& InGeometry)
{
	return GetImplicitType(InGeometry);
}

PHYSICSCORE_API float GetRadius(const Chaos::TCapsule<float>& InCapsule);

PHYSICSCORE_API float GetHalfHeight(const Chaos::TCapsule<float>& InCapsule);


inline bool HadInitialOverlap(const FLocationHit& Hit)
{
	return Hit.Distance <= 0.f;
}

inline const Chaos::TPerShapeData<float, 3>* GetShape(const FActorShape& Hit)
{
	return Hit.Shape;
}

inline Chaos::TGeometryParticle<float,3>* GetActor(const FActorShape& Hit)
{
	return Hit.Actor;
}

inline float GetDistance(const FLocationHit& Hit)
{
	return Hit.Distance;
}

inline FVector GetPosition(const FLocationHit& Hit)
{
	return Hit.WorldPosition;
}

inline FVector GetNormal(const FLocationHit& Hit)
{
	return Hit.WorldNormal;
}

inline FDummyPhysType* GetMaterialFromInternalFaceIndex(const FDummyPhysType& Shape, uint32 InternalFaceIndex)
{
	return nullptr;
}

inline FHitFlags GetFlags(const FLocationHit& Hit)
{
	return Hit.Flags;
}

FORCEINLINE void SetFlags(FLocationHit& Hit, FHitFlags Flags)
{
	Hit.Flags = Flags;
}

inline uint32 GetInternalFaceIndex(const FQueryHit& Hit)
{
	return Hit.FaceIndex;
}

inline void SetInternalFaceIndex(FQueryHit& Hit, uint32 FaceIndex)
{
	Hit.FaceIndex = FaceIndex;
}


inline uint32 GetInvalidPhysicsFaceIndex()
{
	return 0xffffffff;
}

inline uint32 GetTriangleMeshExternalFaceIndex(const FDummyPhysType& Shape, uint32 InternalFaceIndex)
{
	return GetInvalidPhysicsFaceIndex();
}

inline FTransform GetGlobalPose(const FDummyPhysActor& RigidActor)
{
	return FTransform::Identity;
}

inline uint32 GetNumShapes(const FDummyPhysActor& RigidActor)
{
	return 0;
}

inline void GetShapes(const FDummyPhysActor& RigidActor, Chaos::TImplicitObject<float, 3>** ShapesBuffer, uint32 NumShapes)
{

}

inline void SetActor(FDummyPhysType& Hit, FDummyPhysActor* Actor)
{

}

inline void SetShape(FDummyPhysType& Hit, Chaos::TImplicitObject<float, 3>* Shape)
{

}

template <typename HitType>
HitType* GetBlock(FSQHitBuffer<HitType>& Callback)
{
	return Callback.GetBlock();
}

template <typename HitType>
bool GetHasBlock(const FSQHitBuffer<HitType>& Callback)
{
	return Callback.HasBlockingHit();
}

} // namespace ChaosInterface

#if WITH_CHAOS && (!defined(PHYSICS_INTERFACE_PHYSX) || !PHYSICS_INTERFACE_PHYSX)
using namespace ChaosInterface;
#endif
