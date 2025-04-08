// Copyright (C) 2021-2023 Space Raccoon Game Studio. All rights reserved. Contacts: <business@space-raccoon.com>
// Created by AfroStalin

#include "OptimizationComponent.h"
#include "OptimizationManager.h"
#include "NPC_Optimizator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Async/Async.h"
#include "AIController.h"
#include "GroomComponent.h"
#include "NPC_Optimizator_Types.h"
#include "Animation/AnimInstance.h"
#include "Navigation/PathFollowingComponent.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

DECLARE_CYCLE_STAT(TEXT("Optimization - Sync with Game thread"), STAT_OptimizationThreadSync, STATGROUP_NPC_Optimizator);
DECLARE_CYCLE_STAT(TEXT("Optimization - Skeletal meshes"), STAT_OptimizationMeshes, STATGROUP_NPC_Optimizator);
DECLARE_CYCLE_STAT(TEXT("Optimization - Movement"), STAT_OptimizationMovement, STATGROUP_NPC_Optimizator);
DECLARE_CYCLE_STAT(TEXT("Optimization - Groom"), STAT_OptimizationGroom, STATGROUP_NPC_Optimizator);

static TAutoConsoleVariable<bool> CVarDrawDebug(
	TEXT("npc.optimizator.DrawDebug"),
	false,
	TEXT("Force enable debug drawing\n"),
	ECVF_Scalability);

UOptimizationProxyComponent::UOptimizationProxyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = FMath::RandRange(0.01f, 0.1f);
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostUpdateWork;
}

void UOptimizationProxyComponent::BeginPlay()
{
	Super::BeginPlay();

	ENetMode OwnerNetMode = GetOwner()->GetNetMode();
	ENetRole OwnerNetRole = GetOwner()->GetLocalRole();
	
	if(OwnerNetMode == NM_ListenServer && bDisableOptimizationOnListenServer)
	{
		// Skip optimization for listen server
		return;
	}
	
	if (bDisableOptimizationOnDedicatedServer && OwnerNetMode == NM_DedicatedServer && OwnerNetRole == ROLE_Authority)
	{
		// Skip optimization for dedicated server
		return;
	}

#if WITH_EDITOR
	if (GIsEditor && GEditor->IsSimulatingInEditor() && bDisableOptimizationInSimulationMode)
	{
		return;
	}
#endif

	if (IsValid(GetOwner()) && GetOwner()->IsA(APawn::StaticClass()))
	{
		APawn* Pawn = Cast<APawn>(GetOwner());
		check(Pawn);

		if (Pawn->IsPlayerControlled()) // Ignore all optimization if current owner is a local player
			return;

		if (UPawnMovementComponent* MoveComponent = Pawn->GetMovementComponent())
		{
			MovementSettings.DefaultMovementComponentTick = MoveComponent->GetComponentTickInterval();

			if (UCharacterMovementComponent* CharMove = Cast<UCharacterMovementComponent>(MoveComponent))
			{
				MovementSettings.AlwaysCheckFloor = CharMove->bAlwaysCheckFloor;
				MovementSettings.EnablePhysicsInteraction = CharMove->bEnablePhysicsInteraction;
				MovementSettings.MaxSimulationTimeStep = CharMove->MaxSimulationTimeStep;
				MovementSettings.MaxSimulationIterations = CharMove->MaxSimulationIterations;
				MovementSettings.RunPhysicsWithNoController = CharMove->bRunPhysicsWithNoController;
				MovementSettings.SweepWhileNavWalking = CharMove->bSweepWhileNavWalking;
			}
		}
	}

	//
	{
		TArray<UActorComponent*> TempComponents;

		if (UseOptimizationByTag)
		{
			TempComponents = GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), OptimizationTag);
		}
		else
		{
			GetOwner()->GetComponents(UPrimitiveComponent::StaticClass(), TempComponents);
		}

		TempComponents.RemoveAll([this](const UActorComponent* Component)
		{
			if(Component->ComponentHasTag(IgnoreOptimizationTag))
			{
				return true;
			}

			return false;
		});

		for (const auto& it : TempComponents)
		{
			if (it->IsA(USkeletalMeshComponent::StaticClass()))
			{
				USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(it);
				check(Component);

				FSDefaultSkeletalMeshSettings Settings;

				Settings.IsVisible = Component->IsVisible();
				Settings.CollisionResponseToChannels = Component->GetCollisionResponseToChannels();
				Settings.IsCastShadows = Component->CastShadow;
				Settings.Collision = Component->GetCollisionEnabled();
				Settings.EnableUpdateRateOptimizations = Component->bEnableUpdateRateOptimizations;
				Settings.PerBoneMotionBlur = Component->bPerBoneMotionBlur;
				Settings.AllowRigidBodyAnimNode = Component->GetAllowRigidBodyAnimNode();
				Settings.SkipKinematicUpdateWhenInterpolating = Component->bSkipKinematicUpdateWhenInterpolating;
				Settings.SkipBoundsUpdateWhenInterpolating = Component->bSkipBoundsUpdateWhenInterpolating;
				Settings.GenerateOverlapEvents = Component->GetGenerateOverlapEvents();
				Settings.VisibilityBasedAnimTickOption = Component->VisibilityBasedAnimTickOption;
				Settings.MaxDrawDistance = Component->CachedMaxDrawDistance;
				
				Component->SetCachedMaxDrawDistance(MaxVisibleDistance);
				Component->SetCullDistance(MaxVisibleDistance);

				TArray<FSOptimizedStaticMesh> ChildStaticMeshes;

				// Collect child static meshes for skeletal mesh
				{
					TArray<USceneComponent*> ChildMeshes;
					Component->GetChildrenComponents(true, ChildMeshes);

					for (const auto& ChildMesh : ChildMeshes)
					{
						FSOptimizedStaticMesh OptimizedStaticMesh;
						OptimizedStaticMesh.MeshSettings.IsVisible = ChildMesh->IsVisible();
						OptimizedStaticMesh.Component = Cast<UStaticMeshComponent>(ChildMesh);

						if (HideAllStaticMeshes)
						{
							if (ChildMesh->IsA(UStaticMeshComponent::StaticClass()))
							{
								ChildStaticMeshes.Add(OptimizedStaticMesh);
							}
						}
						else
						{
							if (ChildMesh->IsA(UStaticMeshComponent::StaticClass()) && ChildMesh->ComponentHasTag(OptimizationTag))
							{
								ChildStaticMeshes.Add(OptimizedStaticMesh);
							}
						}
					}
				}

				ComponentsForOptimization.Add(FSOptimizedComponent(Component, Settings, ChildStaticMeshes));
			}
			else if(it->IsA(UGroomComponent::StaticClass()))
			{
				UGroomComponent* Component = Cast<UGroomComponent>(it);
				check(Component);

				FSDefaultSkeletalMeshSettings Settings;

				Settings.IsVisible = Component->IsVisible();
				Settings.CollisionResponseToChannels = Component->GetCollisionResponseToChannels();
				Settings.IsCastShadows = Component->CastShadow;
				Settings.Collision = Component->GetCollisionEnabled();
				Settings.GenerateOverlapEvents = Component->GetGenerateOverlapEvents();
				Settings.MaxDrawDistance = Component->CachedMaxDrawDistance;

				Component->SetCachedMaxDrawDistance(MaxVisibleDistance);
				Component->SetCullDistance(MaxVisibleDistance);
				
				ComponentsForOptimization.Add(FSOptimizedComponent(Component, Settings));
			}
		}
	}

	if (UWorld* LocalWorld = GetWorld())
	{
		if (FOptimizationManager* LocalOptimizationManager = FNPC_OptimizatorModule::GetOptimizationManager(LocalWorld))
		{
			LocalOptimizationManager->RegisterComponent(this);
		}
	}
}

void UOptimizationProxyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (!IsUnreachable())
	{
		if (UWorld* LocalWorld = GetWorld())
		{
			if (const FOptimizationManager* LocalOptimizationManager = FNPC_OptimizatorModule::GetOptimizationManager(LocalWorld))
			{
				LocalOptimizationManager->UnregisterComponent(this);
			}
		}
	}
}

EOptimizationBaseType UOptimizationProxyComponent::GetOptimizationBasedType() const
{
	return OptimizationBasedOn;
}

AActor* UOptimizationProxyComponent::GetOwnerActor()
{
	return GetOwner();
}

float UOptimizationProxyComponent::GetMaxVisibleDistance() const
{
	return MaxVisibleDistance;
}

float UOptimizationProxyComponent::GetDistToWave(EOptimizationWave Wave) const
{
	switch (Wave)
	{
	case EOptimizationWave::Invisible:
		break;
	case EOptimizationWave::NoOptimization:
		break;
	case EOptimizationWave::FirstWave:
		return DistanceToFirstOptimization;
	case EOptimizationWave::SecondWave:
		return DistanceToSecondOptimization;
	case EOptimizationWave::ThirdWave:
		return DistanceToThirdOptimization;
	default:
		break;
	}

	return 0.0f;
}

bool UOptimizationProxyComponent::IsIgnoreCameraSightOnSmallDistance() const
{
	return IgnoreCameraSightOnSmallDistance;
}

bool UOptimizationProxyComponent::IsForceOptimizationWaveEnabled() const
{
	return bIsForceOptimizationWaveEnabled;
}

void UOptimizationProxyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (IsNeedToBeOptimized())
	{
		SCOPE_CYCLE_COUNTER(STAT_OptimizationThreadSync);
		
		OptimizeComponent();

		SetIsNeedToBeOptimized(false);
	}

#if ENABLE_DRAW_DEBUG
	// Draw debug boxes
	{
		FLinearColor Color = FLinearColor::Red;

		bool DrawDebug = false;

		switch (GetOptimizationWave())
		{
		case EOptimizationWave::NoOptimization:
			Color = FLinearColor::Green;
			DrawDebug = NoOptimization_DrawDebug;
			break;
		case EOptimizationWave::FirstWave:
			Color = FLinearColor::Blue;
			DrawDebug = FirstWave_DrawDebug;
			break;
		case EOptimizationWave::SecondWave:
			Color = FLinearColor::Yellow;
			DrawDebug = SecondWave_DrawDebug;
			break;
		case EOptimizationWave::ThirdWave:
			Color = FLinearColor::Gray;
			DrawDebug = ThirdWave_DrawDebug;
			break;
		case EOptimizationWave::Invisible:
			DrawDebug = Invisible_DrawDebug;
			break;
		default:
			break;
		}

		if (!DrawDebug && CVarDrawDebug.GetValueOnGameThread())
		{
			DrawDebug = true;
		}

		if (DrawDebug)
		{
			FVector Origin, Extent;

			GetOwner()->GetActorBounds(true, Origin, Extent);

			UKismetSystemLibrary::DrawDebugBox(GetWorld(), Origin, Extent, Color, FRotator::ZeroRotator, PrimaryComponentTick.TickInterval);
		}
	}
#endif

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

#if WITH_EDITOR
void UOptimizationProxyComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UOptimizationProxyComponent::OptimizeComponent()
{
	check(IsInGameThread());

	const EOptimizationWave Wave = GetOptimizationWave();

	OnChangeWave.Broadcast(Wave);

	OptimizeMovement(Wave);

	OptimizeSkeletalMeshes(Wave);

	OptimizeGrooms(Wave);

	// Disable actor ticking if it needed
	const bool ShouldDisableActorTicking = Wave == EOptimizationWave::Invisible && DisableMovementWhenNotVisible;

	if (GetOwner()->PrimaryActorTick.IsTickFunctionEnabled() != !ShouldDisableActorTicking)
	{
		GetOwner()->PrimaryActorTick.SetTickFunctionEnable(ShouldDisableActorTicking ? false : true);
	}
}

