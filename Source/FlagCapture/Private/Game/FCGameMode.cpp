#include "Game/FCGameMode.h"

#include "Player/FCPlayerController.h"
#include "Player/FCPlayerState.h"
#include "Game/FCGameState.h"
#include "Player/FCSpectatorPawn.h"
#include "World/FCSpawnArea.h"
#include "EngineUtils.h"
#include "GameFramework/GameSession.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Character/FCCharacter.h"
#include "Utility/FCFunctionLibrary.h"
#include "World/FCPlayableArea.h"

DEFINE_LOG_CATEGORY_STATIC(LogFCGameMode, Log, All);

AFCGameMode::AFCGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MatchLengthMultiplier(1.0f)
	, TicketSizeMultiplier(1.0f)
	, RespirationTimeMultiplier(1.0f)
	, BulletDamageMultiplier(1.0f)
	, PenaltyPoint(-100)
	, MinimumPlayerToStartMatch(1)
	, TimeToRemovePlayerBody(10.0f)
	, bFriendlyFire(false)
	, bEnableSpectator(true)
	, bDevelopmentMode(true)
{
	DefaultPawnClass = nullptr;

	PlayerControllerClass = AFCPlayerController::StaticClass();
	PlayerStateClass = AFCPlayerState::StaticClass();
	GameStateClass = AFCGameState::StaticClass();
	SpectatorClass = AFCSpectatorPawn::StaticClass();

	MinRespawnDelay = 1.5f;

	GameTimers[EGameTimer::EGT_Listen] = FGameTimerData(5.0f, 1.0f);
	GameTimers[EGameTimer::EGT_Waiting] = FGameTimerData(10.0f, 1.0f);
	GameTimers[EGameTimer::EGT_Preparing] = FGameTimerData(20.0f, 1.0f);
	GameTimers[EGameTimer::EGT_InProgress] = FGameTimerData(10.0f, 1.0f);
	GameTimers[EGameTimer::EGT_Ended] = FGameTimerData(4.0f, 1.0f);
	GameTimers[EGameTimer::EGT_NextGame] = FGameTimerData(10.0f, 1.0f);

	MaxPlayers = 32;
}

void AFCGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	SetGameInitialState(bDevelopmentMode ? EGameInitialState::InProgress : EGameInitialState::Listen);

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AFCGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation() / GetWorldSettings()->DemoPlayTimeDilation, true);

}

void AFCGameMode::Reset()
{
	Super::Reset();
}

void AFCGameMode::InitGameState()
{
	Super::InitGameState();

	if (AFCGameState* const GS = Cast<AFCGameState>(GameState))
	{
		GS->SetMaxPlayers(MaxPlayers);
	}
}

void AFCGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	ParseOptions(Options);

	for (TActorIterator<AFCPlayableArea> ActorItr(GetWorld(), AFCPlayableArea::StaticClass()); ActorItr; ++ActorItr)
	{
		if (AFCPlayableArea* const PlayableArea = *ActorItr)
		{
			PlayableAreas.Add(PlayableArea);
		}
	}

	TObjectPtr<AFCPlayableArea>* PlayableAreaRef = PlayableAreas.FindByPredicate([this](const TObjectPtr<AFCPlayableArea>& InMaverickArea)
	{
		return InMaverickArea.Get(); // Apenas escolha a primeira area que encontrar por enquanto...
	});

	if (PlayableAreaRef)
	{
		UE_LOG(LogFCGameMode, Verbose, TEXT("%s Found Area: %s"), *FC_LOGS_LINE, *PlayableAreaRef->GetName());

		CurrentArea = PlayableAreaRef->Get();
	}
}

void AFCGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		UE_LOG(LogFCGameMode, Warning, TEXT("%s ErrorMessage: %s"), *FC_LOGS_LINE, *ErrorMessage);

		return;
	}
}

APlayerController* AFCGameMode::SpawnPlayerController(ENetRole InRemoteRole, const FString& Options)
{
	return SpawnPlayerControllerCommon(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator, PlayerControllerClass);
}

void AFCGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AFCGameState* const GS = Cast<AFCGameState>(GameState))
	{
		if (AFCPlayerController* const PC = Cast<AFCPlayerController>(NewPlayer))
		{
			if (AFCPlayerState* PS = PC->GetPlayerState<AFCPlayerState>())
			{
				const FUniqueNetIdRepl& PlayerId = PS->GetUniqueId();
				if (PlayerId.IsValid())
				{
					UE_LOG(LogFCGameMode, Verbose, TEXT("%s Player ID: %s"), *FC_LOGS_LINE, *PlayerId.ToString());
				}
			}

			AActor* PlayerStart = FindTeamPlayerStart(PC);
			PC->OnPlayerLogin(PlayerStart, GameInitialState);

			// remover bots caso necessário
		}
	}
}

void AFCGameMode::Logout(AController* Exiting)
{
	if (GetMatchState() != MatchState::WaitingToStart && GetMatchState() != MatchState::InProgress)
	{
		Super::Logout(Exiting);

		return;
	}

	if (AFCGameState* GS = Cast<AFCGameState>(GameState))
	{
		if (AFCPlayerState* PS = Exiting->GetPlayerState<AFCPlayerState>())
		{
			GS->UnRegisterTeam(PS);
		}
	}

	// adicionar bots caso necessário

	Super::Logout(Exiting);
}

void AFCGameMode::StartPlay()
{
	Super::StartPlay();

	// inicializar actors?
}

void AFCGameMode::RestartGame()
{
	Super::RestartGame();

	if (GameSession->CanRestartGame())
	{
		if (GetMatchState() == MatchState::LeavingMap)
		{
			return;
		}

		FString RestartStr(TEXT("?Restart"));

		UE_LOG(LogFCGameMode, Verbose, TEXT("%s ServerTravel: %s"), *FC_LOGS_LINE, *RestartStr);

		GetWorld()->ServerTravel(RestartStr, GetTravelType());
	}
}

void AFCGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();
}

void AFCGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// adicionar bots?
}

void AFCGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

void AFCGameMode::EndMatch()
{
	Super::EndMatch();

	if (GetMatchState() == MatchState::WaitingPostMatch && GetNetMode() != NM_Client)
	{
		if (NumPlayers == 0)
		{
			OnEndMatch();
		}
		else
		{
			OnStateEnded();
		}
	}
}

void AFCGameMode::ParseOptions(const FString& Options)
{
	// Usar options?

	FServerMapAndModeArgs CurrentMapAndMode = GetCurrentMapAndMode();

	UE_LOG(LogFCGameMode, Verbose, TEXT("%s LevelPath: %s"), *FC_LOGS_LINE, *CurrentMapAndMode.LevelPath);
	UE_LOG(LogFCGameMode, Verbose, TEXT("%s GameMode: %s"), *FC_LOGS_LINE, *CurrentMapAndMode.GameMode);

	// Rotacionar mapas?

	NextURL = CurrentMapAndMode.LevelPath;
	NextMapAndMode = CurrentMapAndMode;

	if (!NextURL.IsEmpty())
	{
		UE_LOG(LogFCGameMode, Verbose, TEXT("%s NextURL: %s"), *FC_LOGS_LINE, *NextURL);
	}
}

FServerMapAndModeArgs AFCGameMode::GetCurrentMapAndMode() const
{
	FString LevelPath = GetWorld()->GetPathName();
	FString GameMode = GetClass()->GetPathName();

	// Mais mapas?

	return FServerMapAndModeArgs(LevelPath, GameMode);
}

FGameTimerData AFCGameMode::GetTimerData()
{
	FGameTimerData TimerProperties = GameTimers[(uint8)GameInitialState];
	if (GameInitialState == EGameInitialState::Ended)
	{
		TimerProperties.TimerLength = TimerProperties.TimerLength < 10.0f ? 10.0f : TimerProperties.TimerLength;
	}

	TimerProperties.DelayBeforeNextState = TimerProperties.DelayBeforeNextState < 1.0f ? 1.0f : TimerProperties.DelayBeforeNextState;

	return TimerProperties;
}

