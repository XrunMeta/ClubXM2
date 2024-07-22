// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Structure/PlayerInfoStruct.h"
#include "PlayerInfoSubsystem.generated.h"

class AClubXPlayerState;

/**
 * 
 */
UCLASS()
class CLUBX_API UPlayerInfoSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite)
	FPlayerInfoStruct PlayerInfoStruct;
	
	UPROPERTY(BlueprintReadWrite)
	TMap<TObjectPtr<AClubXPlayerState>, FPlayerInfoStruct> AllPlayersMap;
};
