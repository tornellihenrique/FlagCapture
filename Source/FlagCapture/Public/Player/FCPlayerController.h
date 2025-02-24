#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FCTypes.h"
#include "GenericTeamAgentInterface.h"
#include "FCPlayerController.generated.h"

class AFCBaseHUD;
class UFCGameOverlay;
class UFCScoreboard;
class UInputMappingContext;
class UInputAction;
class AFCCharacter;
class AFCCaptureFlag;
class AFCPlayableArea;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadoutClassChanged, TEnumAsByte<EPlayerClassType>, PlayerClass);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStartChanged, AFCSpawnArea*, SpawnArea);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetViewPlayerChanged, APlayerState*, TargetViewPlayer);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTookDamageEvent, AFCPlayerController*, ControllerRef);

UCLASS()
class FLAGCAPTURE_API AFCPlayerController : public APlayerController
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
	TEnumAsByte<EPlayerClassType> PlayerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Configs")
	FClassLoadout LoadoutPresets[EPCT_Max];

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|HUD")
	TSubclassOf<AFCBaseHUD> RegisterHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|HUD")
	TSubclassOf<AFCBaseHUD> SpawnHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|HUD")
	TSubclassOf<AFCBaseHUD> PlayingHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|HUD")
	TSubclassOf<AFCBaseHUD> DeathHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|HUD")
	TSubclassOf<AFCBaseHUD> SpectatingHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|HUD")
	TSubclassOf<AFCBaseHUD> MatchEndedHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI")
	TSubclassOf<UFCGameOverlay> GameOverlayWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI")
	TSubclassOf<UFCScoreboard> ScoreboardWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Damage")
	TSubclassOf<UCameraShakeBase> HitShake;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ScoreboardAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI|Fade")
	float FadeToRegisterState = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI|Fade")
	float FadeToSpawnDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI|Fade")
	float FadeToPlayingDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI|Fade")
	float FadeToDeathDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI|Fade")
	float FadeToSpectateDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|UI|Fade")
	float FadeToEndDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Death State")
	float DelayBeforeFade = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Death State")
	float DelayBeforeInactive = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Death State")
	float SpawnTransitionBlendTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs|Death State")
	TEnumAsByte<enum EViewTargetBlendFunction> SpawnTransitionBlendFunction = EViewTargetBlendFunction::VTBlend_Linear;

public:
	UPROPERTY(BlueprintAssignable)
	FOnLoadoutClassChanged OnLoadoutClassChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStartChanged OnPlayerStartChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTargetViewPlayerChanged OnTargetViewPlayerChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTookDamageEvent OnTookDamageEvent;

public:
	AFCPlayerController(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnRep_Pawn() override;
	virtual void OnRep_PlayerState() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaSeconds) override;
	virtual void ReceivedPlayer() override;
	virtual void InitPlayerState() override;
	virtual void SetupInputComponent() override;
	virtual void BeginInactiveState() override;
	virtual void UnFreeze() override;
	virtual void AutoManageActiveCameraTarget(AActor* SuggestedTarget) override;

	//~IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~IGenericTeamAgentInterface

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UFCGameOverlay> GameOverlayWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UFCScoreboard> ScoreboardWidget;

	UPROPERTY(ReplicatedUsing = OnRep_ControllerState)
	EControllerState ControllerState;

	UPROPERTY()
	EControllerState PrevControllerState;

	UPROPERTY(Replicated)
	TObjectPtr<AFCPlayableArea> CurrentArea;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerStart)
	TObjectPtr<AActor> PlayerStart;

	UPROPERTY(ReplicatedUsing = OnRep_LastKillerPlayer)
	TObjectPtr<APlayerState> LastKillerPlayer;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> TargetViewPlayer;

	UPROPERTY()
	FDamageData LastKillerDamageData;

	UPROPERTY(ReplicatedUsing = OnRep_MainCharacter)
	TObjectPtr<AFCCharacter> MainCharacter;

	UPROPERTY()
	ETeamSide CachedTeam;

	UPROPERTY()
	EGameInitialState CachedGameInitialState;

	UPROPERTY()
	TArray<TObjectPtr<AFCCaptureFlag>> AvailableCaptureFlags;

	UPROPERTY()
	TObjectPtr<AFCCaptureFlag> CurrentCaptureFlag;

	UPROPERTY()
	uint8 bScoreboardOpen : 1;

	UPROPERTY()
	uint8 bFirstPlay : 1;

	FTimerHandle RegisterHandle;
	FTimerHandle MatchEndHandle;
	FTimerHandle DeathFadeHandle;
	FTimerHandle DeathInactiveHandle;
	FTimerHandle LoadoutInitializedHandle;
	FTimerHandle ResetPlayerStateHanlde;
	FTimerHandle SpawnTransitionHanlde;
	FTimerHandle LevelCheckHanlde;

protected:
	void SetMainCharacter(AFCCharacter* InCharacter);
	UFUNCTION(Server, Reliable)
	void ServerSetMainCharacter(AFCCharacter* InCharacter);

	UFUNCTION(BlueprintCallable)
	void TrySetMainCharacter(AFCCharacter* InCharacter);

