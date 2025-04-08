// Copyright 2024 RLoris

#pragma once

#include "UObject/NoExportTypes.h"
#include "IWebSocket.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "WebSocketHandler.generated.h"

UENUM(BlueprintType)
enum class EWebSocketProtocol : uint8
{
	WS       UMETA(DisplayName = "WS"),
	WSS      UMETA(DisplayName = "WSS")
};

USTRUCT(BlueprintType)
struct FWebSocketOptions
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|WebSocket")
	EWebSocketProtocol Protocol = EWebSocketProtocol::WS;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|WebSocket")
	FString Url;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|WebSocket")
	TMap<FString, FString> Headers;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|WebSocket")
	float ReconnectTimeout = 0;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|WebSocket")
	int32 ReconnectAmount = 3;
};

UCLASS(BlueprintType, Blueprintable)
class HTTPHELPER_API UWebSocketHandler : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectedDelegate);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnConnectedDelegate OnConnected;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnClosedDelegate, const int32&, Code, const FString&, Reason, bool, ClosedByPeer);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnClosedDelegate OnClosed;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionErrorDelegate, const FString&, ErrorReason);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnConnectionErrorDelegate OnConnectionError;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextMessageDelegate, const FString&, Message);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnTextMessageDelegate OnTextMessage;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBinaryMessageDelegate, const TArray<uint8>&, Message);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnBinaryMessageDelegate OnBinaryMessage;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageSentDelegate, const FString&, Message);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnMessageSentDelegate OnMessageSent;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRetryDelegate, const int32&, RetryCount);
	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|WebSocket|Event")
	FOnRetryDelegate OnConnectionRetry;

	UFUNCTION(BlueprintPure, Category = "HttpHelper|WebSocket", meta=(WorldContext="WorldContext"))
	static UWebSocketHandler* CreateWebSocket(const UObject* WorldContext);

	UFUNCTION(BlueprintPure, Category = "HttpHelper|WebSocket")
	bool IsConnected() const;

	UFUNCTION(BlueprintCallable, Category = "HttpHelper|WebSocket")
	bool Open(const FWebSocketOptions& Options);

	UFUNCTION(BlueprintCallable, Category = "HttpHelper|WebSocket")
	bool Close(int32 Code = 1000, const FString& Reason = "");

	UFUNCTION(BlueprintCallable, Category = "HttpHelper|WebSocket")
	bool SendText(const FString& Data);

	UFUNCTION(BlueprintCallable, Category = "HttpHelper|WebSocket")
	bool SendBytes(const TArray<uint8>& Data, bool IsBinary = false);

private:
	void OnWebSocketConnected();
	void OnWebSocketConnectionError(const FString& Error);
	void OnWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void OnWebSocketTextMessage(const FString& Message);
	void OnWebSocketBinaryMessage(const void* Data, SIZE_T Size, bool bIsLastFragment);
	void OnWebSocketMessageSent(const FString& MessageString);

	UPROPERTY()
	TObjectPtr<UWorld> WorldContextObject = nullptr;

	TSharedPtr<IWebSocket> Socket;

	FWebSocketOptions Options;

	TArray<uint8> Buffer;

	FTimerHandle ReconnectHandle;

	int32 ReconnectAmount = 0;
};
