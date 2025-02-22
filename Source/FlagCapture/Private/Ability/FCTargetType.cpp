#include "Ability/FCTargetType.h"

#include "Character/FCCharacterBase.h"

UFCTargetType::UFCTargetType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCTargetType::GetTargets_Implementation(AFCCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	// ...
}

UFCTargetType_UseOwner::UFCTargetType_UseOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCTargetType_UseOwner::GetTargets_Implementation(AFCCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	OutActors.Add(TargetingCharacter);
}

UFCTargetType_UseEventData::UFCTargetType_UseEventData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCTargetType_UseEventData::GetTargets_Implementation(AFCCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	const FHitResult* FoundHitResult = EventData.ContextHandle.GetHitResult();
	if (FoundHitResult)
	{
		OutHitResults.Add(*FoundHitResult);
	}
	else if (IsValid(EventData.Target))
	{
		OutActors.Add(const_cast<AActor*>(EventData.Target.Get()));
	}
}
