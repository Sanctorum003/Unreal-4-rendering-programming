// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "Components/MeshComponent.h"
#include "RayBasicComponent.generated.h"

//����ǰ����������FPrimitiveSceneProxy����࣬������ǳ�������������Ⱦ�̺߳��߼��̵߳Ľ��������Ὣ���㻺�壬�������������ύ��
class FPrimitiveSceneProxy;

// ����һ����������Ļ������ݽṹ
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
	//GENERATED_UCLASS_BODY() ��������Ĭ�Ϲ��캯���Լ��������
	//GENERATED_BODY() ֻ����������ã�Ĭ�Ϲ��캯�����Լ�д

	GENERATED_BODY()	

public:
	URayBasicComponent(const FObjectInitializer & ObjectInitializer);
	
	//UPROPERTY��,��¶����ͼ����������������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineComponent)
		float DebugSec;
	
	//~ Begin UPrimitiveComponent Interface.
	//�ᴴ����������
	//	������������þ��Ǹ�������Ⱦ�̶߳˰��߼��߳���ߵ�����ѹ����Ⱦ����
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	

	//~ Begin UActorComponent Interface.
	//����������������ע�ᣬ�������������ʱ����á�
	virtual void OnRegister() override;
	//���������ÿ֡�����á�
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//���߼��̵߳Ķ������ݷ��͵���Ⱦ�߳�
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void CreateRenderState_Concurrent() override;
	//~ End UActorComponent Interface.

	TArray<FRayLineHitPointDesc> RayLineHitPoints;

	friend class FRayLineMeshSceneProxy;
};