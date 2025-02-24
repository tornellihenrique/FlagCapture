#include "Character/FCCharacter.h"

#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Kismet/GameplayStatics.h"

#include "Ability/FCAbilitySystemComponent.h"
#include "Player/FCPlayerState.h"
#include "Weapon/FCWeapon.h"
#include "Player/FCPlayerController.h"
#include "Ability/AttributeSet/FCCharacterAttributeSet.h"
#include "Ability/AttributeSet/FCAmmoAttributeSet.h"
#include "Character/FCAnimInstance.h"
#include "World/FCPlayableArea.h"


DEFINE_LOG_CATEGORY_STATIC(LogFCCharacter, Log, All);

AFCCharacter::AFCCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bInsidePlayableArea(true)
{
	AIPerceptionStimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(FName{ TEXTVIEW("AIPerceptionStimuliSourceComponent") });

	DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
	EffectRemoveOnDeathTag = FGameplayTag::RequestGameplayTag(FName("Effect.RemoveOnDeath"));

	NoWeaponTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Equipped.None"));
	WeaponChangingDelayReplicationTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.IsChangingDelayReplication"));
	WeaponAmmoTypeNoneTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));
	WeaponAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon"));
	CurrentWeaponTag = NoWeaponTag;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AFCCharacter::StaticClass();
}

void AFCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_None;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCCharacter, Inventory, Parameters);

	Parameters.Condition = COND_SimulatedOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCCharacter, CurrentWeapon, Parameters);

	Parameters.Condition = COND_OwnerOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCCharacter, bLoadoutInitialized, Parameters);
}

void AFCCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AFCCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AnimInstance = Cast<UFCAnimInstance>(GetMesh()->GetAnimInstance());
}

void AFCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSyncCurrentWeapon();
	}
}

void AFCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		CurrentWeaponTag = NoWeaponTag;
		AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
	}

	Super::EndPlay(EndPlayReason);
}

void AFCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AnimInstance)
	{
		const FRotator BaseAimRotation = IsLocallyControlled() ? GetControlRotation() : GetBaseAimRotation();
		
		FRotator Delta = BaseAimRotation - GetActorRotation();
		Delta.Normalize();
		
		AnimInstance->AimRotation = Delta;
	}

	if (IsLocallyControlled() && IsAlive())
	{
		if (::IsValid(CurrentArea))
		{
			const bool bCurrentInsidePlayableArea = CurrentArea->IsPontInsideArea3d(GetActorLocation());

			if (bCurrentInsidePlayableArea != bInsidePlayableArea)
			{
				bInsidePlayableArea = bCurrentInsidePlayableArea;

				if (AFCPlayerController* const PC = GetController<AFCPlayerController>())
				{
					PC->OnPlayableAreaStateChanged(bInsidePlayableArea);
				}
			}
		}
	}
}

#if WITH_EDITOR
void AFCCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AFCCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AFCPlayerState* PS = GetPlayerState<AFCPlayerState>();
	if (PS)
	{
		// Server
		AbilitySystemComponent = Cast<UFCAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);

		WeaponChangingDelayReplicationTagChangedDelegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(WeaponChangingDelayReplicationTag) .AddUObject(this, &AFCCharacter::WeaponChangingDelayReplicationTagChanged);

		CharacterAttributeSet = PS->GetCharacterAttributeSet();
		AmmoAttributeSet = PS->GetAmmoAttributeSet();

		InitializeAttributes();

		AddStartupEffects();
		AddCharacterAbilities();

		// Apenas para respawn
		AbilitySystemComponent->SetTagMapCount(DeadTag, 0);

		// Apenas para respawn
		SetHealth(GetMaxHealth());
		SetStamina(GetMaxStamina());
	}
}

void AFCCharacter::UnPossessed()
{
	Super::UnPossessed();
}

void AFCCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void AFCCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AFCPlayerState* PS = GetPlayerState<AFCPlayerState>();
	if (PS)
	{
		// Client
		AbilitySystemComponent = Cast<UFCAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		AbilitySystemComponent->InitAbilityActorInfo(PS, this);

		AbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &AFCCharacter::OnAbilityActivationFailed);

		CharacterAttributeSet = PS->GetCharacterAttributeSet();
		AmmoAttributeSet = PS->GetAmmoAttributeSet();

		InitializeAttributes();

		AFCPlayerController* PC = Cast<AFCPlayerController>(GetController());
		if (PC)
		{
			// @todo: Notify Player Controller for UI stuff
		}

		if (CurrentWeapon)
		{
			AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
			
			CurrentWeapon->OnPickup(this);

			if (!AmmoChangedDelegateHandle.IsValid())
			{
				AmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFCAmmoAttributeSet::GetAmmoAttributeFromTag(CurrentWeapon->AmmoType)).AddUObject(this, &AFCCharacter::CurrentWeaponAmmoReserveChanged);
			}
		}

		AbilitySystemComponent->SetTagMapCount(DeadTag, 0);

		// MaxHealth 0.0 para clients por algum motivo (bug? UE4 funciona)
// 		SetHealth(GetMaxHealth());
// 		SetStamina(GetMaxStamina());
	}
}

void AFCCharacter::NotifyFinishDeath()
{
	if (GetNetMode() == NM_Client) return Super::NotifyFinishDeath();

	RemoveAllWeaponsFromInventory();

	AbilitySystemComponent->RegisterGameplayTagEvent(WeaponChangingDelayReplicationTag).Remove(WeaponChangingDelayReplicationTagChangedDelegateHandle);

	// @todo: Notify gamemode

	RemoveCharacterAbilities();

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	}

	OnCharacterDied.Broadcast(this);

	Super::NotifyFinishDeath();
}

void AFCCharacter::PopulateLoadout(AController* InController, const FPlayerLoadout& InLoadout)
{
	UE_LOG(LogFCCharacter, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	int32 NumWeaponClasses = InLoadout.Weapons.Num();
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (!InLoadout.Weapons[i].IsValid()) continue;

		const TSubclassOf<AFCWeapon>& WeaponClass = InLoadout.Weapons[i].TryLoadClass<AFCWeapon>();
		if (WeaponClass)
		{
			AFCWeapon* NewWeapon = GetWorld()->SpawnActorDeferred<AFCWeapon>(WeaponClass, FTransform::Identity, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			// ...

			UGameplayStatics::FinishSpawningActor(NewWeapon, FTransform::Identity);

			AddWeaponToInventory(NewWeapon);
		}
	}

	bLoadoutInitialized = true;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCCharacter, bLoadoutInitialized, this);

	OnLoadoutInitialized.ExecuteIfBound();
}

bool AFCCharacter::IsWeaponListInitialized() const
{
	return bLoadoutInitialized;
}

void AFCCharacter::OnPlaying(AFCPlayableArea* InCurrentArea)
{
	if (GetWeapon()) return;

	CurrentArea = InCurrentArea;

	K2_OnPlaying();

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerOnPlaying(InCurrentArea);

		return;
	}

	const TArray<AFCWeapon*>& WeaponList = GetWeaponList();
	if (WeaponList.Num() > 0)
	{
		EquipWeapon(WeaponList[0]);
		ClientSyncCurrentWeapon(CurrentWeapon);
	}

	GetWorldTimerManager().SetTimer(TimerHandle_Damageable, this, &AFCCharacter::Damageable, DamageableTimeAfterRespawn, false);
}

void AFCCharacter::ServerOnPlaying_Implementation(AFCPlayableArea* InCurrentArea)
{
	OnPlaying(InCurrentArea);
}

void AFCCharacter::OnMatchEnded()
{
	if (IsLocallyControlled())
	{
		if (AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
			CurrentWeaponTag = NoWeaponTag;
			AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
		}

		DisableInput(GetController<APlayerController>());
	}
}

void AFCCharacter::Damageable()
{
	SetCanBeDamaged(true);
}

