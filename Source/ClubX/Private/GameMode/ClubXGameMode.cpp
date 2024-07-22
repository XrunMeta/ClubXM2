// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/ClubXGameMode.h"

#include "GameFramework/PlayerState.h"
#include "PlayerController/ClubXPlayerController.h"
#include "Subsystem/PlayerInfoSubsystem.h"


void AClubXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	/*if (AClubXPlayerController* PlayerController = Cast<AClubXPlayerController>(NewPlayer))
	{
		if (IsValid(PlayerController))
		{
			PlayerInfoSubsystem = ULocalPlayer::GetSubsystem<UPlayerInfoSubsystem>(PlayerController->GetLocalPlayer());
			if (!IsValid(PlayerInfoSubsystem))
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("PlayerInfoSubsystem nullptr")));
				return;
			}
			
			ListPlayerState.Add(PlayerController->PlayerState);
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("1. ListPlayerState : %s"), *PlayerController->PlayerState->GetActorNameOrLabel()));
		}
	}*/
}
