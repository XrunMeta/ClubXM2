// Copyright 2024 RLoris

#include "HttpRequest/HttpRequest.h"

UHttpRequest* UHttpRequest::AsyncHttpRequest(UObject* WorldContextObject, const FHttpRequestOptions& Options, UHttpRequestHandler*& OutHandler)
{
	UHttpRequest* Node = NewObject<UHttpRequest>();
	Node->Options = Options;
	Node->HttpRequest = UHttpRequestHandler::CreateHttpRequest(WorldContextObject);
	OutHandler = Node->HttpRequest;
	// not garbage collected
	Node->AddToRoot();
	return Node;
}

void UHttpRequest::Activate()
{
	if (Active)
	{
		FFrame::KismetExecutionMessage(TEXT("HttpRequest is already running, cancelling now and retrying"), ELogVerbosity::Warning);
		// cancel request and retry
		HttpRequest->CancelRequest();
		Active = false;
	}

	HttpRequest->OnFailed.AddUniqueDynamic(this, &UHttpRequest::OnRequestFailed);
	HttpRequest->OnCompleted.AddUniqueDynamic(this, &UHttpRequest::OnRequestCompleted);
	HttpRequest->OnProgress.AddUniqueDynamic(this, &UHttpRequest::OnRequestProgress);
	HttpRequest->OnEvent.AddUniqueDynamic(this, &UHttpRequest::OnRequestEvent);

	// Prepare response holder
	Active = HttpRequest->ProcessRequest(Options);
}

void UHttpRequest::OnRequestCompleted(const FHttpResponse& InResult)
{
	Active = false;

	if (Completed.IsBound())
	{
		Completed.Broadcast(InResult);
	}

	if (InResult.IsFinished)
	{
		RemoveFromRoot();
	}
}

void UHttpRequest::OnRequestFailed(const FHttpResponse& InResult)
{
	Active = false;

	if (Failed.IsBound())
	{
		Failed.Broadcast(InResult);
	}

	if (InResult.IsFinished)
	{
		RemoveFromRoot();
	}
}

void UHttpRequest::OnRequestProgress(const FHttpResponse& InResult)
{
	if (Progress.IsBound())
	{
		Progress.Broadcast(InResult);
	}
}

void UHttpRequest::OnRequestEvent(const FHttpResponse& InResult)
{
	if (OnEvent.IsBound())
	{
		OnEvent.Broadcast(InResult);
	}
}
