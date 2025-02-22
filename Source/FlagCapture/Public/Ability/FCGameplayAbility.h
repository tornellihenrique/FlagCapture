#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FCTypes.h"
#include "Ability/FCAbilityTypes.h"
#include "FCGameplayAbility.generated.h"

UCLASS()
class FLAGCAPTURE_API UFCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Id do input que ativa automaticamente a habilidade ao pressionar. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	EAbilityInputID AbilityInputID;

	/** Id para associar a habilidade a um slot, usado em habilidades passivas. Útil pois habilidades passivas não são atreladas a um input id. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	EAbilityInputID AbilityID;

	/** Permite que a habilidade seja ativada via input, usando o AbilityInputID. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool bActivateOnInput;

	/** Se verdadeiro, ativa a habilidade assim que for concedida (útil para passivas ou habilidades forçadas). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool bActivateAbilityOnGranted;

	/** Usado para permitir que a habilidade seja ativada apenas se o player estiver com a arma correta equipada (atrelada à habilidade). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool bSourceObjectMustEqualCurrentItemToActivate;

	/** Map de tags para containers de gameplay effects. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Gameplay Effects")
	TMap<FGameplayTag, FGameplayEffectContainer> EffectContainerMap;

public:
	UFCGameplayAbility(const FObjectInitializer& ObjectInitializer);

public:
	/** Chamada ao definir o avatar; ideal para iniciar lógicas de habilidades passivas. */
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	/** Verifica se a habilidade pode ser ativada, considerando tags e restrições. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Checa os custos necessários para ativação da habilidade. */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Aplica os custos da habilidade. */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

public:
	/** Cria um container spec a partir de um container e dados de evento. */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual FGameplayEffectContainerSpec MakeEffectContainerSpecFromContainer(const FGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	/** Procura e cria um container spec baseado em uma tag. */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual FGameplayEffectContainerSpec MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	/** Aplica os gameplay effects definidos no container spec e retorna os handles dos ativos. */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainerSpec(const FGameplayEffectContainerSpec& ContainerSpec);

	/** Retorna o objeto fonte associado à habilidade (expondo para BPs). */
	UFUNCTION(BlueprintPure, Category = "Ability", meta = (DisplayName = "Get Source Object"))
	UObject* K2_GetSourceObject(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo) const;

	/** Tenta ativar a habilidade agrupando as chamadas RPC do mesmo frame. Usando UFCAbilitySystemComponent::BatchRPCTryActivateAbility. */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual bool BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle, bool EndAbilityImmediately);

	/** Encerra a habilidade externamente, semelhante ao K2_EndAbility. Usado primordialmente pelo UFCAbilitySystemComponent::BatchRPCTryActivateAbility. */
	virtual void ExternalEndAbility();

	/** Permitindo customização via C++ ou BP. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	bool MyCheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo) const;

	/** Permitindo customização via C++ ou BP. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void MyApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const;

};
