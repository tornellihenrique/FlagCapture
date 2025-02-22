#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "FCAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReceivedDamageDelegate, UFCAbilitySystemComponent*, SourceASC, float, UnmitigatedDamage, float, MitigatedDamage);

UCLASS()
class FLAGCAPTURE_API UFCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFCAbilitySystemComponent(const FObjectInitializer& ObjectInitializer);

public:
	/** Helper para substituir UAbilitySystemGlobals::GetAbilitySystemComponentFromActor e retornar a nova classe UFCAbilitySystemComponent. */
	static UFCAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool bLookForComponent = false);

	/** Sobrescrevendo comportamento base para ativar a Ability automaticamente baseado no InputID configurado em UFCGameplayAbility. */
	virtual void AbilityLocalInputPressed(int32 InputID) override;

	/** Chamado por FCDamageExecCalculation afim de executar evento ReceivedDamage sempre que este ASC receber dano. */
	virtual void ReceiveDamage(UFCAbilitySystemComponent* SourceASC, float UnmitigatedDamage, float MitigatedDamage);

public:
	/** Expondo para BPs */
	UFUNCTION(BlueprintPure, Category = "ASC", meta = (DisplayName = "GetTagCount"))
	int32 K2_GetTagCount(FGameplayTag TagToCheck) const;

	/** Helper para retornar possiveis abilidades (filhas) dada uma classe UGameplayAbility pai. */
	UFUNCTION(BlueprintPure, Category = "ASC")
	FGameplayAbilitySpecHandle FindAbilitySpecHandleForClass(TSubclassOf<UGameplayAbility> AbilityClass, UObject* OptionalSourceObject = nullptr);

	/**
	 * Tenta ativar a habilidade agrupando as chamadas RPC do mesmo frame.
	 * Se possível, unifica ActivateAbility, SendTargetData e EndAbility em uma única chamada.
	 * Caso EndAbilityImmediately seja verdadeiro, encerra a habilidade logo após a ativação.
	 */
	UFUNCTION(BlueprintCallable, Category = "ASC")
	virtual bool BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle, bool bEndAbilityImmediately);

	UFUNCTION(BlueprintCallable, Category = "ASC", meta = (AutoCreateRefTerm = "GameplayCueParameters", GameplayTagFilter = "GameplayCue"))
	void ExecuteGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);

	UFUNCTION(BlueprintCallable, Category = "ASC", meta = (AutoCreateRefTerm = "GameplayCueParameters", GameplayTagFilter = "GameplayCue"))
	void AddGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);

	UFUNCTION(BlueprintCallable, Category = "ASC", meta = (AutoCreateRefTerm = "GameplayCueParameters", GameplayTagFilter = "GameplayCue"))
	void RemoveGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FReceivedDamageDelegate ReceivedDamage;

public:
	uint8 bCharacterAbilitiesGiven : 1;
	uint8 bStartupEffectsApplied : 1;

};
