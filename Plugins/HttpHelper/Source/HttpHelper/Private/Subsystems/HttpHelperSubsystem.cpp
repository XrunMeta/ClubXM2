// Copyright RLoris 2024

#include "Subsystems/HttpHelperSubsystem.h"

#include "HttpRequest/HttpRequestHandler.h"
#include "WebSocket/WebSocketHandler.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogHttpHelperSubsystem, Log, All);

UHttpHelperSubsystem* UHttpHelperSubsystem::Get(const UObject* InOuter)
{
	const UWorld* World = nullptr;
	if (InOuter && InOuter->GetWorld())
	{
		World = InOuter->GetWorld();
	}
	else if(GEngine)
	{
		World = GEngine->GetCurrentPlayWorld();
	}

	if (!World)
	{
		World = GWorld;
	}

	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World))
	{
		return GameInstance->GetSubsystem<UHttpHelperSubsystem>();
	}

	return nullptr;
}

void UHttpHelperSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogHttpHelperSubsystem, Log, TEXT("HttpHelper Initialize Subsystem"));

	ActiveHttpClients.Empty();
	ActiveWebsocketClients.Empty();
}

void UHttpHelperSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(LogHttpHelperSubsystem, Log, TEXT("HttpHelper Uninitialize Subsystem"));

	for (const TWeakObjectPtr<UHttpRequestHandler>& HttpClient : ActiveHttpClients.Array())
	{
		if (UHttpRequestHandler* Handler = HttpClient.Get())
		{
			Handler->CancelRequest();
			ActiveHttpClients.Remove(Handler);
		}
	}

	for (const TWeakObjectPtr<UWebSocketHandler>& WebsocketClient : ActiveWebsocketClients.Array())
	{
		if (UWebSocketHandler* Handler = WebsocketClient.Get())
		{
			Handler->Close();
			ActiveWebsocketClients.Remove(Handler);
		}
	}
}

bool UHttpHelperSubsystem::RegisterHttpClients(UHttpRequestHandler* InClient)
{
	if (IsValid(InClient) && !ActiveHttpClients.Contains(InClient))
	{
		ActiveHttpClients.Add(InClient);
		return true;
	}
	return false;
}

bool UHttpHelperSubsystem::UnregisterHttpClients(UHttpRequestHandler* InClient)
{
	return ActiveHttpClients.Remove(InClient) != 0;
}

bool UHttpHelperSubsystem::RegisterWebsocketClients(UWebSocketHandler* InClient)
{
	if (IsValid(InClient) && !ActiveWebsocketClients.Contains(InClient))
	{
		ActiveWebsocketClients.Add(InClient);
		return true;
	}
	return false;
}

bool UHttpHelperSubsystem::UnregisterWebsocketClients(UWebSocketHandler* InClient)
{
	return ActiveWebsocketClients.Remove(InClient) != 0;
}
