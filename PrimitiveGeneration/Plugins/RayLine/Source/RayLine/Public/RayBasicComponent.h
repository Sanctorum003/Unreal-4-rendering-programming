// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "Components/MeshComponent.h"
#include "RayBasicComponent.generated.h"

//这里前置先声明了FPrimitiveSceneProxy这个类，这个类是场景代理，负责渲染线程和逻辑线程的交互，它会将顶点缓冲，索引缓冲数据提交。
class FPrimitiveSceneProxy;

// 创建一个射线组件的基本数据结构
USTRUCT(BlueprintType)
struct FRayLineHitPointDesc
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineHitPoint)
		FVector HitPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineHitPoint)
		FVector HitNextDir;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineHitPoint)
		int32 HitPointIndex;
};

/** Component that allows you to specify custom triangle mesh geometry */
UCLASS(hidecategories = (Object, LOD, Physics, Collision, "Components|Collision"), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class URayBasicComponent : public UMeshComponent
{
	//GENERATED_UCLASS_BODY() 用于生成默认构造函数以及相关配置
	//GENERATED_BODY() 只生成相关配置，默认构造函数需自己写

	GENERATED_BODY()	

public:
	URayBasicComponent(const FObjectInitializer & ObjectInitializer);
	
	//UPROPERTY宏,暴露给蓝图并纳入垃圾回收中
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineComponent)
		float DebugSec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineComponent)
		FVector RayDirection;

	UPROPERTY(EditAnywhere, Category = RayConfig)
		int32 MaxHitTimes;

	UPROPERTY(EditAnywhere, Category = RayConfig)
		float RayDisappearDistance;

	UPROPERTY(EditAnywhere, Category = RayConfig)
		float RayWidth;
	
	//~ Begin UPrimitiveComponent Interface.
	//会创建场景代理
	//	场景代理的作用就是负责在渲染线程端把逻辑线程这边的数据压入渲染管线
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	

	//~ Begin UActorComponent Interface.
	//这个函数负责组件的注册，会在组件构建的时候调用。
	virtual void OnRegister() override;
	//这个函数会每帧都调用。
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//将逻辑线程的顶点数据发送到渲染线程
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void CreateRenderState_Concurrent() override;
	//~ End UActorComponent Interface.

	TArray<FRayLineHitPointDesc> RayLineHitPoints;

	bool RayTracingHit(FVector RayOrigin, FVector RayDirection, float RayMarhingLength, FHitResult& OutHitResoult);
	
	friend class FRayLineMeshSceneProxy;
};