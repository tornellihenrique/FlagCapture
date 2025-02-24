#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCTypes.h"
#include "FCGameOverlay.generated.h"

class AFCCaptureFlag;

UCLASS()
class FLAGCAPTURE_API UFCGameOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> MainPanel;

public:
	UFCGameOverlay(const FObjectInitializer& ObjectInitializer);

	//~UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~UUserWidget

public:
	void OnDeath();

public:
	void OnGameInitialStateChanged(EGameInitialState InGameInitialState);
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnGameInitialStateChangedEvent();

	void OnControllerStateChanged(EControllerState InControllerState);
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnControllerStateChangedEvent();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnDeathEvent();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnTookDamage(float DamageAmount, AActor* Causer);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnPlayerDied(const FDamageData& InDamageData, APlayerState* SoldierPlayerState, APlayerState* KillerPlayerState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnPlayerKillEnemy(const FDamageData& InDamageData, APlayerState* OtherPlayerState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnPlayerKillTeammate(const FDamageData& InDamageData, APlayerState* OtherPlayerState, int32 Score);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnPlayableAreaStateChanged(bool bIsInsidePlayableArea);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnTeamGrabFlag(AFCCaptureFlag* CaptureFlag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnTeamCaptureFlag(AFCCaptureFlag* CaptureFlag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnEnemyGrabFlag(AFCCaptureFlag* CaptureFlag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
	void OnEnemyCaptureFlag(AFCCaptureFlag* CaptureFlag);

protected:
	UPROPERTY(BlueprintReadOnly)
	bool bIsDead;

	UPROPERTY(BlueprintReadOnly)
	EGameInitialState GameInitialState;

	UPROPERTY(BlueprintReadOnly)
	EControllerState ControllerState;

};
