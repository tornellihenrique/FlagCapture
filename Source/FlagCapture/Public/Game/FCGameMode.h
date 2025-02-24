#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FCTypes.h"
#include "FCGameMode.generated.h"

class AFCPlayableArea;

UCLASS()
class FLAGCAPTURE_API AFCGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	FGameTimerData GameTimers[EGT_Max];

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	float MatchLengthMultiplier;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	float TicketSizeMultiplier;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	float RespirationTimeMultiplier;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	float BulletDamageMultiplier;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	int32 PenaltyPoint;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs", meta = (MetaClass = "/Script/FlagCapture.FCPlayerCharacter"))
	FSoftClassPath CharacterClass;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	int32 MinimumPlayerToStartMatch;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	int32 MaxPlayers;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	float TimeToRemovePlayerBody;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	uint8 bFriendlyFire : 1;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	uint8 bEnableSpectator : 1;

	UPROPERTY(config, EditDefaultsOnly, Category = "Configs")
	uint8 bDevelopmentMode : 1;

public:
	AFCGameMode(const FObjectInitializer& ObjectInitializer);

public:
	//~AActor
	virtual void PreInitializeComponents() override;
	virtual void Reset() override;
	//~AActor

	//~AGameModeBase
	virtual void InitGameState() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, const FString& Options) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void StartPlay() override;
	//~AGameModeBase

	//~AGameMode
	virtual void RestartGame() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void EndMatch() override;
	//~AGameMode

protected:
	UPROPERTY(Transient)
	EGameInitialState GameInitialState;

	UPROPERTY(Transient)
	FString NextURL;

	UPROPERTY(Transient)
	FServerMapAndModeArgs NextMapAndMode;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFCPlayableArea>> PlayableAreas;

	UPROPERTY()
	TObjectPtr<AFCPlayableArea> CurrentArea;

	FTimerHandle TimerHandle_OnEndMatch;
	FTimerHandle TimerHandle_DefaultTimer;
	FTimerHandle TimerHandle_OnInitialStateExpired;

protected:
	virtual void ParseOptions(const FString& Options);
	FServerMapAndModeArgs GetCurrentMapAndMode() const;

	virtual FGameTimerData GetTimerData();

	AActor* FindTeamPlayerStart(AController* Player);

	void SetGameInitialState(EGameInitialState InGameInitialState);
	void UpdateInitialState();
	void DefaultTimer();
	void ServerChangeMap(const FString& URL);
	void UpdateAllPlayerState();

public:
	virtual void OnTimeExpired();
	virtual void OnStateEnded();
	virtual void OnEndMatch();

	virtual void OnNextGame();
	virtual void ProcessNextGame();

	bool CanPlayerJoinTeam(ETeamSide InTeam) const;
	int32 GetNumPlayersInTeam(ETeamSide InTeam) const;
	ETeamSide GetDesiredAutoJoinTeam();
	void PlayerJoinTeam(APlayerController* InController, ETeamSide InTeam);

	void RequestPlayerStart(APlayerController* InController);
	void PlayerSpawn(APlayerController* InController);
	void PlayerSpawnAtDesired(APlayerController* InController, FVector const& SpawnLocation, FRotator const& SpawnRotation);

public:
	UFUNCTION(BlueprintPure, Category = "FC Game Mode")
	FORCEINLINE EGameInitialState GetGameInitialState() { return GameInitialState; }

public:
	virtual void OnFlagCaptured(ACharacter* InCharacter, ETeamSide InTeam);

	virtual void CalcDamage(float& OutDamageAmount, AController* PC, AController* OtherPC);
	virtual void OnCharacterDied(ACharacter* Character, AController* EventInstigator, const FKillEventData& KillEventData);
	virtual void OnPlayerDied(AController* PC, const FKillEventData& KillEventData, APlayerState* KillerPS);
	virtual void OnPlayerKilledEnemy(const FKillEventData& KillEventData, AController* PC, AController* OtherPC);
	virtual void OnPlayerKilledTeammate(const FKillEventData& KillEventData, AController* PC, AController* OtherPC);
	
};
