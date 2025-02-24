// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FCCharacterBase.h"
#include "GameplayEffectTypes.h"
#include "FCTypes.h"
#include "FCCharacter.generated.h"

class UAIPerceptionStimuliSourceComponent;

class UFCAmmoAttributeSet;
class UFCAnimInstance;
class AFCPlayableArea;

USTRUCT(BlueprintType)
struct FCharacterInventory
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<AFCWeapon*> Weapons;

	// Qualquer outro tipo de item...

	FCharacterInventory() {}
};

DECLARE_DELEGATE(FOnLoadoutInitialized);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoAmountChanged, int32, Ammo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoReserveChanged, int32, Ammo);

UCLASS()
class FLAGCAPTURE_API AFCCharacter : public AFCCharacterBase
{
	GENERATED_BODY()

	UPROPERTY(Category = "Character", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionStimuliSourceComponent> AIPerceptionStimuliSourceComponent;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float DamageableTimeAfterRespawn = 3.0f;

public:
	AFCCharacter(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~AActor

	//~APawn
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	//~APawn
	
	//~AFCCharacterBase
	virtual void NotifyFinishDeath() override;
	//~AFCCharacterBase

public:
	FOnLoadoutInitialized OnLoadoutInitialized;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoAmountChanged OnAmmoAmountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoReserveChanged OnAmmoReserveChanged;

public:
	virtual void PopulateLoadout(AController* InController, const FPlayerLoadout& InLoadout);

	bool IsWeaponListInitialized() const;

	virtual void OnPlaying(AFCPlayableArea* InCurrentArea);
	UFUNCTION(Server, Reliable)
	void ServerOnPlaying(AFCPlayableArea* InCurrentArea);

	UFUNCTION(BlueprintImplementableEvent, Category = "Character", meta = (DisplayName = "On Playing"))
	void K2_OnPlaying();

	virtual void OnMatchEnded();

protected:
	virtual void Damageable();

	void SetCurrentWeapon(AFCWeapon* NewWeapon, AFCWeapon* LastWeapon);

	void UnEquipWeapon(AFCWeapon* InWeapon);
	void UnEquipCurrentWeapon();

	UFUNCTION(Server, Reliable)
	void ServerSyncCurrentWeapon();

	UFUNCTION(Client, Reliable)
	void ClientSyncCurrentWeapon(AFCWeapon* InWeapon);

	void OnAbilityActivationFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags);

public:
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual bool AddWeaponToInventory(AFCWeapon* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual bool RemoveWeaponFromInventory(AFCWeapon* WeaponToRemove);

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void RemoveAllWeaponsFromInventory();

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void EquipWeapon(AFCWeapon* NewWeapon);
	UFUNCTION(Server, Reliable)
	virtual void ServerEquipWeapon(AFCWeapon* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void NextWeapon();
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void PreviousWeapon();

	virtual bool HasWeapon(AFCWeapon* InWeapon);

protected:
	TWeakObjectPtr<UFCAmmoAttributeSet> AmmoAttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	AFCWeapon* CurrentWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	FCharacterInventory Inventory;

	UPROPERTY(Replicated)
	uint8 bLoadoutInitialized : 1;

	UPROPERTY()
	uint8 bChangedWeaponLocally : 1;

	UPROPERTY()
	TObjectPtr<AFCPlayableArea> CurrentArea;

	UPROPERTY()
	uint8 bInsidePlayableArea : 1;

	FGameplayTag CurrentWeaponTag;
	FGameplayTag NoWeaponTag;

	FGameplayTag WeaponChangingDelayReplicationTag;
	FGameplayTag WeaponAmmoTypeNoneTag;
	FGameplayTag WeaponAbilityTag;

	FDelegateHandle AmmoChangedDelegateHandle;
	FDelegateHandle WeaponChangingDelayReplicationTagChangedDelegateHandle;

	FTimerHandle TimerHandle_Damageable;

	UPROPERTY()
	UFCAnimInstance* AnimInstance;

public:
	UFUNCTION(BlueprintPure, Category = "Character")
	ETeamSide GetPlayerSide() const;

	UFUNCTION(BlueprintPure, Category = "Character")
	AFCWeapon* GetWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintPure, Category = "Character")
	TArray<AFCWeapon*> GetWeaponList() const { return Inventory.Weapons; }

	template<class T>
	T* GetWeapon() const
	{
		return Cast<T>(GetWeapon());
	}

	UFUNCTION(BlueprintPure, Category = "Character")
	int32 GetAmmoAmount() const;

	UFUNCTION(BlueprintPure, Category = "Character")
	int32 GetMaxAmmoAmount() const;

	UFUNCTION(BlueprintPure, Category = "Character")
	int32 GetAmmoReserveAmount() const;

	UFUNCTION(BlueprintPure, Category = "Character")
	int32 GetNumWeapons() const;

	UFUNCTION(BlueprintPure, Category = "Character")
	virtual AFCWeapon* GetNextWeapon();

	UFUNCTION(BlueprintPure, Category = "Character")
	virtual AFCWeapon* GetPreviousWeapon();

protected:
	/** Callback para quando a munição do pente alterar. */
	UFUNCTION()
	virtual void CurrentWeaponAmmoAmountChanged(int32 OldAmmoAmount, int32 NewAmmoAmount);

	/** Callback para quando a munição do inventario alterar. */
	virtual void CurrentWeaponAmmoReserveChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	virtual void WeaponChangingDelayReplicationTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void OnRep_CurrentWeapon(AFCWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_Inventory();

};
