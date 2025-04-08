// Copyright (C) 2021-2023 Space Raccoon Game Studio. All rights reserved. Contacts: <business@space-raccoon.com>
// Created by AfroStalin

#pragma once

#include "CoreMinimal.h"
#include "NPC_Optimizator_Types.h"
#include "Components/ActorComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/ThreadSafeBool.h"
#include "OptimizationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOptimizationChangeWave, EOptimizationWave, NewWave);

UCLASS(BlueprintType, Blueprintable, Category = "NPC Optimizator", ClassGroup="NPCOptimizator", DisplayName="NPC Optimization Component", meta=(BlueprintSpawnableComponent),
	HideCategories=(ComponentReplication, Activation, Replication, Collision, Sockets, Tags))
class NPC_OPTIMIZATOR_API UOptimizationProxyComponent : public UActorComponent, public IOptimizationProxy
{
	GENERATED_BODY()
public:
	UOptimizationProxyComponent();
public:
	// UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~UActorComponent

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:
	// IOptimizationProxy
	virtual EOptimizationBaseType GetOptimizationBasedType() const override;
	virtual AActor*               GetOwnerActor() override;
	virtual float                 GetMaxVisibleDistance() const override;
	virtual float                 GetDistToWave(EOptimizationWave Wave) const override;
	virtual bool                  IsIgnoreCameraSightOnSmallDistance() const override;
	virtual bool                  IsForceOptimizationWaveEnabled() const override;
	//~IOptimizationProxy
public:
	int32                    GetOptimizationHandle() const { return OptimizationHandle; }
	EOptimizationWave        GetOptimizationWave() const;

	void                     SetOptimizationHandle(int32 Handle) { OptimizationHandle = Handle; }
	void                     SetOptimizationWave(EOptimizationWave Wave);

	void                     SetIsNeedToBeOptimized(bool NeedToOptimize);
	bool                     IsNeedToBeOptimized() const;

	void                     OptimizeComponent();
public:
	// Execute this function for disable all optimization for current owner
	UFUNCTION(BlueprintCallable, Category = "NPC Optimizator")
	void                     DisableAllOptimizations();

	// Execute this function for enable optimization after disabling it
	UFUNCTION(BlueprintCallable, Category = "NPC Optimizator")
	void                     EnableOptimizations();

	// Execute this function for force set optimization wave
	UFUNCTION(BlueprintCallable, Category = "NPC Optimizator")
	void                     EnableForceOptimizationWave(EOptimizationWave Wave, bool Enable);
public:
	// The distance from the player to the bots at which the first optimization level is enabled. 
	// Bots closer than this distance are not optimized
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Distance to first wave")
	float DistanceToFirstOptimization = 1500.f;

	// The distance from the player to the bots at which the second level of optimizations is enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Distance to second wave")
	float DistanceToSecondOptimization = 2500.f;

	// The distance from the player to the bots at which the third level of optimizations is enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Distance to third wave")
	float DistanceToThirdOptimization = 3500.f;

	// Bots outside this distance will be hidden and simplified as much as possible
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Max visible distance")
	float MaxVisibleDistance = 8000.f;

	// Optimization is based on the distance from the player's pawn or camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Optimization type")
	EOptimizationBaseType OptimizationBasedOn = EOptimizationBaseType::PlayerPawn;

	// If you want optimize only component with special tag - enable it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Use optimization by tag")
	bool UseOptimizationByTag = false;

	// Special tag for component needed for optimization it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Optimization tag", meta=(EditCondition=UseOptimizationByTag))
	FName OptimizationTag = "NeedOptimization";

	// Special tag to ignore any component optimizations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName="Ignore optimization tag")
	FName IgnoreOptimizationTag = "IgnoreOptimization";

	// If you want stop NPC movement when it not visible
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Disable movement when invisible")
	bool DisableMovementWhenNotVisible = false;

	// Enable this setting in order to ignore the calculation of the visibility of NPCs by the camera and calculate only by distance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Ignore camera sight")
	bool IgnoreCameraSight = false;

