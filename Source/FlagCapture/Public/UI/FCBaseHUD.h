#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FCBaseHUD.generated.h"

UCLASS()
class FLAGCAPTURE_API AFCBaseHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
	TSubclassOf<UUserWidget> MainWidgetClass;

public:
	AFCBaseHUD(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	//~AActor
	
	//~AHUD
	virtual void DrawHUD() override;
	//~AHUD

public:
	virtual void OnMainWidgetInitialized();

	template<class T>
	T* GetMainWidget() const
	{
		return Cast<T>(MainWidget);
	}

protected:
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<UUserWidget> MainWidget;

	FTimerHandle TimerHandle_Delay;

public:
	virtual void NotifyHitEvent(const FVector& HitLocation, float InDamage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events", meta = (DisplayName = "On Notify Hit Event"))
	void K2_OnNotifyHitEvent(const FVector& HitLocation, float InDamage);
	
};
