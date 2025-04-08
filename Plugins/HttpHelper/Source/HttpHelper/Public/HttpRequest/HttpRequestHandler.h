// Copyright 2024 RLoris

#pragma once

#include "Http.h"
#include "Interfaces/IHttpRequest.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Tickable.h"
#include "HttpRequestHandler.generated.h"

UENUM(BlueprintType)
enum class EHttpContentType : uint8
{
	Custom																					UMETA(DisplayName = "Custom"),
	Application_Java__Archive																UMETA(DisplayName = "application/java-archive"),
	Application_EDI__X12																	UMETA(DisplayName = "application/EDI-X12"),
	Application_EDIFACT																		UMETA(DisplayName = "application/EDIFACT"),
	Application_Javascript																	UMETA(DisplayName = "application/javascript"),
	Application_Octet__Stream																UMETA(DisplayName = "application/octet-stream"),
	Application_Ogg																			UMETA(DisplayName = "application/ogg"),
	Application_Pdf																			UMETA(DisplayName = "application/pdf"),
	Application_Xhtml___Xml																	UMETA(DisplayName = "application/xhtml+xml"),
	Application_X__Shockwave__Flash															UMETA(DisplayName = "application/x-shockwave-flash"),
	Application_Json																		UMETA(DisplayName = "application/json"),
	Application_Ld___Json																	UMETA(DisplayName = "application/ld+json"),
	Application_Xml																			UMETA(DisplayName = "application/xml"),
	Application_Zip																			UMETA(DisplayName = "application/zip"),
	Application_X__Www__Form__Urlencoded													UMETA(DisplayName = "application/x-www-form-urlencoded"),
	Audio_Mpeg																				UMETA(DisplayName = "audio/mpeg"),
	Audio_X__Ms__Wma																		UMETA(DisplayName = "audio/x-ms-wma"),
	Audio_Vnd____Rn__Realaudio																UMETA(DisplayName = "audio/vnd.rn-realaudio"),
	Audio_X__Wav																			UMETA(DisplayName = "audio/x-wav"),
	Image_Gif																				UMETA(DisplayName = "image/gif"),
	Image_Jpeg																				UMETA(DisplayName = "image/jpeg"),
	Image_Png																				UMETA(DisplayName = "image/png"),
	Image_Tiff																				UMETA(DisplayName = "image/tiff"),
	Image_Vnd____Microsoft____Icon															UMETA(DisplayName = "image/vnd.microsoft.icon"),
	Image_X__Icon																			UMETA(DisplayName = "image/x-icon"),
	Image_Vnd____Djvu																		UMETA(DisplayName = "image/vnd.djvu"),
	Image_Svg___Xml																			UMETA(DisplayName = "image/svg+xml"),
	Multipart_Mixed																			UMETA(DisplayName = "multipart/mixed"),
	Multipart_Alternative																	UMETA(DisplayName = "multipart/alternative"),
	Multipart_Related																		UMETA(DisplayName = "multipart/related"),
	Multipart_Form__Data																	UMETA(DisplayName = "multipart/form-data"),
	Text_Css																				UMETA(DisplayName = "text/css"),
	Text_Csv																				UMETA(DisplayName = "text/csv"),
	Text_Html																				UMETA(DisplayName = "text/html"),
	Text_Javascript																			UMETA(DisplayName = "text/javascript"),
	Text_Plain																				UMETA(DisplayName = "text/plain"),
	Text_Xml																				UMETA(DisplayName = "text/xml"),
	Video_Mpeg																				UMETA(DisplayName = "video/mpeg"),
	Video_Mp4																				UMETA(DisplayName = "video/mp4"),
	Video_Quicktime																			UMETA(DisplayName = "video/quicktime"),
	Video_X__Ms__Wmv																		UMETA(DisplayName = "video/x-ms-wmv"),
	Video_X__Msvideo																		UMETA(DisplayName = "video/x-msvideo"),
	Video_X__Flv																			UMETA(DisplayName = "video/x-flv"),
	Video_Webm																				UMETA(DisplayName = "video/webm"),
	Application_Vnd____Android____Package__Archive											UMETA(DisplayName = "application/vnd.android.package-archive"),
	Application_Vnd____Oasis____Opendocument____Text										UMETA(DisplayName = "application/vnd.oasis.opendocument.text"),
	Application_Vnd____Oasis____Opendocument____Spreadsheet									UMETA(DisplayName = "application/vnd.oasis.opendocument.spreadsheet"),
	Application_Vnd____Oasis____Opendocument____Presentation								UMETA(DisplayName = "application/vnd.oasis.opendocument.presentation"),
	Application_Vnd____Oasis____Opendocument____Graphics									UMETA(DisplayName = "application/vnd.oasis.opendocument.graphics"),
	Application_Vnd____Ms__Excel															UMETA(DisplayName = "application/vnd.ms-excel"),
	Application_Vnd____Openxmlformats__Officedocument____Spreadsheetml____Sheet				UMETA(DisplayName = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
	Application_Vnd____Ms__Powerpoint														UMETA(DisplayName = "application/vnd.ms-powerpoint"),
	Application_Vnd____Openxmlformats__Officedocument____Presentationml____Presentation		UMETA(DisplayName = "application/vnd.openxmlformats-officedocument.presentationml.presentation"),
	Application_Msword																		UMETA(DisplayName = "application/msword"),
	Application_Vnd____Openxmlformats__Officedocument____Wordprocessingml____Document		UMETA(DisplayName = "application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
	Application_Vnd____Mozilla____Xul___Xml													UMETA(DisplayName = "application/vnd.mozilla.xul+xml"),
};

UENUM(BlueprintType)
enum class EHttpRequestError : uint8
{
	/** No error */
	None			UMETA(DisplayName = "None"),
	/** Network connection error */
	Connection		UMETA(DisplayName = "Connection"),
	/** Body content is invalid */
	InvalidBody		UMETA(DisplayName = "InvalidBody"),
	/** Response object is invalid */
	InvalidResponse UMETA(DisplayName = "InvalidResponse"),
	/** General fail request */
	FailedRequest	UMETA(DisplayName = "FailedRequest"),
	/** Request object is invalid */
	InvalidRequest	UMETA(DisplayName = "InvalidRequest"),
	/** Canceled by timeout */
	TimedOut		UMETA(DisplayName = "TimedOut"),
	/** Canceled by user */
	Canceled		UMETA(DisplayName = "Canceled")
};

UENUM(BlueprintType)
enum class EBodyType : uint8
{
	None   UMETA(DisplayName = "None"),
	Bytes  UMETA(DisplayName = "Bytes"),
	Text   UMETA(DisplayName = "Text"),
	File   UMETA(DisplayName = "File")
};

UENUM(BlueprintType)
enum class EHttpMethod : uint8
{
	Get       UMETA(DisplayName = "GET"),
	Post      UMETA(DisplayName = "POST"),
	Put       UMETA(DisplayName = "PUT"),
	Delete    UMETA(DisplayName = "DELETE"),
	Head      UMETA(DisplayName = "HEAD"),
	Patch	  UMETA(DisplayName = "PATCH")
};

USTRUCT(BlueprintType)
struct FHttpRequestOptions
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	EHttpMethod Method = EHttpMethod::Get;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	FString Url;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	TMap<FString, FString> Headers;

	/** When setting a positive value, a timeout will be used for this request */
	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	float Timeout = -1.f;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	EHttpContentType ContentType = EHttpContentType::Custom;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	EBodyType BodyType = EBodyType::None;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	FString	TextBody;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	TArray<uint8> BytesBody;

	UPROPERTY(BlueprintReadWrite, Category = "HttpHelper|Request")
	FString	FilepathBody;
};

USTRUCT(BlueprintType)
struct FHttpDataContent
{
	GENERATED_BODY()

