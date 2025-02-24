#include "Player/FCPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Player/FCPlayerState.h"
#include "Character/FCCharacter.h"
#include "World/FCCaptureFlag.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Player/FCSpectatorPawn.h"
#include "Net/Core/PushModel/PushModel.h"
#include "UI/FCPlayerOverlay.h"
#include "UI/FCGameOverlay.h"
#include "UI/FCScoreboard.h"
#include "World/FCSpawnArea.h"
#include "FCSettings.h"
#include "Game/FCGameViewportClient.h"
#include "UI/FCBaseHUD.h"
#include "Game/FCGameState.h"
#include "Game/FCGameMode.h"
#include "Utility/FCFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogFCPlayerController, Log, All);

AFCPlayerController::AFCPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PlayerClass(EPlayerClassType::EPCT_Class1)
	, GameOverlayWidget(nullptr)
	, ControllerState(EControllerState::None)
	, CurrentArea(nullptr)
	, PlayerStart(nullptr)
	, LastKillerPlayer(nullptr)
	, LastKillerDamageData(FDamageData())
{
}

void AFCPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_OwnerOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerController, ControllerState, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerController, CurrentArea, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerController, PlayerStart, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerController, MainCharacter, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(AFCPlayerController, LastKillerPlayer, Parameters);
}

void AFCPlayerController::OnPossess(APawn* InPawn)
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (MainCharacter && ControllerState == EControllerState::Playing)
	{
		FadeOut(3.0f);
	}

	Super::OnPossess(InPawn);
}

void AFCPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	if (MainCharacter && MainCharacter->IsAlive()) return;

	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	SetMainCharacter(nullptr);
}

void AFCPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (GetPawn() && IsLocalController())
	{
		// Truque para ajustar a camera pro personagem e não o contrário
		SetControlRotation(GetPawn()->GetActorRotation());
	}
}

void AFCPlayerController::OnRep_PlayerState()
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	Super::OnRep_PlayerState();

	if (!IsLocalController()) return;

	if (AFCPlayerState* PS = GetPlayerState<AFCPlayerState>())
	{
		if (APawn* PSPawn = PS->GetPawn())
		{
			if (PSPawn && PSPawn->IsA<AFCCharacter>() && MainCharacter != PSPawn)
			{
				UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

				SetMainCharacter(Cast<AFCCharacter>(PSPawn));
			}
		}

		PS->OnPawnSet.AddDynamic(this, &AFCPlayerController::OnPawnSet);
	}
}

void AFCPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		BuildUI();

		for (TActorIterator<AFCCaptureFlag> ActorItr(GetWorld(), AFCCaptureFlag::StaticClass()); ActorItr; ++ActorItr)
		{
			if (AFCCaptureFlag* CaptureFlag = *ActorItr)
			{
				CaptureFlag->OnFlagGrabbedByTeam.AddDynamic(this, &AFCPlayerController::OnFlagGrabbedByTeam);
				CaptureFlag->OnFlagCapturedByTeam.AddDynamic(this, &AFCPlayerController::OnFlagCapturedByTeam);

				AvailableCaptureFlags.Add(CaptureFlag);
			}
		}
	}
}

void AFCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RegisterHandle);
	GetWorldTimerManager().ClearTimer(MatchEndHandle);
	GetWorldTimerManager().ClearTimer(DeathFadeHandle);
	GetWorldTimerManager().ClearTimer(DeathInactiveHandle);

	GetWorldTimerManager().ClearTimer(ResetPlayerStateHanlde);

	GetWorldTimerManager().ClearTimer(SpawnTransitionHanlde);
	GetWorldTimerManager().ClearTimer(LevelCheckHanlde);

	for (AFCCaptureFlag* CaptureFlag : AvailableCaptureFlags)
	{
		if (CaptureFlag)
		{
			CaptureFlag->OnFlagGrabbedByTeam.RemoveDynamic(this, &AFCPlayerController::OnFlagGrabbedByTeam);
			CaptureFlag->OnFlagCapturedByTeam.RemoveDynamic(this, &AFCPlayerController::OnFlagCapturedByTeam);
		}
	}

	AvailableCaptureFlags.Empty();

	Super::EndPlay(EndPlayReason);
}

void AFCPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
}

void AFCPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void AFCPlayerController::InitPlayerState()
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	Super::InitPlayerState();

	if (PlayerState && GetNetMode() != NM_DedicatedServer)
	{
		UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

		OnRep_PlayerState();
	}
}

void AFCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(InputMappingContext, 0);
	}

	if (UEnhancedInputComponent* const EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(ScoreboardAction, ETriggerEvent::Triggered, this, &AFCPlayerController::InputScoreboard);
	}
}

void AFCPlayerController::BeginInactiveState()
{
	Super::BeginInactiveState();

	if (ControllerState == EControllerState::Playing)
	{
		SetMainCharacter(nullptr);

		SetControllerState(EControllerState::Death);

		bPlayerIsWaiting = true;
		PlayerState->SetIsSpectator(true);

		ChangeState(NAME_Spectating);
		ClientGotoState(NAME_Spectating);

		BeginSpectatingState();

		if (LastKillerPlayer != PlayerState)
		{
			FadeOut(FadeToSpectateDuration);
		}
	}
}

void AFCPlayerController::UnFreeze()
{
	Super::UnFreeze();

	if (ControllerState == EControllerState::Death)
	{
		if (LastKillerPlayer != PlayerState)
		{
			EnterSpectatingState();

			return;
		}

		RequestPlayerStart();
	}
}

void AFCPlayerController::AutoManageActiveCameraTarget(AActor* SuggestedTarget)
{
	if (ControllerState == EControllerState::Death)
	{
		SuggestedTarget = GetViewTarget();
	}

	Super::AutoManageActiveCameraTarget(SuggestedTarget);
}

void AFCPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	const ETeamSide Team = static_cast<ETeamSide>(GenericTeamIdToInteger(NewTeamID));
	JoinTeam(Team);
}

FGenericTeamId AFCPlayerController::GetGenericTeamId() const
{
	return IntegerToGenericTeamId(static_cast<int32>(GetPlayerSide()));
}

ETeamAttitude::Type AFCPlayerController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(OtherPawn->GetController()))
		{
			const FGenericTeamId OtherTeamID = TeamAgent->GetGenericTeamId();
			if (OtherTeamID.GetId() != GetGenericTeamId().GetId())
			{
				return ETeamAttitude::Hostile;
			}
			else
			{
				return ETeamAttitude::Friendly;
			}
		}
	}

	return ETeamAttitude::Neutral;
}