void UOptimizationProxyComponent::DisableAllOptimizations()
{
	if (UWorld* LocalWorld = GetWorld())
	{
		if (const FOptimizationManager* LocalOptimizationManager = FNPC_OptimizatorModule::GetOptimizationManager(LocalWorld))
		{
			LocalOptimizationManager->UnregisterComponent(this);
		}
	}

	SetIsNeedToBeOptimized(false);

	SetOptimizationWave(EOptimizationWave::NoOptimization);

	for (const auto& it : ComponentsForOptimization)
	{
		if(!it.Component.IsValid())
			continue;
		
		if (it.Component.Get()->IsA(USkeletalMeshComponent::StaticClass()))
		{
			USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(it.Component);
			check(Component);

			Component->VisibilityBasedAnimTickOption = it.MeshSettings.VisibilityBasedAnimTickOption;
			Component->SetCachedMaxDrawDistance(it.MeshSettings.MaxDrawDistance);
			Component->SetCullDistance(it.MeshSettings.MaxDrawDistance);
			Component->bEnableUpdateRateOptimizations = it.MeshSettings.EnableUpdateRateOptimizations;
			Component->bPerBoneMotionBlur = it.MeshSettings.PerBoneMotionBlur;
			Component->bSkipKinematicUpdateWhenInterpolating = it.MeshSettings.SkipKinematicUpdateWhenInterpolating;
			Component->bSkipBoundsUpdateWhenInterpolating = it.MeshSettings.SkipBoundsUpdateWhenInterpolating;

			if (DisableSkeletalMeshTickWhenNotVisible)
			{
				Component->PrimaryComponentTick.SetTickFunctionEnable(true);
			}

			Component->SetCastShadow(true);

			// Also need setup cast shadow for attached static meshes
			for (const auto& StaticMesh : it.ChildStaticMeshes)
			{
				if (StaticMesh.Component.IsValid() && StaticMesh.Component->IsVisible() && !StaticMesh.Component->CastShadow)
				{
					StaticMesh.Component->SetCastShadow(true);
				}
			}

			if (Component->IsVisible() != it.MeshSettings.IsVisible)
			{
				Component->SetVisibility(it.MeshSettings.IsVisible);
			}

			// Static meshes visibility
			{
				for (const auto& StaticMesh : it.ChildStaticMeshes)
				{
					if (StaticMesh.Component.IsValid() && StaticMesh.MeshSettings.IsVisible && StaticMesh.Component->IsVisible() != it.MeshSettings.IsVisible)
					{
						StaticMesh.Component->SetVisibility(it.MeshSettings.IsVisible, true);
					}
				}
			}

			if (Component->GetGenerateOverlapEvents() != it.MeshSettings.GenerateOverlapEvents)
			{
				Component->SetGenerateOverlapEvents(it.MeshSettings.GenerateOverlapEvents);
			}

			if (Component->GetAllowRigidBodyAnimNode() != it.MeshSettings.AllowRigidBodyAnimNode)
			{
				Component->SetAllowRigidBodyAnimNode(it.MeshSettings.AllowRigidBodyAnimNode, false);
			}

			// Need to be executed when bSkipKinematicUpdateWhenInterpolating changed
			Component->UpdateKinematicBonesToAnim(Component->GetEditableComponentSpaceTransforms(), ETeleportType::TeleportPhysics, false, EAllowKinematicDeferral::DisallowDeferral);
			Component->UpdateRBJointMotors();


			// Bullshit, but workable method for fixing issue with flying on going to rigid body
			{
				Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Component->SetCollisionEnabled(it.MeshSettings.Collision);
			}
		}
		else if(it.Component.Get()->IsA(UGroomComponent::StaticClass()))
		{
			UGroomComponent* Component = Cast<UGroomComponent>(it.Component);
			check(Component);
			
			Component->SetCachedMaxDrawDistance(it.MeshSettings.MaxDrawDistance);
			Component->SetCullDistance(it.MeshSettings.MaxDrawDistance);

			if (DisableSkeletalMeshTickWhenNotVisible)
			{
				Component->PrimaryComponentTick.SetTickFunctionEnable(true);
			}

			Component->SetCastShadow(true);
			
			if (Component->IsVisible() != it.MeshSettings.IsVisible)
			{
				Component->SetVisibility(it.MeshSettings.IsVisible);
			}
			
			if (Component->GetGenerateOverlapEvents() != it.MeshSettings.GenerateOverlapEvents)
			{
				Component->SetGenerateOverlapEvents(it.MeshSettings.GenerateOverlapEvents);
			}
			

			{
				Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Component->SetCollisionEnabled(it.MeshSettings.Collision);
			}
		}
	}

	if (GetOwner() != nullptr && GetOwner()->IsA(APawn::StaticClass()))
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		check(Pawn);

		UPawnMovementComponent* PawnMovement = Pawn->GetMovementComponent();
		
		AAIController* AIController = Cast<AAIController>(Pawn->GetController());
		UPathFollowingComponent* PathFollow = AIController ? AIController->GetPathFollowingComponent() : nullptr;

		if (PawnMovement && PawnMovement->IsA(UCharacterMovementComponent::StaticClass()))
		{
			UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(PawnMovement);

			check(MovementComponent);

			MovementComponent->bAlwaysCheckFloor = MovementSettings.AlwaysCheckFloor;
			MovementComponent->bEnablePhysicsInteraction = MovementSettings.EnablePhysicsInteraction;
			MovementComponent->MaxSimulationTimeStep = MovementSettings.MaxSimulationTimeStep;

			MovementComponent->MaxSimulationIterations = MovementSettings.MaxSimulationIterations;
			MovementComponent->bRunPhysicsWithNoController = MovementSettings.RunPhysicsWithNoController;

			if (MovementComponent->MovementMode == EMovementMode::MOVE_NavWalking)
			{
				MovementComponent->SetDefaultMovementMode();
			}

			MovementComponent->bSweepWhileNavWalking = MovementSettings.SweepWhileNavWalking;

			MovementComponent->PrimaryComponentTick.SetTickFunctionEnable(true);
			MovementComponent->SetComponentTickInterval(MovementSettings.DefaultMovementComponentTick);
		}
		else if (PawnMovement && PawnMovement->IsA(UPawnMovementComponent::StaticClass()))
		{
			PawnMovement->SetComponentTickInterval(MovementSettings.DefaultMovementComponentTick);
		}

		if (DisableMovementWhenNotVisible)
		{
			if (AIController != nullptr)
			{
				AIController->SetActorTickEnabled(true);
				AIController->SetActorTickInterval(MovementSettings.DefaultMovementComponentTick);
			}

			if (PathFollow != nullptr)
			{
				PathFollow->PrimaryComponentTick.SetTickFunctionEnable(true);
				PathFollow->SetComponentTickInterval(MovementSettings.DefaultMovementComponentTick);
			}

			GetOwner()->PrimaryActorTick.SetTickFunctionEnable(true);
		}
	}

	OnChangeWave.Broadcast(EOptimizationWave::NoOptimization);
}

