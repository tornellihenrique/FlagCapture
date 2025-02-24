// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "FCCharacterBase.generated.h"

class UGameplayEffect;

class UFCGameplayAbility;
class UFCAbilitySystemComponent;
class UFCCharacterAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AFCCharacterBase*, Character);

UCLASS()
class FLAGCAPTURE_API AFCCharacterBase : public ACharacter
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Ability")
	TArray<TSubclassOf<UFCGameplayAbility>> CharacterAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Ability")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Ability")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

public:
	AFCCharacterBase(const FObjectInitializer& ObjectInitializer);

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

	//~IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~IAbilitySystemInterface

public:
	virtual void InitializeAttributes();
	virtual void AddCharacterAbilities();
	virtual void RemoveCharacterAbilities();

	virtual void AddStartupEffects();

	/** Usado apenas em casos específicos como respawn. */
	virtual void SetHealth(float Health);
	/** Usado apenas em casos específicos como respawn. */
	virtual void SetStamina(float Stamina);

	virtual void NotifyDeath();
	virtual void NotifyFinishDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hit Reaction", meta = (DisplayName = "On Hit Status"))
	void OnHitStatus(EDamageResultType DamageResultType, const FVector& ImpactPoint, float DamageAmount);

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FCharacterDiedDelegate OnCharacterDied;

protected:
	TWeakObjectPtr<UFCAbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UFCCharacterAttributeSet> CharacterAttributeSet;

	FGameplayTag DeadTag;
	FGameplayTag EffectRemoveOnDeathTag;

	FGameplayTag AimingTag;

public:
	UFUNCTION(BlueprintPure, Category = "Character Base")
	virtual bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	virtual bool IsAiming() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	virtual int32 GetAbilityLevel(EAbilityInputID AbilityID) const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	int32 GetCharacterLevel() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	float GetStamina() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "Character Base")
	float GetMoveSpeed() const;

};
