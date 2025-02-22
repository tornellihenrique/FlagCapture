#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "NativeGameplayTags.h"
#include "FCTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "FCWeapon.generated.h"

class USkeletalMeshComponent;
class UBoxComponent;

class UFCGameplayAbility;
class AFCGATA_LineTrace;
class AFCGATA_SphereTrace;
class UFCAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponAmmoChangedDelegate, int32, OldValue, int32, NewValue);

UCLASS(Abstract, meta = (DisplayName = "Weapon"))
class FLAGCAPTURE_API AFCWeapon : public AActor
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

	UPROPERTY(Category = "Weapon", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(Category = "Weapon", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FGameplayTag WeaponTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	TArray<TSubclassOf<UFCGameplayAbility>> Abilities;

	/** Tags that block a character from pickup this weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FGameplayTagContainer RestrictedPickupTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	int32 MagazineCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FGameplayTag AmmoType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	TObjectPtr<UAnimSequence> AnimPose;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	TObjectPtr<UAnimMontage> EquipAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FVector PositionOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FTransform PlacementOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	TObjectPtr<USoundBase> PickupSound;

public:
	AFCWeapon(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_Owner() override;
	virtual void OnRep_Instigator() override;
	//~AActor

	//~IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~IAbilitySystemInterface

public:
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FWeaponAmmoChangedDelegate OnAmmoAmountChanged;;

public:
	virtual void AddAbilities();
	virtual void RemoveAbilities();
	virtual int32 GetAbilityLevel(EAbilityInputID AbilityID);

	virtual void OnPickup(ACharacter* InCharacter);
	virtual void OnDrop(ACharacter* InCharacter);

	virtual void OnEquip();
	virtual void OnUnEquip();

	UFUNCTION(NetMulticast, Reliable)
	virtual void DropWeapon(FVector_NetQuantize InLocation, FVector_NetQuantizeNormal InRotation, FVector_NetQuantizeNormal InDirection);

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SetAmmoAmount(int32 InAmmoAmount);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AFCGATA_LineTrace* GetLineTraceTargetActor();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AFCGATA_SphereTrace* GetSphereTraceTargetActor();

protected:
	virtual void UpdateCollisionBox();

	UFUNCTION()
	virtual void OnCollisionBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TWeakObjectPtr<UFCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon", ReplicatedUsing = OnRep_AmmoAmount)
	int32 AmmoAmount;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	AFCGATA_LineTrace* LineTraceTargetActor;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	AFCGATA_SphereTrace* SphereTraceTargetActor;

	FGameplayTag WeaponPrimaryInstantAbilityTag;
	FGameplayTag WeaponSecondaryInstantAbilityTag;
	FGameplayTag WeaponActionAbilityTag;
	FGameplayTag WeaponIsFiringTag;

	UPROPERTY()
	float Timespan;

	float DropSpeed;
	FVector PreviousLocation;
	float PreviousLocationTime;

public:
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE int32 GetAmmoAmount() const { return AmmoAmount; }

protected:
	UFUNCTION()
	virtual void OnRep_AmmoAmount(int32 OldAmmoAmount);

};