AActor* AFCGameMode::FindTeamPlayerStart(AController* Player)
{
	AActor* TmpPlayerStart = nullptr;

	TArray<AFCSpawnArea*> SpawnArea;
	for (TActorIterator<AFCSpawnArea> ActorItr(GetWorld(), AFCSpawnArea::StaticClass()); ActorItr; ++ActorItr)
	{
		AFCSpawnArea* PlayerStart = *ActorItr;

		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Player))
		{
			const FGenericTeamId TeamID = TeamAgent->GetGenericTeamId();
			const ETeamSide Team = static_cast<ETeamSide>(GenericTeamIdToInteger(TeamID));

			if (PlayerStart && Team == ETeamSide::None)
			{
				SpawnArea.Add(PlayerStart);
			}
			else
			{
				if (PlayerStart && PlayerStart->CanTeamSpawn(Team))
				{
					SpawnArea.Add(PlayerStart);
				}
			}
		}
	}

	if (SpawnArea.Num() != 0)
	{
		// Shuffle Array
		int32 LastIndex = SpawnArea.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index)
			{
				SpawnArea.Swap(i, Index);
			}
		}

		int32 Min = 0;
		int32 Max = SpawnArea.Num() - 1;
		int32 Index = FMath::RandRange(Min, Max);

		if (AFCSpawnArea* SelectedArea = SpawnArea[Index])
		{
			TmpPlayerStart = SelectedArea;
		}
	}

	if (!TmpPlayerStart)
	{
		TmpPlayerStart = FindPlayerStart(Player);
	}

	return TmpPlayerStart;
}

void AFCGameMode::SetGameInitialState(EGameInitialState InGameInitialState)
{
	GameInitialState = InGameInitialState;

	float TimeRemaining = GetTimerData().TimerLength;
	if (GameInitialState == EGameInitialState::InProgress)
	{
		TimeRemaining = (TimeRemaining * MatchLengthMultiplier) * 60;
	}
	else if (GameInitialState == EGameInitialState::Ended)
	{
		TimeRemaining *= RespirationTimeMultiplier;
	}

	if (AFCGameState* const GS = Cast<AFCGameState>(GameState))
	{
		GS->SetGameInitialState(TimeRemaining, GameInitialState);

		UpdateAllPlayerState();
	}
}

void AFCGameMode::UpdateInitialState()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_OnInitialStateExpired);

	if ((HasMatchStarted()) && NumPlayers < MinimumPlayerToStartMatch)
	{
		if (!HasMatchEnded())
		{
			SetGameInitialState(EGameInitialState::Listen);

			return;
		}
	}

	if (GameInitialState == EGameInitialState::Ended && NextMapAndMode.IsValid())
	{
		for (FConstControllerIterator PlayerItr = GetWorld()->GetControllerIterator(); PlayerItr; ++PlayerItr)
		{
			if (AFCPlayerController* PlayerController = Cast<AFCPlayerController>(*PlayerItr))
			{
				// Deslogar PCs?
			}
		}
	}

	switch (GameInitialState)
	{
		case EGameInitialState::Listen: SetGameInitialState(EGameInitialState::Waiting); break;
		case EGameInitialState::Waiting: SetGameInitialState(EGameInitialState::Preparing); break;
		case EGameInitialState::Preparing: SetGameInitialState(EGameInitialState::InProgress); break;
		case EGameInitialState::InProgress: SetGameInitialState(EGameInitialState::Ended); break;
		case EGameInitialState::Ended: SetGameInitialState(EGameInitialState::NextGame); break;
	}
}

void AFCGameMode::DefaultTimer()
{
	if (GetMatchState() == MatchState::WaitingToStart) return;

	if (IsMatchInProgress())
	{
		int32 TottalPawn = NumPlayers + NumBots;
		if (NumPlayers == 0 && GameInitialState < EGameInitialState::Ended)
		{
			for (FConstControllerIterator PlayerItr = GetWorld()->GetControllerIterator(); PlayerItr; ++PlayerItr)
			{
				if (AAIController* AiController = Cast<AAIController>(*PlayerItr))
				{
					AiController->Destroy();
				}
			}

			GameInitialState = EGameInitialState::Ended;
			if (AFCGameState* const GS = Cast<AFCGameState>(GameState))
			{
				GS->ForceEnded();
			}
		}
	}
}

