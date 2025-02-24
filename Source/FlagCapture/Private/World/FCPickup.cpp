#include "World/FCPickup.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/FCCharacterBase.h"
#include "Ability/FCGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AFCPickup::AFCPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCanRespawn(true)
	, RespawnTime(5.0f)
	, bIsActive(true)
{

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(FName("CollisionComp"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));

	bReplicates = true;
	bAlwaysRelevant = true;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	SetReplicateMovement(false);

	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
void AFCPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AFCPickup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AFCPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_None;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPickup, bIsActive, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPickup, PickedUpBy, Parameters);
}

void AFCPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AFCPickup::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCPickup::NotifyActorBeginOverlap(class AActor* Other)
{
	if (GetLocalRole() == ROLE_Authority && Other && Other != this)
	{
		PickupOnTouch(Cast<AFCCharacterBase>(Other));
	}
}

bool AFCPickup::CanBePickedUp(AFCCharacterBase* TestCharacter) const
{
	return bIsActive && TestCharacter && TestCharacter->IsAlive() && IsValidLowLevel() && !TestCharacter->GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(RestrictedPickupTags) && K2_CanBePickedUp(TestCharacter);
}

bool AFCPickup::K2_CanBePickedUp_Implementation(AFCCharacterBase* TestCharacter) const
{
	return true;
}

void AFCPickup::PickupOnTouch(AFCCharacterBase* Pawn)
{
	if (CanBePickedUp(Pawn))
	{
		GivePickupTo(Pawn);
		PickedUpBy = Pawn;
		bIsActive = false;
		OnPickedUp();

		if (bCanRespawn && RespawnTime > 0.0f)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_RespawnPickup, this, &AFCPickup::RespawnPickup, RespawnTime, false);
		}
		else
		{
			Destroy();
		}
	}
}

void AFCPickup::GivePickupTo(AFCCharacterBase* Pawn)
{
	UAbilitySystemComponent* ASC = Pawn->GetAbilitySystemComponent();

	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("%s Pawn's ASC is null."), *FString(__FUNCTION__));
		return;
	}

	for (TSubclassOf<UFCGameplayAbility> AbilityClass : AbilityClasses)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1, static_cast<int32>(AbilityClass.GetDefaultObject()->AbilityInputID), this);
		ASC->GiveAbilityAndActivateOnce(AbilitySpec);
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> EffectClass : EffectClasses)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectSpecHandle NewHandle = ASC->MakeOutgoingSpec(EffectClass, Pawn->GetCharacterLevel(), EffectContext);

		if (NewHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
		}
	}
}

void AFCPickup::OnPickedUp()
{
	K2_OnPickedUp();

	if (PickupSound && PickedUpBy)
	{
		UGameplayStatics::SpawnSoundAttached(PickupSound, PickedUpBy->GetRootComponent());
	}
}

void AFCPickup::RespawnPickup()
{
	bIsActive = true;
	PickedUpBy = nullptr;
	OnRespawned();

	TSet<AActor*> OverlappingPawns;
	GetOverlappingActors(OverlappingPawns, AFCCharacterBase::StaticClass());

	for (AActor* OverlappingPawn : OverlappingPawns)
	{
		PickupOnTouch(CastChecked<AFCCharacterBase>(OverlappingPawn));
	}
}

void AFCPickup::OnRespawned()
{
	K2_OnRespawned();
}

void AFCPickup::OnRep_IsActive()
{
	if (bIsActive)
	{
		OnRespawned();
	}
	else
	{
		OnPickedUp();
	}
}
