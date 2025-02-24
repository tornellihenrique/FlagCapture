#include "Character/FCCharacterBase.h"

#include "Components/CapsuleComponent.h"

#include "Character/FCCharacterMovementComponent.h"
#include "Ability/FCAbilitySystemComponent.h"
#include "Ability/FCGameplayAbility.h"
#include "Ability/AttributeSet/FCCharacterAttributeSet.h"
#include "Player/FCPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FCCharacterBase)

AFCCharacterBase::AFCCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFCCharacterMovementComponent>(CharacterMovementComponentName))
{
	AimingTag = FGameplayTag::RequestGameplayTag("State.Aiming");

	if (::IsValid(GetMesh()))
	{
		GetMesh()->SetRelativeLocation_Direct(FVector(0.0f, 0.0f, -92.0f));
		GetMesh()->SetRelativeRotation_Direct(FRotator(0.0f, -90.0f, 0.0f));

		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	PrimaryActorTick.bCanEverTick = true;
	bAlwaysRelevant = true;
}

void AFCCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AFCCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AFCCharacterBase::PostInitializeComponents()
{
	// Apenas garantindo que a mesh e AnimBP estão com o tick após o character.
	GetMesh()->AddTickPrerequisiteActor(this);

	Super::PostInitializeComponents();
}

void AFCCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AFCCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

void AFCCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void AFCCharacterBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AFCCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void AFCCharacterBase::UnPossessed()
{
	Super::UnPossessed();
}

void AFCCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void AFCCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

class UAbilitySystemComponent* AFCCharacterBase::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent.Get();
	}

	AFCPlayerState* PS = GetPlayerState<AFCPlayerState>();
	if (PS)
	{
		return PS->GetAbilitySystemComponent();
	}

	return nullptr;
}

void AFCCharacterBase::InitializeAttributes()
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!DefaultAttributes) return;

	// Server e Client
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, GetCharacterLevel(), EffectContext);
	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent.Get());
	}
}

void AFCCharacterBase::AddCharacterAbilities()
{
	// Apenas Server
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || AbilitySystemComponent->bCharacterAbilitiesGiven) return;

	for (TSubclassOf<UFCGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, GetAbilityLevel(StartupAbility.GetDefaultObject()->AbilityID), static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = true;
}

void AFCCharacterBase::RemoveCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || !AbilitySystemComponent->bCharacterAbilitiesGiven) return;

	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if ((Spec.SourceObject == this) && CharacterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		AbilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = false;
}

void AFCCharacterBase::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || AbilitySystemComponent->bStartupEffectsApplied) return;

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent.Get());
		}
	}

	AbilitySystemComponent->bStartupEffectsApplied = true;
}

void AFCCharacterBase::SetHealth(float Health)
{
	if (CharacterAttributeSet.IsValid())
	{
		CharacterAttributeSet->SetHealth(Health);
	}
}

void AFCCharacterBase::SetStamina(float Stamina)
{
	if (CharacterAttributeSet.IsValid())
	{
		CharacterAttributeSet->SetStamina(Stamina);
	}
}

void AFCCharacterBase::NotifyDeath()
{
	RemoveCharacterAbilities();

	OnCharacterDied.Broadcast(this);

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	}

	if (IsValid(DeathMontage))
	{
		PlayAnimMontage(DeathMontage);
	}
	else
	{
		NotifyFinishDeath();
	}
}

void AFCCharacterBase::NotifyFinishDeath()
{
	SetReplicateMovement(false);
	GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = true;

	GetCharacterMovement()->SetMovementMode(MOVE_None);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true, true);

	SetLifeSpan(5.f);
}

bool AFCCharacterBase::IsAlive() const
{
	return GetHealth() > 0.f;
}

bool AFCCharacterBase::IsAiming() const
{
	if (!AbilitySystemComponent.IsValid()) return false;

	return AbilitySystemComponent->GetTagCount(AimingTag) > 0;
}

int32 AFCCharacterBase::GetAbilityLevel(EAbilityInputID AbilityID) const
{
	// Não usado, a princípio.
	return 1;
}

int32 AFCCharacterBase::GetCharacterLevel() const
{
	if (CharacterAttributeSet.IsValid())
	{
		return static_cast<int32>(CharacterAttributeSet->GetCharacterLevel());
	}

	return 0;
}

float AFCCharacterBase::GetHealth() const
{
	if (CharacterAttributeSet.IsValid())
	{
		return CharacterAttributeSet->GetHealth();
	}

	return 0.0f;
}

float AFCCharacterBase::GetMaxHealth() const
{
	if (CharacterAttributeSet.IsValid())
	{
		return CharacterAttributeSet->GetMaxHealth();
	}

	return 0.0f;
}

float AFCCharacterBase::GetStamina() const
{
	if (CharacterAttributeSet.IsValid())
	{
		return CharacterAttributeSet->GetStamina();
	}

	return 0.0f;
}

float AFCCharacterBase::GetMaxStamina() const
{
	if (CharacterAttributeSet.IsValid())
	{
		return CharacterAttributeSet->GetMaxStamina();
	}

	return 0.0f;
}

float AFCCharacterBase::GetMoveSpeed() const
{
	if (CharacterAttributeSet.IsValid())
	{
		return CharacterAttributeSet->GetMoveSpeed();
	}

	return 0.0f;
}
