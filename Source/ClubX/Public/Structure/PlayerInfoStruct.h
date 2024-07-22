#pragma once

//#include "Engine/DataTable.h"
#include "PlayerInfoStruct.generated.h"

class AClubXPlayerState;

USTRUCT(BlueprintType)
struct FPlayerInfoStruct //: public FTableRowBase
{
	//GENERATED_BODY()
	GENERATED_USTRUCT_BODY()

//public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AClubXPlayerState> PlayerState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PhoneNumber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Gender;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IDBackend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Country;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CountryCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Region;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Wallet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DateJoin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool isLockable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Email;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PhoneCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UUID;
};
