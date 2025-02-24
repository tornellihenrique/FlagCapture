#include "Game/FCGameState.h"
#include "Net/UnrealNetwork.h"
#include "Game/FCGameMode.h"
#include "World/FCSpawnArea.h"
#include "EngineUtils.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Player/FCPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogFCGameState, Log, All);

AFCGameState::AFCGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CapturesForWinning(3)
	, GameInitialState(EGameInitialState::Listen)
	, TimeRemaining(INDEX_NONE)
	, MatchWinner(ETeamSide::None)
	, MaxPlayers(INDEX_NONE)
{
}

void AFCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, ATeamCaptures, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, BTeamCaptures, Parameters);

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, MaxPlayers, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, GameInitialState, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, TimeRemaining, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, MatchWinner, Parameters);

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, ATeamSide, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, BTeamSide, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCGameState, SpawnableAreas, Parameters);
}

void AFCGameState::PostInitializeComponents()
{
	Super::Super::PostInitializeComponents(); // Para uso customizado de timer

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_DefaultTimer))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_DefaultTimer);
	}
}

void AFCGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

float AFCGameState::GetPlayerRespawnDelay(AController* Controller) const
{
	return Super::GetPlayerRespawnDelay(Controller);
}

void AFCGameState::DefaultTimer()
{
	if (IsMatchInProgress())
	{
		++ElapsedTime;
		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_ElapsedTime();
		}
	}

	bool bCanTimerRun = IsMatchInProgress() || HasMatchStarted();
	if (bCanTimerRun && TimeRemaining > 0)
	{
		TimeRemaining--;
		MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, TimeRemaining, this);

		if (TimeRemaining <= 0)
		{
			if (AFCGameMode* GM = GetWorld()->GetAuthGameMode<AFCGameMode>())
			{
				switch (GameInitialState)
				{
					case EGameInitialState::Listen:
					case EGameInitialState::Waiting:
					case EGameInitialState::Preparing:
					case EGameInitialState::InProgress: OnStateEnded();		break;
					case EGameInitialState::Ended:		OnTimeExpired();	break;
					case EGameInitialState::NextGame:	OnEndMatch();		break;
				}
			}
		}
	}
}

void AFCGameState::ReceivedGameModeClass()
{
	Super::ReceivedGameModeClass();

	InitializeGameState();
}

void AFCGameState::AddPlayerState(APlayerState* PlayerState)
{
	if (GetNetMode() != NM_Client)
	{
		MulticastOnPlayerState(PlayerState, false);
	}

	Super::AddPlayerState(PlayerState);
}

void AFCGameState::RemovePlayerState(APlayerState* PlayerState)
{
	if (GetNetMode() != NM_Client)
	{
		MulticastOnPlayerState(PlayerState, true);
	}

	Super::RemovePlayerState(PlayerState);
}

void AFCGameState::MulticastOnPlayerState_Implementation(APlayerState* PlayerState, bool bRemove)
{
	if (GetNetMode() == NM_DedicatedServer) return;

	if (OnPlayerStateEvent.IsBound())
	{
		OnPlayerStateEvent.Broadcast(PlayerState, bRemove);
	}
}

void AFCGameState::InitializeGameState()
{
	if (!bInitialized)
	{
		bInitialized = true;

		if (GetNetMode() != NM_Client)
		{
			SpawnableAreas.Empty();
		}

		TArray<AFCSpawnArea*> MarkAsDeleted;
		for (TActorIterator<AFCSpawnArea> ActorItr(GetWorld(), AFCSpawnArea::StaticClass()); ActorItr; ++ActorItr)
		{
			if (AFCSpawnArea* const SpawnableArea = *ActorItr)
			{
				if (GetNetMode() != NM_Client)
				{
					SpawnableAreas.Add(SpawnableArea);
				}
			}
		}

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, SpawnableAreas, this);

		for (AFCSpawnArea* SpawnableArea : MarkAsDeleted)
		{
			SpawnableArea->Destroy();
		}
	}
}

void AFCGameState::RegisterTeam(AFCPlayerState* PlayerState)
{
	if (!PlayerState) return;

	ETeamSide Team = PlayerState->GetPlayerSide();
	if (Team == ETeamSide::Team_A)
	{
		int32 Index = ATeamSide.IndexOfByPredicate([PlayerState](AFCPlayerState* InPlayerState) { return InPlayerState == PlayerState; });
		if (Index == INDEX_NONE)
		{
			ATeamSide.Add(PlayerState);

			MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, ATeamSide, this);

			if (GetNetMode() != NM_DedicatedServer)
			{
				OnRep_ATeamSide();
			}
		}
	}
	else if (Team == ETeamSide::Team_B)
	{
		int32 Index = BTeamSide.IndexOfByPredicate([PlayerState](AFCPlayerState* InPlayerState) { return InPlayerState == PlayerState; });
		if (Index == INDEX_NONE)
		{
			BTeamSide.Add(PlayerState);

			MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, BTeamSide, this);

			if (GetNetMode() != NM_DedicatedServer)
			{
				OnRep_BTeamSide();
			}
		}
	}
}