void AFCGameMode::ServerChangeMap(const FString& URL)
{
	bUseSeamlessTravel = false;
	GetWorld()->ServerTravel(URL, true, false);
}

void AFCGameMode::UpdateAllPlayerState()
{
	// Guardando ponteiro de função de outra classe em variavel
	typedef void (AFCPlayerController::* FunctionPtr)(void);
	FunctionPtr Function = nullptr;
	
	switch (GameInitialState)
	{
	case EGameInitialState::Preparing:
		Function = &AFCPlayerController::EnterPreparingState;
		break;
	case EGameInitialState::Ended:
		Function = &AFCPlayerController::EnterMatchEndedState;
		break;
	case EGameInitialState::NextGame:
		Function = &AFCPlayerController::EnterNextGameState;
		break;
	}

	for (FConstControllerIterator PlayerItr = GetWorld()->GetControllerIterator(); PlayerItr; ++PlayerItr)
	{
		if (AFCPlayerController* PlayerController = Cast<AFCPlayerController>(*PlayerItr))
		{
			PlayerController->OnGameInitialStateChanged(GameInitialState);

			if (GameInitialState == EGameInitialState::Ended)
			{
				if (APawn* PlayerPawn = PlayerController->GetPawn())
				{
					PlayerPawn->SetCanBeDamaged(false);
				}
			}

			if (Function != nullptr)
			{
				(PlayerController->*Function)();
			}
		}

		// Lidar com bots?
	}
}

void AFCGameMode::OnTimeExpired()
{
	EndMatch();
}

void AFCGameMode::OnStateEnded()
{
	const float Delay = GetTimerData().DelayBeforeNextState;
	if (Delay != 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_OnInitialStateExpired, this, &AFCGameMode::UpdateInitialState, GetTimerData().DelayBeforeNextState, false);
	}
	else
	{
		UpdateInitialState();
	}
}

void AFCGameMode::OnEndMatch()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_OnEndMatch);

	const float Delay = NumPlayers == 0 ? 0.0f : GetTimerData().DelayBeforeNextState;
	if (Delay != 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_OnInitialStateExpired, this, &AFCGameMode::OnNextGame, GetTimerData().DelayBeforeNextState, false);
	}
	else
	{
		OnNextGame();
	}
}

void AFCGameMode::OnNextGame()
{
	ProcessNextGame();
}

void AFCGameMode::ProcessNextGame()
{
#if 0 // Inverter caso implementar rotação de mapas
	ServerChangeMap(NextURL);
#else
	RestartGame();
#endif
}

bool AFCGameMode::CanPlayerJoinTeam(ETeamSide InTeam) const
{
	if (HasMatchEnded()) return false;

	int32 NumA = GetNumPlayersInTeam(ETeamSide::Team_A);
	int32 NumB = GetNumPlayersInTeam(ETeamSide::Team_B);

	if (InTeam == ETeamSide::Team_A)
	{
		return NumA <= NumB + 1;
	}
	else
	{
		return NumB <= NumA + 1;
	}
}

int32 AFCGameMode::GetNumPlayersInTeam(ETeamSide InTeam) const
{
	if (const AFCGameState* GS = Cast<AFCGameState>(GameState))
	{
		return GS->GetPlayersInSide(InTeam).Num();
	}

	return 0;
}

ETeamSide AFCGameMode::GetDesiredAutoJoinTeam()
{
	int32 NumA = GetNumPlayersInTeam(ETeamSide::Team_A);
	int32 NumB = GetNumPlayersInTeam(ETeamSide::Team_B);

	return NumA > NumB ? ETeamSide::Team_B : ETeamSide::Team_A;
}

