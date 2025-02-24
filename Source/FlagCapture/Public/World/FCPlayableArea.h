#pragma once

#include "CoreMinimal.h"
#include "World/FCSplineArea.h"
#include "FCPlayableArea.generated.h"

UCLASS()
class FLAGCAPTURE_API AFCPlayableArea : public AFCSplineArea
{
	GENERATED_BODY()

public:
	AFCPlayableArea(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
};