void AFCGameState::UnRegisterTeam(AFCPlayerState* PlayerState)
{
	if (!PlayerState) return;

	ETeamSide Team = PlayerState->GetPlayerSide();
	if (Team == ETeamSide::Team_A)
	{
		int32 Index = ATeamSide.IndexOfByPredicate([PlayerState](AFCPlayerState* InPlayerState) { return InPlayerState == PlayerState; });
		if (Index != INDEX_NONE)
		{
			ATeamSide.RemoveAtSwap(Index);

			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ATeamSide, this);

			if (GetNetMode() != NM_DedicatedServer)
			{
				OnRep_ATeamSide();
			}
		}
	}
	else if (Team == ETeamSide::Team_B)
	{
		int32 Index = BTeamSide.IndexOfByPredicate([PlayerState](AFCPlayerState* InPlayerState) { return InPlayerState == PlayerState; });
		if (Index != INDEX_NONE)
		{
			BTeamSide.RemoveAtSwap(Index);

			MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, BTeamSide, this);

			if (GetNetMode() != NM_DedicatedServer)
			{
				OnRep_BTeamSide();
			}
		}
	}
}

bool AFCGameState::AddTeamACapture()
{
	ATeamCaptures = FMath::Clamp(ATeamCaptures + 1, 0, CapturesForWinning);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, ATeamCaptures, this);

	if (ATeamCaptures == CapturesForWinning)
	{
		SetMatchWinner(ETeamSide::Team_A);
		return true;
	}

	return false;
}

bool AFCGameState::AddTeamBCapture()
{
	BTeamCaptures = FMath::Clamp(BTeamCaptures + 1, 0, CapturesForWinning);
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, BTeamCaptures, this);

	if (BTeamCaptures == CapturesForWinning)
	{
		SetMatchWinner(ETeamSide::Team_B);
		return true;
	}

	return false;
}

void AFCGameState::SetMatchWinner(ETeamSide InMatchWinner)
{
	if (MatchWinner != InMatchWinner)
	{
		MatchWinner = InMatchWinner;

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, MatchWinner, this);
	}
}

void AFCGameState::OnStateEnded()
{
	if (AFCGameMode* GM = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GM->OnStateEnded();
	}
}

void AFCGameState::OnTimeExpired()
{
	if (AFCGameMode* GM = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GM->OnTimeExpired();
	}
}

void AFCGameState::OnEndMatch()
{
	if (AFCGameMode* GM = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GM->OnEndMatch();
	}
}

void AFCGameState::SetMaxPlayers(int32 InMaxPlayers)
{
	MaxPlayers = InMaxPlayers;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, MaxPlayers, this);
}

void AFCGameState::SetGameInitialState(int32 InTimeRemaining, EGameInitialState InGameInitialState)
{
	TimeRemaining = InTimeRemaining;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, TimeRemaining, this);

	GameInitialState = InGameInitialState;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, GameInitialState, this);

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_DefaultTimer))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_DefaultTimer);
	}

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AFCGameState::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation() / GetWorldSettings()->DemoPlayTimeDilation, true);
}

void AFCGameState::ForceEnded()
{
	UE_LOG(LogFCGameState, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_DefaultTimer))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_DefaultTimer);
	}

	TimeRemaining = 0;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, TimeRemaining, this);

	GameInitialState = EGameInitialState::Ended;
	MARK_PROPERTY_DIRTY_FROM_NAME(AFCGameState, GameInitialState, this);

	if (AFCGameMode* GM = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GM->OnTimeExpired();
	}
}

void AFCGameState::OnPlayableArea(AFCPlayableArea* PlayableArea)
{
	if (CurrentPlayableArea) return;
	if (!PlayableArea) return;

	CurrentPlayableArea = PlayableArea;

	// Talvez "desligar" actors fora da area?
}

void AFCGameState::OnRep_ATeamSide()
{
	if (OnTeamChangedEvent.IsBound())
	{
		OnTeamChangedEvent.Broadcast(ETeamSide::Team_A);
	}
}

void AFCGameState::OnRep_BTeamSide()
{
	if (OnTeamChangedEvent.IsBound())
	{
		OnTeamChangedEvent.Broadcast(ETeamSide::Team_B);
	}
}

TArray<AFCPlayerState*> AFCGameState::GetPlayersInSide(ETeamSide Team) const
{
	if (Team == ETeamSide::None) return {};

	return Team == ETeamSide::Team_A ? ATeamSide : BTeamSide;
}

TArray<AFCSpawnArea*> AFCGameState::GetSpawnableAreasByTeam(ETeamSide Team) const
{
	TArray<AFCSpawnArea*> FilteredAreas = SpawnableAreas.FilterByPredicate([Team](AFCSpawnArea* InArea) {
		return Team == ETeamSide::None ? false : InArea->CanTeamSpawn(Team);
	});

	return FilteredAreas;
}