void AFCGameMode::PlayerJoinTeam(APlayerController* InController, ETeamSide InTeam)
{
	RequestPlayerStart(InController);

	if (AFCGameState* GS = Cast<AFCGameState>(GameState))
	{
		if (AFCPlayerState* PlayerPS = InController->GetPlayerState<AFCPlayerState>())
		{
			GS->RegisterTeam(PlayerPS);
		}
	}
}

void AFCGameMode::RequestPlayerStart(APlayerController* InController)
{
	if (AActor* const PlayerStart = FindTeamPlayerStart(InController))
	{
		if (AFCPlayerController* const PC = Cast<AFCPlayerController>(InController))
		{
			PC->SetPlayerStart(PlayerStart, CurrentArea);
		}
	}
}

void AFCGameMode::PlayerSpawn(APlayerController* InController)
{
	if (AFCPlayerController* const PlayerController = Cast<AFCPlayerController>(InController))
	{
		if (AActor* const PlayerStart = PlayerController->GetPlayerStart())
		{
			FVector SpawnLocation = PlayerStart->GetActorLocation();
			FRotator SpawnRotation = FRotator(0.f, PlayerStart->GetActorRotation().Yaw, 0.f);

			if (AFCSpawnArea* const SpawnArea = Cast<AFCSpawnArea>(PlayerStart))
			{
				const FTransform& SafeSpawnTransform = SpawnArea->GetSafeSpawnTransform();
				SpawnLocation = SafeSpawnTransform.GetLocation();
				SpawnRotation = SafeSpawnTransform.GetRotation().Rotator();
			}

			PlayerController->SetControlRotation(SpawnRotation);

			const TSubclassOf<AFCCharacter>& LoadedCharacterClass = CharacterClass.TryLoadClass<AFCCharacter>();
			if (LoadedCharacterClass)
			{
				FTransform SpawnTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
				AFCCharacter* Character = GetWorld()->SpawnActorDeferred<AFCCharacter>(LoadedCharacterClass, SpawnTransform, PlayerController, PlayerController->GetPawn(), ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
				if (Character)
				{
					// ...

					UGameplayStatics::FinishSpawningActor(Character, SpawnTransform);

					Character->PossessedBy(PlayerController);
					Character->DispatchRestart(true);
					if (Character->PrimaryActorTick.bStartWithTickEnabled)
					{
						Character->SetActorTickEnabled(true);
					}
					Character->SetCanBeDamaged(false);

					PlayerController->OnCharacterSpawned(Character);
				}
			}
		}
	}
}

void AFCGameMode::PlayerSpawnAtDesired(APlayerController* InController, FVector const& SpawnLocation, FRotator const& SpawnRotation)
{
	if (AFCPlayerController* const PlayerController = Cast<AFCPlayerController>(InController))
	{
		PlayerController->SetControlRotation(SpawnRotation);

		const TSubclassOf<AFCCharacter>& LoadedCharacterClass = CharacterClass.TryLoadClass<AFCCharacter>();
		if (LoadedCharacterClass)
		{
			FTransform SpawnTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
			AFCCharacter* Character = GetWorld()->SpawnActorDeferred<AFCCharacter>(LoadedCharacterClass, SpawnTransform, PlayerController, PlayerController->GetPawn(), ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (Character)
			{
				// ...

				UGameplayStatics::FinishSpawningActor(Character, SpawnTransform);

				Character->PossessedBy(PlayerController);
				Character->DispatchRestart(true);
				if (Character->PrimaryActorTick.bStartWithTickEnabled)
				{
					Character->SetActorTickEnabled(true);
				}
				Character->SetCanBeDamaged(false);

				PlayerController->OnCharacterSpawned(Character);
			}
		}
	}
}

void AFCGameMode::OnFlagCaptured(ACharacter* InCharacter, ETeamSide InTeam)
{
	if (AFCGameState* const GS = Cast<AFCGameState>(GameState))
	{
		if (InTeam == ETeamSide::Team_A)
		{
			if (GS->AddTeamACapture())
			{
				EndMatch();
			}
		}
		else if (InTeam == ETeamSide::Team_B)
		{
			if (GS->AddTeamBCapture())
			{
				EndMatch();
			}
		}
	}
}

void AFCGameMode::CalcDamage(float& OutDamageAmount, AController* PC, AController* OtherPC)
{
	if (AFCPlayerController* const MyPC = Cast<AFCPlayerController>(PC))
	{
		if (OtherPC && OtherPC != PC && !bFriendlyFire && !MyPC->IsEnemyFor(OtherPC))
		{
			OutDamageAmount = 0.f;
		}
		else
		{
			OutDamageAmount *= BulletDamageMultiplier;
		}
	}

	// Lidar com bots?
}

void AFCGameMode::OnCharacterDied(ACharacter* Character, AController* EventInstigator, const FKillEventData& KillEventData)
{
	AFCPlayerState* OtherPS = Cast<AFCPlayerState>(EventInstigator != NULL ? EventInstigator->PlayerState : NULL);
	AFCPlayerState* PlayerPS = Character->GetPlayerState<AFCPlayerState>();

	if (PlayerPS)
	{
		for (FConstControllerIterator PlayerItr = GetWorld()->GetControllerIterator(); PlayerItr; ++PlayerItr)
		{
			if (AFCPlayerController* const PC = Cast<AFCPlayerController>(*PlayerItr))
			{
				PC->NotifyOnCharacterDied(Character, KillEventData, PlayerPS, OtherPS, PenaltyPoint);
			}
		}

		const FDamageData& DamageData = UFCFunctionLibrary::GetDamageData(KillEventData);

		AFCPlayerController* MyPC = Character->GetController<AFCPlayerController>();
		if (MyPC)
		{
			OnPlayerDied(MyPC, KillEventData, OtherPS);

			if (EventInstigator)
			{
				if (PlayerPS != OtherPS && PlayerPS->IsEnemyFor(OtherPS))
				{
					OnPlayerKilledEnemy(KillEventData, EventInstigator, MyPC);
				}
				else
				{
					OnPlayerKilledTeammate(KillEventData, EventInstigator, MyPC);
				}
			}
		}

		// Lidar com bots?
	}
}

void AFCGameMode::OnPlayerDied(AController* PC, const FKillEventData& KillEventData, APlayerState* KillerPS)
{
	AFCCharacter* Character = Cast<AFCCharacter>(PC->GetPawn());

	// Lidar com bots?

	if (AFCPlayerController* MyPC = Cast<AFCPlayerController>(PC))
	{
		MyPC->AddDeath();

		MyPC->StoreLastKillerData(KillerPS, KillEventData);

		if (bEnableSpectator)
		{
			MyPC->EnterDeathState();
		}
		else
		{
			MyPC->EnterDeathStateFast();
		}

		if (Character)
		{
			Character->SetLifeSpan(TimeToRemovePlayerBody);
		}
	}
}

void AFCGameMode::OnPlayerKilledEnemy(const FKillEventData& KillEventData, AController* PC, AController* OtherPC)
{
	if (AFCPlayerController* MyPC = Cast<AFCPlayerController>(PC))
	{
		if (AFCPlayerState* OtherPS = OtherPC->GetPlayerState<AFCPlayerState>())
		{
			MyPC->AddKilledPlayer(OtherPS);
		}

		const FDamageData& DamageData = UFCFunctionLibrary::GetDamageData(KillEventData);
		int32 Score = DamageData.KillReward;
		if (DamageData.bIsHeadshot)
		{
			Score += DamageData.HeadshotReward;
			MyPC->AddHeadshot();
		}

		MyPC->AddScore(Score);
		MyPC->AddKill();

		MyPC->NotifyOnKill(OtherPC, DamageData.bIsHeadshot);
	}
}

void AFCGameMode::OnPlayerKilledTeammate(const FKillEventData& KillEventData, AController* PC, AController* OtherPC)
{
	if (AFCPlayerController* MyPC = Cast<AFCPlayerController>(PC))
	{
		MyPC->AddScore(PenaltyPoint);
	}
}
