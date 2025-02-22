#include "Ability/FCDamageExecutionCalc.h"

#include "Ability/AttributeSet/FCCharacterAttributeSet.h"

// Declaração de atributes a serem capturados e como devem ser capturados, para o calcudo de dano final
struct FCDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);

	FCDamageStatics()
	{
		// Outros atributos a serem capturados podem ser adicionados aqui

		// Dano base
		DEFINE_ATTRIBUTE_CAPTUREDEF(UFCCharacterAttributeSet, Damage, Source, true);
	}
};

static const FCDamageStatics& DamageStatics()
{
	static FCDamageStatics DamageStatics;
	return DamageStatics;
}

UFCDamageExecutionCalc::UFCDamageExecutionCalc(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HeadShotMultiplier(2.0f)
{
	RelevantAttributesToCapture.Add(DamageStatics().DamageDef);
}

void UFCDamageExecutionCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Damage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageDef, EvaluationParameters, Damage);

	// Dano adicional opcional caso exista
	Damage += FMath::Max<float>(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), false, -1.0f), 0.0f);

	float UnmitigatedDamage = Damage;

	const FHitResult* Hit = Spec.GetContext().GetHitResult();

	// Multiplicador headshot
	if (AssetTags.HasTagExact(FGameplayTag::RequestGameplayTag(FName("Effect.Damage.CanHeadShot"))) && Hit && Hit->BoneName == "head")
	{
		UnmitigatedDamage *= HeadShotMultiplier;
		FGameplayEffectSpec* MutableSpec = ExecutionParams.GetOwningSpecForPreExecuteMod();
		MutableSpec->AddDynamicAssetTag(FGameplayTag::RequestGameplayTag(FName("Effect.Damage.HeadShot")));
	}

	const float MitigatedDamage = (UnmitigatedDamage);

	if (MitigatedDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamageProperty, EGameplayModOp::Additive, MitigatedDamage));
	}
}
