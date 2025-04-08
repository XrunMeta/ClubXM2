#pragma once

//#include "Engine/DataTable.h"
#include "Misc//Guid.h"
#include "PlayerInfoStruct.generated.h"

class AClubXPlayerState;

USTRUCT(BlueprintType)
struct FPlayerInfoStruct //: public FTableRowBase
{
	//GENERATED_BODY()
	GENERATED_USTRUCT_BODY()

//public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AClubXPlayerState> PlayerState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerName = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Nickname = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PhoneNumber = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Gender = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IDBackend = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Country = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CountryCode = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Region = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Status = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Wallet = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DateJoin = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool isLockable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Email = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PhoneCode = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UUID = FString();
};
