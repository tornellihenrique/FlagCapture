#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Ability/AttributeSet/FCCharacterAttributeSet.h"
#include "FCPlayerState.generated.h"

class UFCAbilitySystemComponent;
class AFCCharacter;

class UFCAmmoAttributeSet;
class UFCCharacterAttributeSet;

DECLARE_DELEGATE_OneParam(FOnPlayerJoinTeam, ETeamSide /* InTeam */);

UCLASS()
class FLAGCAPTURE_API AFCPlayerState : public APlayerState
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

	UPROPERTY(Category = "FC Player State", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCAbilitySystemComponent> AbilitySystemComponent;
	
public:
	AFCPlayerState(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~AActor

	//~APlayerState
	virtual void RegisterPlayerWithSession(bool bWasFromInvite) override;
	virtual void UnregisterPlayerWithSession() override;
	virtual void OnRep_PlayerName() override;
	//~APlayerState

	//~IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~IAbilitySystemInterface

public:
	void JoinTeam(ETeamSide InTeam);

	void AddKill();
	void AddDeath();
	void AddHeadshot();
	void AddDamage(float DamageAmount);
	void AddFlagsCaptured();
	void AddKilledPlayer(AFCPlayerState* InPS);

	void SetMainCharacter(AFCCharacter* Character);
	void ViewTargetChanged(APawn* InPawn);

	UFCCharacterAttributeSet* GetCharacterAttributeSet() const;
	UFCAmmoAttributeSet* GetAmmoAttributeSet() const;

public:
	FOnPlayerJoinTeam OnPlayerJoinTeam;

protected:
	virtual void HealthChanged(const FOnAttributeChangeData& Data);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "FC Player State", meta = (DisplayName = "On Rep PlayerName"))
	void K2_OnRep_PlayerName();

	UFUNCTION(BlueprintImplementableEvent, Category = "FC Player State", meta = (DisplayName = "On View Target Changed"))
	void K2_OnViewTargetChanged(APawn* InPanw);
	
protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 Kills;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 Deaths;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 Headshots;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	float DamageDealt;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	ETeamSide PlayerSide;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 FlagsCaptured;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	TArray<TObjectPtr<AFCPlayerState>> KilledPlayers;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	TObjectPtr<AFCCharacter> MainCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	UFCCharacterAttributeSet* CharacterAttributeSet;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	UFCAmmoAttributeSet* AmmoAttributeSet;

	FGameplayTag DeadTag;

	FDelegateHandle HealthChangedDelegateHandle;

public:
	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE int32 GetKills() { return Kills; };

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE int32 GetDeaths() { return Deaths; };

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE int32 GetHeadshots() { return Headshots; }
	
	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetDamageDealt() { return DamageDealt; }

	UFUNCTION(BlueprintPure, Category = "State")
	virtual ETeamSide GetPlayerSide() { return PlayerSide; };

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE int32 GetFlagsCaptured() { return FlagsCaptured; };

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE TArray<AFCPlayerState*> GetKilledPlayers() { return KilledPlayers; }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE AFCCharacter* GetMainCharacter() { return MainCharacter; }

	UFUNCTION(BlueprintPure, Category = "State")
	virtual bool IsEnemyFor(AFCPlayerState* Other);

	UFUNCTION(BlueprintPure, Category = "State")
	float GetKDRatio() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetHSRatio() const;

	UFUNCTION(BlueprintPure, Category = "State")
	int32 GetKilledPlayerNum(AFCPlayerState* InPS) const;

	UFUNCTION(BlueprintPure, Category = "State")
	FUniqueNetIdRepl GetUniqueNetId() const;

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE bool IsAlive() const { return GetHealth() > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetHealth() const { return CharacterAttributeSet->GetHealth(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetMaxHealth() const { return CharacterAttributeSet->GetMaxHealth(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetHealthRegenRate() const { return CharacterAttributeSet->GetHealthRegenRate(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetStamina() const { return CharacterAttributeSet->GetStamina(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetMaxStamina() const { return CharacterAttributeSet->GetMaxStamina(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetStaminaRegenRate() const { return CharacterAttributeSet->GetStaminaRegenRate(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE float GetMoveSpeed() const { return CharacterAttributeSet->GetMoveSpeed(); }

	UFUNCTION(BlueprintPure, Category = "State")
	FORCEINLINE int32 GetCharacterLevel() const { return CharacterAttributeSet->GetCharacterLevel(); }

	UFUNCTION(BlueprintPure, Category = "State")
	int32 GetAmmoReserveAmount() const;

};