void UOptimizationProxyComponent::EnableOptimizations()
{
	if (UWorld* LocalWorld = GetWorld())
	{
		if (const FOptimizationManager* LocalOptimizationManager = FNPC_OptimizatorModule::GetOptimizationManager(LocalWorld))
		{
			LocalOptimizationManager->RegisterComponent(this);
		}
	}

	for (const auto& it : ComponentsForOptimization)
	{
		if (it.Component.IsValid() && it.Component->IsA(USkeletalMeshComponent::StaticClass()))
		{
			USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(it.Component);

			check(Component);

			//Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
			Component->SetCachedMaxDrawDistance(MaxVisibleDistance);
			Component->SetCullDistance(MaxVisibleDistance);
		}
	}

	SetIsNeedToBeOptimized(true);
}

void UOptimizationProxyComponent::EnableForceOptimizationWave(EOptimizationWave Wave, bool Enable)
{
	bIsForceOptimizationWaveEnabled = Enable;

	if(Enable)
	{
		SetOptimizationWave(Wave);
		SetIsNeedToBeOptimized(true);
	}
}

void UOptimizationProxyComponent::OptimizeSkeletalMeshes(EOptimizationWave Wave)
{
	SCOPE_CYCLE_COUNTER(STAT_OptimizationMeshes);

	for (const auto& it : ComponentsForOptimization)
	{
		if (!it.Component.IsValid() || !it.Component->IsA(USkeletalMeshComponent::StaticClass()))
			continue;

		USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(it.Component);
		check(Component);

		ECollisionEnabled::Type NeedEnableCollision = ECollisionEnabled::QueryAndPhysics;
		bool NeedCastShadow = false;
		bool NeedEnableTick = true;
		bool NeedSetIsVisibility = false;
		bool NeedGenerateOverlapEvents = false;
		bool NeedAllowRigidBodyAnimNode = false;
		bool NeedHideStaticMeshes = false;

		switch (Wave)
		{
		case EOptimizationWave::NoOptimization:
			{
				Component->bEnableUpdateRateOptimizations = it.MeshSettings.EnableUpdateRateOptimizations;
				Component->bPerBoneMotionBlur = it.MeshSettings.PerBoneMotionBlur;
				Component->bSkipKinematicUpdateWhenInterpolating = it.MeshSettings.SkipKinematicUpdateWhenInterpolating;
				Component->bSkipBoundsUpdateWhenInterpolating = it.MeshSettings.SkipBoundsUpdateWhenInterpolating;
				Component->VisibilityBasedAnimTickOption = it.MeshSettings.VisibilityBasedAnimTickOption;

				NeedEnableCollision = it.MeshSettings.Collision;
				NeedCastShadow = it.MeshSettings.IsCastShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = it.MeshSettings.IsVisible ? true : false;
				NeedGenerateOverlapEvents = it.MeshSettings.GenerateOverlapEvents;
				NeedAllowRigidBodyAnimNode = it.MeshSettings.AllowRigidBodyAnimNode;
				NeedHideStaticMeshes = false;
			}
			break;
		case EOptimizationWave::FirstWave:
			{
				if (FirstWave_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}

				Component->bEnableUpdateRateOptimizations = FirstWave_UseUpdateRateOptimizations;
				Component->bPerBoneMotionBlur = FirstWave_UsePerBoneMotionBlur;
				Component->bDisableClothSimulation = FirstWave_DisableClothSimulation;
				Component->bDisableMorphTarget = FirstWave_DisableMorphTarget;
				Component->bSkipKinematicUpdateWhenInterpolating = FirstWave_SkipKinematicUpdateWhenInterpolating;
				Component->bSkipBoundsUpdateWhenInterpolating = FirstWave_SkipBoundsUpdateWhenInterpolating;
				Component->VisibilityBasedAnimTickOption = FirstWave_VisibilityBasedAnimTickOption;

				NeedCastShadow = !FirstWave_HideShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = it.MeshSettings.IsVisible ? true : false;
				NeedGenerateOverlapEvents = FirstWave_GenerateOverlapEvents;
				NeedAllowRigidBodyAnimNode = it.MeshSettings.AllowRigidBodyAnimNode ? FirstWave_AllowRigidBodyAnimNode : false;
				NeedHideStaticMeshes = FirstWave_NeedHideStaticMeshes;
			}
			break;
		case EOptimizationWave::SecondWave:
			{
				if (SecondWave_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}

				if (SecondWave_UseUpdateRateOptimizations)
				{
					Component->bEnableUpdateRateOptimizations = true;
				}

				Component->bPerBoneMotionBlur = SecondWave_UsePerBoneMotionBlur;
				Component->bDisableClothSimulation = SecondWave_DisableClothSimulation;
				Component->bDisableMorphTarget = SecondWave_DisableMorphTarget;
				Component->bSkipKinematicUpdateWhenInterpolating = SecondWave_SkipKinematicUpdateWhenInterpolating;
				Component->bSkipBoundsUpdateWhenInterpolating = SecondWave_SkipBoundsUpdateWhenInterpolating;
				Component->VisibilityBasedAnimTickOption = SecondWave_VisibilityBasedAnimTickOption;

				NeedCastShadow = !SecondWave_HideShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = it.MeshSettings.IsVisible ? true : false;
				NeedGenerateOverlapEvents = SecondWave_GenerateOverlapEvents;
				NeedAllowRigidBodyAnimNode = it.MeshSettings.AllowRigidBodyAnimNode ? SecondWave_AllowRigidBodyAnimNode : false;
				NeedHideStaticMeshes = SecondWave_NeedHideStaticMeshes;
			}
			break;
		case EOptimizationWave::ThirdWave:
			{
				if (ThirdWave_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}

				if (ThirdWave_UseUpdateRateOptimizations)
				{
					Component->bEnableUpdateRateOptimizations = true;
				}

				Component->bPerBoneMotionBlur = ThirdWave_UsePerBoneMotionBlur;
				Component->bDisableClothSimulation = ThirdWave_DisableClothSimulation;
				Component->bDisableMorphTarget = ThirdWave_DisableMorphTarget;
				Component->bSkipKinematicUpdateWhenInterpolating = ThirdWave_SkipKinematicUpdateWhenInterpolating;
				Component->bSkipBoundsUpdateWhenInterpolating = ThirdWave_SkipBoundsUpdateWhenInterpolating;
				Component->VisibilityBasedAnimTickOption = ThirdWave_VisibilityBasedAnimTickOption;

				NeedCastShadow = !ThirdWave_HideShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = it.MeshSettings.IsVisible ? true : false;
				NeedGenerateOverlapEvents = ThirdWave_GenerateOverlapEvents;
				NeedAllowRigidBodyAnimNode = it.MeshSettings.AllowRigidBodyAnimNode ? ThirdWave_AllowRigidBodyAnimNode : false;
				NeedHideStaticMeshes = ThirdWave_NeedHideStaticMeshes;
			}
			break;
		case EOptimizationWave::Invisible:
			{
				if (Invisible_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}

				if (Invisible_UseUpdateRateOptimizations)
				{
					Component->bEnableUpdateRateOptimizations = true;
				}

				Component->bPerBoneMotionBlur = Invisible_UsePerBoneMotionBlur;
				Component->bDisableClothSimulation = Invisible_DisableClothSimulation;
				Component->bDisableMorphTarget = Invisible_DisableMorphTarget;
				Component->bSkipKinematicUpdateWhenInterpolating = Invisible_SkipKinematicUpdateWhenInterpolating;
				Component->bSkipBoundsUpdateWhenInterpolating = Invisible_SkipBoundsUpdateWhenInterpolating;
				Component->VisibilityBasedAnimTickOption = Invisible_VisibilityBasedAnimTickOption;

				if (DisableSkeletalMeshTickWhenNotVisible)
				{
					NeedEnableTick = false;
				}

				NeedCastShadow = !Invisible_HideShadows;
				NeedSetIsVisibility = it.MeshSettings.IsVisible ? !Invisible_HideSkeletalMesh : false;
				NeedGenerateOverlapEvents = Invisible_GenerateOverlapEvents;
				NeedAllowRigidBodyAnimNode = it.MeshSettings.AllowRigidBodyAnimNode ? Invisible_AllowRigidBodyAnimNode : false;
				NeedHideStaticMeshes = Invisible_NeedHideStaticMeshes;
			}
			break;
		default:
			break;
		}

		if (DisableSkeletalMeshTickWhenNotVisible)
		{
			if (!NeedEnableTick)
			{
				if (Component->PrimaryComponentTick.IsTickFunctionEnabled())
				{
					AsyncTask(ENamedThreads::GameThread, [Component]
					{
						Component->PrimaryComponentTick.SetTickFunctionEnable(false);
					});
				}
			}
			else
			{
				if (!Component->PrimaryComponentTick.IsTickFunctionEnabled())
				{
					AsyncTask(ENamedThreads::GameThread, [Component]
					{
						Component->PrimaryComponentTick.SetTickFunctionEnable(true);
					});
				}
			}
		}

		if (Component->CastShadow != NeedCastShadow)
		{
			Component->SetCastShadow(NeedCastShadow);

			// Also need setup cast shadow for attached static meshes
			for (const auto& StaticMesh : it.ChildStaticMeshes)
			{
				if (!StaticMesh.Component.IsValid())
					continue;

				if (StaticMesh.Component->IsVisible() && StaticMesh.Component->CastShadow != NeedCastShadow)
				{
					StaticMesh.Component->SetCastShadow(NeedCastShadow);
				}
			}
		}

		if (Component->GetCollisionEnabled() != NeedEnableCollision)
		{
			Component->SetCollisionEnabled(NeedEnableCollision);
		}

		if (Component->IsVisible() != NeedSetIsVisibility)
		{
			Component->SetVisibility(NeedSetIsVisibility);
		}


		// Static meshes visibility
		{
			for (const auto& StaticMesh : it.ChildStaticMeshes)
			{
				if (!StaticMesh.Component.IsValid())
					continue;

				if (StaticMesh.MeshSettings.IsVisible && StaticMesh.Component->IsVisible() != !NeedHideStaticMeshes)
				{
					StaticMesh.Component->SetVisibility(!NeedHideStaticMeshes);
				}
			}
		}

		if (Component->GetGenerateOverlapEvents() != NeedGenerateOverlapEvents)
		{
			Component->SetGenerateOverlapEvents(NeedGenerateOverlapEvents);
		}
		
		if (Component->GetAllowRigidBodyAnimNode() != NeedAllowRigidBodyAnimNode)
		{
#if DO_CHECK
			const int32 RootLOD = Component->GetAnimInstance() && Component->GetAnimInstance()->GetRequiredBones().GetCalculatedForLOD();
			const int32 PostProcLod = Component->GetPostProcessInstance() ? Component->GetPostProcessInstance()->GetRequiredBones().GetCalculatedForLOD() : -1;

			if(RootLOD != PostProcLod)
#endif
			{
				Component->SetAllowRigidBodyAnimNode(NeedAllowRigidBodyAnimNode);
			}
			//else
			//{
				// WILL BE CRASH HERE! See AnimInstance.cpp - check(RootLOD == MeshComp->GetPostProcessInstance()->GetRequiredBones().GetCalculatedForLOD());
			//}
		}
	}
}

