#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCTypes.h"
#include "FCScoreboard.generated.h"

class UFCScoreboardSlot;
class AFCPlayerState;

UCLASS()
class FLAGCAPTURE_API UFCScoreboard : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Default)
	TSubclassOf<UFCScoreboardSlot> ScoreboardSlotClass;

	//~ A Team
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class USizeBox> AScoreBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UBorder> AScoreBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UScrollBox> AScrollBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UVerticalBox> AContainer;
	//~ A Team

	//~ B Team
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class USizeBox> BScoreBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UBorder> BScoreBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UScrollBox> BScrollBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UVerticalBox> BContainer;
	//~ B Team

public:
	UFCScoreboard(const FObjectInitializer& ObjectInitializer);

	//~UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void SetVisibility(ESlateVisibility InVisibility) override;
	//~UUserWidget

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AFCPlayerState> ActivePlayerState;

protected:
	UFUNCTION()
	void OnTeamChangedEvent(ETeamSide Team);

	UFUNCTION()
	void OnScoreboardSlotClicked(UFCScoreboardSlot* WidgetRef);

public:
	void BuildList();
	void UpdateList();

protected:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Player State Changed"))
	void K2_OnPlayerStateChanged();

};
