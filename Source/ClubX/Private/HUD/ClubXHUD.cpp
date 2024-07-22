// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ClubXHUD.h"

#include "Blueprint/UserWidget.h"
#include "PlayerState/ClubXPlayerState.h"
#include "Widgets/ClubXMainWidget.h"


void AClubXHUD::BeginPlay()
{
	Super::BeginPlay();

	MainWidget = CreateWidget<UClubXMainWidget>(GetOwningPlayerController(), MainWidgetRef);
	if (MainWidget)
	{
		MainWidget->AddToViewport();
	}
	
	//AClubXPlayerState* PlayerState = GetOwningPlayerController()->GetPlayerState<AClubXPlayerState>();
}
