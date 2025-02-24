#include "Weapon/FCWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "GameFramework/Character.h"

#include "Ability/FCAbilitySystemComponent.h"
#include "Ability/FCGameplayAbility.h"
#include "Ability/Utility/FCGATA_LineTrace.h"
#include "Ability/Utility/FCGATA_SphereTrace.h"
#include "FCTypes.h"
#include "GameFramework/Controller.h"
#include "Character/FCCharacter.h"


DEFINE_LOG_CATEGORY_STATIC(LogFCWeapon, Log, All);

AFCWeapon::AFCWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MagazineCapacity(30)
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->AlwaysLoadOnClient = true;
	WeaponMesh->AlwaysLoadOnServer = true;
	WeaponMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetCanEverAffectNavigation(false);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(GetRootComponent());
	CollisionBox->SetNotifyRigidBodyCollision(false);
	CollisionBox->SetGenerateOverlapEvents(false);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));

	WeaponPrimaryInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Primary.Instant");
	WeaponSecondaryInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Secondary.Instant");
	WeaponActionAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Action");

	WeaponIsFiringTag = FGameplayTag::RequestGameplayTag("Weapon.Firing");

	AmmoAmount = MagazineCapacity;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	SetReplicateMovement(false);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0f;
}

void AFCWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_None;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCWeapon, AmmoAmount, Parameters);
}

void AFCWeapon::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	DOREPLIFETIME_ACTIVE_OVERRIDE(AFCWeapon, AmmoAmount, (AbilitySystemComponent.IsValid() && !AbilitySystemComponent->HasMatchingGameplayTag(WeaponIsFiringTag)));
}

#if WITH_EDITOR
void AFCWeapon::OnConstruction(const FTransform& Transform)
{
	if (GetWorld() && GetWorld()->WorldType == EWorldType::EditorPreview)
	{
		UpdateCollisionBox();
	}
}

void AFCWeapon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AFCWeapon::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UpdateCollisionBox();
}

void AFCWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCWeapon::BeginPlay()
{
	Super::BeginPlay();

	ForEachComponent<UPrimitiveComponent>(true, [this](UPrimitiveComponent* InChild)
	{
		if (InChild->IsA<UStaticMeshComponent>() || InChild->IsA<USkeletalMeshComponent>())
		{
			InChild->SetCanEverAffectNavigation(false);
		}
	});

	if (GetNetMode() != NM_DedicatedServer)
	{
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AFCWeapon::OnCollisionBoxOverlap);
	}

	SetActorTickEnabled(false);
}

void AFCWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (GetNetMode() != NM_DedicatedServer)
	{
		CollisionBox->OnComponentBeginOverlap.RemoveDynamic(this, &AFCWeapon::OnCollisionBoxOverlap);
	}

	Super::EndPlay(EndPlayReason);
}

void AFCWeapon::Tick(float DeltaTime)
{
	Timespan += DeltaTime;

	// Calculate Velocity
	const FVector CurrentLocation = WeaponMesh->GetComponentLocation();
	const FRotator CurrentRotation = WeaponMesh->GetComponentRotation();
	const FVector CurrentVelocity = (CurrentLocation - PreviousLocation) / (DeltaTime - PreviousLocationTime);

	const float DistanceTraveled = FVector::Distance(PreviousLocation, CurrentLocation) / 100.0f;
	DropSpeed = DistanceTraveled / DeltaTime;

	PreviousLocation = CurrentLocation;
	PreviousLocationTime = DeltaTime;

	if (Timespan >= 1.0f && CurrentVelocity.Size() <= 0.0f && DropSpeed <= 0.0f)
	{
		SetActorTickEnabled(false);

		WeaponMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		WeaponMesh->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		SetActorTickEnabled(false);
		SetActorHiddenInGame(false);

		Timespan = 0.0f;
	}
}

void AFCWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
}

