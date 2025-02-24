#include "World/FCCaptureFlag.h"

#include "EngineUtils.h"
#include "Character/FCCharacter.h"
#include "World/FCSpawnArea.h"
#include "Game/FCGameMode.h"

AFCCaptureFlag::AFCCaptureFlag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GrabbedByTeam(ETeamSide::None)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	SetReplicateMovement(false);

	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
}

void AFCCaptureFlag::BeginPlay()
{
	Super::BeginPlay();

	InitialPosition = GetActorTransform();
}

void AFCCaptureFlag::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AFCCaptureFlag::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCCaptureFlag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetNetMode() == NM_Client) return;

	if (bGrabbed)
	{
		if (!GrabbedByCharacter)
		{
			OnReleased(GrabbedByCharacter, GrabbedByTeam);

			return;
		}

		if (!GrabbedByCharacter->IsAlive())
		{
			OnReleased(GrabbedByCharacter, GrabbedByTeam);

			return;
		}

		for (TActorIterator<AFCSpawnArea> ActorItr(GetWorld(), AFCSpawnArea::StaticClass()); ActorItr; ++ActorItr)
		{
			if (AFCSpawnArea* SpawnArea = *ActorItr)
			{
				if (SpawnArea->OwnerTeam() == GrabbedByTeam && SpawnArea->IsLocationInsideArea(GetActorLocation()))
				{
					OnCapture(GrabbedByCharacter, GrabbedByTeam);
				}
			}
		}
	}
}

void AFCCaptureFlag::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (GetNetMode() == NM_Client) return;
	if (bGrabbed) return;

	if (AFCCharacter* OtherCharacter = Cast<AFCCharacter>(OtherActor))
	{
		OnGrab(OtherCharacter, OtherCharacter->GetPlayerSide());
	}
}

void AFCCaptureFlag::OnGrab(AFCCharacter* Character, ETeamSide Team)
{
	if (GetNetMode() == NM_Client) return;
	if (!Character) return;
	if (bGrabbed) return;

	bGrabbed = true;
	GrabbedByCharacter = Character;
	GrabbedByTeam = Team;

	NetOnGrab(Character, Team);
}

void AFCCaptureFlag::OnCapture(AFCCharacter* Character, ETeamSide Team)
{
	if (GetNetMode() == NM_Client) return;
	if (!Character) return;
	if (!bGrabbed) return;

	if (AFCGameMode* const GM = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GM->OnFlagCaptured(Character, Team);
	}

	ETeamSide OldTeam = GrabbedByTeam;

	bGrabbed = false;
	GrabbedByCharacter = nullptr;
	GrabbedByTeam = ETeamSide::None;

	NetOnCapture(Character, OldTeam);
}

void AFCCaptureFlag::OnReleased(AFCCharacter* Character, ETeamSide Team)
{
	if (GetNetMode() == NM_Client) return;
	if (!bGrabbed) return;

	ETeamSide OldTeam = GrabbedByTeam;

	bGrabbed = false;
	GrabbedByCharacter = nullptr;
	GrabbedByTeam = ETeamSide::None;

	NetOnReleased(Character, OldTeam);
}

void AFCCaptureFlag::NetOnGrab_Implementation(AFCCharacter* Character, ETeamSide Team)
{
	if (!Character) return;

	K2_OnGrab(Character, Team);

	OnFlagGrabbedByTeam.Broadcast(this, Team);
}

void AFCCaptureFlag::NetOnCapture_Implementation(AFCCharacter* Character, ETeamSide Team)
{
	if (!Character) return;

	K2_OnCapture(Character, Team);

	OnFlagCapturedByTeam.Broadcast(this, Team);
}

void AFCCaptureFlag::NetOnReleased_Implementation(AFCCharacter* Character, ETeamSide Team)
{
	if (!Character) return;

	K2_OnReleased(Character, Team);
}
