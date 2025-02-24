#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameplayTagContainer.h"
#include "FCSpectatorPawn.generated.h"

class UCameraComponent;
class AFCCharacter;
class AFCPlayerState;

UCLASS()
class FLAGCAPTURE_API AFCSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Default, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
	float FOV = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
	float CameraInterpSpeed = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
	float CameraRotationInterpSpeed = 5.0f;

public:
	AFCSpectatorPawn(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(class AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void InitializeCamera();

	UFUNCTION()
	virtual void RespawnCheck();

	UFUNCTION(Client, Reliable)
	virtual void ClientPossessedBy(AController* NewController);

	UFUNCTION(Client, Reliable)
	virtual void ClientUnPossessed();

	UFUNCTION()
	void OnBlendComplete();

	UFUNCTION()
	void OnTargetViewPlayerChanged(APlayerState* InPlayerState);

	FVector GetGroundLocationWithOffsets(float Offsets);

public:
	void OnStateReset();
	void OnMatchEnded();

	UFUNCTION(BlueprintPure)
	FORCEINLINE APawn* GetFocusedPawn() const { return FocusedPawn; }

	UFUNCTION(BlueprintPure)
	AFCCharacter* GetFocusedCharacter() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<APawn> FocusedPawn;

	UPROPERTY()
	TObjectPtr<AFCPlayerState> CachedTargetPlayer;

	UPROPERTY()
	FGameplayTag CurrentViewMode;

	UPROPERTY()
	FBoxSphereBounds LastKnowingBounds;

	UPROPERTY()
	FVector LastKnowingLocation;

	UPROPERTY()
	bool bTargetPawnDead;

	UPROPERTY()
	bool bBlendFinish;

};
