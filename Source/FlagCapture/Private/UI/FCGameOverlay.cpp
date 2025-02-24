#include "UI/FCGameOverlay.h"

#include "Components/CanvasPanel.h"

UFCGameOverlay::UFCGameOverlay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsDead(false)
{
}

void UFCGameOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	MainPanel->SetRenderOpacity(0.0f);
}

void UFCGameOverlay::NativeDestruct()
{
	Super::NativeDestruct();
}

void UFCGameOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (MainPanel->GetRenderOpacity() <= 1.0f && !bIsDead)
	{
		const float TargetOpacity = FMath::FInterpTo(MainPanel->GetRenderOpacity(), 1.0f, InDeltaTime, 4.0f);
		MainPanel->SetRenderOpacity(TargetOpacity);
	}
}

void UFCGameOverlay::OnDeath()
{
	bIsDead = true;

	OnDeathEvent();
}

void UFCGameOverlay::OnGameInitialStateChanged(EGameInitialState InGameInitialState)
{
	GameInitialState = InGameInitialState;
	OnGameInitialStateChangedEvent();
}

void UFCGameOverlay::OnControllerStateChanged(EControllerState InControllerState)
{
	ControllerState = InControllerState;
	OnControllerStateChangedEvent();
}