void AFCPlayerController::SetMainCharacter(AFCCharacter* InCharacter)
{
	if (MainCharacter && MainCharacter == InCharacter) return;

	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetMainCharacter(InCharacter);

		return;
	}

	if (AFCPlayerState* PS = GetPlayerState<AFCPlayerState>())
	{
		if (PS->GetMainCharacter() != InCharacter)
		{
			PS->SetMainCharacter(InCharacter);

			AFCCharacter* OldCharacter = MainCharacter;
			MainCharacter = InCharacter;

			MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, MainCharacter, this);

			if (GetNetMode() != NM_DedicatedServer)
			{
				OnRep_MainCharacter(OldCharacter);
			}

			if (OldCharacter)
			{
				OldCharacter->SetPlayerState(nullptr);
			}
		}
	}
}

void AFCPlayerController::ServerSetMainCharacter_Implementation(AFCCharacter* InCharacter)
{
	SetMainCharacter(InCharacter);
}

void AFCPlayerController::TrySetMainCharacter(AFCCharacter* InCharacter)
{
	SetMainCharacter(InCharacter);
}

void AFCPlayerController::SetPlayerStart(AActor* InPlayerStart, AFCPlayableArea* InPlayableArea)
{
	if (PlayerStart != InPlayerStart)
	{
		AActor* OldPlayerStart = PlayerStart;
		PlayerStart = InPlayerStart;

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, PlayerStart, this);

		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_PlayerStart(OldPlayerStart);
		}

		if (InPlayableArea)
		{
			CurrentArea = InPlayableArea;

			MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, CurrentArea, this);
		}
	}
}

void AFCPlayerController::OnCharacterSpawned(ACharacter* InCharacter)
{
	if (InCharacter)
	{
		Possess(InCharacter);

		AActor* OldPlayerStart = PlayerStart;
		SetPlayerStart(nullptr, nullptr);

		ClientSetViewTarget(InCharacter);
	}
}

void AFCPlayerController::OnLoadoutInitialized()
{
	if (MainCharacter)
	{
		MainCharacter->OnLoadoutInitialized.Unbind();
	}

	ClientOnLoadoutInitialized();
}

void AFCPlayerController::ClientOnLoadoutInitialized_Implementation()
{
	if (MainCharacter)
	{
		CheckLoadoutInitialized();
	}
}

void AFCPlayerController::CheckLoadoutInitialized()
{
	if (MainCharacter)
	{
		const bool bWeaponListInitialized = MainCharacter->IsWeaponListInitialized();
		if (bWeaponListInitialized)
		{
			UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

			GetWorldTimerManager().ClearTimer(LoadoutInitializedHandle);

			RequestControllerState(EControllerState::Playing);
		}
		else
		{
			GetWorldTimerManager().SetTimer(LoadoutInitializedHandle, this, &AFCPlayerController::CheckLoadoutInitialized, 0.025f, false);
		}
	}
	else
	{
		UE_LOG(LogFCPlayerController, Warning, TEXT("%s MainCharacter nullptr"), *FC_LOGS_LINE);
	}
}

void AFCPlayerController::OnPlayerLogin(AActor* InPlayerStart, EGameInitialState GameInitialState)
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	CachedGameInitialState = GameInitialState;

	ClientOnPlayerLogin(InPlayerStart, CachedGameInitialState);
}

void AFCPlayerController::ClientOnPlayerLogin_Implementation(AActor* InPlayerStart, EGameInitialState GameInitialState)
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	CachedGameInitialState = GameInitialState;

	if (InPlayerStart)
	{
		ClientSetViewTarget(InPlayerStart);
	}

	if (UWorldPartition* WorldPartition = GetWorld()->GetWorldPartition())
	{
		CheckLevel();
	}
	else
	{
		switch (CachedGameInitialState)
		{
			case EGameInitialState::Listen:
			case EGameInitialState::Waiting:
				RequestPlayerStart();
				break;
			case EGameInitialState::Preparing:
			case EGameInitialState::InProgress:
				RequestEnterRegisterState();
				break;
			case EGameInitialState::Ended:
				RequestEnterMatchEndedState();
				break;
			case EGameInitialState::NextGame:
				RequestEnterMatchEnded();
				break;
		}
	}
}

void AFCPlayerController::CheckLevel()
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	GetWorldTimerManager().ClearTimer(LevelCheckHanlde);

	if (PlayerCameraManager)
	{
		const FVector CameraLocation = PlayerCameraManager->GetCameraLocation();
		const FRotator CameraRotation = PlayerCameraManager->GetCameraRotation();

		const float TraceZOffsets = 50000.0f;
		const float ZOffsets = 32.0f;
		FVector UpVector = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Z) * TraceZOffsets;

		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);
		CollisionParams.bReturnPhysicalMaterial = false;

		bool HitTheGround = false;
		if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation + UpVector, CameraLocation - UpVector, COLLISION_PROJECTILE, CollisionParams))
		{
			HitTheGround = HitResult.bBlockingHit;
		}

		if (HitTheGround)
		{
			switch (CachedGameInitialState)
			{
				case EGameInitialState::Listen:
				case EGameInitialState::Waiting:
					RequestPlayerStart();
					break;
				case EGameInitialState::Preparing:
				case EGameInitialState::InProgress:
					RequestEnterRegisterState();
					break;
				case EGameInitialState::Ended:
					RequestEnterMatchEndedState();
					break;
				case EGameInitialState::NextGame:
					RequestEnterMatchEnded();
					break;
			}

			return;
		}
	}

	GetWorldTimerManager().SetTimer(LevelCheckHanlde, this, &AFCPlayerController::CheckLevel, 1.0f, false);
}

void AFCPlayerController::OnGameInitialStateChanged(EGameInitialState GameInitialState)
{
	if (CachedGameInitialState != GameInitialState)
	{
		CachedGameInitialState = GameInitialState;

		ClientOnGameInitialStateChanged(CachedGameInitialState);
	}
}