	// Ignore camera visibility when bots are close to the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Ignore camera sight in 'no optimization' wave")
	bool IgnoreCameraSightOnSmallDistance = false;

	// Enabling this setting gives you more frames per second, but when mesh switch from invisibility back to visibility, the number of frames may drop slightly
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Disable skeletal mesh tick when invisible")
	bool DisableSkeletalMeshTickWhenNotVisible = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Disable groom tick when invisible")
	bool DisableGroomTickWhenNotVisible = false;

	// If you want hide all static meshes attached to skeletal meshes
	// If you need hide only special static meshes use Optimization Tag for it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Hide all attached static meshes")
	bool HideAllStaticMeshes = true;

	// When using a listener server, there are moments when the client-server does not see bots and because of this,
	// other players have visual glitches, to avoid this, bots on the listener server ignore any optimizations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Disable optimization on listen server")
	bool bDisableOptimizationOnListenServer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Disable optimization on dedicated server")
	bool bDisableOptimizationOnDedicatedServer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Disable optimization in simulation mode")
	bool bDisableOptimizationInSimulationMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Optimize AI controller")
	bool bOptimizeAIController = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization", DisplayName = "Optimize path follow component")
	bool bOptimizePathFollowComponent = true; 
public:
	// Movement component need always check floor on this wave?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Always check floor")
	bool FirstWave_AlwaysCheckFloor = true;

	// Enable physics interaction on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Enable physics interaction")
	bool FirstWave_EnablePhysicsInteraction = true;

	// Max simulation time step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Max simulation time step")
	float FirstWave_MaxSimulationTimeStep = 0.025f;

	// Max simulation iterations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Max simulation iterations")
	int32 FirstWave_MaxSimulationIterations = 4;

	// Run physics with no controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Run physics with no controller")
	bool FirstWave_RunPhysicsWithNoController = true;

	// Movement mode on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Movement mode")
	TEnumAsByte<EMovementMode> FirstWave_MovementMode = EMovementMode::MOVE_NavWalking;

	// Whether or not the character should sweep for collision geometry while walking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Sweep while nav walking")
	bool FirstWave_SweepWhileNavWalking = true;
public:
	// Movement component tick rate when random tick not used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Fixed movement tick")
	float FirstWave_OptimizatedMovementTick = 0.05f;

	// Movement component min tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Min movement random tick")
	float FirstWave_OptimizatedMovementTickMin = 0.01f;

	// Movement component max tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Max movement random tick")
	float FirstWave_OptimizatedMovementTickMax = 0.05f;

	// Use random tick for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Movement", DisplayName = "Use random movement tick")
	bool FirstWave_UseRandomOptimizationTickForMovement = true;
public:
	// Hide shadows from skeletal mesh on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Hide shadows")
	bool FirstWave_HideShadows = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Groom", DisplayName = "Hide shadows")
	bool FirstWaveGroom_HideShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Groom", DisplayName = "Hide groom")
	bool FirstWaveGroom_Hide = false;

	// Disable skeletal meshes collision on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Disable mesh collision")
	bool FirstWave_DisableMeshCollision = false;

	// Disable skeletal meshes collision on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Groom", DisplayName = "Disable mesh collision")
	bool FirstWaveGroom_DisableMeshCollision = true;

	// Whether or not the character should hide attached static meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Need hide static meshes")
	bool FirstWave_NeedHideStaticMeshes = false;

	// Use update rate optimizations for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Use URO")
	bool FirstWave_UseUpdateRateOptimizations = true;

	// Use per bone motion blur for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Use per bone motion blur")
	bool FirstWave_UsePerBoneMotionBlur = true;

	// Disable cloth simulation for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Disable cloth simulation")
	bool FirstWave_DisableClothSimulation = true;

	// Disable morph target simulation for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Disable morph target")
	bool FirstWave_DisableMorphTarget = true;

	// Skip kinematic update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Skip kinematic update when interpolating")
	bool FirstWave_SkipKinematicUpdateWhenInterpolating = true;

