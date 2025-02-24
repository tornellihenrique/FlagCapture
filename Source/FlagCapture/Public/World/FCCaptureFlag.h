#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FCTypes.h"
#include "FCCaptureFlag.generated.h"

class AFCCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlagGrabbedByTeam, AFCCaptureFlag*, CaptureFlag, ETeamSide, ByTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlagCapturedByTeam, AFCCaptureFlag*, CaptureFlag, ETeamSide, ByTeam);

UCLASS()
class FLAGCAPTURE_API AFCCaptureFlag : public AActor
{
	GENERATED_BODY()

public:
	AFCCaptureFlag(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

public:
	UPROPERTY(BlueprintAssignable)
	FOnFlagGrabbedByTeam OnFlagGrabbedByTeam;

	UPROPERTY(BlueprintAssignable)
	FOnFlagCapturedByTeam OnFlagCapturedByTeam;

public:
	virtual void OnGrab(AFCCharacter* Character, ETeamSide Team);
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture Flag", meta = (DisplayName = "On Grab"))
	void K2_OnGrab(AFCCharacter* Character, ETeamSide Team);

	virtual void OnCapture(AFCCharacter* Character, ETeamSide Team);
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture Flag", meta = (DisplayName = "On Capture"))
	void K2_OnCapture(AFCCharacter* Character, ETeamSide Team);

	virtual void OnReleased(AFCCharacter* Character, ETeamSide Team);
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture Flag", meta = (DisplayName = "On Released"))
	void K2_OnReleased(AFCCharacter* Character, ETeamSide Team);

	UFUNCTION(NetMulticast, Reliable)
	void NetOnGrab(AFCCharacter* Character, ETeamSide Team);

	UFUNCTION(NetMulticast, Reliable)
	void NetOnCapture(AFCCharacter* Character, ETeamSide Team);

	UFUNCTION(NetMulticast, Reliable)
	void NetOnReleased(AFCCharacter* Character, ETeamSide Team);

	virtual ETeamSide GetOwnerTeam() const { return GrabbedByTeam; }

protected:
	UPROPERTY(BlueprintReadOnly)
	bool bGrabbed;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AFCCharacter> GrabbedByCharacter;

	UPROPERTY(BlueprintReadOnly)
	ETeamSide GrabbedByTeam;

	UPROPERTY(BlueprintReadOnly)
	FTransform InitialPosition;

};