void UOptimizationProxyComponent::OptimizeMovement(EOptimizationWave Wave) const
{
	SCOPE_CYCLE_COUNTER(STAT_OptimizationMovement);

	if (GetOwner() != nullptr && GetOwner()->IsA(APawn::StaticClass()))
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		check(Pawn);

		AAIController* AIController = Cast<AAIController>(Pawn->GetController());
		UPathFollowingComponent* PathFollow = AIController ? AIController->GetPathFollowingComponent() : nullptr;
		UPawnMovementComponent* PawnMovement = Pawn->GetMovementComponent();
		
		float Tick = MovementSettings.DefaultMovementComponentTick;
		bool MovementNeedToBeDisabled = false;

		if (PawnMovement && PawnMovement->IsA(UCharacterMovementComponent::StaticClass()))
		{
			UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(PawnMovement);

			switch (Wave)
			{
			case EOptimizationWave::Invisible:
			{
				if (DisableMovementWhenNotVisible)
				{
					MovementNeedToBeDisabled = true;
				}
				else
				{
					if (Invisible_UseRandomOptimizationTickForMovement)
					{
						Tick = FMath::RandRange(Invisible_OptimizatedMovementTickMin, Invisible_OptimizatedMovementTickMax);
					}
					else
					{
						Tick = Invisible_OptimizatedMovementTick;
					}
				}

				MovementComponent->bAlwaysCheckFloor = Invisible_AlwaysCheckFloor;
				MovementComponent->bEnablePhysicsInteraction = Invisible_EnablePhysicsInteraction;
				MovementComponent->MaxSimulationTimeStep = Invisible_MaxSimulationTimeStep;
				MovementComponent->MaxSimulationIterations = Invisible_MaxSimulationIterations;
				MovementComponent->bRunPhysicsWithNoController = Invisible_RunPhysicsWithNoController;

				if (MovementComponent->MovementMode != Invisible_MovementMode)
				{
					MovementComponent->SetMovementMode(Invisible_MovementMode);
				}

				MovementComponent->bSweepWhileNavWalking = Invisible_SweepWhileNavWalking;
			}
			break;
			case EOptimizationWave::NoOptimization:
			{
				MovementComponent->bAlwaysCheckFloor = MovementSettings.AlwaysCheckFloor;
				MovementComponent->bEnablePhysicsInteraction = MovementSettings.EnablePhysicsInteraction;
				MovementComponent->MaxSimulationTimeStep = MovementSettings.MaxSimulationTimeStep;

				MovementComponent->MaxSimulationIterations = MovementSettings.MaxSimulationIterations;
				MovementComponent->bRunPhysicsWithNoController = MovementSettings.RunPhysicsWithNoController;
				MovementComponent->SetDefaultMovementMode();
				MovementComponent->bSweepWhileNavWalking = MovementSettings.SweepWhileNavWalking;
			}
			break;
			case EOptimizationWave::FirstWave:
			{
				if (FirstWave_UseRandomOptimizationTickForMovement)
				{
					Tick = FMath::RandRange(FirstWave_OptimizatedMovementTickMin, FirstWave_OptimizatedMovementTickMax);
				}
				else
				{
					Tick = FirstWave_OptimizatedMovementTick;
				}

				MovementComponent->bAlwaysCheckFloor = FirstWave_AlwaysCheckFloor;
				MovementComponent->bEnablePhysicsInteraction = FirstWave_EnablePhysicsInteraction;
				MovementComponent->MaxSimulationTimeStep = FirstWave_MaxSimulationTimeStep;

				MovementComponent->MaxSimulationIterations = FirstWave_MaxSimulationIterations;
				MovementComponent->bRunPhysicsWithNoController = FirstWave_RunPhysicsWithNoController;
				MovementComponent->bSweepWhileNavWalking = FirstWave_SweepWhileNavWalking;

				if (MovementComponent->MovementMode != FirstWave_MovementMode)
				{
					MovementComponent->SetMovementMode(FirstWave_MovementMode);
				}
			}
			break;
			case EOptimizationWave::SecondWave:
			{
				if (SecondWave_UseRandomOptimizationTickForMovement)
				{
					Tick = FMath::RandRange(SecondWave_OptimizatedMovementTickMin, SecondWave_OptimizatedMovementTickMax);
				}
				else
				{
					Tick = SecondWave_OptimizatedMovementTick;
				}

				MovementComponent->bAlwaysCheckFloor = SecondWave_AlwaysCheckFloor;
				MovementComponent->bEnablePhysicsInteraction = SecondWave_EnablePhysicsInteraction;
				MovementComponent->MaxSimulationTimeStep = SecondWave_MaxSimulationTimeStep;

				MovementComponent->MaxSimulationIterations = SecondWave_MaxSimulationIterations;
				MovementComponent->bRunPhysicsWithNoController = SecondWave_RunPhysicsWithNoController;
				MovementComponent->bSweepWhileNavWalking = SecondWave_SweepWhileNavWalking;

				if (MovementComponent->MovementMode != SecondWave_MovementMode)
				{
					MovementComponent->SetMovementMode(SecondWave_MovementMode);
				}
			}
			break;
			case EOptimizationWave::ThirdWave:
			{
				if (ThirdWave_UseRandomOptimizationTickForMovement)
				{
					Tick = FMath::RandRange(ThirdWave_OptimizatedMovementTickMin, ThirdWave_OptimizatedMovementTickMax);
				}
				else
				{
					Tick = ThirdWave_OptimizatedMovementTick;
				}

				MovementComponent->bAlwaysCheckFloor = ThirdWave_AlwaysCheckFloor;
				MovementComponent->bEnablePhysicsInteraction = ThirdWave_EnablePhysicsInteraction;
				MovementComponent->MaxSimulationTimeStep = ThirdWave_MaxSimulationTimeStep;

				MovementComponent->MaxSimulationIterations = ThirdWave_MaxSimulationIterations;
				MovementComponent->bRunPhysicsWithNoController = ThirdWave_RunPhysicsWithNoController;
				MovementComponent->bSweepWhileNavWalking = ThirdWave_SweepWhileNavWalking;

				if (MovementComponent->MovementMode != ThirdWave_MovementMode)
				{
					MovementComponent->SetMovementMode(ThirdWave_MovementMode);
				}
			}
			break;
			default:
				break;
			}

			if (DisableMovementWhenNotVisible)
			{
				if (MovementComponent->PrimaryComponentTick.IsTickFunctionEnabled() != !MovementNeedToBeDisabled)
				{
					MovementComponent->PrimaryComponentTick.SetTickFunctionEnable(!MovementNeedToBeDisabled);
				}

				if (AIController != nullptr && AIController->PrimaryActorTick.IsTickFunctionEnabled() != !MovementNeedToBeDisabled)
				{
					AIController->SetActorTickEnabled(!MovementNeedToBeDisabled);
				}
				
				if (PathFollow != nullptr && PathFollow->PrimaryComponentTick.IsTickFunctionEnabled() != !MovementNeedToBeDisabled)
				{
					PathFollow->PrimaryComponentTick.SetTickFunctionEnable(!MovementNeedToBeDisabled);
				}
			}

			if (PreviousWave == EOptimizationWave::Invisible && Wave != EOptimizationWave::Invisible && !DisableMovementWhenNotVisible)
			{
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26) || ENGINE_MAJOR_VERSION == 5
				AsyncTask(ENamedThreads::GameThread, [MovementComponent, Tick]
					{
						if (IsValid(MovementComponent))
						{
							MovementComponent->SetComponentTickIntervalAndCooldown(Tick);
						}
					});			
#elif ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 25
				MovementComponent->SetComponentTickInterval(Tick);
#endif
			}
			
			if (MovementComponent->GetComponentTickInterval() != Tick)
			{
				MovementComponent->SetComponentTickInterval(Tick);
			}

			if (AIController != nullptr && AIController->GetActorTickInterval() != Tick && bOptimizeAIController)
			{
				AIController->SetActorTickInterval(Tick);
			}

			if (PathFollow != nullptr && PathFollow->GetComponentTickInterval() != Tick && bOptimizePathFollowComponent)
			{
				PathFollow->SetComponentTickInterval(Tick);
			}
		}
		else if (PawnMovement && PawnMovement->IsA(UPawnMovementComponent::StaticClass()))
		{
			switch (Wave)
			{
			case EOptimizationWave::Invisible:
			{
				if (DisableMovementWhenNotVisible)
				{
					MovementNeedToBeDisabled = true;
				}
				else
				{
					Tick = 1.f;
				}
			}
			break;
			case EOptimizationWave::NoOptimization:
			{
				Tick = MovementSettings.DefaultMovementComponentTick;
			}
			break;
			case EOptimizationWave::FirstWave:
			{
				if (FirstWave_UseRandomOptimizationTickForMovement)
				{
					Tick = FMath::RandRange(FirstWave_OptimizatedMovementTickMin, FirstWave_OptimizatedMovementTickMax);
				}
				else
				{
					Tick = FirstWave_OptimizatedMovementTick;
				}
			}
			break;
			case EOptimizationWave::SecondWave:
			{
				if (SecondWave_UseRandomOptimizationTickForMovement)
				{
					Tick = FMath::RandRange(SecondWave_OptimizatedMovementTickMin, SecondWave_OptimizatedMovementTickMax);
				}
				else
				{
					Tick = SecondWave_OptimizatedMovementTick;
				}
			}
			break;
			case EOptimizationWave::ThirdWave:
			{
				if (ThirdWave_UseRandomOptimizationTickForMovement)
				{
					Tick = FMath::RandRange(ThirdWave_OptimizatedMovementTickMin, ThirdWave_OptimizatedMovementTickMax);
				}
				else
				{
					Tick = ThirdWave_OptimizatedMovementTick;
				}
			}
			break;
			}

			if (DisableMovementWhenNotVisible)
			{
				if (PawnMovement && PawnMovement->PrimaryComponentTick.IsTickFunctionEnabled() != !MovementNeedToBeDisabled)
				{
					PawnMovement->PrimaryComponentTick.SetTickFunctionEnable(!MovementNeedToBeDisabled);
				}

				if (AIController != nullptr && AIController->PrimaryActorTick.IsTickFunctionEnabled() != !MovementNeedToBeDisabled)
				{
					AIController->SetActorTickEnabled(!MovementNeedToBeDisabled);
				}

				if (PathFollow != nullptr && PathFollow->PrimaryComponentTick.IsTickFunctionEnabled() != !MovementNeedToBeDisabled)
				{
					PathFollow->PrimaryComponentTick.SetTickFunctionEnable(!MovementNeedToBeDisabled);
				}
			}

			if (AIController != nullptr && bOptimizeAIController)
			{
				AIController->PrimaryActorTick.TickInterval = Tick;
			}

			if (PathFollow != nullptr && bOptimizePathFollowComponent)
			{
				PathFollow->PrimaryComponentTick.TickInterval = Tick;
			}

			if(PawnMovement)
			{
				PawnMovement->SetComponentTickInterval(Tick);
			}
		}
	}
}

