#include "Game/FCGameViewportClient.h"
#include "Engine/Canvas.h"

void UFCGameViewportClient::PostRender(UCanvas* Canvas)
{
	Super::PostRender(Canvas);

	if (bFading)
	{
		if (const UWorld* ActiveWorld = World.Get())
		{
			const float Time = ActiveWorld->GetTimeSeconds();
			FadeAlpha = FMath::Clamp((Time - FadeStartTime) / FadeDuration, 0.f, 1.f);

			FColor OldColor = Canvas->DrawColor;
			FLinearColor FadeColor = FLinearColor::Black;
			FadeColor.A = bFadeToBlack ? FadeAlpha : 1 - FadeAlpha;
			Canvas->DrawColor = FadeColor.ToFColor(true);
			Canvas->DrawTile(Canvas->DefaultTexture, 0, 0, Canvas->ClipX, Canvas->ClipY, 0, 0, Canvas->DefaultTexture->GetSizeX(), Canvas->DefaultTexture->GetSizeY());
			Canvas->DrawColor = OldColor;

			if (FadeAlpha >= 1.f)
			{
				bFading = false;
			}
		}
	}
}

void UFCGameViewportClient::FadeScreenIn(const float Duration)
{
	if (const UWorld* ActiveWorld = World.Get())
	{
		bFading = true;
		bFadeToBlack = true;
		FadeDuration = Duration;
		FadeStartTime = ActiveWorld->GetTimeSeconds();
	}
}

void UFCGameViewportClient::FadeScreenOut(const float Duration)
{
	if (const UWorld* ActiveWorld = World.Get())
	{
		bFading = true;
		bFadeToBlack = false;
		FadeDuration = Duration;
		FadeStartTime = ActiveWorld->GetTimeSeconds();
	}
}
