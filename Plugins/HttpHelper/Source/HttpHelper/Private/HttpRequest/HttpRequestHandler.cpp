// Copyright 2024 RLoris

#include "HttpRequest/HttpRequestHandler.h"

#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Subsystems/HttpHelperSubsystem.h"

#define LOCTEXT_NAMESPACE "HttpRequestHandler"

DEFINE_LOG_CATEGORY_STATIC(LogHttpRequestHandler, Log, All);

void FHttpDataContent::GetContentAsString(FString& OutString) const
{
	OutString = FString();

	if (bValidString)
	{
		OutString = StringContent;
	}

	if (bValidBytes)
	{
		const_cast<FHttpDataContent*>(this)->StringContent = UHttpRequestHandler::ConvertBytesToString(ByteContent);
		const_cast<FHttpDataContent*>(this)->bValidString = true;
		OutString = StringContent;
	}
}

void FHttpDataContent::GetContentAsByte(TArray<uint8>& OutBytes) const
{
	OutBytes = TArray<uint8>();

	if (bValidBytes)
	{
		OutBytes = ByteContent;
	}

	if (bValidString)
	{
		const_cast<FHttpDataContent*>(this)->ByteContent = UHttpRequestHandler::ConvertStringToBytes(StringContent);
		const_cast<FHttpDataContent*>(this)->bValidBytes = true;
		OutBytes = ByteContent;
	}
}

UHttpRequestHandler* UHttpRequestHandler::CreateHttpRequest(const UObject* InWorldContext)
{
	UHttpRequestHandler* Node = NewObject<UHttpRequestHandler>();
	return Node;
}

void UHttpRequestHandler::GetContentAsString(const FHttpDataContent& InData, FString& OutString)
{
	InData.GetContentAsString(OutString);
}

void UHttpRequestHandler::GetContentAsByte(const FHttpDataContent& InData, TArray<uint8>& OutBytes)
{
	InData.GetContentAsByte(OutBytes);
}

bool UHttpRequestHandler::IsProcessing() const
{
	return HttpRequest.IsValid() && HttpRequest->GetStatus() == EHttpRequestStatus::Processing;
}

bool UHttpRequestHandler::ProcessRequest(const FHttpRequestOptions& InOptions)
{
	// Request already in progress
	if (IsProcessing())
	{
		UE_LOG(LogHttpRequestHandler, Warning, TEXT("Http request is already processing"));
		return false;
	}

	if (InOptions.Url.IsEmpty())
	{
		OnHttpRequestFailed(EHttpRequestError::InvalidRequest, LOCTEXT("InvalidURL", "Invalid URL provided for the request"));
		return false;
	}

	// Reset
	Options = InOptions;
	Result = FHttpResponse();

	// Create HTTP Request
	HttpRequest.Reset();
	HttpRequest = FHttpModule::Get().CreateRequest();

	// Verb / Action
	FString StrMethod = UEnum::GetValueAsString(Options.Method);
	FString Verb;
	StrMethod.Split("::", &StrMethod, &Verb);
	HttpRequest->SetVerb(Verb.ToUpper());

	// Headers
	for (const TPair<FString,FString>& Header : Options.Headers)
	{
		HttpRequest->SetHeader(Header.Key, Header.Value);
	}

	// Url
	HttpRequest->SetURL(Options.Url);

	// body
	if (Options.BodyType != EBodyType::None)
	{
		// Content type header
		if (Options.ContentType != EHttpContentType::Custom)
		{
			FString ContentType = UEnum::GetDisplayValueAsText(Options.ContentType).ToString();

			// fix android
			if (ContentType.Contains(TEXT("_")))
			{
				ContentType.ReplaceInline(TEXT("____"), TEXT("."));
				ContentType.ReplaceInline(TEXT("___"), TEXT("+"));
				ContentType.ReplaceInline(TEXT("__"), TEXT("-"));
				ContentType.ReplaceInline(TEXT("_"), TEXT("/"));
				ContentType.ToLowerInline();
			}

			HttpRequest->SetHeader(TEXT("Content-Type"), ContentType);
		}

		// check content type for body
		if (HttpRequest->GetHeader(TEXT("Content-Type")).IsEmpty())
		{
			OnHttpRequestFailed(EHttpRequestError::InvalidRequest, LOCTEXT("MissingContentType", "Content-Type header is missing, cannot complete request"));
			return false;
		}

		switch (Options.BodyType)
		{
			case EBodyType::Bytes:
				HttpRequest->SetContent(Options.BytesBody);
			break;
			case EBodyType::Text:
				HttpRequest->SetContentAsString(Options.TextBody);
			break;
			case EBodyType::File:
				if (!HttpRequest->SetContentAsStreamedFile(Options.FilepathBody))
				{
					OnHttpRequestFailed(EHttpRequestError::InvalidBody, LOCTEXT("InvalidFile", "Invalid filepath for http payload, cannot complete request"));
					return false;
				}
			break;
			default: ;
		}
	}

	// start request
	TimeoutSeconds = Options.Timeout;

	// callback progress
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	HttpRequest->OnRequestProgress64().BindUObject(this, &UHttpRequestHandler::OnHttpRequestProgress64);
#else
	HttpRequest->OnRequestProgress().BindUObject(this, &UHttpRequestHandler::OnHttpRequestProgress);
#endif

	// callback headers
	HttpRequest->OnHeaderReceived().BindUObject(this, &UHttpRequestHandler::OnHttpHeaderReceived);

	// callback complete
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UHttpRequestHandler::OnHttpRequestCompleted);

	if (UHttpHelperSubsystem* Subsystem = UHttpHelperSubsystem::Get())
	{
		Subsystem->RegisterHttpClients(this);
	}

	return HttpRequest->ProcessRequest();
}