void AFCPlayerController::ClientOnGameInitialStateChanged_Implementation(EGameInitialState GameInitialState)
{
	CachedGameInitialState = GameInitialState;

	if (GameOverlayWidget)
	{
		GameOverlayWidget->OnGameInitialStateChanged(CachedGameInitialState);
	}

	if (CachedGameInitialState == EGameInitialState::Preparing)
	{
		if (MainCharacter)
		{
			MainCharacter->DisableInput(this);
		}
	}
	else if (CachedGameInitialState == EGameInitialState::InProgress)
	{
		BuildScoreboard();

		if (UFCPlayerOverlay* PlayerOverlay = GetPlayerOverlay())
		{
			PlayerOverlay->OnPlaying();
		}

		if (MainCharacter)
		{
			MainCharacter->OnPlaying(CurrentArea);
			MainCharacter->EnableInput(this);
		}

		if (AFCGameState* const GS = GetWorld()->GetGameState<AFCGameState>())
		{
			GS->OnPlayableArea(CurrentArea);
		}

		// Tocar som?
	}
	else if (CachedGameInitialState == EGameInitialState::Ended || CachedGameInitialState == EGameInitialState::NextGame)
	{
		if (CachedGameInitialState == EGameInitialState::Ended)
		{
			// Tocar som?

			if (AFCSpectatorPawn* Spectator = Cast<AFCSpectatorPawn>(GetSpectatorPawn()))
			{
				Spectator->OnMatchEnded();
			}
		}

		if (MainCharacter)
		{
			MainCharacter->OnMatchEnded();
			MainCharacter->DisableInput(this);
		}
	}
}

void AFCPlayerController::SetControllerState(EControllerState InControllerState)
{
	if (GetLocalRole() < ROLE_Authority) return;

	if (ControllerState != InControllerState)
	{
		EControllerState OldControllerState = ControllerState;
		ControllerState = InControllerState;

		MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, ControllerState, this);

		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_ControllerState(OldControllerState);
		}
	}
}

void AFCPlayerController::RequestControllerState(EControllerState InControllerState)
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerRequestControllerState(InControllerState);

		return;
	}

	SetControllerState(InControllerState);
}

void AFCPlayerController::ServerRequestControllerState_Implementation(EControllerState InControllerState)
{
	RequestControllerState(InControllerState);
}

void AFCPlayerController::EnterPreparingState()
{
	FadeIn(FadeToRegisterState);

	GetWorldTimerManager().SetTimer(RegisterHandle, FTimerDelegate::CreateLambda([this]() {
		if (!IsValidLowLevel()) return;

		EnterRegisterState();
	}), FadeToRegisterState, false);
}

void AFCPlayerController::EnterMatchEndedState()
{
	ClearLastKillerData();

	GetWorldTimerManager().SetTimer(MatchEndHandle, FTimerDelegate::CreateLambda([this]() {
		if (!IsValidLowLevel()) return;

		EnterMatchEnded();
	}), FadeToEndDuration, false);
}

void AFCPlayerController::EnterNextGameState()
{
	FadeIn(FadeToEndDuration);

	GetWorldTimerManager().SetTimer(MatchEndHandle, FTimerDelegate::CreateLambda([this]() {
		if (!IsValidLowLevel()) return;

		if (APawn* PawnOrSpectator = GetPawnOrSpectator())
		{
			UnPossess();
			PawnOrSpectator->Destroy();
		}
	}), FadeToEndDuration, false);
}

void AFCPlayerController::ClientEnterPlayingState_Implementation()
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	FPlayerLoadout PlayerLoadout = GetLoadout();

	RequestSpawnLoadout(PlayerLoadout);
}

void AFCPlayerController::RequestSpawnLoadout(const FPlayerLoadout& PlayerLoadout)
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerRequestSpawnLoadout(PlayerLoadout);

		return;
	}

	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	SpawnLoadout(PlayerLoadout);
}

void AFCPlayerController::ServerRequestSpawnLoadout_Implementation(const FPlayerLoadout& PlayerLoadout)
{
	RequestSpawnLoadout(PlayerLoadout);
}

void AFCPlayerController::SpawnLoadout(const FPlayerLoadout& PlayerLoadout)
{
	if (MainCharacter)
	{
		if (CachedGameInitialState == EGameInitialState::Preparing || CachedGameInitialState == EGameInitialState::InProgress)
		{
			UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

			MainCharacter->OnLoadoutInitialized.BindUObject(this, &AFCPlayerController::OnLoadoutInitialized);
			MainCharacter->PopulateLoadout(this, PlayerLoadout);
		}
	}
}

void AFCPlayerController::EnterRegisterState()
{
	APawn* LastPawn = GetPawnOrSpectator();
	UnPossess();
	
	if (LastPawn)
	{
		LastPawn->Destroy();
	}

	SetControllerState(EControllerState::Register);
}

void AFCPlayerController::EnterSpawnState()
{
	if (ControllerState == EControllerState::None)
	{
		Spawn();
		return;
	}

	ClearLastKillerData();
	SetControllerState(EControllerState::Spawn);
}

void AFCPlayerController::EnterPlayingState()
{
	ClientEnterPlayingState();
}

void AFCPlayerController::EnterDeathState()
{
	GetWorldTimerManager().SetTimer(DeathFadeHandle, FTimerDelegate::CreateLambda([this]() {
		if (!IsValidLowLevel()) return;

		FadeIn(FadeToDeathDuration);
	}), DelayBeforeFade, false);

	if (PlayerCameraManager)
	{
		// Truque para garantir que o spectator faça spawn na posição correta: https://wizardcell.com/unreal/spectating-system/
		SetSpawnLocation(PlayerCameraManager->GetCameraLocation());
	}

	GetWorldTimerManager().SetTimer(DeathInactiveHandle, FTimerDelegate::CreateLambda([this]() {
		if (!IsValidLowLevel()) return;

		BeginInactiveState();
	}), DelayBeforeInactive, false);
}

void AFCPlayerController::EnterDeathStateFast()
{
	GetWorldTimerManager().SetTimer(DeathFadeHandle, FTimerDelegate::CreateLambda([this]()
	{
		if (!IsValidLowLevel()) return;

		FadeIn(FadeToDeathDuration);
	}), DelayBeforeFade, false);

	if (PlayerCameraManager)
	{
		// Truque para garantir que o spectator faça spawn na posição correta: https://wizardcell.com/unreal/spectating-system/
		SetSpawnLocation(PlayerCameraManager->GetCameraLocation());
	}

	GetWorldTimerManager().SetTimer(DeathInactiveHandle, FTimerDelegate::CreateLambda([this]()
	{
		if (!IsValidLowLevel()) return;

		SetMainCharacter(NULL);
		SetControllerState(EControllerState::Death);

		bPlayerIsWaiting = true;
		PlayerState->SetIsSpectator(true);

		ChangeState(NAME_Spectating);
		ClientGotoState(NAME_Spectating);

		BeginSpectatingState();

		if (LastKillerPlayer != PlayerState)
		{
			FadeOut(FadeToSpectateDuration);
		}

		RequestPlayerStart();
	}), DelayBeforeInactive, false);
}