	// Skip bounds update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Skip bounds update when interpolating")
	bool FirstWave_SkipBoundsUpdateWhenInterpolating = true;

	// Allow rigid body anim node for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Allow rigid body anim node")
	bool FirstWave_AllowRigidBodyAnimNode = true;

	// Generate overlap events?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Generate overlap events")
	bool FirstWave_GenerateOverlapEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Groom", DisplayName = "Generate overlap events")
	bool FirstWaveGroom_GenerateOverlapEvents = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|First wave|Skeletal meshes", DisplayName = "Visibility Based Anim Tick Option")
	EVisibilityBasedAnimTickOption FirstWave_VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
public:
	// Movement component need always check floor on this wave?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Always check floor")
	bool SecondWave_AlwaysCheckFloor = false;

	// Enable physics interaction on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Enable physics interaction")
	bool SecondWave_EnablePhysicsInteraction = false;

	// Max simulation time step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Max simulation time step")
	float SecondWave_MaxSimulationTimeStep = 0.035f;

	// Max simulation iterations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Max simulation iterations")
	int32 SecondWave_MaxSimulationIterations = 2;

	// Run physics with no controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Run physics with no controller")
	bool SecondWave_RunPhysicsWithNoController = false;

	// Movement mode on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Movement mode")
	TEnumAsByte<EMovementMode> SecondWave_MovementMode = EMovementMode::MOVE_NavWalking;

	// Whether or not the character should sweep for collision geometry while walking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Sweep while nav walking")
	bool SecondWave_SweepWhileNavWalking = false;
public:
	// Movement component tick rate when random tick not used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Fixed movement tick")
	float SecondWave_OptimizatedMovementTick = 0.1f;

	// Movement component min tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Min movement random tick")
	float SecondWave_OptimizatedMovementTickMin = 0.05f;

	// Movement component max tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Max movement random tick")
	float SecondWave_OptimizatedMovementTickMax = 0.1f;

	// Use random tick for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Movement", DisplayName = "Use random movement tick")
	bool SecondWave_UseRandomOptimizationTickForMovement = true;
public:
	// Hide shadows from skeletal mesh on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Hide shadows")
	bool SecondWave_HideShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Groom", DisplayName = "Hide shadows")
	bool SecondWaveGroom_HideShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Groom", DisplayName = "Hide groom")
	bool SecondWaveGroom_Hide = true;

	// Disable skeletal meshes collision on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Disable mesh collision")
	bool SecondWave_DisableMeshCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Groom", DisplayName = "Disable mesh collision")
	bool SecondWaveGroom_DisableMeshCollision = true;

	// Whether or not the character should hide attached static meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Need hide static meshes")
	bool SecondWave_NeedHideStaticMeshes = true;

	// Use update rate optimizations for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Use URO")
	bool SecondWave_UseUpdateRateOptimizations = true;

	// Use per bone motion blur for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Use per bone motion blur")
	bool SecondWave_UsePerBoneMotionBlur = false;

	// Disable cloth simulation for skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Disable cloth simulation")
	bool SecondWave_DisableClothSimulation = true;

	// Disable morph target simulation for skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Disable morph target")
	bool SecondWave_DisableMorphTarget = true;

	// Skip kinematic update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Skip kinematic update when interpolating")
	bool SecondWave_SkipKinematicUpdateWhenInterpolating = true;

	// Skip bounds update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Skip bounds update when interpolating")
	bool SecondWave_SkipBoundsUpdateWhenInterpolating = true;

	// Allow rigid body anim node for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Allow rigid body anim node")
	bool SecondWave_AllowRigidBodyAnimNode = false;

	// Generate overlap events?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Generate overlap events")
	bool SecondWave_GenerateOverlapEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Groom", DisplayName = "Generate overlap events")
	bool SecondWaveGroom_GenerateOverlapEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Second wave|Skeletal meshes", DisplayName = "Visibility Based Anim Tick Option")
	EVisibilityBasedAnimTickOption SecondWave_VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
public:
	// Movement component need always check floor on this wave?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Always check floor")
	bool ThirdWave_AlwaysCheckFloor = false;