void AFCWeapon::OnRep_Instigator()
{
	Super::OnRep_Instigator();

	Timespan = 0.0f;

	if (GetInstigator())
	{
		AttachToComponent(GetInstigator()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		WeaponMesh->CastShadow = false;
		SetActorHiddenInGame(true);

		CollisionBox->SetGenerateOverlapEvents(false);

		WeaponMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		WeaponMesh->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (IAbilitySystemInterface* ASI = GetInstigator<IAbilitySystemInterface>())
		{
			AbilitySystemComponent = Cast<UFCAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
		}
	}
	else
	{
		CollisionBox->SetGenerateOverlapEvents(true);

		if (UAnimInstance* const AnimInstance = WeaponMesh->GetAnimInstance())
		{
			AnimInstance->StopAllMontages(0.0f);
		}

		AbilitySystemComponent = nullptr;
	}
}

class UAbilitySystemComponent* AFCWeapon::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

void AFCWeapon::AddAbilities()
{
	if (!GetInstigator()) return;

	IAbilitySystemInterface* ASI = GetInstigator<IAbilitySystemInterface>();

	if (!ASI) return;

	UFCAbilitySystemComponent* ASC = Cast<UFCAbilitySystemComponent>(ASI->GetAbilitySystemComponent());

	if (!ASC) return;

	if (GetNetMode() != NM_Client)
	{
		for (TSubclassOf<UFCGameplayAbility>& Ability : Abilities)
		{
			AbilitySpecHandles.Add(ASC->GiveAbility(FGameplayAbilitySpec(Ability, GetAbilityLevel(Ability.GetDefaultObject()->AbilityID), static_cast<int32>(Ability.GetDefaultObject()->AbilityInputID), this)));
		}
	}
}

void AFCWeapon::RemoveAbilities()
{
	if (!GetInstigator()) return;

	IAbilitySystemInterface* ASI = GetInstigator<IAbilitySystemInterface>();

	if (!ASI) return;

	UFCAbilitySystemComponent* ASC = Cast<UFCAbilitySystemComponent>(ASI->GetAbilitySystemComponent());

	if (!ASC) return;

	if (GetNetMode() != NM_Client)
	{
		for (FGameplayAbilitySpecHandle& SpecHandle : AbilitySpecHandles)
		{
			ASC->ClearAbility(SpecHandle);
		}
	}
}

int32 AFCWeapon::GetAbilityLevel(EAbilityInputID AbilityID)
{
	// Não usado, a princípio.
	return 1;
}

void AFCWeapon::OnPickup(ACharacter* InCharacter)
{
	if (GetNetMode() != NM_Client)
	{
		Timespan = 0.0f;

		SetOwner(InCharacter);
		SetInstigator(InCharacter);
		SetLifeSpan(0.0f);

		OnRep_Instigator();
	}
}

void AFCWeapon::OnDrop(ACharacter* InCharacter)
{
	if (GetNetMode() != NM_Client)
	{
		Timespan = 0.0f;

		if (InCharacter)
		{
			if (AController* AC = InCharacter->GetController())
			{
				const FRotator& ControlRotation = AC->GetControlRotation();

				const FVector& ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
				const FVector& UpVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Z);
				const FVector& CombinedVector = UpVector.RotateAngleAxis(15.0f, ForwardVector);

				DropWeapon(InCharacter->GetActorLocation(), GetActorRotation().GetNormalized().Vector(), CombinedVector);
			}
		}

		SetOwner(nullptr);
		SetInstigator(nullptr);

		OnRep_Instigator();
	}
}

void AFCWeapon::OnEquip()
{
	ACharacter* OwningCharacter = GetInstigator<ACharacter>();

	if (!::IsValid(OwningCharacter))
	{
		UE_LOG(LogFCWeapon, Error, TEXT("%s %s OwningCharacter is nullptr"), *FC_LOGS_LINE, *GetName());
		return;
	}

	if (::IsValid(WeaponMesh))
	{
		AttachToComponent(OwningCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("VB WeaponPivot"));
		WeaponMesh->SetRelativeTransform(PlacementOffset);
		WeaponMesh->CastShadow = true;
		SetActorHiddenInGame(false);
	}
}

