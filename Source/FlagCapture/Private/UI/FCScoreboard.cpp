#include "UI/FCScoreboard.h"

#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"

#include "Game/FCGameState.h"
#include "Player/FCPlayerState.h"
#include "UI/FCScoreboardSlot.h"

UFCScoreboard::UFCScoreboard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCScoreboard::NativeConstruct()
{
	Super::NativeConstruct();

	if (AFCGameState* GS = GetWorld()->GetGameState<AFCGameState>())
	{
		GS->OnTeamChangedEvent.AddDynamic(this, &UFCScoreboard::OnTeamChangedEvent);
	}

	ActivePlayerState = GetOwningPlayerState<AFCPlayerState>();
	K2_OnPlayerStateChanged();

	BuildList();
}

void UFCScoreboard::NativeDestruct()
{
	if (AFCGameState* GS = GetWorld()->GetGameState<AFCGameState>())
	{
		GS->OnTeamChangedEvent.RemoveDynamic(this, &UFCScoreboard::OnTeamChangedEvent);
	}

	for (int32 i = 0; i < AContainer->GetChildrenCount(); i++)
	{
		if (UFCScoreboardSlot* ScoreboardSlot = Cast<UFCScoreboardSlot>(AContainer->GetChildAt(i)))
		{
			ScoreboardSlot->OnScoreboardSlotClicked.RemoveDynamic(this, &UFCScoreboard::OnScoreboardSlotClicked);
		}
	}

	for (int32 i = 0; i < BContainer->GetChildrenCount(); i++)
	{
		if (UFCScoreboardSlot* ScoreboardSlot = Cast<UFCScoreboardSlot>(BContainer->GetChildAt(i)))
		{
			ScoreboardSlot->OnScoreboardSlotClicked.RemoveDynamic(this, &UFCScoreboard::OnScoreboardSlotClicked);
		}
	}

	Super::NativeDestruct();
}

void UFCScoreboard::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float AContainerY = AContainer->GetCachedGeometry().GetLocalSize().Y;
	const float AScrollY = AScrollBox->GetCachedGeometry().GetLocalSize().Y;

	const bool bOverflowA = AContainerY >= AScrollY;

	const float ATargetWidth = bOverflowA ? 113.0f : 100.0f;
	const float ATargetPadding = bOverflowA ? 13.0f : 0.0f;

	AScoreBox->SetWidthOverride(ATargetWidth);
	AScoreBorder->SetPadding(FMargin(0.0f, 0.0f, ATargetPadding, 0.0f));

	const float BContainerY = BContainer->GetCachedGeometry().GetLocalSize().Y;
	const float BScrollY = BScrollBox->GetCachedGeometry().GetLocalSize().Y;

	const bool bOverflowB = BContainerY >= BScrollY;

	const float BTargetWidth = bOverflowB ? 113.0f : 100.0f;
	const float BTargetPadding = bOverflowB ? 13.0f : 0.0f;

	BScoreBox->SetWidthOverride(BTargetWidth);
	BScoreBorder->SetPadding(FMargin(0.0f, 0.0f, BTargetPadding, 0.0f));
}

void UFCScoreboard::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);

	if (!IsVisible())
	{
		ActivePlayerState = GetOwningPlayerState<AFCPlayerState>();
		K2_OnPlayerStateChanged();
	}
}

void UFCScoreboard::OnTeamChangedEvent(ETeamSide Team)
{
	BuildList();
}

void UFCScoreboard::OnScoreboardSlotClicked(UFCScoreboardSlot* WidgetRef)
{
	if (!WidgetRef) return;

	for (int32 i = 0; i < AContainer->GetChildrenCount(); i++)
	{
		if (UFCScoreboardSlot* ScoreboardSlot = Cast<UFCScoreboardSlot>(AContainer->GetChildAt(i)))
		{
			ScoreboardSlot->bActive = false;
		}
	}

	for (int32 i = 0; i < BContainer->GetChildrenCount(); i++)
	{
		if (UFCScoreboardSlot* ScoreboardSlot = Cast<UFCScoreboardSlot>(BContainer->GetChildAt(i)))
		{
			ScoreboardSlot->bActive = false;
		}
	}

	WidgetRef->bActive = true;

	ActivePlayerState = WidgetRef->PlayerState;
	K2_OnPlayerStateChanged();
}

