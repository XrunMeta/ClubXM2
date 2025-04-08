// Copyright (C) 2021-2023 Space Raccoon Game Studio. All rights reserved. Contacts: <business@space-raccoon.com>
// Created by AfroStalin

#include "OptimizationManager.h"

#include "NPC_Optimizator_Types.h"
#include "OptimizationComponent.h"
#include "Kismet/GameplayStatics.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "Camera/PlayerCameraManager.h"
#else
#include "Engine/Classes/Camera/PlayerCameraManager.h"
#endif
#include "GameFramework/Pawn.h"
#include "Async/Async.h"

DECLARE_CYCLE_STAT(TEXT("Manager - Thread tick"), STAT_OptimizationThreadTick, STATGROUP_NPC_Optimizator);
DECLARE_CYCLE_STAT(TEXT("Manager - Optimization tick"), STAT_OptimizationComponentsTick, STATGROUP_NPC_Optimizator);

DECLARE_DWORD_COUNTER_STAT(TEXT("NPC count (all with optimization)"), STAT_NPC_Count, STATGROUP_NPC_Optimizator);
DECLARE_DWORD_COUNTER_STAT(TEXT("NPC count - Invisible"), STAT_InvisibleCount, STATGROUP_NPC_Optimizator);
DECLARE_DWORD_COUNTER_STAT(TEXT("NPC count - Without optimization"), STAT_NoOptimizedCount, STATGROUP_NPC_Optimizator);
DECLARE_DWORD_COUNTER_STAT(TEXT("NPC count - First wave"), Stat_FirstWaveCount, STATGROUP_NPC_Optimizator);
DECLARE_DWORD_COUNTER_STAT(TEXT("NPC count - Second wave"), Stat_SecondWaveCount, STATGROUP_NPC_Optimizator);
DECLARE_DWORD_COUNTER_STAT(TEXT("NPC count - Third wave"), Stat_ThirdWaveCount, STATGROUP_NPC_Optimizator);


static TAutoConsoleVariable<float> CVarMaxFOV(
	TEXT("npc.optimizator.fov"),
	90.f,
	TEXT("Camera FOV for checking NPCs in frame\n"),
	ECVF_Scalability);

static TAutoConsoleVariable<int32> CVarMaxOptimizationOnOneFrame(
	TEXT("npc.optimizator.MaxOptimizedOnOneFrame"),
	15,
	TEXT("How many NPC can be optimizated in one frame\n"),
	ECVF_Scalability);

FOptimizationThread::FOptimizationThread(UWorld* World, FOptimizationManager* InManager) : bIsRunning(true), GameWorld(World), Manager(InManager)
{
	if (IsValid(GameWorld) /*&& GameWorld->GetNetMode() != NM_DedicatedServer*/)
	{
		Thread.Reset(FRunnableThread::Create(this, *FString::Printf(TEXT("NPC_Optimizator_Thread_%s"), *GameWorld->GetName())));
	}
}