bool UHttpRequestHandler::CancelRequest()
{
	if (IsProcessing())
	{
		Result.bIsCanceled = true;
		HttpRequest->CancelRequest();

		return true;
	}

	return false;
}

bool UHttpRequestHandler::IsServerSentEvents() const
{
	return IsProcessing() && !Result.ContentType.IsEmpty() && Result.ContentType.StartsWith(FHttpServerSentEvent::ContentType);
}

void UHttpRequestHandler::OnHttpRequestProgress(FHttpRequestPtr InRequest, int32 InSentBytes, int32 InReceivedBytes)
{
	OnHttpRequestProgress64(InRequest, static_cast<uint64>(InSentBytes), static_cast<uint64>(InReceivedBytes));
}

void UHttpRequestHandler::OnHttpRequestProgress64(FHttpRequestPtr InRequest, uint64 InSentBytes, uint64 InReceivedBytes)
{
	Result.BytesReceived = InReceivedBytes;
	Result.BytesSent = InSentBytes;

	// upload
	const int32 UploadLength = InRequest->GetContent().Num();
	const FHttpResponsePtr Response = InRequest->GetResponse();

	if (UploadLength != 0)
	{
		Result.UploadPercentage = ((InSentBytes * 1.0) / (UploadLength * 1.0)) * 100.f;
	}

	// download
	if (Response.IsValid())
	{
		const int32 DownloadLength = Response->GetContentLength();

		if (DownloadLength != 0)
		{
			Result.DownloadPercentage = (static_cast<float>(InReceivedBytes) / static_cast<float>(DownloadLength)) * 100.0f;
		}

		if (IsServerSentEvents())
		{
			Result.IsServerSentEvent = true;

			// Get content as bytes instead of string to avoid making a copy,
			// when these events could be infinite but memory is not
			OnHttpServerSideEventReceived(Response->GetContent());
		}
	}

	if (OnProgress.IsBound())
	{
		OnProgress.Broadcast(Result);
	}
}

void UHttpRequestHandler::OnHttpHeaderReceived(FHttpRequestPtr InRequest, const FString& InKey, const FString& InValue)
{
	Result.Headers.Add(InKey, InValue);

	if (InKey == FHttpResponse::ContentTypeName)
	{
		Result.ContentType = InValue;
	}

	if (OnHeaderReceived.IsBound())
	{
		OnHeaderReceived.Broadcast(Result);
	}
}