void AFCPlayerController::EnterSpectatingState()
{
	SetControllerState(EControllerState::Spectating);
}

void AFCPlayerController::EnterMatchEnded()
{
	SetControllerState(EControllerState::MatchEnded);
}

void AFCPlayerController::OnEnterRegisterState()
{
	ClientSetHUD(RegisterHUD);
}

void AFCPlayerController::OnEnterSpawnState()
{
	if (IsFirstSpawn())
	{
		FadeIn(0.25f);
	}
	else
	{
		if (CachedGameInitialState == EGameInitialState::InProgress)
		{
			FadeOut(FadeToSpawnDuration);
		}
		else
		{
			FadeIn(0.25f);
		}
	}

	if (CachedGameInitialState == EGameInitialState::InProgress)
	{
		FadeOut(FadeToSpawnDuration);
	}
	else
	{
		FadeIn(0.25f);
	}

	ClientSetHUD(SpawnHUD);
}

void AFCPlayerController::OnEnterPlayingState()
{
	if (IsFirstSpawn())
	{
		FadeOut(FadeToPlayingDuration);
	}
	else if (PrevControllerState == EControllerState::None || CachedGameInitialState != EGameInitialState::InProgress)
	{
		FadeOut(FadeToPlayingDuration);
	}

	ClientSetHUD(PlayingHUD);
}

void AFCPlayerController::OnEnterDeathState()
{
	ClientSetHUD(DeathHUD);
}

void AFCPlayerController::OnEnterSpectatingState()
{
	ClientSetHUD(SpectatingHUD);

	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		if (APawn* TargetPawn = GetSpectatorPawn())
		{
			PS->ViewTargetChanged(TargetPawn);
		}
	}
}

void AFCPlayerController::OnEnterMatchEnded()
{
	ClientSetHUD(MatchEndedHUD);
}

bool AFCPlayerController::IsFirstSpawn()
{
	if (AFCPlayerState* CurrentlayerState = GetPlayerState<AFCPlayerState>())
	{
		return CurrentlayerState->GetDeaths() == 0;
	}

	return true;
}

void AFCPlayerController::BuildUI()
{
	if (GameOverlayWidget)
	{
		GameOverlayWidget->RemoveFromParent();
	}

	if (GameOverlayWidgetClass)
	{
		GameOverlayWidget = CreateWidget<UFCGameOverlay>(this, GameOverlayWidgetClass);
		if (GameOverlayWidget)
		{
			GameOverlayWidget->AddToViewport(10);
			GameOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			GameOverlayWidget->OnGameInitialStateChanged(CachedGameInitialState);
		}
	}
}

void AFCPlayerController::BuildScoreboard()
{
	if (CachedGameInitialState != EGameInitialState::InProgress) return;

	if (ScoreboardWidgetClass)
	{
		ClearScoreboard();

		ScoreboardWidget = CreateWidget<UFCScoreboard>(this, ScoreboardWidgetClass);
		if (ScoreboardWidget)
		{
			ScoreboardWidget->AddToViewport(21);
			ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void AFCPlayerController::ClearScoreboard()
{
	if (ScoreboardWidget)
	{
		ScoreboardWidget->RemoveFromParent();
		ScoreboardWidget = nullptr;
	}
}

void AFCPlayerController::OnMainWidgetInitialized()
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (!bFirstPlay)
	{
		bFirstPlay = true;
	}

	AFCPlayerState* CurrentPlayerState = GetPlayerState<AFCPlayerState>();

	if (ControllerState == EControllerState::Spawn)
	{
		if (CachedGameInitialState == EGameInitialState::InProgress)
		{
			if (GameOverlayWidget)
			{
				GameOverlayWidget->RemoveFromParent();
			}
		}
	}
	else if (ControllerState == EControllerState::Playing)
	{
		if (MainCharacter && CurrentPlayerState)
		{
			if (CachedGameInitialState == EGameInitialState::InProgress)
			{
				BuildUI();

				if (PlayerCameraManager)
				{
					UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

					if (CurrentPlayerState->GetDeaths() != 0)
					{
						FadeOut(0.5f);
						ClientSetViewTarget(MainCharacter);
						BeginPlaying();
					}
					else
					{
						ClientSetViewTarget(MainCharacter);
						BeginPlaying();
					}
				}
			}
			else
			{
				ClientSetViewTarget(MainCharacter);
			}
		}
	}
	else if (ControllerState == EControllerState::MatchEnded)
	{
		if (AFCGameState* const GS = GetWorld()->GetGameState<AFCGameState>())
		{
			if (CurrentPlayerState)
			{
				const ETeamSide WinnerTeam = GS->GetMatchWinnerResult();
				const bool MatchWinner = WinnerTeam == CurrentPlayerState->GetPlayerSide();

				// Tocar som?
			}
		}
	}
}

void AFCPlayerController::BeginPlaying()
{
	if (MainCharacter/* && MainCharacter->IsAlive()*/)
	{
		if (UFCPlayerOverlay* PlayerOverlay = GetPlayerOverlay())
		{
			PlayerOverlay->OnPlaying();
		}

		MainCharacter->OnPlaying(CurrentArea);
		MainCharacter->EnableInput(this);
	}
}

void AFCPlayerController::OnPlayerJoinTeam(ETeamSide InTeam)
{
	if (InTeam == ETeamSide::None) return;

	if (AFCPlayerState* PS = GetPlayerState<AFCPlayerState>())
	{
		PS->OnPlayerJoinTeam.Unbind();
	}

	ClientOnPlayerJoinTeam(InTeam);
}

void AFCPlayerController::ClientOnPlayerJoinTeam_Implementation(ETeamSide InTeam)
{
	if (!IsLocalController()) return;

	RegisterTeam(InTeam);
}

void AFCPlayerController::RegisterTeam(ETeamSide InTeam)
{
	CachedTeam = InTeam;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerRegisterTeam(InTeam);

		return;
	}

	if (AFCGameMode* const GameMode = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GameMode->PlayerJoinTeam(this, InTeam);
	}
}

void AFCPlayerController::ServerRegisterTeam_Implementation(ETeamSide InTeam)
{
	RegisterTeam(InTeam);
}

void AFCPlayerController::RequestEnterRegisterState()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerEnterRegisterState();

		return;
	}

	EnterRegisterState();
}

