#include "UI/FCScoreboardSlot.h"
#include "Components/Button.h"

UFCScoreboardSlot::UFCScoreboardSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCScoreboardSlot::NativeConstruct()
{
	Super::NativeConstruct();

	MainButton->OnClicked.AddDynamic(this, &UFCScoreboardSlot::OnMainButtonClicked);
}

void UFCScoreboardSlot::NativeDestruct()
{
	MainButton->OnClicked.RemoveDynamic(this, &UFCScoreboardSlot::OnMainButtonClicked);

	Super::NativeDestruct();
}

void UFCScoreboardSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CachedPlayerState != PlayerState)
	{
		AFCPlayerState* OldPlayerState = CachedPlayerState;
		CachedPlayerState = PlayerState;
		K2_OnPlayerStateChanged(OldPlayerState);
	}
}

void UFCScoreboardSlot::OnMainButtonClicked()
{
	if (OnScoreboardSlotClicked.IsBound())
	{
		OnScoreboardSlotClicked.Broadcast(this);
	}
}