	FHttpDataContent()
	{}

	explicit FHttpDataContent(const FString& InString)
		: bValidString(true)
		, StringContent(InString)
	{}

	explicit FHttpDataContent(const TArray<uint8>& InBytes)
		: bValidBytes(true)
		, ByteContent(InBytes)
	{}

	explicit FHttpDataContent(const FString& InString, const TArray<uint8>& InBytes)
		: bValidString(true)
		, StringContent(InString)
		, bValidBytes(true)
		, ByteContent(InBytes)
	{}

	void GetContentAsString(FString& OutString) const;
	void GetContentAsByte(TArray<uint8>& OutBytes) const;

protected:
	bool bValidString = false;
	FString StringContent;
	bool bValidBytes = false;
	TArray<uint8> ByteContent;
};

USTRUCT(BlueprintType)
struct FHttpServerSentEvent
{
	GENERATED_BODY()

	static inline const FString ContentType = TEXT("text/event-stream");

	static inline const TArray<uint8> EventFieldBytes = {'e', 'v', 'e', 'n', 't', ':', ' '};

	static inline const TArray<uint8> IdFieldBytes = {'i', 'd', ':', ' '};

	static inline const TArray<uint8> DataFieldBytes = {'d', 'a', 't', 'a', ':', ' '};

	static inline const TArray<uint8> EventEndBytes = {'\n', '\n'};