void AFCWeapon::OnUnEquip()
{
	ACharacter* OwningCharacter = GetInstigator<ACharacter>();

	if (!::IsValid(OwningCharacter))
	{
		UE_LOG(LogFCWeapon, Error, TEXT("%s %s OwningCharacter is nullptr"), *FC_LOGS_LINE, *GetName());
		return;
	}

	if (::IsValid(WeaponMesh))
	{
		AttachToComponent(OwningCharacter->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		WeaponMesh->CastShadow = false;
		WeaponMesh->bCastHiddenShadow = false;
		SetActorHiddenInGame(true);
	}
}

void AFCWeapon::DropWeapon_Implementation(FVector_NetQuantize InLocation, FVector_NetQuantizeNormal InRotation, FVector_NetQuantizeNormal InDirection)
{
	Timespan = 0.0f;

	SetActorHiddenInGame(false);

	PreviousLocation = WeaponMesh->GetComponentLocation();
	SetActorTickEnabled(GetNetMode() != NM_DedicatedServer);
	SetActorLocationAndRotation(InLocation, InRotation.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	WeaponMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	WeaponMesh->SetMassOverrideInKg(NAME_None, 10.0f);

	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepRelative, false));
	}

	if (GetNetMode() == NM_DedicatedServer) return;

	CollisionBox->SetGenerateOverlapEvents(true);
	WeaponMesh->SetSimulatePhysics(true);

	const FVector Impulse = InDirection * 50.0f * WeaponMesh->GetMass();
	const FVector AngularVelocity = FRotationMatrix(InRotation.Rotation()).GetScaledAxis(EAxis::X) * 10.0f;

	WeaponMesh->SetAllPhysicsLinearVelocity(Impulse);
	WeaponMesh->SetAllPhysicsAngularVelocityInRadians(AngularVelocity);
}

void AFCWeapon::SetAmmoAmount(int32 InAmmoAmount)
{
	int32 OldAmmoAmount = AmmoAmount;
	
	AmmoAmount = InAmmoAmount;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCWeapon, AmmoAmount, this);

	OnAmmoAmountChanged.Broadcast(OldAmmoAmount, AmmoAmount);
}

AFCGATA_LineTrace* AFCWeapon::GetLineTraceTargetActor()
{
	if (LineTraceTargetActor)
	{
		return LineTraceTargetActor;
	}

	LineTraceTargetActor = GetWorld()->SpawnActor<AFCGATA_LineTrace>();
	LineTraceTargetActor->SetOwner(this);
	return LineTraceTargetActor;
}

AFCGATA_SphereTrace* AFCWeapon::GetSphereTraceTargetActor()
{
	if (SphereTraceTargetActor)
	{
		return SphereTraceTargetActor;
	}

	SphereTraceTargetActor = GetWorld()->SpawnActor<AFCGATA_SphereTrace>();
	SphereTraceTargetActor->SetOwner(this);
	return SphereTraceTargetActor;
}

void AFCWeapon::UpdateCollisionBox()
{
	if (::IsValid(WeaponMesh) && WeaponMesh->GetSkeletalMeshAsset())
	{
		const FBoxSphereBounds& Bounds = WeaponMesh->GetSkeletalMeshAsset()->GetImportedBounds();
		const FVector& Origin = Bounds.Origin;
		const FVector& BoxExtent = (Bounds.GetBox().GetSize() + 10.0f) / 2.0f;

		FVector Min = Origin - BoxExtent;
		FVector Max = Origin + BoxExtent;
		const FVector& BoxCenter = FVector((Max.X + Min.X) / 2.0f, (Max.Y + Min.Y) / 2.0f, (Max.Z + Min.Z) / 2.0f);

		CollisionBox->SetBoxExtent(FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
		CollisionBox->SetRelativeLocation(FVector(BoxCenter.X, BoxCenter.Y, BoxCenter.Z));
	}
}

void AFCWeapon::OnCollisionBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidLowLevel()) return;

	if (!GetInstigator())
	{
		if (AFCCharacter* OtherCharacter = Cast<AFCCharacter>(OtherActor))
		{
			if (!OtherCharacter->IsAlive() || !OtherCharacter->GetAbilitySystemComponent() || OtherCharacter->GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(RestrictedPickupTags))
			{
				if (OtherCharacter->AddWeaponToInventory(this))
				{
					UE_LOG(LogFCWeapon, Verbose, TEXT("%s %s Pickup %s"), *FC_LOGS_LINE, *OtherCharacter->GetName(), *GetName());
				}
			}
		}
	}
}

void AFCWeapon::OnRep_AmmoAmount(int32 OldAmmoAmount)
{
	OnAmmoAmountChanged.Broadcast(OldAmmoAmount, AmmoAmount);
}
