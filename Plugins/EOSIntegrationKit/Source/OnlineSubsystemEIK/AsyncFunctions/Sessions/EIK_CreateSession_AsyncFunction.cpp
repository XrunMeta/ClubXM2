﻿// Copyright Epic Games, Inc. All Rights Reserved.


#include "EIK_CreateSession_AsyncFunction.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/NetDriver.h"
#include "Online/OnlineSessionNames.h"

void UEIK_CreateSession_AsyncFunction::Activate()
{
	CreateSession();
	Super::Activate();
}

void UEIK_CreateSession_AsyncFunction::CreateSession()
{
	if(IOnlineSubsystem *SubsystemRef = Online::GetSubsystem(this->GetWorld()))
	{
		if(IOnlineSessionPtr SessionPtrRef = SubsystemRef->GetSessionInterface())
		{
			FOnlineSessionSettings SessionCreationInfo;
			SessionCreationInfo.bIsDedicated = DedicatedServerSettings.bIsDedicatedServer;
			SessionCreationInfo.bUsesPresence = ExtraSettings.bUsePresence;
			SessionCreationInfo.bAllowJoinViaPresence = ExtraSettings.bAllowJoinViaPresence;
			SessionCreationInfo.bAllowJoinViaPresenceFriendsOnly = ExtraSettings.bAllowJoinViaPresenceFriendsOnly;
			SessionCreationInfo.bAllowInvites = true;
			if(DedicatedServerSettings.bIsDedicatedServer)
			{
				SessionCreationInfo.bUsesPresence = false;
				SessionCreationInfo.bAllowJoinViaPresence = false;
				SessionCreationInfo.bAllowJoinViaPresenceFriendsOnly = false;
				SessionCreationInfo.bAllowInvites = false;
			}
			SessionCreationInfo.bIsLANMatch = ExtraSettings.bIsLanMatch;
			SessionCreationInfo.NumPublicConnections = NumberOfPublicConnections;
			SessionCreationInfo.NumPrivateConnections = ExtraSettings.NumberOfPrivateConnections;
			SessionCreationInfo.bUseLobbiesIfAvailable = false;
			SessionCreationInfo.bUseLobbiesVoiceChatIfAvailable = false;
			SessionCreationInfo.bShouldAdvertise = ExtraSettings.bShouldAdvertise;
			SessionCreationInfo.bAllowJoinInProgress = ExtraSettings.bAllowJoinInProgress;
			
			SessionCreationInfo.Settings.Add( FName(TEXT("REGIONINFO")), FOnlineSessionSetting(UEnum::GetValueAsString(ExtraSettings.Region), EOnlineDataAdvertisementType::ViaOnlineService));
			if(DedicatedServerSettings.bIsDedicatedServer)
			{
				FString Port = FString::FromInt(DedicatedServerSettings.PortInfo);				
				SessionCreationInfo.Settings.Add( FName(TEXT("PortInfo")), FOnlineSessionSetting(Port, EOnlineDataAdvertisementType::ViaOnlineService));
				SessionCreationInfo.Settings.Add( FName(TEXT("IsDedicatedServer")), FOnlineSessionSetting(true, EOnlineDataAdvertisementType::ViaOnlineService));
			}
			for (auto& Settings_SingleValue : SessionSettings)
			{
				if (Settings_SingleValue.Key.Len() == 0)
				{
					continue;
				}

				FOnlineSessionSetting Setting;
				Setting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineService;
				Setting.Data = Settings_SingleValue.Value.GetVariantData();
				SessionCreationInfo.Set(FName(*Settings_SingleValue.Key), Setting);
			}
			SessionPtrRef->OnCreateSessionCompleteDelegates.AddUObject(this, &UEIK_CreateSession_AsyncFunction::OnCreateSessionCompleted);
			SessionPtrRef->CreateSession(0,VSessionName,SessionCreationInfo);
		}
		else
		{
			if(bDelegateCalled == false)
			{
				OnFail.Broadcast("");
				SetReadyToDestroy();
				MarkAsGarbage();
				bDelegateCalled = true;
			}
		}
	}
	else
	{
		if(bDelegateCalled == false)
		{
			OnFail.Broadcast("");
			bDelegateCalled = true;
			SetReadyToDestroy();
			MarkAsGarbage();
		}
	}
}

void UEIK_CreateSession_AsyncFunction::OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	if(bWasSuccessful)
	{
		if(bDelegateCalled == false)
		{
			const IOnlineSessionPtr Sessions = IOnlineSubsystem::Get()->GetSessionInterface();
			if(const FOnlineSession* CurrentSession = Sessions->GetNamedSession(VSessionName))
			{
				OnSuccess.Broadcast(CurrentSession->SessionInfo.Get()->GetSessionId().ToString());
				bDelegateCalled = true;
				SetReadyToDestroy();
				MarkAsGarbage();
			}
			else
			{
				OnSuccess.Broadcast("");
				bDelegateCalled = true;
				SetReadyToDestroy();
				MarkAsGarbage();
			}
		}
	}
	else
	{
		if(bDelegateCalled == false)
		{
			OnFail.Broadcast("");
			bDelegateCalled = true;
			SetReadyToDestroy();
			MarkAsGarbage();
		}
	}
}

UEIK_CreateSession_AsyncFunction* UEIK_CreateSession_AsyncFunction::CreateEIKSession(
	TMap<FString, FEIKAttribute> SessionSettings,
		FName SessionName,
		int32 NumberOfPublicConnections,
	FDedicatedServerSettings DedicatedServerSettings, FCreateSessionExtraSettings ExtraSettings)
{
	UEIK_CreateSession_AsyncFunction* Ueik_CreateSessionObject= NewObject<UEIK_CreateSession_AsyncFunction>();
	Ueik_CreateSessionObject->SessionSettings = SessionSettings;
	Ueik_CreateSessionObject->NumberOfPublicConnections = NumberOfPublicConnections;
	Ueik_CreateSessionObject->DedicatedServerSettings = DedicatedServerSettings;
	Ueik_CreateSessionObject->VSessionName = SessionName;
	Ueik_CreateSessionObject->ExtraSettings = ExtraSettings;
	return Ueik_CreateSessionObject;
}
