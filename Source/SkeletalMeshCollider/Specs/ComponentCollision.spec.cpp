#include "Animation/SkeletalMeshActor.h"
#include "AutomationEditorCommon.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#define TEST_TRUE_THROW(expression) \
	do { if (!(expression)) { AddError(TEXT("Expected '") TEXT(#expression) TEXT("' to be true."), 0); throw 0; } } while (0)

#define TEST_FALSE_THROW(expression) \
	do { if ((expression)) { AddError(TEXT("Expected '") TEXT(#expression) TEXT("' to be false."), 0); throw 0; } } while (0)

#define TEST_NOT_NULL_THROW(expression) \
	do { if (!(expression)) { AddError(TEXT("Expected '") TEXT(#expression) TEXT("' to be not null."), 0); throw 0; } } while (0)

#define TEST_EQUAL_THROW(expression, expected) \
	do { if ((expression) != (expected)) { AddError(FString::Printf(TEXT("%s%d"), TEXT("Expected ") TEXT(#expression) TEXT(" to be equal to ") TEXT(#expected) TEXT(", but it was "), (expression)), 0); throw 0; } } while (0)

template <typename T>
FString ToString(const T& object);

template <>
FString ToString(const FVector& object)
{
	return object.ToString();
}

template <>
FString ToString(const int& value)
{
	return FString::Printf(TEXT("%d"), value);
}

template <>
FString ToString(const float& value)
{
	return FString::Printf(TEXT("%f"), value);
}


template <typename T>
bool TestEqualWithToleranceComparison(const T& a, const T& b, float Tolerance);

template <>
bool TestEqualWithToleranceComparison(const FVector& a, const FVector& b, const float Tolerance)
{
	return a.Equals(b, Tolerance);
}

template <>
bool TestEqualWithToleranceComparison(const int& a, const int& b, const float Tolerance)
{
	return FMath::IsNearlyEqual(a, b, Tolerance);
}

template <>
bool TestEqualWithToleranceComparison(const float& a, const float& b, const float Tolerance)
{
	return FMath::IsNearlyEqual(a, b, Tolerance);
}

#define TEST_EQUAL_TOLERANCE_THROW(expression, expected, tolerance) \
	do { if (!TestEqualWithToleranceComparison((expression), (expected), (tolerance))) { AddError(FString::Printf(TEXT("Expected ") TEXT(#expression) TEXT(" to be equal to %s but it was %s"), *ToString(expected), *ToString(expression)), 0); throw 0; } } while (0)

UWorld* CreateWorld()
{
	auto World = FAutomationEditorCommonUtils::CreateNewMap();
	check(World);
	return World;
}

UBoxComponent* CreateFloor(UWorld* World)
{
	auto FloorPlane = World->SpawnActor<AActor>();
	check(FloorPlane);
	auto FloorPlaneCollider = NewObject<UBoxComponent>(FloorPlane, UBoxComponent::StaticClass(), TEXT("FloorPlaneCollider"));
	check(FloorPlaneCollider);
	FloorPlaneCollider->RegisterComponent();
	FloorPlaneCollider->SetWorldLocation(FVector{ 0, 0, -281.f });
	FloorPlaneCollider->SetBoxExtent(FVector{ 1000, 1000, 1.f });
	FloorPlaneCollider->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	FloorPlaneCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	return FloorPlaneCollider;
}

template <class ComponentType>
ComponentType* CreatePrimitiveCollider(UWorld* World)
{
	auto Actor = World->SpawnActor<AActor>();
	check(Actor);
	auto Component = NewObject<ComponentType>(Actor, ComponentType::StaticClass(), TEXT("Collider"));
	check(Component);
	Component->RegisterComponent();
	Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	return Component;
}

BEGIN_DEFINE_SPEC(FComponentSweepSpec, "ComponentCollision", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
END_DEFINE_SPEC(FComponentSweepSpec)
void FComponentSweepSpec::Define()
{
	BeforeEach([this]() {
		});

	Describe("Sweeps", [this]()
		{
			Describe("Sphere shape sweeps against UBoxComponent", [this]()
			{
				It("Sphere Sweep hits the floor from above", [this]()
					{
						try {
							const FVector SweepStart = FVector{ 0, 0, 1000.f };
							const FVector SweepEnd = FVector{ 0, 0, -1000.f };
							const float SphereRadius = 100.f;

							UWorld* World = CreateWorld();
							TEST_NOT_NULL_THROW(World);
							UBoxComponent* FloorPlaneCollider = CreateFloor(World);
							TEST_NOT_NULL_THROW(FloorPlaneCollider);

							FHitResult OutHit;
							FCollisionShape CollisionShape;
							CollisionShape.SetSphere(SphereRadius);
							bool HitFound = World->SweepSingleByChannel(OutHit, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), ECollisionChannel::ECC_Visibility, CollisionShape);
							TEST_TRUE_THROW(HitFound);
							TEST_EQUAL_TOLERANCE_THROW(OutHit.ImpactPoint.Z, FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
							TEST_EQUAL_TOLERANCE_THROW(OutHit.Distance, SweepStart.Z - (FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z) - SphereRadius, 0.1f);
						} catch (...) {}
					});
				});

			Describe("USphereComponent sweeps against UBoxComponent", [this]()
				{
					It("USphereComponent Sweep hits the floor from above", [this]()
					{
						try {
							const FVector SweepStart = FVector{ 0, 0, 1000.f };
							const FVector SweepEnd = FVector{ 0, 0, -1000.f };
							const float SphereRadius = 100.f;

							UWorld* World = CreateWorld();
							TEST_NOT_NULL_THROW(World);
							UBoxComponent* FloorPlaneCollider = CreateFloor(World);
							TEST_NOT_NULL_THROW(FloorPlaneCollider);
							USphereComponent* Component = CreatePrimitiveCollider<USphereComponent>(World);
							TEST_NOT_NULL_THROW(Component);
							Component->SetSphereRadius(SphereRadius );

							TArray<FHitResult> OutHits;
							FComponentQueryParams Params;
							Params.AddIgnoredActor(Component->GetOwner());
							bool HitFound = World->ComponentSweepMulti(OutHits, Component, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
							TEST_TRUE_THROW(HitFound);
							TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
							TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - (FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z) - SphereRadius, 0.1f);
						} catch (...) {}
					});
				});

			Describe("USkeletalMeshComponent sweeps against UBoxComponent", [this]()
				{

					It("USkeletalMeshComponent (box at origin, identity ref pose) Sweep hits the floor from above", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, 1000.f };
								const FVector SweepEnd = FVector{ 0, 0, -1000.f };
								const float CubeHalfZExtent = 100.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<ASkeletalMeshActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/Cube-IdentityRefPose/BP_Cube-IdentityRefPose.BP_Cube-IdentityRefPose_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<ASkeletalMeshActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								auto Component = Cast<USkeletalMeshComponent>(Actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
								TEST_NOT_NULL_THROW(Component);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(Component->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, Component, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - (FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z) - CubeHalfZExtent, 0.1f);
							} catch (...) {}
						});

					It("USkeletalMeshComponent (box at origin, rotated ref pose) Sweep hits the floor from above", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, 1000.f };
								const FVector SweepEnd = FVector{ 0, 0, -1000.f };
								const float CubeHalfZExtent = 100.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<ASkeletalMeshActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/Cube-RotatedRefPose/BP_Cube-RotatedRefPose.BP_Cube-RotatedRefPose_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<ASkeletalMeshActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								auto Component = Cast<USkeletalMeshComponent>(Actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
								TEST_NOT_NULL_THROW(Component);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(Component->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, Component, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - (FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z) - CubeHalfZExtent, 0.1f);
							}
							catch (...) {}
						});

					It("USkeletalMeshComponent (box at origin, scaled ref pose) Sweep hits the floor from above", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, 1000.f };
								const FVector SweepEnd = FVector{ 0, 0, -1000.f };
								const float CubeHalfZExtent = 200.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<ASkeletalMeshActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/Cube-ScaledRefPose/BP_Cube-ScaledRefPose.BP_Cube-ScaledRefPose_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<ASkeletalMeshActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								auto Component = Cast<USkeletalMeshComponent>(Actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
								TEST_NOT_NULL_THROW(Component);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(Component->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, Component, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - (FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z) - CubeHalfZExtent, 0.1f);
							}
							catch (...) {}
						});

					It("USkeletalMeshComponent (box at origin, scaled and rotated ref pose) Sweep hits the floor from above", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, 1000.f };
								const FVector SweepEnd = FVector{ 0, 0, -1000.f };
								const float CubeHalfZExtent = 200.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<ASkeletalMeshActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/Cube-ScaledAndRotatedRefPose/BP_Cube-ScaledAndRotatedRefPose.BP_Cube-ScaledAndRotatedRefPose_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<ASkeletalMeshActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								auto Component = Cast<USkeletalMeshComponent>(Actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
								TEST_NOT_NULL_THROW(Component);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(Component->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, Component, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - (FloorPlaneCollider->GetComponentLocation().Z + FloorPlaneCollider->GetScaledBoxExtent().Z) - CubeHalfZExtent, 0.1f);
							}
							catch (...) {}
						});
				});

			Describe("UBoxComponent sweeps against runtime-welded UBoxComponents", [this]()
				{

					It("Floor hits UBoxCollider pair from above", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, 1000.f };
								const FVector SweepEnd = FVector{ 0, 0, -1000.f };
								const float ObstacleMaxZ = 100.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<AActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/CubePair-UBoxColliders/BP_CubePair-UBoxColliders.BP_CubePair-UBoxColliders_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<AActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								TArray<UBoxComponent*> Boxes;
								Actor->GetComponents(Boxes);
								TEST_EQUAL_THROW(Boxes.Num(), 2);
								auto Box1 = Boxes[0];
								TEST_NOT_NULL_THROW(Box1);
								auto Box2 = Boxes[1];
								TEST_NOT_NULL_THROW(Box2);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(FloorPlaneCollider->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, FloorPlaneCollider, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, ObstacleMaxZ, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - FloorPlaneCollider->GetScaledBoxExtent().Z - ObstacleMaxZ, 0.1f);
							}
							catch (...) {}
						});

					It("Floor hits UBoxCollider pair from below", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, -1000.f };
								const FVector SweepEnd = FVector{ 0, 0, 1000.f };
								const float ObstacleMinZ = -150.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<AActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/CubePair-UBoxColliders/BP_CubePair-UBoxColliders.BP_CubePair-UBoxColliders_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<AActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								TArray<UBoxComponent*> Boxes;
								Actor->GetComponents(Boxes);
								TEST_EQUAL_THROW(Boxes.Num(), 2);
								auto Box1 = Boxes[0];
								TEST_NOT_NULL_THROW(Box1);
								auto Box2 = Boxes[1];
								TEST_NOT_NULL_THROW(Box2);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(FloorPlaneCollider->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, FloorPlaneCollider, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, ObstacleMinZ, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, ObstacleMinZ - SweepStart.Z - FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
							}
							catch (...) {}
						});

					It("Floor hits UBoxCollider pair from above (runtime-welded)", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, 1000.f };
								const FVector SweepEnd = FVector{ 0, 0, -1000.f };
								const float ObstacleMaxZ = 100.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<AActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/CubePair-UBoxColliders/BP_CubePair-UBoxColliders.BP_CubePair-UBoxColliders_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<AActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								TArray<UBoxComponent*> Boxes;
								Actor->GetComponents(Boxes);
								TEST_EQUAL_THROW(Boxes.Num(), 2);
								auto Box1 = Boxes[0];
								TEST_NOT_NULL_THROW(Box1);
								auto Box2 = Boxes[1];
								TEST_NOT_NULL_THROW(Box2);
								Box2->WeldTo(Box1);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(FloorPlaneCollider->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, FloorPlaneCollider, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, ObstacleMaxZ, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, SweepStart.Z - FloorPlaneCollider->GetScaledBoxExtent().Z - ObstacleMaxZ, 0.1f);
							}
							catch (...) {}
						});

					It("Floor hits UBoxCollider pair from below (runtime-welded)", [this]()
						{
							try {
								const FVector SweepStart = FVector{ 0, 0, -1000.f };
								const FVector SweepEnd = FVector{ 0, 0, 1000.f };
								const float ObstacleMinZ = -150.f;

								UWorld* World = CreateWorld();
								TEST_NOT_NULL_THROW(World);
								UBoxComponent* FloorPlaneCollider = CreateFloor(World);
								TEST_NOT_NULL_THROW(FloorPlaneCollider);

								auto ActorClass = ::LoadClass<AActor>(nullptr, TEXT("Blueprint'/Game/ComponentCollision/CubePair-UBoxColliders/BP_CubePair-UBoxColliders.BP_CubePair-UBoxColliders_C'"));
								TEST_NOT_NULL_THROW(ActorClass);
								auto Actor = World->SpawnActor<AActor>(ActorClass);
								TEST_NOT_NULL_THROW(Actor);
								TArray<UBoxComponent*> Boxes;
								Actor->GetComponents(Boxes);
								TEST_EQUAL_THROW(Boxes.Num(), 2);
								auto Box1 = Boxes[0];
								TEST_NOT_NULL_THROW(Box1);
								auto Box2 = Boxes[1];
								TEST_NOT_NULL_THROW(Box2);
								Box2->WeldTo(Box1);

								TArray<FHitResult> OutHits;
								FComponentQueryParams Params;
								Params.AddIgnoredActor(FloorPlaneCollider->GetOwner());
								bool HitFound = World->ComponentSweepMulti(OutHits, FloorPlaneCollider, SweepStart, SweepEnd, FQuat(EForceInit::ForceInit), Params);
								TEST_TRUE_THROW(HitFound);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].ImpactPoint.Z, ObstacleMinZ, 0.1f);
								TEST_EQUAL_TOLERANCE_THROW(OutHits[0].Distance, ObstacleMinZ - SweepStart.Z - FloorPlaneCollider->GetScaledBoxExtent().Z, 0.1f);
							}
							catch (...) {}
						});
				});
		});

	AfterEach([this]() {
		});
}
 

#undef TEST_TRUE_THROW
#undef TEST_FALSE_THROW
#undef TEST_NOT_NULL_THROW
#undef TEST_EQUAL_THROW
#undef TEST_EQUAL_TOLERANCE_THROW