uint32 FOptimizationThread::Run()
{
	while (bIsRunning)
	{
		SCOPE_CYCLE_COUNTER(STAT_OptimizationThreadTick);

		APlayerCameraManager* PlayerCamera = UGameplayStatics::GetPlayerCameraManager(GameWorld, 0);
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GameWorld, 0);
		APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;

		SET_DWORD_STAT(STAT_NPC_Count, 0);
		SET_DWORD_STAT(STAT_InvisibleCount, 0);
		SET_DWORD_STAT(STAT_NoOptimizedCount, 0);
		SET_DWORD_STAT(Stat_FirstWaveCount, 0);
		SET_DWORD_STAT(Stat_SecondWaveCount, 0);
		SET_DWORD_STAT(Stat_ThirdWaveCount, 0);

		const float FOV = CVarMaxFOV.GetValueOnAnyThread();

		// Calculate visible & optimization waves for proxy components
		Mutex.Lock();
		{
			SCOPE_CYCLE_COUNTER(STAT_OptimizationComponentsTick);

			const uint32 MaxOptimizedInOneFrame = CVarMaxOptimizationOnOneFrame.GetValueOnAnyThread();
			uint32 OptimizedCount = 0;

			for (auto& it : Components)
			{
				if (!IsValid(it))
					continue;

				const EOptimizationWave CurrentWave = it->GetOptimizationWave();

				INC_DWORD_STAT(STAT_NPC_Count);

				// Stats counters
				{
					switch (CurrentWave)
					{
					case EOptimizationWave::Invisible:
						INC_DWORD_STAT(STAT_InvisibleCount);
						break;
					case EOptimizationWave::NoOptimization:
						INC_DWORD_STAT(STAT_NoOptimizedCount);
						break;
					case EOptimizationWave::FirstWave:
						INC_DWORD_STAT(Stat_FirstWaveCount);
						break;
					case EOptimizationWave::SecondWave:
						INC_DWORD_STAT(Stat_SecondWaveCount);
						break;
					case EOptimizationWave::ThirdWave:
						INC_DWORD_STAT(Stat_ThirdWaveCount);
						break;
					}
				}

				if (it->IsNeedToBeOptimized())
					continue;

				if (OptimizedCount >= MaxOptimizedInOneFrame)
					break;

				const bool IsVisible = it->IgnoreCameraSight ? true : IsInCameraSight(PlayerCamera, PlayerPawn, it, FOV);
				const EOptimizationWave NewWave = GetOptimizationWave(PlayerPawn, PlayerCamera, it, IsVisible);

				if (CurrentWave != NewWave && !it->IsForceOptimizationWaveEnabled())
				{
					it->SetOptimizationWave(NewWave);
					it->SetIsNeedToBeOptimized(true);

					OptimizedCount++;
				}
			}
		}
		Mutex.Unlock();
		
		FPlatformProcess::Sleep(0.01f);
	}

	return 0;
}

void FOptimizationThread::Exit()
{
	Manager = nullptr;
	bIsRunning = false;
}

void FOptimizationThread::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(GameWorld);

	for (auto& it : Components)
	{
		Collector.AddReferencedObject(it);
	}
}

FString FOptimizationThread::GetReferencerName() const
{
	return TEXT("FOptimizationThread");
}

void FOptimizationThread::RegisterComponent(UOptimizationProxyComponent* Component)
{
	Mutex.Lock();
	{
		check(Component);

		if (Component->GetOptimizationHandle() == INDEX_NONE)
		{
			Component->SetOptimizationHandle(Components.Num());

			Components.Add(Component);
		}
	} 
	Mutex.Unlock();
}

void FOptimizationThread::UnregisterComponent(const UOptimizationProxyComponent* Component)
{
	Mutex.Lock();
	{ 
		check(Component);

		const int32 Handle = Component->GetOptimizationHandle();
		if (Handle != INDEX_NONE)
		{
			RemoveComponentByHandle(Handle);
		}
	}
	Mutex.Unlock();
}

void FOptimizationThread::HandlePostGarbageCollect()
{
	Mutex.Lock();
	{
		bool bRemoved;
		do
		{
			bRemoved = false;
			for (int32 DataIndex = 0; DataIndex < Components.Num(); ++DataIndex)
			{
				if (!IsValid(Components[DataIndex]))
				{
					RemoveComponentByHandle(DataIndex);
					bRemoved = true;
				}
			}
		} while (bRemoved);
	}
	Mutex.Unlock();
}

bool FOptimizationThread::IsInCameraSight(const APlayerCameraManager* Camera, const APawn* LocalPlayer, IOptimizationProxy* Proxy, float InFOV)
{
	if (IsValid(Camera) && Proxy != nullptr)
	{
		float Distance = 10000.f;

		if (Proxy->GetOptimizationBasedType() == EOptimizationBaseType::PlayerCamera)
		{
			const FVector CameraLocation = Camera->GetCameraLocation();

			Distance = FVector::Distance(CameraLocation, Proxy->GetOwnerActor()->GetActorLocation());
		}
		else if (Proxy->GetOptimizationBasedType() == EOptimizationBaseType::PlayerPawn && LocalPlayer)
		{
			Distance = FVector::Distance(LocalPlayer->GetActorLocation(), Proxy->GetOwnerActor()->GetActorLocation());
		}

		const float CameraDotProd = Camera->GetDotProductTo(Proxy->GetOwnerActor());
		const float CamAngle = 1.f - (InFOV * 0.01f);

		if (CameraDotProd < CamAngle || Distance > Proxy->GetMaxVisibleDistance())
		{
			return false;
		}
		
		return true;
	}

	return true;
}