void AFCPlayerController::ServerEnterRegisterState_Implementation()
{
	RequestEnterRegisterState();
}

void AFCPlayerController::RequestEnterPlayingState()
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s %s"), *FC_LOGS_LINE, *UEnum::GetValueAsString(ControllerState));

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerEnterPlayingState();

		return;
	}

	EnterPlayingState();
}

void AFCPlayerController::ServerEnterPlayingState_Implementation()
{
	RequestEnterPlayingState();
}

void AFCPlayerController::RequestPlayerStart()
{
	if (PlayerStart) return;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerRequestPlayerStart();

		return;
	}

	if (AFCGameMode* const GameMode = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GameMode->RequestPlayerStart(this);
	}
}

void AFCPlayerController::ServerRequestPlayerStart_Implementation()
{
	RequestPlayerStart();
}

void AFCPlayerController::RequestEnterSpawnState()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerRequestEnterSpawnState();

		return;
	}

	EnterSpawnState();
}

void AFCPlayerController::ServerRequestEnterSpawnState_Implementation()
{
	RequestEnterSpawnState();
}

void AFCPlayerController::RequestEnterMatchEndedState()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerEnterMatchEndedState();

		return;
	}

	EnterMatchEndedState();
}

void AFCPlayerController::ServerEnterMatchEndedState_Implementation()
{
	RequestEnterMatchEndedState();
}

void AFCPlayerController::RequestEnterMatchEnded()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerEnterMatchEnded();

		return;
	}

	EnterMatchEnded();
}

void AFCPlayerController::ServerEnterMatchEnded_Implementation()
{
	RequestEnterMatchEnded();
}

void AFCPlayerController::AddScore(int32 Val)
{
	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		const int32 CurrentScore = PS->GetScore();
		float FinalScore = CurrentScore + Val;
		if (FinalScore < 0.0f)
		{
			FinalScore = 0.0f;
		}

		PS->SetScore(FinalScore);
	}
}

void AFCPlayerController::AddKill()
{
	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		PS->AddKill();
	}
}

void AFCPlayerController::AddDeath()
{
	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		PS->AddDeath();
	}
}

void AFCPlayerController::AddHeadshot()
{
	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		PS->AddHeadshot();
	}
}

void AFCPlayerController::AddDamage(float DamageAmount)
{
	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		PS->AddDamage(DamageAmount);
	}
}

void AFCPlayerController::AddKilledPlayer(APlayerState* PlayerPS)
{
	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		PS->AddKilledPlayer(Cast<AFCPlayerState>(PlayerPS));
	}
}

void AFCPlayerController::StoreLastKillerData(APlayerState* KillerPlayerState, const FKillEventData& KillEventData)
{
	LastKillerPlayer = KillerPlayerState;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, LastKillerPlayer, this);

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_LastKillerPlayer();
	}

	ClientSetDamageData(KillEventData);
}

void AFCPlayerController::ClearLastKillerData()
{
	LastKillerPlayer = nullptr;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, LastKillerPlayer, this);

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_LastKillerPlayer();
	}

	ClientClearDamageData();
}

void AFCPlayerController::OnTookDamage(float DamageAmount, AActor* Causer)
{
	ClientOnTookDamage(DamageAmount, Causer);
}

void AFCPlayerController::ClientOnTookDamage_Implementation(float DamageAmount, AActor* Causer)
{
	// Tocar som?

	if (GameOverlayWidget)
	{
		GameOverlayWidget->OnTookDamage(DamageAmount, Causer);
	}

	if (HitShake)
	{
		ClientStartCameraShake(HitShake, 1.0f, ECameraShakePlaySpace::CameraLocal);
	}

	if (OnTookDamageEvent.IsBound())
	{
		OnTookDamageEvent.Broadcast(this);
	}
}

void AFCPlayerController::NotifyOnCharacterDied(ACharacter* InCharacter, const FKillEventData& KillEventData, AFCPlayerState* InPlayerState, AFCPlayerState* KillerPlayerState, int32 InScore)
{
	ClientOnCharacterDied(InCharacter, KillEventData, InPlayerState, KillerPlayerState, InScore);
}

void AFCPlayerController::NotifyOnHit(AActor* InActor, const FHitResult& HitResult, float DamageAmount)
{
	AddDamage(DamageAmount);

	ClientOnHit(InActor, HitResult.Location, HitResult.BoneName, DamageAmount);
}

void AFCPlayerController::NotifyOnKill(AController* KilledPC, bool bIsHeadshot)
{
	ClientOnKill(KilledPC->GetPlayerState<APlayerState>(), bIsHeadshot);
}

void AFCPlayerController::ClientSetDamageData_Implementation(const FKillEventData& KillEventData)
{
	LastKillerDamageData = UFCFunctionLibrary::GetDamageData(KillEventData);
}

void AFCPlayerController::ClientClearDamageData_Implementation()
{
	LastKillerDamageData = FDamageData();
}

void AFCPlayerController::ClientOnCharacterDied_Implementation(ACharacter* InCharacter, const FKillEventData& KillEventData, AFCPlayerState* InPlayerState, AFCPlayerState* KillerPlayerState, int32 InScore)
{
	const FDamageData& DamageData = UFCFunctionLibrary::GetDamageData(KillEventData);

	if (InPlayerState)
	{
		if (GameOverlayWidget)
		{
			GameOverlayWidget->OnPlayerDied(DamageData, InPlayerState, KillerPlayerState);
		}

		if (ScoreboardWidget)
		{
			ScoreboardWidget->UpdateList();
		}

		if (KillerPlayerState)
		{
			AFCCharacter* const DiedCharacter = Cast<AFCCharacter>(InCharacter);

			if (DiedCharacter)
			{
				if (InPlayerState && InPlayerState == LastKillerPlayer)
				{
					UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

					GetWorldTimerManager().SetTimer(ResetPlayerStateHanlde, FTimerDelegate::CreateLambda([this, InCharacter]() {
						if (!IsValidLowLevel()) return;

						UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);
						InCharacter->SetPlayerState(nullptr);
					}), 1.0f, false);
				}
			}
			else
			{
				UE_LOG(LogFCPlayerController, Error, TEXT("%s"), *FC_LOGS_LINE);
			}

			if (KillerPlayerState == PlayerState)
			{
				if (GameOverlayWidget)
				{
					if (InPlayerState != KillerPlayerState && InPlayerState->IsEnemyFor(KillerPlayerState))
					{
						GameOverlayWidget->OnPlayerKillEnemy(DamageData, InPlayerState);
					}
					else
					{
						GameOverlayWidget->OnPlayerKillTeammate(DamageData, InPlayerState, InScore);
					}
				}
			}
		}
	}
}