public:
	void SetPlayerStart(AActor* InPlayerStart, AFCPlayableArea* InPlayableArea);
	void OnCharacterSpawned(ACharacter* InCharacter);

	virtual void OnLoadoutInitialized();
	UFUNCTION(Client, Reliable)
	void ClientOnLoadoutInitialized();

	void CheckLoadoutInitialized();

	void OnPlayerLogin(AActor* InPlayerStart, EGameInitialState GameInitialState);
	UFUNCTION(Client, Reliable)
	void ClientOnPlayerLogin(AActor* InPlayerStart, EGameInitialState GameInitialState);

	void CheckLevel();

	void OnGameInitialStateChanged(EGameInitialState GameInitialState);
	UFUNCTION(Client, Reliable)
	void ClientOnGameInitialStateChanged(EGameInitialState GameInitialState);

	void SetControllerState(EControllerState InControllerState);

	void RequestControllerState(EControllerState InControllerState);
	UFUNCTION(Server, Reliable)
	void ServerRequestControllerState(EControllerState InControllerState);

	void EnterPreparingState();
	void EnterMatchEndedState();
	void EnterNextGameState();

	UFUNCTION(Client, Reliable)
	virtual void ClientEnterPlayingState();

	void RequestSpawnLoadout(const FPlayerLoadout& PlayerLoadout);
	UFUNCTION(Server, Reliable)
	void ServerRequestSpawnLoadout(const FPlayerLoadout& PlayerLoadout);

	virtual void SpawnLoadout(const FPlayerLoadout& PlayerLoadout);

public:
	void EnterRegisterState();
	void EnterSpawnState();
	void EnterPlayingState();
	void EnterDeathState();
	void EnterDeathStateFast();
	void EnterSpectatingState();
	void EnterMatchEnded();

	void OnEnterRegisterState();
	void OnEnterSpawnState();
	void OnEnterPlayingState();
	void OnEnterDeathState();
	void OnEnterSpectatingState();
	void OnEnterMatchEnded();
	
	bool IsFirstSpawn();
	void BuildUI();
	void BuildScoreboard();
	void ClearScoreboard();
	void OnMainWidgetInitialized();
	void BeginPlaying();

public:
	void OnPlayerJoinTeam(ETeamSide InTeam);
	UFUNCTION(Client, Reliable)
	void ClientOnPlayerJoinTeam(ETeamSide InTeam);

	void RegisterTeam(ETeamSide InTeam);
	UFUNCTION(Server, Reliable)
	void ServerRegisterTeam(ETeamSide InTeam);

	void RequestEnterRegisterState();
	UFUNCTION(Server, Reliable)
	void ServerEnterRegisterState();

	void RequestEnterPlayingState();
	UFUNCTION(Server, Reliable)
	void ServerEnterPlayingState();

	void RequestPlayerStart();
	UFUNCTION(Server, Reliable)
	void ServerRequestPlayerStart();

	void RequestEnterSpawnState();
	UFUNCTION(Server, Reliable)
	void ServerRequestEnterSpawnState();

	void RequestEnterMatchEndedState();
	UFUNCTION(Server, Reliable)
	void ServerEnterMatchEndedState();

	void RequestEnterMatchEnded();
	UFUNCTION(Server, Reliable)
	void ServerEnterMatchEnded();

public:
	virtual void AddScore(int32 Val);

	virtual void AddKill();
	virtual void AddDeath();
	virtual void AddHeadshot();
	virtual void AddDamage(float DamageAmount);
	virtual void AddKilledPlayer(APlayerState* PlayerPS);

	virtual void StoreLastKillerData(APlayerState* KillerPlayerState, const FKillEventData& KillEventData);
	virtual void ClearLastKillerData();

	virtual void OnTookDamage(float DamageAmount, AActor* Causer);
	UFUNCTION(Client, Reliable)
	virtual void ClientOnTookDamage(float DamageAmount, AActor* Causer);

	virtual void NotifyOnCharacterDied(ACharacter* InCharacter, const FKillEventData& KillEventData, AFCPlayerState* InPlayerState, AFCPlayerState* KillerPlayerState, int32 InScore);
	virtual void NotifyOnHit(AActor* InActor, const FHitResult& HitResult, float DamageAmount);
	virtual void NotifyOnKill(AController* KilledPC, bool bIsHeadshot);

	UFUNCTION(Client, Reliable)
	virtual void ClientSetDamageData(const FKillEventData& KillEventData);
	UFUNCTION(Client, Reliable)
	virtual void ClientClearDamageData();
	UFUNCTION(Client, Reliable)
	virtual void ClientOnCharacterDied(ACharacter* InCharacter, const FKillEventData& KillEventData, AFCPlayerState* InPlayerState, AFCPlayerState* KillerPlayerState, int32 InScore);
	UFUNCTION(Client, Reliable)
	virtual void ClientOnHit(AActor* HitActor, FVector_NetQuantize HitLocation, FName HitBoneName, float DamageAmount);
	UFUNCTION(Client, Reliable)
	virtual void ClientOnKill(APlayerState* KilledPlayerState, bool bIsHeadshot);

