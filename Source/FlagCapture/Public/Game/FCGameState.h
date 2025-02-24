#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "FCTypes.h"
#include "FCGameState.generated.h"

class AFCPlayerState;
class AFCSpawnArea;
class AFCPlayableArea;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStateEvent, APlayerState*, PlayerState, bool, bRemove);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChangedEvent, ETeamSide, Team);

UCLASS()
class FLAGCAPTURE_API AFCGameState : public AGameState
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
	int32 CapturesForWinning;

public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStateEvent OnPlayerStateEvent;

	UPROPERTY(BlueprintAssignable)
	FOnTeamChangedEvent OnTeamChangedEvent;

public:
	AFCGameState(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float GetPlayerRespawnDelay(AController* Controller) const override;
	virtual void DefaultTimer() override;
	virtual void ReceivedGameModeClass() override;
	//~AActor

	//~AGameStateBase
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	//~AGameStateBase

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPlayerState(APlayerState* PlayerState, bool bRemove);

public:
	void InitializeGameState();

	virtual void RegisterTeam(AFCPlayerState* PlayerState);
	virtual void UnRegisterTeam(AFCPlayerState* PlayerState);

	bool AddTeamACapture();
	bool AddTeamBCapture();

protected:
	void SetMatchWinner(ETeamSide InMatchWinner);

	virtual void OnStateEnded();
	virtual void OnTimeExpired();
	virtual void OnEndMatch();

public:
	void SetMaxPlayers(int32 InMaxPlayers);
	void SetGameInitialState(int32 InTimeRemaining, EGameInitialState InGameInitialState);
	void ForceEnded();

	void OnPlayableArea(AFCPlayableArea* PlayableArea);
	
protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	EGameInitialState GameInitialState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 ATeamCaptures;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 BTeamCaptures;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 TimeRemaining;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	ETeamSide MatchWinner;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 MaxPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_ATeamSide, BlueprintReadOnly, Category = "Team")
	TArray<TObjectPtr<AFCPlayerState>> ATeamSide;

	UPROPERTY(ReplicatedUsing = OnRep_BTeamSide, BlueprintReadOnly, Category = "Team")
	TArray<TObjectPtr<AFCPlayerState>> BTeamSide;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
	TArray<TObjectPtr<AFCSpawnArea>> SpawnableAreas;
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<AFCPlayableArea> CurrentPlayableArea;
	
	UPROPERTY()
	bool bInitialized;

protected:
	UFUNCTION()
	virtual void OnRep_ATeamSide();

	UFUNCTION()
	virtual void OnRep_BTeamSide();

public:
	TArray<AFCPlayerState*> GetPlayersInSide(ETeamSide Team) const;
	TArray<AFCSpawnArea*> GetSpawnableAreasByTeam(ETeamSide Team) const;

	FORCEINLINE int32 GetMaxPlayers() const { return MaxPlayers; }

	FORCEINLINE EGameInitialState GetGameInitialState() const { return GameInitialState; }
	FORCEINLINE int32 GetTimeRemaining() const { return TimeRemaining; }
	FORCEINLINE ETeamSide GetMatchWinnerResult() const { return MatchWinner; }
	FORCEINLINE TArray<AFCSpawnArea*> GetSpawnableAreas() const { return SpawnableAreas; }

	UFUNCTION(BlueprintPure, Category = "State")
	int32 GetATeamCaptures() const { return ATeamCaptures; };
	UFUNCTION(BlueprintPure, Category = "State")
	int32 GetBTeamCaptures() const { return BTeamCaptures; };
};
