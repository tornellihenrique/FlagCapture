#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FCAnimInstance.generated.h"

class AFCCharacter;

UCLASS()
class FLAGCAPTURE_API UFCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFCAnimInstance();

public:
	virtual void NativeInitializeAnimation() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	TObjectPtr<UAnimSequence> BasePose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	FRotator AimRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	FVector WeaponOffset;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	AFCCharacter* Character;
	
};
