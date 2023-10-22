// Copyright Epic Games, Inc. All Rights Reserved.

#include "PeakPursuitGameMode.h"
#include "PeakPursuitCharacter.h"
#include "UObject/ConstructorHelpers.h"

APeakPursuitGameMode::APeakPursuitGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
