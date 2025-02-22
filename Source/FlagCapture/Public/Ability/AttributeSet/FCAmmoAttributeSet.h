#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "FCAmmoAttributeSet.generated.h"

// Conjunto de macros para acesso rápido aos atributos
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class FLAGCAPTURE_API UFCAmmoAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFCAmmoAttributeSet(const FObjectInitializer& ObjectInitializer);

public:
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	static FGameplayAttribute GetAmmoAttributeFromTag(FGameplayTag& AmmoTag);
	static FGameplayAttribute GetMaxAmmoAttributeFromTag(FGameplayTag& AmmoTag);

public:
	UPROPERTY(BlueprintReadOnly, Category = "Ammo", ReplicatedUsing = OnRep_GeneralAmmo)
	FGameplayAttributeData GeneralAmmo;
	ATTRIBUTE_ACCESSORS(UFCAmmoAttributeSet, GeneralAmmo)

	UPROPERTY(BlueprintReadOnly, Category = "Ammo", ReplicatedUsing = OnRep_MaxGeneralAmmo)
	FGameplayAttributeData MaxGeneralAmmo;
	ATTRIBUTE_ACCESSORS(UFCAmmoAttributeSet, MaxGeneralAmmo)

protected:
	UFUNCTION()
	virtual void OnRep_GeneralAmmo(const FGameplayAttributeData& OldGeneralAmmo);

	UFUNCTION()
	virtual void OnRep_MaxGeneralAmmo(const FGameplayAttributeData& OldMaxGeneralAmmo);

private:
	FGameplayTag GeneralAmmoTag;

};
