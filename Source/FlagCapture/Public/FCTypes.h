#pragma once

#include "GenericTeamAgentInterface.h"

#include "FCTypes.generated.h"

#define FC_PRINT_FILE (FString(FPaths::GetCleanFilename(TEXT(__FILE__))))
#define FC_PRINT_FUNC (FString(__FUNCTION__))
#define FC_PRINT_LINE (FString::FromInt(__LINE__))
#define FC_LOGS_LINE (FC_PRINT_FUNC + TEXT(" [") + FC_PRINT_FILE + TEXT(":") + FC_PRINT_LINE + TEXT("]"))

#define COLLISION_ABILITY						ECollisionChannel::ECC_GameTraceChannel1
#define COLLISION_PROJECTILE					ECollisionChannel::ECC_GameTraceChannel2
#define COLLISION_ABILITYOVERLAPPROJECTILE		ECollisionChannel::ECC_GameTraceChannel3
#define COLLISION_PICKUP						ECollisionChannel::ECC_GameTraceChannel4

inline int32 GenericTeamIdToInteger(FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam) ? INDEX_NONE : (int32)ID;
}

inline FGenericTeamId IntegerToGenericTeamId(int32 ID)
{
	return (ID == INDEX_NONE) ? FGenericTeamId::NoTeam : FGenericTeamId((uint8)ID);
}

UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None			UMETA(DisplayName = "None"),
	Ability1		UMETA(DisplayName = "Ability 1"),
	Ability2		UMETA(DisplayName = "Ability 2"),
	Ability3		UMETA(DisplayName = "Ability 3"),
	Ability4		UMETA(DisplayName = "Ability 4"),
	Sprint			UMETA(DisplayName = "Sprint"),
	Jump			UMETA(DisplayName = "Jump"),
	Reload			UMETA(DisplayName = "Reload"),
	NextWeapon		UMETA(DisplayName = "NextWeapon"),
	PreviousWeapon	UMETA(DisplayName = "PreviousWeapon"),

	Confirm			UMETA(DisplayName = "Confirm"),
	Cancel			UMETA(DisplayName = "Cancel"),
};

UENUM(BlueprintType)
enum class ETeamSide : uint8
{
	None			UMETA(DisplayName = "None"),
	Team_A			UMETA(DisplayName = "Team A"),
	Team_B			UMETA(DisplayName = "Team B")
};

UENUM(BlueprintType)
enum EGameTimer
{
	EGT_Listen		UMETA(DisplayName = "Listen"),
	EGT_Waiting		UMETA(DisplayName = "Waiting"),
	EGT_Preparing	UMETA(DisplayName = "Preparing"),
	EGT_InProgress	UMETA(DisplayName = "In Progress"),
	EGT_Ended		UMETA(DisplayName = "Ended"),
	EGT_NextGame	UMETA(DisplayName = "Next Game"),

	EGT_Max			UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EControllerState : uint8
{
	None			UMETA(DisplayName = "None"),
	Register		UMETA(DisplayName = "Register"),
	Spawn			UMETA(DisplayName = "Spawn"),
	Playing			UMETA(DisplayName = "Playing"),
	Death			UMETA(DisplayName = "Death"),
	Spectating		UMETA(DisplayName = "Spectating"),
	MatchEnded		UMETA(DisplayName = "Match Ended")
};

UENUM(BlueprintType)
enum class EGameInitialState : uint8
{
	Listen			UMETA(DisplayName = "Listen"),
	Waiting			UMETA(DisplayName = "Waiting"),
	Preparing		UMETA(DisplayName = "Preparing"),
	InProgress		UMETA(DisplayName = "In Progress"),
	Ended			UMETA(DisplayName = "Ended"),
	NextGame		UMETA(DisplayName = "Next Game")
};

UENUM(BlueprintType)
enum EPlayerClassType
{
	EPCT_Class1			UMETA(DisplayName = "Class 1"),
	EPCT_Class2			UMETA(DisplayName = "Class 2"),
	EPCT_Class3			UMETA(DisplayName = "Class 3"),

