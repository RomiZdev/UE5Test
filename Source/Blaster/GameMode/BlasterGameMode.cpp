// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	 
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		AttackerPlayerState->AddtoScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
	}
	
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ELimmedController)
{
		if (ELimmedController)
		{
			TArray<AActor*> PlayerStarts;
			TArray<AActor*> OtherPlayers;
			UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(),PlayerStarts);
			UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(),OtherPlayers);
			
			for (AActor* Player : OtherPlayers)
			{
				if (Player == ElimmedCharacter) continue;
				for (AActor* PlayerStart : PlayerStarts)
				{
					float PlayerDistanceFromStart = Player->GetDistanceTo(PlayerStart);
					if (Distances.Num() < PlayerStarts.Num()) 
					{
						Distances.Add(PlayerDistanceFromStart);
					}
					else
					{
						Distances[Index] += PlayerDistanceFromStart;
						if (Distances[Index] > MaxDistance)
						{
							MaxDistance = Distances[Index];
							SpawnLocation = PlayerStart;
						}
						if (Index  >= Distances.Num()) break;
						Index += 1;
					}
				}
			}
			ElimmedCharacter->Reset();
			ElimmedCharacter->Destroy();
		}
		if (ELimmedController)
		{
			RestartPlayerAtPlayerStart(ELimmedController, SpawnLocation);
			MaxDistance = 0.f;
			Index = 0.f;
			Distances.Empty();
		}
}