void UFCScoreboard::BuildList()
{
	if (!ScoreboardSlotClass) return;

	if (AFCGameState* GS = GetWorld()->GetGameState<AFCGameState>())
	{
		TArray<AFCPlayerState*> ATeam = GS->GetPlayersInSide(ETeamSide::Team_A);
		const int32 MaxSlotA = ATeam.Num();

		AContainer->ClearChildren();

		for (int32 i = 0; i < MaxSlotA; i++)
		{
			UFCScoreboardSlot* ScoreboardSlot = CreateWidget<UFCScoreboardSlot>(GetOwningPlayer(), ScoreboardSlotClass);
			if (ScoreboardSlot)
			{
				AContainer->AddChildToVerticalBox(ScoreboardSlot);

				ScoreboardSlot->OnScoreboardSlotClicked.AddDynamic(this, &UFCScoreboard::OnScoreboardSlotClicked);
			}
		}

		TArray<AFCPlayerState*> BTeam = GS->GetPlayersInSide(ETeamSide::Team_B);
		const int32 MaxSlotB = BTeam.Num();

		BContainer->ClearChildren();

		for (int32 i = 0; i < MaxSlotB; i++)
		{
			UFCScoreboardSlot* ScoreboardSlot = CreateWidget<UFCScoreboardSlot>(GetOwningPlayer(), ScoreboardSlotClass);
			if (ScoreboardSlot)
			{
				BContainer->AddChildToVerticalBox(ScoreboardSlot);

				ScoreboardSlot->OnScoreboardSlotClicked.AddDynamic(this, &UFCScoreboard::OnScoreboardSlotClicked);
			}
		}

		UpdateList();
	}
}

void UFCScoreboard::UpdateList()
{
	if (AFCGameState* GS = GetWorld()->GetGameState<AFCGameState>())
	{
		TArray<AFCPlayerState*> ATeam = GS->GetPlayersInSide(ETeamSide::Team_A);
		ATeam.Sort([](const AFCPlayerState& A, const AFCPlayerState& B)
		{
			return &A && &B && A.GetScore() > B.GetScore();
		});

		for (int32 i = 0; i < AContainer->GetChildrenCount(); i++)
		{
			if (UFCScoreboardSlot* ScoreboardSlot = Cast<UFCScoreboardSlot>(AContainer->GetChildAt(i)))
			{
				ScoreboardSlot->PlayerState = NULL;

				if (ATeam.IsValidIndex(i))
				{
					ScoreboardSlot->PlayerState = ATeam[i];
				}

				ScoreboardSlot->bActive = ScoreboardSlot->PlayerState == ActivePlayerState;

				ESlateVisibility TargetVisibility = ScoreboardSlot->PlayerState != nullptr ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
				ScoreboardSlot->SetVisibility(TargetVisibility);
			}
		}

		TArray<AFCPlayerState*> BTeam = GS->GetPlayersInSide(ETeamSide::Team_B);
		BTeam.Sort([](const AFCPlayerState& A, const AFCPlayerState& B)
		{
			return &A && &B && A.GetScore() > B.GetScore();
		});

		for (int32 i = 0; i < BContainer->GetChildrenCount(); i++)
		{
			if (UFCScoreboardSlot* ScoreboardSlot = Cast<UFCScoreboardSlot>(BContainer->GetChildAt(i)))
			{
				ScoreboardSlot->PlayerState = NULL;

				if (BTeam.IsValidIndex(i))
				{
					ScoreboardSlot->PlayerState = BTeam[i];
				}

				ScoreboardSlot->bActive = ScoreboardSlot->PlayerState == ActivePlayerState;

				ESlateVisibility TargetVisibility = ScoreboardSlot->PlayerState != nullptr ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
				ScoreboardSlot->SetVisibility(TargetVisibility);
			}
		}
	}
}
