// Copyright RLoris 2024

#pragma once

#include "Engine/World.h"
#include "HttpRequest/HttpRequestHandler.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "WebSocket/WebSocketHandler.h"
#include "HttpHelperSubsystem.generated.h"

UCLASS()
class HTTPHELPER_API UHttpHelperSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	friend class UHttpRequestHandler;
	friend class UWebSocketHandler;

public:
	static UHttpHelperSubsystem* Get(const UObject* InOuter= GWorld);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	bool RegisterHttpClients(UHttpRequestHandler* InClient);
	bool UnregisterHttpClients(UHttpRequestHandler* InClient);

	bool RegisterWebsocketClients(UWebSocketHandler* InClient);
	bool UnregisterWebsocketClients(UWebSocketHandler* InClient);

	UPROPERTY()
	TSet<TWeakObjectPtr<UHttpRequestHandler>> ActiveHttpClients;

	UPROPERTY()
	TSet<TWeakObjectPtr<UWebSocketHandler>> ActiveWebsocketClients;
};