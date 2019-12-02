// Fill out your copyright notice in the Description page of Project Settings.

#include "SkeletalMeshCollider.h"
#include "Modules/ModuleManager.h"
#include "AutomationTest.h"

class FTestModuleImpl : public FDefaultGameModuleImpl {

	void ShutdownModule() override {
		/* Workaround for UE-25350 */
		FAutomationTestFramework::Get().UnregisterAutomationTest("FComponentSweepSpec");
		// ... for every test you defined.
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FTestModuleImpl, SkeletalMeshCollider, "SkeletalMeshCollider" );