void AFCPlayerController::ClientOnHit_Implementation(AActor* HitActor, FVector_NetQuantize HitLocation, FName HitBoneName, float DamageAmount)
{
	bool bShoultNotifyHit = true;
	if (HitActor && HitActor->IsA<APawn>())
	{
		bShoultNotifyHit = false;
		APawn* const OtherPawn = Cast<APawn>(HitActor);
		if (OtherPawn && OtherPawn != GetPawn() && IsEnemyFor(OtherPawn))
		{
			bShoultNotifyHit = true;
		}
	}

	if (bShoultNotifyHit)
	{
		if (AFCBaseHUD* const HUD = Cast<AFCBaseHUD>(MyHUD))
		{
			HUD->NotifyHitEvent(HitLocation, DamageAmount);
		}

		// Tocar som de hitmarker? seria top

		if (AFCCharacterBase* HitCharacter = Cast<AFCCharacterBase>(HitActor))
		{
			EDamageResultType DamageResultType = EDamageResultType::EDRT_Normal;

			const FString BoneName = HitBoneName.ToString().ToLower();
			if (BoneName.Equals(TEXT("head"), ESearchCase::CaseSensitive))
			{
				if (DamageAmount >= HitCharacter->GetMaxHealth())
				{
					DamageResultType = EDamageResultType::EDRT_Headshoot;
				}
				else
				{
					DamageResultType = EDamageResultType::EDRT_Critical;
				}
			}
			else if (BoneName.Equals(TEXT("neck_01"), ESearchCase::CaseSensitive)
				|| BoneName.Equals(TEXT("pelvis"), ESearchCase::CaseSensitive)
				|| BoneName.Equals(TEXT("spine_01"), ESearchCase::CaseSensitive)
				|| BoneName.Equals(TEXT("spine_02"), ESearchCase::CaseSensitive)
				|| BoneName.Equals(TEXT("spine_03"), ESearchCase::CaseSensitive))
			{
				DamageResultType = EDamageResultType::EDRT_Critical;
			}

			HitCharacter->OnHitStatus(DamageResultType, HitLocation, DamageAmount);
		}
	}
}

void AFCPlayerController::ClientOnKill_Implementation(APlayerState* KilledPlayerState, bool bIsHeadshot)
{
	AFCPlayerState* KilledPS = Cast<AFCPlayerState>(KilledPlayerState);
	if (KilledPS && KilledPS->GetPlayerSide() == GetPlayerSide()) return;

	// Tocar som?
}

void AFCPlayerController::OnPlayableAreaStateChanged(bool bIsInsidePlayableArea)
{
	if (GameOverlayWidget)
	{
		GameOverlayWidget->OnPlayableAreaStateChanged(bIsInsidePlayableArea);
	}
}

void AFCPlayerController::OnFlagGrabbedByTeam(AFCCaptureFlag* CaptureFlag, ETeamSide ByTeam)
{
	if (!CaptureFlag) return;

	if (!GameOverlayWidget) return;

	const ETeamSide PlayerSide = GetPlayerSide();
	if (ByTeam == PlayerSide)
	{
		GameOverlayWidget->OnTeamGrabFlag(CaptureFlag);
	}
	else
	{
		GameOverlayWidget->OnEnemyGrabFlag(CaptureFlag);
	}
}

void AFCPlayerController::OnFlagCapturedByTeam(AFCCaptureFlag* CaptureFlag, ETeamSide ByTeam)
{
	if (!CaptureFlag) return;

	if (!GameOverlayWidget) return;

	const ETeamSide PlayerSide = GetPlayerSide();
	if (ByTeam == PlayerSide)
	{
		if (CurrentCaptureFlag == CaptureFlag)
		{
			AddFlagsCaptured();
		}
		else
		{
			GameOverlayWidget->OnTeamCaptureFlag(CaptureFlag);
		}
	}
	else
	{
		GameOverlayWidget->OnEnemyCaptureFlag(CaptureFlag);
	}
}

void AFCPlayerController::AddFlagsCaptured()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerAddFlagsCaptured();

		return;
	}

	if (AFCPlayerState* const PS = GetPlayerState<AFCPlayerState>())
	{
		PS->AddFlagsCaptured();
	}
}

void AFCPlayerController::ServerAddFlagsCaptured_Implementation()
{
	AddFlagsCaptured();
}

void AFCPlayerController::InputScoreboard()
{
	if (ControllerState != EControllerState::Playing) return;

	ToggleScoreboard();
}