void UHttpRequestHandler::OnHttpRequestCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInSuccess)
{
	if (!InRequest.IsValid())
	{
		OnHttpRequestFailed(EHttpRequestError::InvalidRequest, LOCTEXT("RequestInvalid", "Invalid request object, cannot complete request"));
		return;
	}

	Result.ElapsedTime = InRequest->GetElapsedTime();
	Result.Status = EHttpRequestStatus::ToString(InRequest->GetStatus());
	Result.IsFinished = EHttpRequestStatus::IsFinished(InRequest->GetStatus());

	if (InResponse.IsValid())
	{
		Result.Code = InResponse->GetResponseCode();
		Result.ContentType = InResponse->GetContentType();
		Result.IsResponseCodeOk = EHttpResponseCodes::IsOk(InResponse->GetResponseCode());
		Result.Content = FHttpDataContent(InResponse->GetContentAsString(), InResponse->GetContent());
	}

	if (bInSuccess)
	{
		if (OnCompleted.IsBound())
		{
			OnCompleted.Broadcast(Result);
		}

		if (UHttpHelperSubsystem* Subsystem = UHttpHelperSubsystem::Get())
		{
			Subsystem->UnregisterHttpClients(this);
		}
	}
	else
	{
		EHttpRequestError ErrorReason;
		FText ErrorText;

		if (Result.bIsTimedOut
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
			|| (InRequest->GetStatus() == EHttpRequestStatus::Failed && InRequest->GetFailureReason() == EHttpFailureReason::TimedOut)
#endif
			)
		{
			ErrorReason = EHttpRequestError::TimedOut;
			ErrorText = LOCTEXT("RequestTimeout", "Request canceled because timed out");
		}
		else if (Result.bIsCanceled
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
			|| (InRequest->GetStatus() == EHttpRequestStatus::Failed && InRequest->GetFailureReason() == EHttpFailureReason::Cancelled)
#endif
			)
		{
			ErrorReason = EHttpRequestError::Canceled;
			ErrorText = LOCTEXT("RequestCanceled", "Request canceled by user");
		}
		else if (
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
			InRequest->GetStatus() == EHttpRequestStatus::Failed && InRequest->GetFailureReason() == EHttpFailureReason::ConnectionError
#else
			InRequest->GetStatus() == EHttpRequestStatus::Failed_ConnectionError
#endif
			)
		{
			ErrorReason = EHttpRequestError::Connection;
			ErrorText = LOCTEXT("ConnectionError", "Request failed due to the network connection");
		}
		else if (!InResponse.IsValid())
		{
			ErrorReason = EHttpRequestError::InvalidResponse;
			ErrorText = LOCTEXT("InvalidResponse", "Invalid response object, cannot complete request");
		}
		else
		{
			ErrorReason = EHttpRequestError::FailedRequest;
			ErrorText = LOCTEXT("FailedRequest", "Request completed with a fail status");
		}

		OnHttpRequestFailed(ErrorReason, ErrorText);
	}
}

void UHttpRequestHandler::OnHttpRequestFailed(EHttpRequestError InErrorReason, const FText& InErrorText)
{
	UE_LOG(LogHttpRequestHandler, Warning, TEXT("Http request failed with code %i : %s"), InErrorReason, *InErrorText.ToString());

	Result.ErrorReason = InErrorReason;
	Result.Error = InErrorText;

	if (OnFailed.IsBound())
	{
		OnFailed.Broadcast(Result);
	}

	if (UHttpHelperSubsystem* Subsystem = UHttpHelperSubsystem::Get())
	{
		Subsystem->UnregisterHttpClients(this);
	}
}

