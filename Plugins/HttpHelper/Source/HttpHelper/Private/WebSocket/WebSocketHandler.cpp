// Copyright 2024 RLoris

#include "WebSocket/WebSocketHandler.h"

#include "WebSocketsModule.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "Subsystems/HttpHelperSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogWebSocketHandler, Log, All);

UWebSocketHandler* UWebSocketHandler::CreateWebSocket(const UObject* WorldContext)
{
	UWebSocketHandler* Node = NewObject<UWebSocketHandler>();
	Node->WorldContextObject = WorldContext->GetWorld();
	return Node;
}

bool UWebSocketHandler::IsConnected() const
{
	if (Socket != nullptr)
	{
		return Socket->IsConnected();
	}
	return false;
}

bool UWebSocketHandler::Open(const FWebSocketOptions& WebSocketOptions)
{
	if (!(Socket == nullptr || !Socket->IsConnected()))
	{
		UE_LOG(LogWebSocketHandler, Warning, TEXT("WS: Socket connection is already open with a peer"));
		return false;
	}

	Options = WebSocketOptions;

	// protocol
	FString StrProtocol = UEnum::GetValueAsString(Options.Protocol);
	FString Protocol;
	StrProtocol.Split("::", &StrProtocol, &Protocol);
	FWebSocketsModule& Mod = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
	Socket = Mod.Get().CreateWebSocket(Options.Url, Protocol.ToLower(), Options.Headers);

	if (Socket == nullptr)
	{
		UE_LOG(LogWebSocketHandler, Warning, TEXT("WebSocketHandler: Failed to initialize socket"))
		return false;
	}

	// callbacks
	Socket->OnConnected().AddUObject(this, &UWebSocketHandler::OnWebSocketConnected);

	Socket->OnConnectionError().AddUObject(this, &UWebSocketHandler::OnWebSocketConnectionError);

	Socket->OnClosed().AddUObject(this, &UWebSocketHandler::OnWebSocketClosed);

	Socket->OnMessage().AddUObject(this, &UWebSocketHandler::OnWebSocketTextMessage);

	Socket->OnBinaryMessage().AddUObject(this, &UWebSocketHandler::OnWebSocketBinaryMessage);

	Socket->OnMessageSent().AddUObject(this, &UWebSocketHandler::OnWebSocketMessageSent);

	// connect
	Socket->Connect();

	if (UHttpHelperSubsystem* Subsystem = UHttpHelperSubsystem::Get())
	{
		Subsystem->RegisterWebsocketClients(this);
	}

	return true;
}

bool UWebSocketHandler::Close(int32 Code, const FString& Reason)
{
	if (Socket != nullptr && Socket->IsConnected())
	{
		Socket->Close(Code, Reason);
		Buffer.Empty();

		if (UHttpHelperSubsystem* Subsystem = UHttpHelperSubsystem::Get())
		{
			Subsystem->UnregisterWebsocketClients(this);
		}

		return true;
	}
	return false;
}

bool UWebSocketHandler::SendText(const FString& Data)
{
	if (Socket != nullptr && Socket->IsConnected())
	{
		Socket->Send(Data);
		return true;
	}
	return false;
}

bool UWebSocketHandler::SendBytes(const TArray<uint8>& Data, bool IsBinary)
{
	if (Socket != nullptr && Socket->IsConnected())
	{
		Socket->Send(Data.GetData(), sizeof(uint8) * Data.Num(), IsBinary);
		return true;
	}
	return false;
}

void UWebSocketHandler::OnWebSocketConnected()
{
	Buffer.Empty();
	ReconnectAmount = 0;
	if (ReconnectHandle.IsValid())
	{
		ReconnectHandle.Invalidate();
	}
	if (OnConnected.IsBound())
	{
		OnConnected.Broadcast();
	}
}

void UWebSocketHandler::OnWebSocketConnectionError(const FString& Error)
{
	Buffer.Empty();

	if (OnConnectionError.IsBound())
	{
		OnConnectionError.Broadcast(Error);
	}

	if (Options.ReconnectTimeout > 0 && ReconnectAmount < Options.ReconnectAmount)
	{
		if (WorldContextObject != nullptr)
		{
			WorldContextObject->GetTimerManager().SetTimer(ReconnectHandle, [this]()
			{
				ReconnectAmount++;

				if (OnConnectionRetry.IsBound())
				{
					OnConnectionRetry.Broadcast(ReconnectAmount);
				}

				Open(Options);

				UE_LOG(LogWebSocketHandler, Warning, TEXT("WS: Retrying to connect %i"), ReconnectAmount);
			}
			, Options.ReconnectTimeout, false, (-1.0f));
		}
		else
		{
			UE_LOG(LogWebSocketHandler, Warning, TEXT("WS: WorldContext invalid, cannot retry to connect"));
		}
	}
}

void UWebSocketHandler::OnWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	if (OnClosed.IsBound())
	{
		OnClosed.Broadcast(StatusCode, Reason, bWasClean);
	}
}

void UWebSocketHandler::OnWebSocketTextMessage(const FString& Message)
{
	if (OnTextMessage.IsBound())
	{
		OnTextMessage.Broadcast(Message);
	}
}

void UWebSocketHandler::OnWebSocketBinaryMessage(const void* Data, SIZE_T Size, bool bIsLastFragment)
{
	if (OnBinaryMessage.IsBound())
	{
		const uint8* Ptr = static_cast<uint8 const*>(Data);
		Buffer.Append(Ptr, Size);

		if (bIsLastFragment)
		{
			const TArray<uint8> Bytes(Buffer);
			OnBinaryMessage.Broadcast(Bytes);
			Buffer.Empty();
		}
	}
}

void UWebSocketHandler::OnWebSocketMessageSent(const FString& MessageString)
{
	if (OnMessageSent.IsBound())
	{
		OnMessageSent.Broadcast(MessageString);
	}
}
