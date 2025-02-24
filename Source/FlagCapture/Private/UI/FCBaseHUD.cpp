#include "UI/FCBaseHUD.h"

#include "Player/FCPlayerController.h"
#include "Blueprint/UserWidget.h"

AFCBaseHUD::AFCBaseHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AFCBaseHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!MainWidgetClass) return;

	MainWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(), MainWidgetClass);
	if (MainWidget)
	{
		MainWidget->AddToViewport(20);

		GetWorldTimerManager().SetTimer(TimerHandle_Delay, FTimerDelegate::CreateLambda([this]()
		{
			if (!IsValidLowLevel()) return;

			OnMainWidgetInitialized();
		}), 0.1f, false);
	}
}

void AFCBaseHUD::Destroyed()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Delay);

	if (MainWidget)
	{
		MainWidget->RemoveFromParent();
	}

	Super::Destroyed();
}

void AFCBaseHUD::DrawHUD()
{
	Super::DrawHUD();
}

void AFCBaseHUD::OnMainWidgetInitialized()
{
	if (AFCPlayerController* PC = Cast<AFCPlayerController>(GetOwningPlayerController()))
	{
		PC->OnMainWidgetInitialized();
	}
}

void AFCBaseHUD::NotifyHitEvent(const FVector& HitLocation, float InDamage)
{
	K2_OnNotifyHitEvent(HitLocation, InDamage);
}