	static inline const TArray<uint8> EventEndLineBytes = {'\n'};

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FHttpDataContent Event;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FHttpDataContent Id;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FHttpDataContent Data;
};

USTRUCT(BlueprintType)
struct FHttpResponse
{
	GENERATED_BODY();

	friend class UHttpRequestHandler;

	static inline const FString ContentTypeName = TEXT("Content-Type");

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	bool IsFinished = false;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	bool bIsCanceled = false;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	bool bIsTimedOut = false;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	bool IsResponseCodeOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	bool IsServerSentEvent = false;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	int32 Code = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	TMap<FString, FString> Headers;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FString ContentType;

	/** Once the request is done, this will contain the full response content */
	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FHttpDataContent Content;

	/** During the request, when we receive a server sent event, this will be set */
	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FHttpServerSentEvent Event;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	float ElapsedTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	int32 BytesSent = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	int32 BytesReceived = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	float DownloadPercentage = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	float UploadPercentage = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	EHttpRequestError ErrorReason = EHttpRequestError::None;

	UPROPERTY(BlueprintReadOnly, Category = "HttpHelper|Request")
	FText Error;

protected:
	/** Used for server sent events, to only read partial data */
	UPROPERTY()
	int32 ContentOffset = 0;
};

UCLASS(BlueprintType, Blueprintable)
class HTTPHELPER_API UHttpRequestHandler : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHttpRequestOutput, const FHttpResponse&, Response);

	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|Request|Event")
	FOnHttpRequestOutput OnProgress;

	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|Request|Event")
	FOnHttpRequestOutput OnHeaderReceived;

	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|Request|Event")
	FOnHttpRequestOutput OnCompleted;

	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|Request|Event")
	FOnHttpRequestOutput OnFailed;

	UPROPERTY(BlueprintAssignable, Category = "HttpHelper|Request|Event")
	FOnHttpRequestOutput OnEvent;

	UFUNCTION(BlueprintPure, Category = "HttpHelper|Request", meta=(WorldContext="InWorldContext"))
	static UHttpRequestHandler* CreateHttpRequest(const UObject* InWorldContext);

	UFUNCTION(BlueprintPure, Category = "HttpHelper|Request")
	static void GetContentAsString(const FHttpDataContent& InData, FString& OutString);

	UFUNCTION(BlueprintPure, Category = "HttpHelper|Request")
	static void GetContentAsByte(const FHttpDataContent& InData, TArray<uint8>& OutBytes);

	/** Is the request already executing */
	UFUNCTION(BlueprintPure, Category = "HttpHelper|Request")
	bool IsProcessing() const;

	/** Execute this request with these options */
	UFUNCTION(BlueprintCallable, Category = "HttpHelper|Request")
	bool ProcessRequest(const FHttpRequestOptions& InOptions);

	/** Cancel ongoing request */
	UFUNCTION(BlueprintCallable, Category = "HttpHelper|Request")
	bool CancelRequest();

	/** If processing, checks if this response content type is server side event */
	UFUNCTION(BlueprintPure, Category = "HttpHelper|Request")
	bool IsServerSentEvents() const;

	static FString ConvertBytesToString(const TArray<uint8>& InBytes);
	static TArray<uint8> ConvertStringToBytes(const FString& InString);
	static int32 FindSubArrayOffset(const TArray<uint8>& InArray, int32 InStartIdx, const TArray<uint8>& InMatchArray, int32 InEndIdx = -1);

private:
	void OnHttpRequestProgress64(FHttpRequestPtr InRequest, uint64 InSentBytes, uint64 InReceivedBytes);
	void OnHttpRequestProgress(FHttpRequestPtr InRequest, int32 InSentBytes, int32 InReceivedBytes);

	void OnHttpHeaderReceived(FHttpRequestPtr InRequest, const FString& InKey, const FString& InValue);
	void OnHttpRequestCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInSuccess);
	void OnHttpRequestFailed(EHttpRequestError InErrorReason, const FText& InErrorText);
	void OnHttpServerSideEventReceived(const TArray<uint8>& InContent);

	// start FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UHttpRequest, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return IsValid(this) && IsProcessing(); }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	// end FTickableGameObject interface

	FHttpRequestPtr HttpRequest;

	FHttpRequestOptions Options;

	FHttpResponse Result;

	float TimeoutSeconds = -1.f;
};