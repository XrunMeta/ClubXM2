// Copyright (C) 2021-2023 Space Raccoon Game Studio. All rights reserved. Contacts: <business@space-raccoon.com>
// Created by AfroStalin

#include "NPC_Optimizator.h"
#include "OptimizationManager.h"

#define LOCTEXT_NAMESPACE "FNPC_OptimizatorModule"

FOptimizationManager* FNPC_OptimizatorModule::GetOptimizationManagerForWorld(UWorld* World)
{
	check(World);

	FOptimizationManager* Manager = WorldOptimizers.FindRef(World);
	if (Manager == nullptr && World->IsGameWorld())
	{
		Manager = new FOptimizationManager(World);
		WorldOptimizers.Add(World, Manager);
	}

	return Manager;
}

void FNPC_OptimizatorModule::StartupModule()
{
	PreWorldInitializationHandle = FWorldDelegates::OnPreWorldInitialization.AddRaw(this, &FNPC_OptimizatorModule::HandleWorldInit);
	PostWorldCleanupHandle = FWorldDelegates::OnPostWorldCleanup.AddRaw(this, &FNPC_OptimizatorModule::HandleWorldCleanup);
}

void FNPC_OptimizatorModule::ShutdownModule()
{
	FWorldDelegates::OnPreWorldInitialization.Remove(PreWorldInitializationHandle);
	FWorldDelegates::OnPostWorldCleanup.Remove(PostWorldCleanupHandle);
}

void FNPC_OptimizatorModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (TPair<TObjectPtr<UWorld>, FOptimizationManager*>& Pair : WorldOptimizers)
	{
		Collector.AddReferencedObject(Pair.Key);
	}
}

FString FNPC_OptimizatorModule::GetReferencerName() const
{
	return TEXT("FNPC_OptimizatorModule");
}

void FNPC_OptimizatorModule::HandleWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
	GetOptimizationManagerForWorld(World);
}

void FNPC_OptimizatorModule::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (const FOptimizationManager* Manager = WorldOptimizers.FindRef(World))
	{
		delete Manager;
		WorldOptimizers.Remove(World);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNPC_OptimizatorModule, NPC_Optimizator)