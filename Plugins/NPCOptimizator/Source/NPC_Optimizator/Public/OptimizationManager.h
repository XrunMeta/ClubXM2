// Copyright (C) 2021-2023 Space Raccoon Game Studio. All rights reserved. Contacts: <business@space-raccoon.com>
// Created by AfroStalin

#pragma once

#include "CoreMinimal.h"
#include "NPC_Optimizator_Types.h"
#include "UObject/ObjectMacros.h"
#include "Engine/World.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"

DECLARE_STATS_GROUP(TEXT("NPC_Optimizator"), STATGROUP_NPC_Optimizator, STATCAT_Advanced);

class FRunnableThread;
class UOptimizationProxyComponent;
class APawn;
class APlayerCameraManager;

class FOptimizationThread : public FRunnable, public FGCObject
{
public:
	FOptimizationThread(UWorld* World, class FOptimizationManager* InManager);
public:
	// FRunnable
	virtual uint32 Run() override;
	virtual void   Exit() override;
	//~FRunnable
public:
	// FGCObject
	virtual void      AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString   GetReferencerName() const override;
	//~FGCObject
public:
	void              RegisterComponent(UOptimizationProxyComponent* Component);
	void              UnregisterComponent(const UOptimizationProxyComponent* Component);
public:
	void              HandlePostGarbageCollect();
private:
	void              RemoveComponentByHandle(int32 Handle);
	static EOptimizationWave GetOptimizationWave(const APawn* LocalPlayer, const APlayerCameraManager* Camera, IOptimizationProxy* Proxy, bool IsVisible);
	static bool              IsInCameraSight(const APlayerCameraManager* Camera, const APawn* LocalPlayer, IOptimizationProxy* Proxy, float InFOV);
protected:
	TArray<TObjectPtr<UOptimizationProxyComponent>> Components;
	
	mutable FCriticalSection    Mutex;
	mutable FCriticalSection    ProxyMutex;
	TUniquePtr<FRunnableThread> Thread;
	FThreadSafeBool             bIsRunning = true;
	TObjectPtr<UWorld>          GameWorld = nullptr;
	TObjectPtr<FOptimizationManager> Manager = nullptr;
};

class FOptimizationManager : public FGCObject
{
public:
	FOptimizationManager(UWorld* InWorld);
	virtual ~FOptimizationManager() override;
public:
	// FGCObject
	virtual void      AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString   GetReferencerName() const override;
	//~FGCObject
public:
	void              RegisterComponent(UOptimizationProxyComponent* Component) const;
	void              UnregisterComponent(const UOptimizationProxyComponent* Component) const;
protected:
	void              HandlePostGarbageCollect() const;
protected:
	FDelegateHandle   OnWorldPreActorTickHandle;
	FDelegateHandle   PostGarbageCollectHandle;
private:
	TObjectPtr<UWorld> GameWorld = nullptr;
	TUniquePtr<FOptimizationThread> OptimizationThread;
	FCriticalSection                Mutex;
};