	// Enable physics interaction on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Enable physics interaction")
	bool ThirdWave_EnablePhysicsInteraction = false;

	// Max simulation time step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Max simulation time step")
	float ThirdWave_MaxSimulationTimeStep = 0.05f;

	// Max simulation iterations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Max simulation iterations")
	int32 ThirdWave_MaxSimulationIterations = 1;

	// Run physics with no controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Run physics with no controller")
	bool ThirdWave_RunPhysicsWithNoController = false;

	// Movement mode on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Movement mode")
	TEnumAsByte<EMovementMode> ThirdWave_MovementMode = EMovementMode::MOVE_NavWalking;

	// Whether or not the character should sweep for collision geometry while walking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Sweep while nav walking")
	bool ThirdWave_SweepWhileNavWalking = false;
public:
	// Movement component tick rate when random tick not used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Fixed movement tick")
	float ThirdWave_OptimizatedMovementTick = 0.2f;

	// Movement component min tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Min movement random tick")
	float ThirdWave_OptimizatedMovementTickMin = 0.1f;

	// Movement component max tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Max movement random tick")
	float ThirdWave_OptimizatedMovementTickMax = 0.2f;

	// Use random tick for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Movement", DisplayName = "Use random movement tick")
	bool ThirdWave_UseRandomOptimizationTickForMovement = true;
public:
	// Hide shadows from skeletal mesh on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Hide shadows")
	bool ThirdWave_HideShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Groom", DisplayName = "Hide shadows")
	bool ThirdWaveGroom_HideShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Groom", DisplayName = "Hide groom")
	bool ThirdWaveGroom_Hide = true;

	// Disable skeletal meshes collision on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Disable mesh collision")
	bool ThirdWave_DisableMeshCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Groom", DisplayName = "Disable mesh collision")
	bool ThirdWaveGroom_DisableMeshCollision = true;

	// Whether or not the character should hide attached static meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Need hide static meshes")
	bool ThirdWave_NeedHideStaticMeshes = true;

	// Use update rate optimizations for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Use URO")
	bool ThirdWave_UseUpdateRateOptimizations = true;

	// Use per bone motion blur for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Use per bone motion blur")
	bool ThirdWave_UsePerBoneMotionBlur = false;

	// Disable cloth simulation for skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Disable cloth simulation")
	bool ThirdWave_DisableClothSimulation = true;

	// Disable morph target simulation for skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Disable morph target")
	bool ThirdWave_DisableMorphTarget = true;

	// Skip kinematic update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Skip kinematic update when interpolating")
	bool ThirdWave_SkipKinematicUpdateWhenInterpolating = true;

	// Skip bounds update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Skip bounds update when interpolating")
	bool ThirdWave_SkipBoundsUpdateWhenInterpolating = true;

	// Allow rigid body anim node for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Allow rigid body anim node")
	bool ThirdWave_AllowRigidBodyAnimNode = false;

	// Generate overlap events?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Generate overlap events")
	bool ThirdWave_GenerateOverlapEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Groom", DisplayName = "Generate overlap events")
	bool ThirdWaveGroom_GenerateOverlapEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Third wave|Skeletal meshes", DisplayName = "Visibility Based Anim Tick Option")
	EVisibilityBasedAnimTickOption ThirdWave_VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
public:
	// Movement component need always check floor on this wave?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Always check floor")
	bool Invisible_AlwaysCheckFloor = false;

	// Enable physics interaction on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Enable physics interaction")
	bool Invisible_EnablePhysicsInteraction = false;

	// Max simulation time step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Max simulation time step")
	float Invisible_MaxSimulationTimeStep = 0.5f;

	// Max simulation iterations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Max simulation iterations")
	int32 Invisible_MaxSimulationIterations = 1;

	// Run physics with no controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Run physics with no controller")
	bool Invisible_RunPhysicsWithNoController = false;

	// Movement mode on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Movement mode")
	TEnumAsByte<EMovementMode> Invisible_MovementMode = EMovementMode::MOVE_NavWalking;