void AFCCharacter::SetCurrentWeapon(AFCWeapon* NewWeapon, AFCWeapon* LastWeapon)
{
	if (NewWeapon == LastWeapon) return;

	if (AbilitySystemComponent.IsValid())
	{
		FGameplayTagContainer AbilityTagsToCancel = FGameplayTagContainer(WeaponAbilityTag);
		AbilitySystemComponent->CancelAbilities(&AbilityTagsToCancel);
	}

	UnEquipWeapon(LastWeapon);

	if (NewWeapon)
	{
		if (AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		}

		CurrentWeapon = NewWeapon;
		MARK_PROPERTY_DIRTY_FROM_NAME(AFCCharacter, CurrentWeapon, this);

		CurrentWeapon->OnEquip();

		if (AnimInstance)
		{
			AnimInstance->BasePose = CurrentWeapon->AnimPose;
			AnimInstance->WeaponOffset = CurrentWeapon->PositionOffset;
		}

		CurrentWeaponTag = CurrentWeapon->WeaponTag;
		if (AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
		}

		OnAmmoAmountChanged.Broadcast(CurrentWeapon->GetAmmoAmount());
		OnAmmoReserveChanged.Broadcast(GetAmmoReserveAmount());

		CurrentWeapon->OnAmmoAmountChanged.AddDynamic(this, &AFCCharacter::CurrentWeaponAmmoAmountChanged);

		if (AbilitySystemComponent.IsValid())
		{
			FGameplayTag AmmoType = CurrentWeapon->AmmoType;
			AmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFCAmmoAttributeSet::GetAmmoAttributeFromTag(AmmoType)).AddUObject(this, &AFCCharacter::CurrentWeaponAmmoReserveChanged);
		}

		if (CurrentWeapon->EquipAnim)
		{
			PlayAnimMontage(CurrentWeapon->EquipAnim);
		}
	}
	else
	{
		UnEquipCurrentWeapon();
	}
}

void AFCCharacter::UnEquipWeapon(AFCWeapon* InWeapon)
{
	if (InWeapon)
	{
		if (AFCWeapon* WeaponToUnEquip = GetWeapon())
		{
			WeaponToUnEquip->OnAmmoAmountChanged.RemoveDynamic(this, &AFCCharacter::CurrentWeaponAmmoAmountChanged);

			if (AbilitySystemComponent.IsValid())
			{
				FGameplayTag AmmoType = WeaponToUnEquip->AmmoType;
				AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFCAmmoAttributeSet::GetAmmoAttributeFromTag(AmmoType)).Remove(AmmoChangedDelegateHandle);
			}
		}

		InWeapon->OnUnEquip();
	}
}

void AFCCharacter::UnEquipCurrentWeapon()
{
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		CurrentWeaponTag = NoWeaponTag;
		AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
	}

	UnEquipWeapon(CurrentWeapon);
	CurrentWeapon = nullptr;

	OnAmmoAmountChanged.Broadcast(0);
	OnAmmoReserveChanged.Broadcast(0);
}

void AFCCharacter::ServerSyncCurrentWeapon_Implementation()
{
	ClientSyncCurrentWeapon(CurrentWeapon);
}

void AFCCharacter::ClientSyncCurrentWeapon_Implementation(AFCWeapon* InWeapon)
{
	AFCWeapon* LastWeapon = CurrentWeapon;
	CurrentWeapon = InWeapon;
	OnRep_CurrentWeapon(LastWeapon);
}

void AFCCharacter::OnAbilityActivationFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags)
{
	if (FailedAbility && FailedAbility->GetAssetTags().HasTagExact(FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.IsChanging"))))
	{
		if (bChangedWeaponLocally)
		{
			UE_LOG(LogFCCharacter, Warning, TEXT("%s Weapon Changing ability activation failed. Syncing CurrentWeapon. %s"), *FC_LOGS_LINE, *FailTags.ToString());

			ServerSyncCurrentWeapon();
		}
	}
}

bool AFCCharacter::AddWeaponToInventory(AFCWeapon* NewWeapon)
{
	if (HasWeapon(NewWeapon))
	{
		// @todo: Finish
	}

	if (GetNetMode() == NM_Client) return false;

	Inventory.Weapons.Add(NewWeapon);
	NewWeapon->OnPickup(this);
	NewWeapon->AddAbilities();

	return true;
}

bool AFCCharacter::RemoveWeaponFromInventory(AFCWeapon* WeaponToRemove)
{
	if (HasWeapon(WeaponToRemove))
	{
		if (WeaponToRemove == CurrentWeapon)
		{
			UnEquipCurrentWeapon();
		}

		Inventory.Weapons.Remove(WeaponToRemove);
		WeaponToRemove->RemoveAbilities();
		WeaponToRemove->OnDrop(this);

		return true;
	}

	return false;
}

void AFCCharacter::RemoveAllWeaponsFromInventory()
{
	if (GetNetMode() == NM_Client) return;

	UnEquipCurrentWeapon();

	for (int32 i = Inventory.Weapons.Num() - 1; i >= 0; i--)
	{
		AFCWeapon* Weapon = Inventory.Weapons[i];
		RemoveWeaponFromInventory(Weapon);
	}
}

void AFCCharacter::EquipWeapon(AFCWeapon* NewWeapon)
{
	if (GetNetMode() == NM_Client)
	{
		ServerEquipWeapon(NewWeapon);
		SetCurrentWeapon(NewWeapon, CurrentWeapon);
		bChangedWeaponLocally = true;
	}
	else
	{
		SetCurrentWeapon(NewWeapon, CurrentWeapon);
	}
}

void AFCCharacter::ServerEquipWeapon_Implementation(AFCWeapon* NewWeapon)
{
	EquipWeapon(NewWeapon);
}

void AFCCharacter::NextWeapon()
{
	if (Inventory.Weapons.Num() < 2) return;

	const auto TargetWeapon = GetNextWeapon();

	UnEquipCurrentWeapon();

	EquipWeapon(TargetWeapon);
}

void AFCCharacter::PreviousWeapon()
{
	if (Inventory.Weapons.Num() < 2) return;

	const auto TargetWeapon = GetPreviousWeapon();

	UnEquipCurrentWeapon();

	EquipWeapon(TargetWeapon);
}

bool AFCCharacter::HasWeapon(AFCWeapon* InWeapon)
{
	for (AFCWeapon* Weapon : Inventory.Weapons)
	{
		if (Weapon && InWeapon && InWeapon->IsA(Weapon->GetClass()))
		{
			return true;
		}
	}

	return false;
}

ETeamSide AFCCharacter::GetPlayerSide() const
{
	if (AFCPlayerState* PS = GetPlayerState<AFCPlayerState>())
	{
		return PS->GetPlayerSide();
	}

	return ETeamSide::None;
}

int32 AFCCharacter::GetAmmoAmount() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetAmmoAmount();
	}

	return 0;
}

