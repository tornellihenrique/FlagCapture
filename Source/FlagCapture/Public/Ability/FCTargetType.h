#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "FCTargetType.generated.h"

class AFCCharacterBase;
struct FGameplayEventData;

/**
 * Base para definir a lógica de targeting em habilidades.
 * Essa classe deve ser extendida via BPs e não é instanciada no mundo.
 */
UCLASS()
class FLAGCAPTURE_API UFCTargetType : public UObject
{
	GENERATED_BODY()

public:
	UFCTargetType(const FObjectInitializer& ObjectInitializer);

public:
	/**
	 * Define os alvos para aplicar os efeitos.
	 * @param TargetingCharacter - Personagem que está realizando o targeting.
	 * @param TargetingActor - Ator responsável pelo targeting.
	 * @param EventData - Dados do evento de gameplay.
	 * @param OutTargetData - Dados dos alvos encontrados.
	 * @param OutHitResults - Resultados de colisão.
	 * @param OutActors - Lista de atores alvos.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Targeting")
	void GetTargets(AFCCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const;

};

/**
 * Classe facilitadora para usar o owner como target.
 */
UCLASS(NotBlueprintable)
class FLAGCAPTURE_API UFCTargetType_UseOwner : public UFCTargetType
{
	GENERATED_BODY()

public:
	UFCTargetType_UseOwner(const FObjectInitializer& ObjectInitializer);

public:
	virtual void GetTargets_Implementation(AFCCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};

/**
 * Classe facilitadora para usar os targets a partir do EventData.
 */
UCLASS(NotBlueprintable)
class FLAGCAPTURE_API UFCTargetType_UseEventData : public UFCTargetType
{
	GENERATED_BODY()

public:
	UFCTargetType_UseEventData(const FObjectInitializer& ObjectInitializer);

public:
	virtual void GetTargets_Implementation(AFCCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};