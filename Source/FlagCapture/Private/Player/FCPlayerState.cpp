#include "Player/FCPlayerState.h"

#include "Ability/FCAbilitySystemComponent.h"
#include "Ability/AttributeSet/FCCharacterAttributeSet.h"
#include "Ability/AttributeSet/FCAmmoAttributeSet.h"
#include "Character/FCCharacter.h"
#include "Weapon/FCWeapon.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogFCPlayerState, Log, All);

AFCPlayerState::AFCPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Kills(0)
	, Deaths(0)
	, Headshots(0)
	, DamageDealt(0.f)
	, PlayerSide(ETeamSide::None)
	, FlagsCaptured(0)
	, KilledPlayers({})
	, MainCharacter(nullptr)
{
	AbilitySystemComponent = CreateDefaultSubobject<UFCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Replicação completa apenas para Autonomous Proxies
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CharacterAttributeSet = CreateDefaultSubobject<UFCCharacterAttributeSet>(TEXT("CharacterAttributeSet"));
	AmmoAttributeSet = CreateDefaultSubobject<UFCAmmoAttributeSet>(TEXT("AmmoAttributeSet"));

	// Cache tags
	DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
}

void AFCPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CharacterAttributeSet->GetHealthAttribute()).AddUObject(this, &AFCPlayerState::HealthChanged);
	}
}

void AFCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, Kills, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, Deaths, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, Headshots, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, DamageDealt, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, PlayerSide, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, FlagsCaptured, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, KilledPlayers, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerState, MainCharacter, Parameters);
}

void AFCPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	Super::RegisterPlayerWithSession(bWasFromInvite);
}

void AFCPlayerState::UnregisterPlayerWithSession()
{
	Super::UnregisterPlayerWithSession();
}

void AFCPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	K2_OnRep_PlayerName();
}

class UAbilitySystemComponent* AFCPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFCPlayerState::JoinTeam(ETeamSide InTeam)
{
	if (PlayerSide != InTeam)
	{
		PlayerSide = InTeam;

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, PlayerSide, this);

		OnPlayerJoinTeam.ExecuteIfBound(PlayerSide);
	}
}

void AFCPlayerState::AddKill()
{
	Kills++;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, Kills, this);
}

void AFCPlayerState::AddDeath()
{
	Deaths++;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, Kills, this);
}

void AFCPlayerState::AddHeadshot()
{
	Headshots++;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, Headshots, this);
}

void AFCPlayerState::AddDamage(float DamageAmount)
{
	DamageDealt += DamageAmount;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, DamageDealt, this);
}

void AFCPlayerState::AddFlagsCaptured()
{
	FlagsCaptured++;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, FlagsCaptured, this);

	SetScore(GetScore() + 10);
}

void AFCPlayerState::AddKilledPlayer(AFCPlayerState* InPS)
{
	if (InPS && InPS != this)
	{
		KilledPlayers.Add(InPS);

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, KilledPlayers, this);
	}
}

void AFCPlayerState::SetMainCharacter(AFCCharacter* Character)
{
	if (MainCharacter != Character)
	{
		MainCharacter = Character;

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerState, MainCharacter, this);
	}
}

void AFCPlayerState::ViewTargetChanged(APawn* InPawn)
{
	if (InPawn && ::IsValid(InPawn))
	{
		K2_OnViewTargetChanged(InPawn);
	}
}

UFCCharacterAttributeSet* AFCPlayerState::GetCharacterAttributeSet() const
{
	return CharacterAttributeSet;
}

UFCAmmoAttributeSet* AFCPlayerState::GetAmmoAttributeSet() const
{
	return AmmoAttributeSet;
}

void AFCPlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
	AFCCharacter* Character = Cast<AFCCharacter>(GetPawn());
	if (IsValid(Character) && !IsAlive() && !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		if (Character)
		{
			Character->NotifyDeath();
		}
	}
}

bool AFCPlayerState::IsEnemyFor(AFCPlayerState* Other)
{
	return Other && Other != this && Other->GetPlayerSide() != GetPlayerSide();
}

float AFCPlayerState::GetKDRatio() const
{
	return Deaths == 0 ? static_cast<float>(Kills) : static_cast<float>(Kills) / static_cast<float>(Deaths);
}

float AFCPlayerState::GetHSRatio() const
{
	return Kills == 0 ? static_cast<float>(Headshots) : static_cast<float>(Headshots) / static_cast<float>(Kills);
}

int32 AFCPlayerState::GetKilledPlayerNum(AFCPlayerState* InPS) const
{
	TArray<AFCPlayerState*> KilledByPlayer = KilledPlayers.FilterByPredicate([InPS](AFCPlayerState* PS)
	{
		return PS && PS == InPS;
	});

	return KilledByPlayer.Num();
}

FUniqueNetIdRepl AFCPlayerState::GetUniqueNetId() const
{
	return GetUniqueId();
}

int32 AFCPlayerState::GetAmmoReserveAmount() const
{
	AFCCharacter* Character = GetPawn<AFCCharacter>();
	if (Character && Character->GetWeapon() && AmmoAttributeSet)
	{
		FGameplayTag AmmoType = Character->GetWeapon()->AmmoType;
		FGameplayAttribute Attribute = AmmoAttributeSet->GetAmmoAttributeFromTag(AmmoType);
		if (Attribute.IsValid())
		{
			return AbilitySystemComponent->GetNumericAttribute(Attribute);
		}
	}

	return 0;
}
