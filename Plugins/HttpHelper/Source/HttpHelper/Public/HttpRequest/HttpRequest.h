// Copyright 2024 RLoris

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "HttpRequestHandler.h"
#include "HttpRequest.generated.h"

UCLASS()
class HTTPHELPER_API UHttpRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutputPin, const FHttpResponse&, Result);

	UPROPERTY(BlueprintAssignable)
	FOutputPin Progress;

	UPROPERTY(BlueprintAssignable)
	FOutputPin Completed;

	UPROPERTY(BlueprintAssignable)
	FOutputPin Failed;

	UPROPERTY(BlueprintAssignable)
	FOutputPin OnEvent;

	UFUNCTION(BlueprintCallable, Category = "HttpHelper|Request", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UHttpRequest* AsyncHttpRequest(UObject* WorldContextObject, const FHttpRequestOptions& Options, UHttpRequestHandler*& OutHandler);
	virtual void Activate() override;

private:
	UFUNCTION()
	void OnRequestCompleted(const FHttpResponse& InResult);

	UFUNCTION()
	void OnRequestFailed(const FHttpResponse& InResult);

	UFUNCTION()
	void OnRequestProgress(const FHttpResponse& InResult);

	UFUNCTION()
	void OnRequestEvent(const FHttpResponse& InResult);

	UPROPERTY()
	TObjectPtr<UHttpRequestHandler> HttpRequest = nullptr;

	FHttpRequestOptions Options;

	bool Active = false;
};
