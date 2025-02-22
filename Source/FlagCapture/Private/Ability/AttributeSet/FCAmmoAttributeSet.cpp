#include "Ability/AttributeSet/FCAmmoAttributeSet.h"

#include "Net/UnrealNetwork.h"

UFCAmmoAttributeSet::UFCAmmoAttributeSet(const FObjectInitializer& ObjectInitializer)
{
	GeneralAmmoTag = FGameplayTag::RequestGameplayTag("Weapon.Ammo.General");
}

void UFCAmmoAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetGeneralAmmoAttribute())
	{
		SetGeneralAmmo(FMath::Clamp<float>(GetGeneralAmmo(), 0, GetMaxGeneralAmmo()));
	}
}

void UFCAmmoAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UFCAmmoAttributeSet, GeneralAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAmmoAttributeSet, MaxGeneralAmmo, COND_None, REPNOTIFY_Always);
}

FGameplayAttribute UFCAmmoAttributeSet::GetAmmoAttributeFromTag(FGameplayTag& AmmoTag)
{
	if (AmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.General")))
	{
		return GetGeneralAmmoAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UFCAmmoAttributeSet::GetMaxAmmoAttributeFromTag(FGameplayTag& AmmoTag)
{
	if (AmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.General")))
	{
		return GetMaxGeneralAmmoAttribute();
	}

	return FGameplayAttribute();
}

void UFCAmmoAttributeSet::OnRep_GeneralAmmo(const FGameplayAttributeData& OldGeneralAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAmmoAttributeSet, GeneralAmmo, OldGeneralAmmo);
}

void UFCAmmoAttributeSet::OnRep_MaxGeneralAmmo(const FGameplayAttributeData& OldMaxGeneralAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAmmoAttributeSet, MaxGeneralAmmo, OldMaxGeneralAmmo);
}
