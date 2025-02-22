#pragma once

#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectStackChange.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"

#include "FCAbilityTypes.generated.h"

class UGameplayEffect;

class UFCAbilitySystemComponent;
class UFCTargetType;

/**
 * Container estático que define o tipo de target e os gameplay effects que serão aplicados.
 * Geralmente definido em BPs e processado em Spec em tempo de execução.
 */
USTRUCT(BlueprintType)
struct FGameplayEffectContainer
{
	GENERATED_USTRUCT_BODY()

	/** Classe que define como será realizada a seleção de targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TSubclassOf<UFCTargetType> TargetType;

	/** Lista de gameplay effects a serem aplicados nos targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TArray<TSubclassOf<UGameplayEffect>> TargetGameplayEffectClasses;

	FGameplayEffectContainer() {}
};

/**
 * Versão processada do container de gameplay effects, contendo os dados e specs prontos para aplicação.
 */
USTRUCT(BlueprintType)
struct FGameplayEffectContainerSpec
{
	GENERATED_USTRUCT_BODY()

	/** Dados dos alvos computados para aplicação dos gameplay effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FGameplayAbilityTargetDataHandle TargetData;

	/** Lista de specs dos efeitos a serem aplicados aos alvos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TArray<FGameplayEffectSpecHandle> TargetGameplayEffectSpecs;

	bool HasValidEffects() const;
	bool HasValidTargets() const;

	void AddTargets(const TArray<FGameplayAbilityTargetDataHandle>& InTargetData, const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors);
	void ClearTargets();

	FGameplayEffectContainerSpec() {}
};