// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCPlayerOverlay.generated.h"

/**
 * 
 */
UCLASS()
class FLAGCAPTURE_API UFCPlayerOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> MainPanel;

public:
	UFCPlayerOverlay(const FObjectInitializer& ObjectInitializer);

	//~UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~UUserWidget

public:
	void OnPlaying();
	void OnDeath();

protected:
	UPROPERTY(BlueprintReadOnly)
	bool bIsDead = true;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnPlayingEvent();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnDeathEvent();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetMaxHealth(float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetCurrentHealth(float CurrentHealth);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetHealthPercentage(float HealthPercentage);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetHealthRegenRate(float HealthRegenRate);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetMaxStamina(float MaxStamina);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetCurrentStamina(float CurrentStamina);	

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetStaminaPercentage(float StaminaPercentage);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetStaminaRegenRate(float StaminaRegenRate);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetAmmoAmount(int32 AmmoAmount);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Events")
	void SetReserveAmmoAmount(int32 ReserveAmmoAmount);
	
};
