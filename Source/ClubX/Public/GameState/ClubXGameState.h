// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ClubXGameState.generated.h"

/**
 * 
 */
UCLASS()
class CLUBX_API AClubXGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	AClubXGameState();

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
