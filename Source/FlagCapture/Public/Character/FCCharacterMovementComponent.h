#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "FCCharacterMovementComponent.generated.h"


UCLASS()
class FLAGCAPTURE_API UFCCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	class FFCSavedMove : public FSavedMove_Character
	{
		public:
			typedef FSavedMove_Character Super;

			virtual void Clear() override;
			virtual uint8 GetCompressedFlags() const override;
			virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

			virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
			virtual void PrepMoveFor(class ACharacter* Character) override;

			uint8 SavedRequestToStartSprinting : 1;
			uint8 SavedRequestToStartAiming : 1;
	};

	class FGSNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
	{
		public:
			FGSNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement);

			typedef FNetworkPredictionData_Client_Character Super;

			virtual FSavedMovePtr AllocateNewMove() override;
	};

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC Character Movement Component")
	float SprintSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC Character Movement Component")
	float AimingSpeedMultiplier;

public:
	UFCCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	uint8 RequestToStartSprinting : 1;
	uint8 RequestToStartAiming : 1;

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

public:
	UFUNCTION(BlueprintCallable, Category = "FC Character Movement Component")
	void StartSprinting();
	UFUNCTION(BlueprintCallable, Category = "FC Character Movement Component")
	void StopSprinting();

	UFUNCTION(BlueprintCallable, Category = "FC Character Movement Component")
	void StartAiming();
	UFUNCTION(BlueprintCallable, Category = "FC Character Movement Component")
	void StopAiming();

};
