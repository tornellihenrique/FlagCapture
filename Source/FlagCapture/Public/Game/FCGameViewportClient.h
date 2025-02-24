#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "FCGameViewportClient.generated.h"

UCLASS()
class FLAGCAPTURE_API UFCGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual void PostRender(UCanvas* Canvas) override;

public:
	void FadeScreenIn(const float Duration);
	void FadeScreenOut(const float Duration);

private:
	uint8 bFading : 1;
	uint8 bFadeToBlack : 1;
	float FadeAlpha;
	float FadeStartTime;
	float FadeDuration;

};