	// Whether or not the character should sweep for collision geometry while walking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Sweep while nav walking")
	bool Invisible_SweepWhileNavWalking = false;
public:
	// Movement component tick rate when random tick not used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Fixed movement tick")
	float Invisible_OptimizatedMovementTick = 1.f;

	// Movement component min tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Min movement random tick")
	float Invisible_OptimizatedMovementTickMin = 1.f;

	// Movement component max tick rate when random tick enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Max movement random tick")
	float Invisible_OptimizatedMovementTickMax = 1.5f;

	// Use random tick for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Movement", DisplayName = "Use random movement tick")
	bool Invisible_UseRandomOptimizationTickForMovement = true;
public:
	// Hide skeletal mesh when it not visible for player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Hide skeletal mesh")
	bool Invisible_HideSkeletalMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Groom", DisplayName = "Hide groom")
	bool InvisibleGroom_Hide = true;

	// Hide shadows from skeletal mesh on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Hide shadows")
	bool Invisible_HideShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Groom", DisplayName = "Hide shadows")
	bool InvisibleGroom_HideShadows = true;

	// Disable skeletal meshes collision on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Disable mesh collision")
	bool Invisible_DisableMeshCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Groom", DisplayName = "Disable mesh collision")
	bool InvisibleGroom_DisableMeshCollision = true;

	// Whether or not the character should hide attached static meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Need hide static meshes")
	bool Invisible_NeedHideStaticMeshes = true;

	// Use update rate optimizations for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Use URO")
	bool Invisible_UseUpdateRateOptimizations = true;

	// Use per bone motion blur for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Use per bone motion blur")
	bool Invisible_UsePerBoneMotionBlur = false;

	// Disable cloth simulation for skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Disable cloth simulation")
	bool Invisible_DisableClothSimulation = true;

	// Disable morph target simulation for skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Disable morph target")
	bool Invisible_DisableMorphTarget = true;

	// Skip kinematic update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Skip kinematic update when interpolating")
	bool Invisible_SkipKinematicUpdateWhenInterpolating = true;

	// Skip bounds update when interpolating for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Skip bounds update when interpolating")
	bool Invisible_SkipBoundsUpdateWhenInterpolating = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Visibility Based Anim Tick Option")
	EVisibilityBasedAnimTickOption Invisible_VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	// Allow rigid body anim node for skeletal meshes on this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Allow rigid body anim node")
	bool Invisible_AllowRigidBodyAnimNode = false;

	// Generate overlap events?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Skeletal meshes", DisplayName = "Generate overlap events")
	bool Invisible_GenerateOverlapEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Invisible|Groom", DisplayName = "Generate overlap events")
	bool InvisibleGroom_GenerateOverlapEvents = false;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Debug draw")
	bool NoOptimization_DrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Debug draw")
	bool FirstWave_DrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Debug draw")
	bool SecondWave_DrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Debug draw")
	bool ThirdWave_DrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization|Debug draw")
	bool Invisible_DrawDebug = false;
public:
	/** Called when component going to new optimization wave */
	UPROPERTY(BlueprintAssignable, Category = "Optimization")
	FOptimizationChangeWave  OnChangeWave;
protected:
	void OptimizeSkeletalMeshes(EOptimizationWave Wave);
	void OptimizeMovement(EOptimizationWave Wave) const;
	void OptimizeGrooms(EOptimizationWave Wave);
public:
	FSDefaultMovementSettings     MovementSettings;
private:
	mutable FCriticalSection      LockCS;
	int32                         OptimizationHandle = INDEX_NONE;
	EOptimizationWave             OptimizationWave = EOptimizationWave::NoOptimization;
	EOptimizationWave             PreviousWave = EOptimizationWave::NoOptimization;
	FThreadSafeBool               bIsNeedToBeOptimized = false;
	bool                          bIsForceOptimizationWaveEnabled = false;
	TArray<FSOptimizedComponent>  ComponentsForOptimization;
};