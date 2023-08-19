// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSAIGuard.h"

#include "UnitConversion.h"
#include "FPSGameMode.h"
#include "Net/UnrealNetwork.h"
// Sets default values
AFPSAIGuard::AFPSAIGuard()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComp"));

	PawnSensingComp -> OnSeePawn.AddDynamic(this , &AFPSAIGuard::OnPawnSeeing);
	PawnSensingComp -> OnHearNoise.AddDynamic(this , &AFPSAIGuard::OnNoiseHeard);

	GuardState = EAIState::Idle;
}

// Called when the game starts or when spawned
void AFPSAIGuard::BeginPlay()
{
	Super::BeginPlay();
	OriginalRotation = GetActorRotation();

	if(bPatrol)
	{
		MoveToNextPatrolPoint();
	}
}

/**
 * 看到玩家
 * @param SeenPawn 
 */
void AFPSAIGuard::OnPawnSeeing(APawn* SeenPawn)
{
	if(SeenPawn == nullptr)
	{
		return;
	}
	DrawDebugSphere(GetWorld() , SeenPawn -> GetActorLocation() , 32.0f , 12 , FColor:: Red , false , 10.0f);
	
	AFPSGameMode* GM = Cast<AFPSGameMode>(GetWorld() -> GetAuthGameMode());
	if(GM)
	{
		GM -> CompleteMission(SeenPawn , true);
	}

	SetGuardState(EAIState::Alerted);

	AController* Controller = GetController();
	if(Controller)
	{
		Controller -> StopMovement();
	}
}


void AFPSAIGuard::OnNoiseHeard(APawn* NoiseInstigator, const FVector& Location, float Volume)
{
	if(GuardState == EAIState::Alerted)
	{
		return;
	}
	DrawDebugSphere(GetWorld() , NoiseInstigator -> GetActorLocation() , 32.0f , 12 , FColor:: Green , false , 10.0f);

	FVector Direction = Location - GetActorLocation();
	Direction.Normalize();
	FRotator NewLookAt = FRotationMatrix::MakeFromX(Direction).Rotator();
	NewLookAt.Pitch = 0.0f;
	NewLookAt.Roll = 0.0f;
	
	SetActorRotation(NewLookAt);

	GetWorldTimerManager().ClearTimer(TimerHandle_ResetOrientation);
	GetWorldTimerManager().SetTimer(TimerHandle_ResetOrientation , this , &AFPSAIGuard::ResetOrientation , 3.0f);


	SetGuardState(EAIState::Suspicious);
	
	AController* Controller = GetController();
	if(Controller)
	{
		Controller -> StopMovement();
	}
}


/**
 * 返回旋转原位
 */
void AFPSAIGuard::ResetOrientation()
{
	if(GuardState == EAIState::Alerted)
	{
		return;
	} 
	SetActorRotation(OriginalRotation);
	SetGuardState(EAIState::Idle);

	if(bPatrol)
	{
		MoveToNextPatrolPoint();
	}
}


void AFPSAIGuard::OnRep_GuardState()
{
	OnStateChange(GuardState);
}

/**
 * 设置状态
 * @param NewState 新状态 
 */
void AFPSAIGuard::SetGuardState(EAIState NewState)
{
	if(GuardState == NewState)
	{
		return;
	}

	GuardState = NewState;
	OnRep_GuardState();
}

void AFPSAIGuard::MoveToNextPatrolPoint()
{
	if(CurrentPatrolPoint == nullptr || CurrentPatrolPoint == SecondPatrolPoint)
	{
		CurrentPatrolPoint = FirstPatrolPoint;
	}
	else
	{
		CurrentPatrolPoint = SecondPatrolPoint;
	}

	UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController() , CurrentPatrolPoint);
}

// Called every frame
void AFPSAIGuard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(CurrentPatrolPoint)
	{
		FVector Delta = GetActorLocation() - CurrentPatrolPoint -> GetActorLocation();
		float DistanceToGoal = Delta.Size();

		if(DistanceToGoal < 100)
		{
			MoveToNextPatrolPoint();
		}
	}
}

void AFPSAIGuard::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSAIGuard, GuardState);
}