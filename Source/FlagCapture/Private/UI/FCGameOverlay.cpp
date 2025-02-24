#include "UI/FCGameOverlay.h"

#include "Components/CanvasPanel.h"

UFCGameOverlay::UFCGameOverlay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsDead(true)
{
}

void UFCGameOverlay::NativeConstruct()
{
	Super::NativeConstruct();
}

void UFCGameOverlay::NativeDestruct()
{
	Super::NativeDestruct();
}

void UFCGameOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UFCGameOverlay::OnDeath()
{

}

void UFCGameOverlay::OnGameInitialStateChanged(EGameInitialState InGameInitialState)
{

}

void UFCGameOverlay::OnControllerStateChanged(EControllerState InControllerState)
{

}
