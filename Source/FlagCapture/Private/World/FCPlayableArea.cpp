#include "World/FCPlayableArea.h"

AFCPlayableArea::AFCPlayableArea(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	SetReplicateMovement(false);

	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
void AFCPlayableArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AFCPlayableArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AFCPlayableArea::BeginPlay()
{
	Super::BeginPlay();
}

void AFCPlayableArea::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCPlayableArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
