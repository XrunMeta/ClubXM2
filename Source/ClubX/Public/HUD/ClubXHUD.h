// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ClubXHUD.generated.h"

class UClubXMainWidget;
/**
 * 
 */
UCLASS()
class CLUBX_API AClubXHUD : public AHUD
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Reference")
	TSubclassOf<UClubXMainWidget> MainWidgetRef;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UClubXMainWidget> MainWidget;
};
