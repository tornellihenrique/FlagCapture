#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FCTypes.h"
#include "FCFunctionLibrary.generated.h"

class UFCGameplayAbility;
class AFCGameMode;
class AFCGameState;
class AFCPlayerController;
class AFCPlayerState;
class AFCSpectatorPawn;

UENUM(BlueprintType)
enum class EOutResultPins : uint8
{
	IsValid,
	IsNotValid
};

UCLASS()
class FLAGCAPTURE_API UFCFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FC Function Library", meta = (DisplayName = "Get Game Mode", WorldContext = WorldContextObject, ExpandEnumAsExecs = "OutResult"))
	static void K2_GetGameMode(const UObject* WorldContextObject, AFCGameMode*& OutValue, EOutResultPins& OutResult);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library", meta = (DisplayName = "Get Game State", WorldContext = WorldContextObject, ExpandEnumAsExecs = "OutResult"))
	static void K2_GetGameState(const UObject* WorldContextObject, AFCGameState*& OutValue, EOutResultPins& OutResult);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library", meta = (DisplayName = "Get Player Controller", WorldContext = WorldContextObject, ExpandEnumAsExecs = "OutResult"))
	static void K2_GetPlayerController(const UObject* WorldContextObject, AFCPlayerController*& OutValue, EOutResultPins& OutResult);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library", meta = (DisplayName = "Get Player State", WorldContext = WorldContextObject, ExpandEnumAsExecs = "OutResult"))
	static void K2_GetPlayerState(const UObject* WorldContextObject, AFCPlayerState*& OutValue, EOutResultPins& OutResult);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library", meta = (DisplayName = "Get Player Character", WorldContext = WorldContextObject, ExpandEnumAsExecs = "OutResult"))
	static void K2_GetPlayerCharacter(const UObject* WorldContextObject, AFCCharacter*& OutValue, EOutResultPins& OutResult);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library", meta = (DisplayName = "Get Spectator Pawn", WorldContext = WorldContextObject, ExpandEnumAsExecs = "OutResult"))
	static void K2_GetSpectatorPawn(const UObject* WorldContextObject, AFCSpectatorPawn*& OutValue, EOutResultPins& OutResult);

	UFUNCTION(BlueprintPure, Category = "FC Function Library", meta = (WorldContext = WorldContextObject))
	static void GetSpawnPointsAtLocation(const UObject* WorldContextObject, FVector const& Location, float Radius, int32 NumPoints, float CapsuleHeight, TArray<FTransform>& Out);

	UFUNCTION(BlueprintPure, Category = "FC Function Library", meta = (WorldContext = WorldContextObject))
	static bool CanSpawnAtLocation(const UObject* WorldContextObject, FVector const& Location, float CapsuleRadius, float CapsuleHeight, TArray<AActor*> const& Ignores);

	UFUNCTION(BlueprintPure, Category = "FC Function Library")
	static void ShuffleArrayTransform(TArray<FTransform>& InArray);

	UFUNCTION(BlueprintPure, Category = "FC Function Library")
	static FDamageData GetDamageData(const FKillEventData& KillEventData);

	UFUNCTION(BlueprintPure, Category = "FC Function Library")
	static FLinearColor GetTeamColor(ETeamSide Team);

	UFUNCTION(BlueprintPure, Category = "FC Function Library")
	static FLinearColor GetOppositeTeamColor(ETeamSide Team);

	/**
	* GameplayAbility
	*/
	UFUNCTION(BlueprintCallable, Category = "FC Function Library")
	static UFCGameplayAbility* GetPrimaryAbilityInstanceFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library")
	static UFCGameplayAbility* GetPrimaryAbilityInstanceFromClass(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UGameplayAbility> InAbilityClass);

	UFUNCTION(BlueprintCallable, Category = "FC Function Library")
	static bool IsPrimaryAbilityInstanceActive(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle);

	/**
	* FGameplayAbilitySpecHandle
	*/
	UFUNCTION(BlueprintPure, Category = "FC Function Library")
	static bool IsAbilitySpecHandleValid(FGameplayAbilitySpecHandle Handle);
	
};