public:
	virtual void OnPlayableAreaStateChanged(bool bIsInsidePlayableArea);

	UFUNCTION()
	void OnFlagGrabbedByTeam(AFCCaptureFlag* CaptureFlag, ETeamSide ByTeam);
	UFUNCTION()
	void OnFlagCapturedByTeam(AFCCaptureFlag* CaptureFlag, ETeamSide ByTeam);

	void AddFlagsCaptured();
	UFUNCTION(Server, Reliable)
	void ServerAddFlagsCaptured();

protected:
	void InputScoreboard();

protected:
	UFUNCTION()
	virtual void OnRep_ControllerState(EControllerState OldControllerState);

	UFUNCTION()
	virtual void OnRep_PlayerStart(AActor* OldPlayerStart);

	UFUNCTION()
	virtual void OnRep_LastKillerPlayer();

	UFUNCTION()
	virtual void OnRep_MainCharacter(AFCCharacter* OldCharacter);

	UFUNCTION()
	virtual void OnPawnSet(APlayerState* InPlayer, APawn* InNewPawn, APawn* InOldPawn);

protected:
	UFUNCTION()
	virtual void OnAmmoAmountChanged(int32 Ammo);

	UFUNCTION()
	virtual void OnAmmoReserveChanged(int32 Ammo);

public:
	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE UFCGameOverlay* GetGameOverlayWidget() { return GameOverlayWidget; }

	UFUNCTION(BlueprintPure, Category = "State")
	UFCPlayerOverlay* GetPlayerOverlay() const;

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE EControllerState GetState() const { return ControllerState; }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE FName GetNativeState() const { return GetStateName(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FPlayerLoadout GetLoadout() const;

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE AActor* GetPlayerStart() const { return PlayerStart; }

	UFUNCTION(BlueprintPure, Category = "State")
	APlayerState* GetSepctatorTarget() const;

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE APlayerState* GetLastKillerPlayer() const { return LastKillerPlayer; }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE FDamageData GetLastKillerDamageData() const { return LastKillerDamageData; };

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE AFCCharacter* GetMainCharacter() const { return MainCharacter; }

	ETeamSide GetPlayerSide() const { return CachedTeam; };

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE EGameInitialState GetGameInitialState() const { return CachedGameInitialState; }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE bool GetScoreboardOpen() const { return bScoreboardOpen; }

	UFUNCTION(BlueprintPure, Category = "State")
	TArray<AFCSpawnArea*> GetSpawnableAreas() const;

	UFUNCTION(BlueprintPure, Category = "State")
	AFCSpawnArea* GetCurrentSpawnableArea() const;

	virtual bool IsEnemyFor(AFCPlayerController* Other) const;
	virtual bool IsEnemyFor(AFCPlayerState* InPlayerState) const;
	virtual bool IsEnemyFor(AController* Other) const;
	virtual bool IsEnemyFor(APawn* InPawn) const;

	UFUNCTION(BlueprintPure, Category = "State")
	FLinearColor GetTeamColor() const;

	UFUNCTION(BlueprintPure, Category = "State")
	FLinearColor GetEnemyColor() const;

	bool ShowCrosshair() const;

public:
	UFUNCTION(BlueprintCallable)
	void SetSepctatorTarget(APlayerState* InPlayerState);

	UFUNCTION(BlueprintCallable, Category = "State")
	void FadeIn(float Duration);
	UFUNCTION(Client, Reliable)
	virtual void ClientFadeIn(float Duration);

	UFUNCTION(BlueprintCallable, Category = "State")
	void FadeOut(float Duration);
	UFUNCTION(Client, Reliable)
	virtual void ClientFadeOut(float Duration);

	UFUNCTION(BlueprintCallable, Category = "State")
	void JoinTeam(ETeamSide InTeam);
	UFUNCTION(Server, Reliable)
	void ServerJoinTeam(ETeamSide InTeam);

	UFUNCTION(BlueprintCallable, Category = "State")
	void Spawn();
	UFUNCTION(Server, Reliable)
	void ServerSpawn();

	UFUNCTION(BlueprintCallable, Category = "State")
	void SpawnAtDesired(const FVector_NetQuantize100& InLocation, const FVector_NetQuantizeNormal& InDirection);
	UFUNCTION(Server, Reliable)
	void ServerSpawnAtDesired(const FVector_NetQuantize100& InLocation, const FVector_NetQuantizeNormal& InDirection);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ExitSpectator();
	UFUNCTION(Server, Reliable)
	void ServerExitSpectator();

	UFUNCTION(BlueprintCallable, Category = "State")
	void UpdatePlayerStart(AActor* InPlayerStart);
	UFUNCTION(Server, Reliable)
	void ServerUpdatePlayerStart(AActor* InPlayerStart);

	UFUNCTION(BlueprintCallable, Category = "State")
	void SetPlayerClass(TEnumAsByte<EPlayerClassType> InPlayerClass);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ToggleScoreboard();
	
};