	EPCT_Max			UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EDamageResultType : uint8
{
	EDRT_Normal			UMETA(DisplayName = "Normal"),
	EDRT_Critical		UMETA(DisplayName = "Critical"),
	EDRT_Headshoot		UMETA(DisplayName = "Headshoot"),
};

USTRUCT(BlueprintType)
struct FServerMapAndModeArgs
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (ExposeOnSpawn = "true"))
	FString LevelPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (ExposeOnSpawn = "true"))
	FString GameMode;

	FServerMapAndModeArgs()
		: LevelPath(TEXT(""))
		, GameMode(TEXT(""))
	{}

	FServerMapAndModeArgs(FString InLevelPath, FString InGameMode)
		: LevelPath(InLevelPath)
		, GameMode(InGameMode)
	{}

	bool operator==(const FServerMapAndModeArgs& OtherServerMapAndModeArgs) const
	{
		return OtherServerMapAndModeArgs.GameMode.Equals(GameMode) && OtherServerMapAndModeArgs.LevelPath.Equals(LevelPath);
	}

	bool IsValid() const
	{
		return !LevelPath.IsEmpty() && !GameMode.IsEmpty();
	}
};

USTRUCT(BlueprintType)
struct FGameTimerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (ExposeOnSpawn = "true"))
	float TimerLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (ExposeOnSpawn = "true"))
	float DelayBeforeNextState;

	FGameTimerData()
		: TimerLength(0.0f)
		, DelayBeforeNextState(0.0f)
	{}

	FGameTimerData(float InTimerLength, float InDelayBeforeNextTask)
		: TimerLength(InTimerLength)
		, DelayBeforeNextState(InDelayBeforeNextTask)
	{}
};

USTRUCT(BlueprintType)
struct FKillEventData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY()
	FActorInstanceHandle DamageCauserHandle;

	UPROPERTY()
	uint8 bIsHeadshot : 1;

	FKillEventData()
		: DamageType(nullptr)
		, DamageCauserHandle(FActorInstanceHandle())
	{}

	bool IsValid() const
	{
		return DamageType != nullptr && GetDamageCauser() != nullptr;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << DamageType;

		if (Ar.IsLoading() && Ar.EngineNetVer() < FEngineNetworkCustomVersion::HitResultInstanceHandle)
		{
			AActor* DamageCauser = nullptr;
			Ar << DamageCauser;
			DamageCauserHandle = DamageCauser;
		}
		else
		{
			Ar << DamageCauserHandle;
		}

		uint8 Flag = (bIsHeadshot & 0x01);
		Ar.SerializeBits(&Flag, 1);
		bIsHeadshot = (Flag & 0x01) != 0 ? 1 : 0;

		bOutSuccess = IsValid();

		return bOutSuccess;
	}

	UDamageType* GetDamageType() const
	{
		return DamageType.GetDefaultObject();
	}

	AActor* GetDamageCauser() const
	{
		return DamageCauserHandle.FetchActor();
	}
};

USTRUCT(BlueprintType)
struct FDamageData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UTexture2D> KillFeedIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UTexture2D> DisplayImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bIsHeadshot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 KillReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 HeadshotReward;

	FDamageData()
		: DisplayName(FText::FromString(TEXT("")))
		, KillFeedIcon(nullptr)
		, DisplayImage(nullptr)
		, bIsHeadshot(false)
		, KillReward(100)
		, HeadshotReward(50)
	{}

	FDamageData(FText InDisplayName, UTexture2D* InKillFeedIcon, UTexture2D* InDisplayImage, bool InHeadshot, int32 InKillReward, int32 InHeadshotReward)
		: DisplayName(InDisplayName)
		, KillFeedIcon(InKillFeedIcon)
		, DisplayImage(InDisplayImage)
		, bIsHeadshot(InHeadshot)
		, KillReward(InKillReward)
		, HeadshotReward(InHeadshotReward)
	{}

	bool IsValid() const
	{
		if (DisplayImage && KillFeedIcon)
		{
			return true;
		}

		return false;
	}
};

USTRUCT(BlueprintType)
struct FPlayerLoadout
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (MetaClass = "/Script/FlagCapture.FCWeapon"))
	TArray<FSoftClassPath> Weapons;

	FPlayerLoadout()
		: Weapons({})
	{}
};

USTRUCT(BlueprintType)
struct FClassLoadout
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FPlayerLoadout Loadout;

	FClassLoadout()
	{}
};