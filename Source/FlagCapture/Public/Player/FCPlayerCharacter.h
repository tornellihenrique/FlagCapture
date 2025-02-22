// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacter.h"
#include "FCPlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class UCameraComponent;

UCLASS()
class FLAGCAPTURE_API AFCPlayerCharacter : public AFCCharacter
{
	GENERATED_BODY()

	UPROPERTY(Category = "Player Character", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Input", meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Input", meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookMouseAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Input", meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Input", meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Input", meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{1.0f};

public:
	AFCPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	//~APawn
	virtual void OnRep_PlayerState() override;
	virtual void NotifyControllerChanged() override;
	//~APawn

protected:
	void BindASCInput();

protected:
	UPROPERTY()
	uint8 bASCInputBound : 1;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	void Input_OnLookMouse(const FInputActionValue& ActionValue);
	void Input_OnMove(const FInputActionValue& ActionValue);

};
