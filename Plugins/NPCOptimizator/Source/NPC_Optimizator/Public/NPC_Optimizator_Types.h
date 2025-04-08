// Copyright (C) 2021-2023 Space Raccoon Game Studio. All rights reserved. Contacts: <business@space-raccoon.com>
// Created by AfroStalin

#pragma once

#include "Engine/EngineTypes.h"
#include "Components/SkinnedMeshComponent.h"
#include "NPC_Optimizator_Types.generated.h"

UENUM(BlueprintType)
enum class EOptimizationWave : uint8
{
	Invisible UMETA(DisplayName = "Invisible"),
	NoOptimization UMETA(DisplayName = "No optimization"),
	FirstWave UMETA(DisplayName = "First wave"),
	SecondWave UMETA(DisplayName = "Second wave"),
	ThirdWave UMETA(DisplayName = "Third wave")
};

UENUM()
enum class EOptimizationBaseType
{
	PlayerPawn UMETA(DisplayName = "Based on player pawn position"),
	PlayerCamera UMETA(DisplayName = "Based on player camera position")
};

struct FSDefaultSkeletalMeshSettings
{
	bool IsVisible = true;
	bool IsCastShadows = true;
	ECollisionEnabled::Type Collision = ECollisionEnabled::QueryAndPhysics;
	FCollisionResponseContainer CollisionResponseToChannels;
	bool EnableUpdateRateOptimizations = false;
	bool PerBoneMotionBlur = true;
	bool AllowRigidBodyAnimNode = true;
	bool SkipKinematicUpdateWhenInterpolating = false;
	bool SkipBoundsUpdateWhenInterpolating = false;
	bool GenerateOverlapEvents = true;
	EVisibilityBasedAnimTickOption VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	float MaxDrawDistance = 12000.f;
};

struct FSDefaultStaticMeshSettings
{
	bool IsVisible = true;
};

struct FSDefaultMovementSettings
{
	bool AlwaysCheckFloor = true;
	bool EnablePhysicsInteraction = true;
	float DefaultMovementComponentTick = 0.f;
	float MaxSimulationTimeStep = 0.05f;
	int32 MaxSimulationIterations = 8;
	bool RunPhysicsWithNoController = true;
	bool SweepWhileNavWalking = true;
};

struct FSOptimizedStaticMesh
{
	FSOptimizedStaticMesh() {}
	FSOptimizedStaticMesh(UStaticMeshComponent* InComp, FSDefaultStaticMeshSettings InMesh)
		: Component(InComp)
		, MeshSettings(InMesh)
	{}
	
	TWeakObjectPtr<UStaticMeshComponent> Component = nullptr;

	FSDefaultStaticMeshSettings MeshSettings;
};

struct FSOptimizedComponent
{
	FSOptimizedComponent() {}
	FSOptimizedComponent(UActorComponent* InComp, FSDefaultSkeletalMeshSettings InMesh, const TArray<FSOptimizedStaticMesh>& ChildMeshes = TArray<FSOptimizedStaticMesh>())
		: Component(InComp)
		, MeshSettings(InMesh)
		, ChildStaticMeshes(ChildMeshes)
	{}


	TWeakObjectPtr<UActorComponent>  Component = nullptr;
	FSDefaultSkeletalMeshSettings    MeshSettings;
	TArray<FSOptimizedStaticMesh>    ChildStaticMeshes;
};

struct IOptimizationProxy
{
	virtual ~IOptimizationProxy() = default;
	
	virtual EOptimizationBaseType GetOptimizationBasedType() const = 0;
	virtual AActor*               GetOwnerActor() = 0;
	virtual float                 GetMaxVisibleDistance() const = 0;
	virtual float                 GetDistToWave(EOptimizationWave Wave) const = 0;
	virtual bool                  IsIgnoreCameraSightOnSmallDistance() const = 0;
	virtual bool                  IsForceOptimizationWaveEnabled() const = 0;
};
