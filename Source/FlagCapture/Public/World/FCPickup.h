#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "FCPickup.generated.h"

class AFCCharacterBase;
class UCapsuleComponent;
class USoundCue;
class UFCGameplayAbility;
class UGameplayEffect;

UCLASS(meta = (DisplayName = "Pickup"))
class FLAGCAPTURE_API AFCPickup : public AActor
{
	GENERATED_BODY()

	UPROPERTY(Category = "Pickup", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CollisionComp;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs")
	bool bCanRespawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs")
	float RespawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs")
	FGameplayTagContainer RestrictedPickupTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs")
	TObjectPtr<USoundCue> PickupSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs")
	TArray<TSubclassOf<UFCGameplayAbility>> AbilityClasses;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs")
	TArray<TSubclassOf<UGameplayEffect>> EffectClasses;

public:
	AFCPickup(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void NotifyActorBeginOverlap(class AActor* Other) override;

public:
	virtual bool CanBePickedUp(AFCCharacterBase* TestCharacter) const;

	UFUNCTION(BlueprintNativeEvent, Meta = (DisplayName = "CanBePickedUp"))
	bool K2_CanBePickedUp(AFCCharacterBase* TestCharacter) const;

protected:
	void PickupOnTouch(AFCCharacterBase* Pawn);

	virtual void GivePickupTo(AFCCharacterBase* Pawn);

	virtual void OnPickedUp();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Picked Up"))
	void K2_OnPickedUp();

	virtual void RespawnPickup();

	virtual void OnRespawned();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Respawned"))
	void K2_OnRespawned();

	UFUNCTION()
	virtual void OnRep_IsActive();

protected:
	UPROPERTY(BlueprintReadOnly, Replicated)
	AFCCharacterBase* PickedUpBy;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsActive)
	bool bIsActive;

	FTimerHandle TimerHandle_RespawnPickup;

};
