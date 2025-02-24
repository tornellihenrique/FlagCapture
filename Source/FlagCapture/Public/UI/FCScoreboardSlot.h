#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCScoreboardSlot.generated.h"

class AFCPlayerState;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreboardSlotClicked, UFCScoreboardSlot*, WidgetRef);

UCLASS()
class FLAGCAPTURE_API UFCScoreboardSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UButton> MainButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
	bool bActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
	TObjectPtr<AFCPlayerState> PlayerState;

public:
	UPROPERTY(BlueprintAssignable)
	FOnScoreboardSlotClicked OnScoreboardSlotClicked;

public:
	UFCScoreboardSlot(const FObjectInitializer& ObjectInitializer);

	//~UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~UUserWidget

protected:
	UPROPERTY()
	TObjectPtr<AFCPlayerState> CachedPlayerState;

protected:
	UFUNCTION()
	void OnMainButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Player State Changed"))
	void K2_OnPlayerStateChanged(AFCPlayerState* OldPlayerState);

};