int32 AFCCharacter::GetMaxAmmoAmount() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->MagazineCapacity;
	}

	return 0;
}

int32 AFCCharacter::GetAmmoReserveAmount() const
{
	if (CurrentWeapon && AmmoAttributeSet.IsValid())
	{
		FGameplayAttribute Attribute = AmmoAttributeSet->GetAmmoAttributeFromTag(CurrentWeapon->AmmoType);
		if (Attribute.IsValid())
		{
			return AbilitySystemComponent->GetNumericAttribute(Attribute);
		}
	}

	return 0;
}

int32 AFCCharacter::GetNumWeapons() const
{
	return Inventory.Weapons.Num();
}

AFCWeapon* AFCCharacter::GetNextWeapon()
{
	if (Inventory.Weapons.Num() < 2)
	{
		return Inventory.Weapons[0];
	}

	const int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);

	if (CurrentWeaponIndex == INDEX_NONE)
	{
		return Inventory.Weapons[0];
	}

	return Inventory.Weapons[(CurrentWeaponIndex + 1) % Inventory.Weapons.Num()];
}

AFCWeapon* AFCCharacter::GetPreviousWeapon()
{
	if (Inventory.Weapons.Num() < 2)
	{
		return Inventory.Weapons[0];
	}

	const int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);

	if (CurrentWeaponIndex == INDEX_NONE)
	{
		return Inventory.Weapons[0];
	}

	return Inventory.Weapons[FMath::Abs(CurrentWeaponIndex - 1 + Inventory.Weapons.Num()) % Inventory.Weapons.Num()];
}

void AFCCharacter::CurrentWeaponAmmoAmountChanged(int32 OldAmmoAmount, int32 NewAmmoAmount)
{
	OnAmmoAmountChanged.Broadcast(NewAmmoAmount);
}

void AFCCharacter::CurrentWeaponAmmoReserveChanged(const FOnAttributeChangeData& Data)
{
	OnAmmoReserveChanged.Broadcast(Data.NewValue);
}

void AFCCharacter::WeaponChangingDelayReplicationTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (CallbackTag == WeaponChangingDelayReplicationTag)
	{
		if (NewCount < 1)
		{
			ClientSyncCurrentWeapon(CurrentWeapon);
		}
	}
}

void AFCCharacter::OnRep_CurrentWeapon(AFCWeapon* LastWeapon)
{
	bChangedWeaponLocally = false;
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AFCCharacter::OnRep_Inventory()
{
	if (GetLocalRole() == ROLE_AutonomousProxy && Inventory.Weapons.Num() > 0 && !CurrentWeapon)
	{
		ServerSyncCurrentWeapon();
	}
}
