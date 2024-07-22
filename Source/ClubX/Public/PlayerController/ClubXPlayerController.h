// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClubXPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class AClubXPlayerCharacter;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRepPlayerState, APlayerState*)

/**
 * 
 */
UCLASS()
class CLUBX_API AClubXPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	FOnRepPlayerState OnRepPlayerState;
	
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;

private:	
	UPROPERTY()
	AClubXPlayerCharacter* PlayerCharacter;
	//TObjectPtr<AClubXPlayerCharacter> PlayerCharacter;
};