void AFCPlayerController::OnRep_ControllerState(EControllerState OldControllerState)
{
	if (!IsLocalController()) return;

	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s %s"), *FC_LOGS_LINE, *UEnum::GetValueAsString(ControllerState));

	PrevControllerState = OldControllerState;

	switch (ControllerState)
	{
		case EControllerState::Register: OnEnterRegisterState(); break;
		case EControllerState::Spawn: OnEnterSpawnState(); break;
		case EControllerState::Playing: OnEnterPlayingState(); break;
		case EControllerState::Death: OnEnterDeathState(); break;
		case EControllerState::Spectating: OnEnterSpectatingState(); break;
		case EControllerState::MatchEnded: OnEnterMatchEnded(); break;
	}

	if (ControllerState == EControllerState::Playing)
	{
		BuildScoreboard();
	}
	else
	{
		ClearScoreboard();
	}

	if (GameOverlayWidget)
	{
		GameOverlayWidget->OnControllerStateChanged(ControllerState);
	}

	bShowMouseCursor = ControllerState != EControllerState::Playing;
	if (bShowMouseCursor)
	{
		FInputModeGameAndUI InputModeUIOnly;
		if (ControllerState == EControllerState::Spawn)
		{
			InputModeUIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		}
		SetInputMode(InputModeUIOnly);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AFCPlayerController::OnRep_PlayerStart(AActor* OldPlayerStart)
{
	if (!PlayerStart) return;

	if (IsLocalController())
	{
		if (TargetViewPlayer)
		{
			SetSepctatorTarget(nullptr);
		}

		if (ControllerState != EControllerState::Spawn)
		{
			RequestEnterSpawnState();
		}
		else
		{
			if (OnPlayerStartChanged.IsBound())
			{
				OnPlayerStartChanged.Broadcast(Cast<AFCSpawnArea>(PlayerStart));
			}
		}
	}
}

void AFCPlayerController::OnRep_LastKillerPlayer()
{
	if (IsLocalController())
	{
		if (!LastKillerPlayer)
		{
			if (AFCSpectatorPawn* Spectator = Cast<AFCSpectatorPawn>(GetSpectatorPawn()))
			{
				Spectator->OnStateReset();
			}
		}
	}
}

void AFCPlayerController::OnRep_MainCharacter(AFCCharacter* OldCharacter)
{
	if (!IsLocalController()) return;

	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s %s"), *FC_LOGS_LINE, *UEnum::GetValueAsString(ControllerState));

	if (OldCharacter)
	{
		OldCharacter->OnAmmoAmountChanged.RemoveDynamic(this, &AFCPlayerController::OnAmmoAmountChanged);
		OldCharacter->OnAmmoReserveChanged.RemoveDynamic(this, &AFCPlayerController::OnAmmoReserveChanged);
	}

	if (MainCharacter)
	{
		MainCharacter->OnAmmoAmountChanged.AddDynamic(this, &AFCPlayerController::OnAmmoAmountChanged);
		MainCharacter->OnAmmoReserveChanged.AddDynamic(this, &AFCPlayerController::OnAmmoReserveChanged);

		if (CachedGameInitialState == EGameInitialState::Preparing || CachedGameInitialState == EGameInitialState::Ended)
		{
			MainCharacter->DisableInput(this);
		}

		UE_LOG(LogFCPlayerController, Verbose, TEXT("%s %s"), *FC_LOGS_LINE, *UEnum::GetValueAsString(CachedGameInitialState));

		RequestEnterPlayingState();
	}
}

void AFCPlayerController::OnPawnSet(APlayerState* InPlayer, APawn* InNewPawn, APawn* InOldPawn)
{
	if (InNewPawn && InNewPawn->IsA<AFCCharacter>() && MainCharacter != InNewPawn)
	{
		UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

		SetMainCharacter(Cast<AFCCharacter>(InNewPawn));
	}
}

void AFCPlayerController::OnAmmoAmountChanged(int32 Ammo)
{
	if (UFCPlayerOverlay* PlayerOverlay = GetPlayerOverlay())
	{
		PlayerOverlay->SetAmmoAmount(Ammo);
	}
}

void AFCPlayerController::OnAmmoReserveChanged(int32 Ammo)
{
	if (UFCPlayerOverlay* PlayerOverlay = GetPlayerOverlay())
	{
		PlayerOverlay->SetReserveAmmoAmount(Ammo);
	}
}

UFCPlayerOverlay* AFCPlayerController::GetPlayerOverlay() const
{
	if (AFCBaseHUD* const HUD = Cast<AFCBaseHUD>(MyHUD))
	{
		return HUD->GetMainWidget<UFCPlayerOverlay>();
	}

	return NULL;
}

FPlayerLoadout AFCPlayerController::GetLoadout() const
{
	const FClassLoadout& ClassLoadout = LoadoutPresets[PlayerClass];
	return ClassLoadout.Loadout;
}

APlayerState* AFCPlayerController::GetSepctatorTarget() const
{
	if (TargetViewPlayer.Get())
	{
		return TargetViewPlayer.Get();
	}

	return LastKillerPlayer;
}

TArray<AFCSpawnArea*> AFCPlayerController::GetSpawnableAreas() const
{
	TArray<AFCSpawnArea*> Temp;
	if (AFCGameState* const GS = GetWorld()->GetGameState<AFCGameState>())
	{
		Temp = GS->GetSpawnableAreasByTeam(GetPlayerSide());
	}

	return Temp;
}

AFCSpawnArea* AFCPlayerController::GetCurrentSpawnableArea() const
{
	return Cast<AFCSpawnArea>(PlayerStart);
}

bool AFCPlayerController::IsEnemyFor(AFCPlayerController* Other) const
{
	return Other && Other->GetPlayerSide() != GetPlayerSide();
}

bool AFCPlayerController::IsEnemyFor(AFCPlayerState* InPlayerState) const
{
	if (InPlayerState && ::IsValid(InPlayerState) && IsValidLowLevel())
	{
		ETeamSide EnemyTeam = InPlayerState->GetPlayerSide();

		return GetPlayerSide() != EnemyTeam;
	}

	return true;
}

bool AFCPlayerController::IsEnemyFor(AController* Other) const
{
	if (AFCPlayerController* const OtherPC = Cast<AFCPlayerController>(Other))
	{
		return IsEnemyFor(OtherPC);
	}

	if (Other->GetPawn())
	{
		return GetTeamAttitudeTowards(*Other->GetPawn()) == ETeamAttitude::Hostile;
	}

	return true;
}

bool AFCPlayerController::IsEnemyFor(APawn* InPawn) const
{
	if (InPawn && InPawn->GetPlayerState())
	{
		if (AFCPlayerState* const OtherPC = InPawn->GetPlayerState<AFCPlayerState>())
		{
			return IsEnemyFor(OtherPC);
		}
	}

	return true;
}

FLinearColor AFCPlayerController::GetTeamColor() const
{
	return UFCSettings::Get()->GetColorForTeam(GetPlayerSide());
}

FLinearColor AFCPlayerController::GetEnemyColor() const
{
	return UFCSettings::Get()->GetOppositeColorForTeam(GetPlayerSide());
}

bool AFCPlayerController::ShowCrosshair() const
{
	// Esconder se algum menu estiver aberto?

	return true;
}

void AFCPlayerController::SetSepctatorTarget(APlayerState* InPlayerState)
{
	if (TargetViewPlayer != InPlayerState)
	{
		TargetViewPlayer = InPlayerState;

		if (OnTargetViewPlayerChanged.IsBound())
		{
			OnTargetViewPlayerChanged.Broadcast(TargetViewPlayer);
		}
	}
}

void AFCPlayerController::FadeIn(float Duration)
{
	ClientFadeIn(Duration);
}

void AFCPlayerController::ClientFadeIn_Implementation(float Duration)
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (UFCGameViewportClient* GVC = Cast<UFCGameViewportClient>(GetLocalPlayer()->ViewportClient))
	{
		GVC->FadeScreenIn(Duration);
	}
}

void AFCPlayerController::FadeOut(float Duration)
{
	ClientFadeOut(Duration);
}

void AFCPlayerController::ClientFadeOut_Implementation(float Duration)
{
	UE_LOG(LogFCPlayerController, Verbose, TEXT("%s"), *FC_LOGS_LINE);

	if (UFCGameViewportClient* GVC = Cast<UFCGameViewportClient>(GetLocalPlayer()->ViewportClient))
	{
		GVC->FadeScreenOut(Duration);
	}
}

void AFCPlayerController::JoinTeam(ETeamSide InTeam)
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerJoinTeam(InTeam);

		return;
	}

	if (AFCGameMode* const GameMode = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		ETeamSide TargetTeam = InTeam == ETeamSide::None ? GameMode->GetDesiredAutoJoinTeam() : InTeam;
		if (AFCPlayerState* PS = GetPlayerState<AFCPlayerState>())
		{
			PS->OnPlayerJoinTeam.BindUObject(this, &AFCPlayerController::OnPlayerJoinTeam);
			PS->JoinTeam(TargetTeam);
		}
	}
}

