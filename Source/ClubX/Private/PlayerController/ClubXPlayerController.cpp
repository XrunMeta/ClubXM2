// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/ClubXPlayerController.h"

void AClubXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	/*PlayerCharacter = Cast<AClubXPlayerCharacter>(GetCharacter());
	if (IsValid(PlayerCharacter))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("PlayerCharacter is Valid")));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("PlayerCharacter is NULLPTR")));
	}*/
}

void AClubXPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Broadcast to HUD to get the player state from player controller
	if (!OnRepPlayerState.IsBound()) return;
	OnRepPlayerState.Broadcast(PlayerState);
}