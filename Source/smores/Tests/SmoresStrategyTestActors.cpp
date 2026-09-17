// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresStrategyTestActors.h"

ATestStrategyNPC::ATestStrategyNPC()
{
	// a test world has no navmesh and wants no AI controller; none of the rules under test
	// involve either, and spawning one is cost and noise for nothing
	AutoPossessAI = EAutoPossessAI::Disabled;
}

ATestStrategyPlayerUnit::ATestStrategyPlayerUnit()
{
	AutoPossessAI = EAutoPossessAI::Disabled;
}