void AFCPlayerController::ServerJoinTeam_Implementation(ETeamSide InTeam)
{
	JoinTeam(InTeam);
}

void AFCPlayerController::Spawn()
{
	if (IsLocalController() && TargetViewPlayer.Get())
	{
		if (APawn* TargetPlayerPawn = TargetViewPlayer->GetPawn())
		{
			const FVector& TargetLocation = TargetPlayerPawn->GetActorLocation();

			const float CapsuleRadius = 34.0f;
			const float CapsuleHeight = 88.0f;
			TArray<AActor*> Ignores;

			TArray<FTransform> SpawnTransforms;
			UFCFunctionLibrary::GetSpawnPointsAtLocation(this, TargetLocation, 176.0f, 16, CapsuleHeight + 10.0f, SpawnTransforms);

			TArray<FTransform> AvailableSpawnTransforms;
			for (const FTransform& SpawnTransform : SpawnTransforms)
			{
				const FVector& PointLocation = SpawnTransform.GetLocation();
				const FRotator& PointRotation = SpawnTransform.GetRotation().Rotator();
				bool bCanSpawnAtLoacation = UFCFunctionLibrary::CanSpawnAtLocation(this, PointLocation, CapsuleRadius, CapsuleHeight, {});
				const FColor CapsuleColor = bCanSpawnAtLoacation ? FColor::Green : FColor::Red;

				if (bCanSpawnAtLoacation)
				{
					AvailableSpawnTransforms.Add(SpawnTransform);
				}
			}

			if (AvailableSpawnTransforms.Num() != 0)
			{
				UFCFunctionLibrary::ShuffleArrayTransform(AvailableSpawnTransforms);
				FTransform TargetTransform = AvailableSpawnTransforms[0];

				SpawnAtDesired(TargetTransform.GetLocation(), TargetTransform.GetRotation().Rotator().Vector());

				return;
			}
		}
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSpawn();

		return;
	}

	if (AFCGameMode* const GameMode = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GameMode->PlayerSpawn(this);
	}
}

void AFCPlayerController::ServerSpawn_Implementation()
{
	Spawn();
}

void AFCPlayerController::SpawnAtDesired(const FVector_NetQuantize100& InLocation, const FVector_NetQuantizeNormal& InDirection)
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSpawnAtDesired(InLocation, InDirection);

		return;
	}

	if (AFCGameMode* const GameMode = GetWorld()->GetAuthGameMode<AFCGameMode>())
	{
		GameMode->PlayerSpawnAtDesired(this, InLocation, InDirection.Rotation());
	}
}

void AFCPlayerController::ServerSpawnAtDesired_Implementation(const FVector_NetQuantize100& InLocation, const FVector_NetQuantizeNormal& InDirection)
{
	SpawnAtDesired(InLocation, InDirection);
}

void AFCPlayerController::ExitSpectator()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerExitSpectator();

		return;
	}

	APawn* LastPawn = GetPawnOrSpectator();
	UnPossess();
	if (LastPawn)
	{
		LastPawn->Destroy();
	}

	RequestPlayerStart();
}

void AFCPlayerController::ServerExitSpectator_Implementation()
{
	ExitSpectator();
}

void AFCPlayerController::UpdatePlayerStart(AActor* InPlayerStart)
{
	if (!InPlayerStart) return;

	if (PlayerStart == InPlayerStart)
	{
		if (TargetViewPlayer)
		{
			SetSepctatorTarget(nullptr);
		}

		if (OnPlayerStartChanged.IsBound())
		{
			OnPlayerStartChanged.Broadcast(Cast<AFCSpawnArea>(PlayerStart));
		}

		return;
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerUpdatePlayerStart(InPlayerStart);

		return;
	}

	AActor* OldPlayerStart = PlayerStart;
	PlayerStart = InPlayerStart;

	MARK_PROPERTY_DIRTY_FROM_NAME(AFCPlayerController, PlayerStart, this);

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_PlayerStart(OldPlayerStart);
	}
}

void AFCPlayerController::ServerUpdatePlayerStart_Implementation(AActor* InPlayerStart)
{
	UpdatePlayerStart(InPlayerStart);
}

void AFCPlayerController::SetPlayerClass(TEnumAsByte<EPlayerClassType> InPlayerClass)
{
	if (InPlayerClass != EPlayerClassType::EPCT_Max && InPlayerClass != PlayerClass)
	{
		PlayerClass = InPlayerClass;
		const FClassLoadout& ClassLoadout = LoadoutPresets[PlayerClass];
		if (OnLoadoutClassChanged.IsBound())
		{
			OnLoadoutClassChanged.Broadcast(PlayerClass);
		}
	}
}

void AFCPlayerController::ToggleScoreboard()
{
	if (!IsLocalController()) return;

	if (ScoreboardWidget)
	{
		bScoreboardOpen = !bScoreboardOpen;

		bShowMouseCursor = bScoreboardOpen;
		if (bShowMouseCursor)
		{
			SetInputMode(FInputModeGameAndUI());
		}
		else
		{
			SetInputMode(FInputModeGameOnly());
		}

		ESlateVisibility TargetVisibility = bScoreboardOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
		ScoreboardWidget->SetVisibility(TargetVisibility);
	}
}
