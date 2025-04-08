// Copyright 2024 RLoris

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "WebSocketHandler.h"
#include "WebSocket.generated.h"

/** Errors that could occur during the use of this async action */
UENUM(BlueprintType)
enum class EWebSocketError : uint8
{
	None			UMETA(DisplayName = "None"),
	Activation		UMETA(DisplayName = "Activation"),
	Connection		UMETA(DisplayName = "Connection")
};

/** Output result of this websocket async action */
USTRUCT(BlueprintType)
struct FWebSocketResult
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	TArray<uint8> BytesMessage;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	FString TextMessage;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	int32 Code = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	FString ClosedReason;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	bool ClosedByPeer = false;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	EWebSocketError ErrorReason = EWebSocketError::None;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|WebSocket")
	FString Error;
};

/** Async action for websocket, wrapper around WebsSocketHandler */
UCLASS()
class HTTPHELPER_API UWebSocket : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebSocketOutputPin, const FWebSocketResult&, Result);
	UPROPERTY(BlueprintAssignable)
	FWebSocketOutputPin OnConnected;

	UPROPERTY(BlueprintAssignable)
	FWebSocketOutputPin OnTextMessage;

	UPROPERTY(BlueprintAssignable)
	FWebSocketOutputPin OnBinaryMessage;

	UPROPERTY(BlueprintAssignable)
	FWebSocketOutputPin OnClosed;

	UPROPERTY(BlueprintAssignable)
	FWebSocketOutputPin OnRetry;

	UPROPERTY(BlueprintAssignable)
	FWebSocketOutputPin OnError;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "HttpHelper|WebSocket", WorldContext = "InWorldContextObject"))
	static UWebSocket* AsyncWebSocket(UObject* InWorldContextObject, const FWebSocketOptions& Options, UWebSocketHandler* & Result);

	virtual void Activate() override;

private:
	UFUNCTION()
	void OnWebSocketConnected();

	UFUNCTION()
	void OnWebSocketClosed(const int32& Code, const FString& Reason, bool ClosedByPeer);

	UFUNCTION()
	void OnWebSocketConnectionError(const FString& Reason);

	UFUNCTION()
	void OnWebSocketTextMessage(const FString& Message);

	UFUNCTION()
	void OnWebSocketBinaryMessage(const TArray<uint8>& Message);

	UFUNCTION()
	void OnWebSocketRetry(const int32& RetryCount);

	UPROPERTY()
	TObjectPtr<UWebSocketHandler> WebSocket = nullptr;

	FWebSocketOptions Options;

	bool Active = false;
};