EOptimizationWave FOptimizationThread::GetOptimizationWave(const APawn* LocalPlayer, const APlayerCameraManager* Camera, IOptimizationProxy* Proxy, bool IsVisible)
{
	if (!IsVisible && !Proxy->IsIgnoreCameraSightOnSmallDistance())
	{
		return EOptimizationWave::Invisible;
	}

	float DistToPlayer = 10000.f;

	if (LocalPlayer)
	{
		DistToPlayer = FVector::Distance(LocalPlayer->GetActorLocation(), Proxy->GetOwnerActor()->GetActorLocation());
	}

	if (Camera && Proxy->GetOptimizationBasedType() == EOptimizationBaseType::PlayerCamera)
	{
		const FVector CameraLocation = Camera->GetCameraLocation();
		DistToPlayer = FVector::Distance(CameraLocation, Proxy->GetOwnerActor()->GetActorLocation());
	}

	if (!IsVisible && Proxy->IsIgnoreCameraSightOnSmallDistance() )
	{
		if (DistToPlayer < Proxy->GetDistToWave(EOptimizationWave::FirstWave))
		{
			return EOptimizationWave::NoOptimization;
		}
		else
		{
			return EOptimizationWave::Invisible;
		}
	}

	if (DistToPlayer < Proxy->GetDistToWave(EOptimizationWave::FirstWave))
	{
		return EOptimizationWave::NoOptimization;
	}
	else if (DistToPlayer >= Proxy->GetDistToWave(EOptimizationWave::FirstWave) && DistToPlayer < Proxy->GetDistToWave(EOptimizationWave::SecondWave))
	{
		return EOptimizationWave::FirstWave;
	}
	else if (DistToPlayer >= Proxy->GetDistToWave(EOptimizationWave::SecondWave) && DistToPlayer < Proxy->GetDistToWave(EOptimizationWave::ThirdWave))
	{
		return EOptimizationWave::SecondWave;
	}
	else if (DistToPlayer >= Proxy->GetDistToWave(EOptimizationWave::ThirdWave) && DistToPlayer < Proxy->GetMaxVisibleDistance())
	{
		return EOptimizationWave::ThirdWave;
	}

	return EOptimizationWave::Invisible;
}

void FOptimizationThread::RemoveComponentByHandle(int32 Handle)
{
	if (Components.IsValidIndex(Handle))
	{
		if (IsValid(Components[Handle]))
		{
			Components[Handle]->SetOptimizationHandle(INDEX_NONE);
		}

		Components.RemoveAtSwap(Handle, 1, false);

		const int32 NumRemaining = Components.Num();
		if (NumRemaining > 0 && Handle != NumRemaining)
		{
			if (IsValid(Components[Handle]))
			{
				Components[Handle]->SetOptimizationHandle(Handle);
			}
		}
	}
}

FOptimizationManager::FOptimizationManager(UWorld* InWorld) : GameWorld(InWorld), OptimizationThread(MakeUnique<FOptimizationThread>(InWorld, this))
{
	PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &FOptimizationManager::HandlePostGarbageCollect);
}

FOptimizationManager::~FOptimizationManager()
{
	if (OptimizationThread.IsValid())
	{
		OptimizationThread->Exit();
	}

	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGarbageCollectHandle);
}

void FOptimizationManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(GameWorld);
}

FString FOptimizationManager::GetReferencerName() const
{
	return TEXT("FOptimizationManager");
}

void FOptimizationManager::RegisterComponent(UOptimizationProxyComponent* Component) const
{
	if (OptimizationThread.IsValid())
	{
		OptimizationThread->RegisterComponent(Component);
	}
}

void FOptimizationManager::UnregisterComponent(const UOptimizationProxyComponent* Component) const
{
	if (OptimizationThread.IsValid())
	{
		OptimizationThread->UnregisterComponent(Component);
	}
}

void FOptimizationManager::HandlePostGarbageCollect() const
{
	if (OptimizationThread.IsValid())
	{
		OptimizationThread->HandlePostGarbageCollect();
	}
}
