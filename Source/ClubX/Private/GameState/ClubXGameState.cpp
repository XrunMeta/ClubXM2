// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/ClubXGameState.h"

#include "Net/UnrealNetwork.h"


AClubXGameState::AClubXGameState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AClubXGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	
}

void AClubXGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	
}

void AClubXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Example Replicated variable in c++
	//DOREPLIFETIME(AClubXGameState, SomeVariableToBeReplicated);
}