void UOptimizationProxyComponent::OptimizeGrooms(EOptimizationWave Wave)
{
	SCOPE_CYCLE_COUNTER(STAT_OptimizationGroom);

	for (const auto& it : ComponentsForOptimization)
	{
		if (!it.Component.IsValid() || !it.Component->IsA(UGroomComponent::StaticClass()))
			continue;

		UGroomComponent* Component = Cast<UGroomComponent>(it.Component);
		check(Component);

		ECollisionEnabled::Type NeedEnableCollision = ECollisionEnabled::QueryAndPhysics;
		bool NeedCastShadow = false;
		bool NeedEnableTick = true;
		bool NeedSetIsVisibility = false;
		bool NeedGenerateOverlapEvents = false;

		switch (Wave)
		{
		case EOptimizationWave::NoOptimization:
			{
				NeedEnableCollision = it.MeshSettings.Collision;
				NeedCastShadow = it.MeshSettings.IsCastShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = it.MeshSettings.IsVisible;
				NeedGenerateOverlapEvents = it.MeshSettings.GenerateOverlapEvents;
			}
			break;
		case EOptimizationWave::FirstWave:
			{
				if (FirstWaveGroom_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}
				
				NeedCastShadow = !FirstWaveGroom_HideShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = !FirstWaveGroom_Hide;
				NeedGenerateOverlapEvents = FirstWaveGroom_GenerateOverlapEvents;
			}
			break;
		case EOptimizationWave::SecondWave:
			{
				if (SecondWaveGroom_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}

				NeedCastShadow = !SecondWaveGroom_HideShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = !SecondWaveGroom_Hide;
				NeedGenerateOverlapEvents = SecondWaveGroom_GenerateOverlapEvents;
			}
			break;
		case EOptimizationWave::ThirdWave:
			{
				if (ThirdWaveGroom_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}
				
				NeedCastShadow = !ThirdWaveGroom_HideShadows;
				NeedEnableTick = true;
				NeedSetIsVisibility = !ThirdWaveGroom_Hide;
				NeedGenerateOverlapEvents = ThirdWaveGroom_GenerateOverlapEvents;
			}
			break;
		case EOptimizationWave::Invisible:
			{
				if (InvisibleGroom_DisableMeshCollision)
				{
					NeedEnableCollision = ECollisionEnabled::NoCollision;
				}
				else
				{
					NeedEnableCollision = it.MeshSettings.Collision;
				}
				
				if (DisableGroomTickWhenNotVisible)
				{
					NeedEnableTick = false;
				}

				NeedCastShadow = !InvisibleGroom_HideShadows;
				NeedSetIsVisibility = !InvisibleGroom_Hide;
				NeedGenerateOverlapEvents = InvisibleGroom_GenerateOverlapEvents;
			}
			break;
		default:
			break;
		}

		if (DisableSkeletalMeshTickWhenNotVisible)
		{
			if (!NeedEnableTick)
			{
				if (Component->PrimaryComponentTick.IsTickFunctionEnabled())
				{
					AsyncTask(ENamedThreads::GameThread, [Component]
					{
						Component->PrimaryComponentTick.SetTickFunctionEnable(false);
					});
				}
			}
			else
			{
				if (!Component->PrimaryComponentTick.IsTickFunctionEnabled())
				{
					AsyncTask(ENamedThreads::GameThread, [Component]
					{
						Component->PrimaryComponentTick.SetTickFunctionEnable(true);
					});
				}
			}
		}

		if (Component->CastShadow != NeedCastShadow)
		{
			Component->SetCastShadow(NeedCastShadow);

			// Also need setup cast shadow for attached static meshes
			for (const auto& StaticMesh : it.ChildStaticMeshes)
			{
				if (!StaticMesh.Component.IsValid())
					continue;

				if (StaticMesh.Component->IsVisible() && StaticMesh.Component->CastShadow != NeedCastShadow)
				{
					StaticMesh.Component->SetCastShadow(NeedCastShadow);
				}
			}
		}

		if (Component->GetCollisionEnabled() != NeedEnableCollision)
		{
			Component->SetCollisionEnabled(NeedEnableCollision);
		}

		if (Component->IsVisible() != NeedSetIsVisibility)
		{
			Component->SetVisibility(NeedSetIsVisibility);
		}

		if (Component->GetGenerateOverlapEvents() != NeedGenerateOverlapEvents)
		{
			Component->SetGenerateOverlapEvents(NeedGenerateOverlapEvents);
		}
	}
}

EOptimizationWave UOptimizationProxyComponent::GetOptimizationWave() const
{
	FScopeLock Lock(&LockCS);
	return OptimizationWave;
}

void UOptimizationProxyComponent::SetOptimizationWave(EOptimizationWave Wave)
{
	FScopeLock Lock(&LockCS);
	PreviousWave = OptimizationWave;
	OptimizationWave = Wave;
}

bool UOptimizationProxyComponent::IsNeedToBeOptimized() const
{
	return bIsNeedToBeOptimized;
}

void UOptimizationProxyComponent::SetIsNeedToBeOptimized(bool NeedToOptimize)
{
	bIsNeedToBeOptimized = NeedToOptimize;
}