void UHttpRequestHandler::OnHttpServerSideEventReceived(const TArray<uint8>& InContent)
{
	if (!InContent.IsValidIndex(Result.ContentOffset))
	{
	    return;
	}

	Result.Event = FHttpServerSentEvent();
	int32 SearchStartOffset = Result.ContentOffset;
	int32 EndEventOffset = FindSubArrayOffset(InContent, SearchStartOffset, FHttpServerSentEvent::EventEndBytes);

	// event did not load fully
	if (EndEventOffset == INDEX_NONE)
	{
		return;
	}

	EndEventOffset += FHttpServerSentEvent::EventEndBytes.Num();

	bool bValidEvent = false;
	int32 EventFieldOffset = FindSubArrayOffset(InContent, SearchStartOffset, FHttpServerSentEvent::EventFieldBytes, EndEventOffset);
	int32 IdFieldOffset = FindSubArrayOffset(InContent, SearchStartOffset, FHttpServerSentEvent::IdFieldBytes, EndEventOffset);
	int32 DataFieldOffset = FindSubArrayOffset(InContent, SearchStartOffset, FHttpServerSentEvent::DataFieldBytes, EndEventOffset);

	// Optional "event" field
	if (EventFieldOffset != INDEX_NONE)
	{
		EventFieldOffset += FHttpServerSentEvent::EventFieldBytes.Num();

		int32 EventFieldEndOffset = FindSubArrayOffset(InContent, EventFieldOffset, FHttpServerSentEvent::EventEndLineBytes, EndEventOffset);

		if (EventFieldEndOffset != INDEX_NONE)
		{
			const TArray<uint8> EventBytes(InContent.GetData() + EventFieldOffset, EventFieldEndOffset - EventFieldOffset);
			Result.Event.Event = FHttpDataContent(EventBytes);
		}
	}

	// Optional "id" field
	if (IdFieldOffset != INDEX_NONE)
	{
		IdFieldOffset += FHttpServerSentEvent::IdFieldBytes.Num();

		int32 IdFieldEndOffset = FindSubArrayOffset(InContent, IdFieldOffset, FHttpServerSentEvent::EventEndLineBytes, EndEventOffset);

		if (IdFieldEndOffset != INDEX_NONE)
		{
			const TArray<uint8> IdBytes(InContent.GetData() + IdFieldOffset, IdFieldEndOffset - IdFieldOffset);
			Result.Event.Id = FHttpDataContent(IdBytes);
		}
	}

	// Required "data" field
	if (DataFieldOffset != INDEX_NONE)
	{
		DataFieldOffset += FHttpServerSentEvent::DataFieldBytes.Num();

		int32 DataFieldEndOffset = FindSubArrayOffset(InContent, DataFieldOffset, FHttpServerSentEvent::EventEndLineBytes, EndEventOffset);

		if (DataFieldEndOffset != INDEX_NONE)
		{
			const TArray<uint8> DataBytes(InContent.GetData() + DataFieldOffset, DataFieldEndOffset - DataFieldOffset);
			Result.Event.Data = FHttpDataContent(DataBytes);
			bValidEvent = true;
		}
	}

	Result.ContentOffset = EndEventOffset;

	if (bValidEvent)
	{
		OnEvent.Broadcast(Result);
	}
}

void UHttpRequestHandler::Tick(float DeltaTime)
{
	if (!HttpRequest.IsValid())
	{
		return;
	}

	if (TimeoutSeconds > 0.f)
	{
		TimeoutSeconds = FMath::Max<float>(TimeoutSeconds - DeltaTime, 0.f);

		if (TimeoutSeconds <= 0.f && IsProcessing())
		{
			Result.bIsTimedOut = true;
			CancelRequest();
		}
	}

	HttpRequest->Tick(DeltaTime);
}

FString UHttpRequestHandler::ConvertBytesToString(const TArray<uint8>& InBytes)
{
	const FUTF8ToTCHAR EventChars(reinterpret_cast<const ANSICHAR*>(InBytes.GetData()), InBytes.Num());
	return FString(EventChars.Length(), EventChars.Get());
}

TArray<uint8> UHttpRequestHandler::ConvertStringToBytes(const FString& InString)
{
	TArray<uint8> Bytes;
	const FTCHARToUTF8 Converter(*InString);
	const char* CharArray = Converter.Get();
	const int32 NumBytes = FCStringAnsi::Strlen(CharArray);
	Bytes.AddUninitialized(NumBytes);
	FMemory::Memcpy(Bytes.GetData(), CharArray, NumBytes);
	return Bytes;
}

int32 UHttpRequestHandler::FindSubArrayOffset(const TArray<uint8>& InArray, int32 InStartIdx, const TArray<uint8>& InMatchArray, int32 InEndIdx)
{
	if (InStartIdx < 0 || InMatchArray.IsEmpty() || InArray.Num() < InMatchArray.Num())
	{
		return INDEX_NONE;
	}

	if (InEndIdx == INDEX_NONE)
	{
		InEndIdx = InArray.Num();
	}

	if (InEndIdx > InArray.Num() || InEndIdx - InMatchArray.Num() <= InStartIdx)
	{
		return INDEX_NONE;
	}

	int32 MatchArrayIdx = 0;
	for (int32 Index = InStartIdx; Index < InEndIdx; ++Index)
	{
		if (InArray[Index] == InMatchArray[MatchArrayIdx])
		{
			if (MatchArrayIdx == InMatchArray.Num() - 1)
			{
				return Index - MatchArrayIdx;
			}

			MatchArrayIdx++;
		}
		else
		{
			MatchArrayIdx = 0;
		}
	}

	// Return -1 if the subarray was not found
	return INDEX_NONE;
}

#undef LOCTEXT_NAMESPACE
