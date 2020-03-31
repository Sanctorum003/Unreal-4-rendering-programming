// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved. 

#include "RayBasicComponent.h"
#include "RenderingThread.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "MaterialShared.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "LocalVertexFactory.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Private/AssetTypeActions/AssetTypeActions_StaticMesh.h"
#include "Kismet/KismetSystemLibrary.h"

/* Vertex Buffer */
/**
 *	在4.17中
 *		Vertex Buffer需要从FVectorBuffer继承，然后自定义
 *  4.19中
 *		Vertex Buffer使用FStaticMeshVertexBuffers VertexBuffers;
 *		所以所有有关Vertex Buffer的操作都会有所改变，下方我会标识出来
 *			注意一点的是FStaticMeshVertexBuffers包含三个Buffer，所以以下很多操作要分别对这三个进行操作，具体可以点进去看
 *
 *  具体参考CableComponent
 */

/*
class FRayLineMeshVertexBuffer : public FVertexBuffer
{
public:

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(NumVerts * sizeof(FDynamicMeshVertex), BUF_Dynamic, CreateInfo);
	}

	int32 NumVerts;
};
*/



/** Index Buffer */
// FRayLineMeshIndexBuffer将定义在SecneProxy中用于接受来自逻辑线程的数据
// 4.17的Vertex Buffer跟Index Buffer类似
class FRayLineMeshIndexBuffer : public FIndexBuffer
{
public:

	//初始化此资源使用的RHI资源。 在进入资源和RHI都已初始化的状态时调用。仅由渲染线程调用。
	//这个用于根据NumIndices的大小，来创建一个缓存区保存索引数据
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		//IndexBufferRHI是FIndexBuffer的成员，那可以猜测FIndexBuffer同一时刻只能给一个Component组件用
		//IndexBufferRHI用于接受来自逻辑线程的Index数据,在渲染线程的场景代理中将index数据传给DrawPolicy
		// Index数据块 ：步长 / 总大小 / 缓冲类型 / 创建信息
		//BUF_Dynamic:The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed.
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), NumIndices * sizeof(int32), BUF_Dynamic, CreateInfo);
	}

	int32 NumIndices;
};

/* Vertex Factory */
/**
 *	在4.17中
 *		Vertex Factory需要从FLocalVertexFactory继承，然后自定义
 *  4.19中
 *		Vertex Factory直接使用FLocalVertexFactory VertexFactory;
 *		所以所有有关Vertex Factory的操作都会有所改变，下方我会标识出来
 *
 * 	具体参考CableComponent
 */

/*
class FCustomMeshVertexFactory : public FLocalVertexFactory
{
public:

	FCustomMeshVertexFactory()
	{}

	void Init(const FRayLineMeshVertexBuffer* VertexBuffer)
	{
		if (IsInRenderingThread())
		{
			// Initialize the vertex factory's stream components.
			FDataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Position, VET_Float3);
			NewData.TextureCoordinates.Add(
				FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FDynamicMeshVertex, TextureCoordinate), sizeof(FDynamicMeshVertex), VET_Float2)
			);
			NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentX, VET_PackedNormal);
			NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentZ, VET_PackedNormal);
			SetData(NewData);
		}
		else
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				InitCableVertexFactory,
				FCustomMeshVertexFactory*, VertexFactory, this,
				const FRayLineMeshVertexBuffer*, VertexBuffer, VertexBuffer,
				{
					// Initialize the vertex factory's stream components.
					FDataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,Position,VET_Float3);
			NewData.TextureCoordinates.Add(
				FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FDynamicMeshVertex,TextureCoordinate),sizeof(FDynamicMeshVertex),VET_Float2)
			);
			NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentX,VET_PackedNormal);
			NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentZ,VET_PackedNormal);
			VertexFactory->SetData(NewData);
				});
		}
	}
};
*/

//FRayLineDynamicData结构体，把它视作一个数据包，方便我们从逻辑层把数据们打包一起发送到渲染线程
//在SendRenderDynamicData_Concurrent中将逻辑线程中的数据打包发送到渲染线程
struct FRayLineDynamicData
{
	TArray<FVector> HitPointsPosition;
	//You can also define some other data to send
};

/** Scene proxy */
// * Encapsulates the data which is mirrored to render a UPrimitiveComponent parallel to the game thread.
// * This is intended to be subclassed to support different primitive types.
// 上面两行是FPrimitiveSceneProxy的描述，创建场景代理的时候需要继承FPrimitiveSceneProxy并自定义
// 渲染线程部分 
class FRayLineMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	/** Return a type (or subtype) specific hash for sorting purposes */
	// 4.17中不用重写，4.19中需要。完全照抄即可，意义不明
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	//Constructor Func
	FRayLineMeshSceneProxy(URayBasicComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, Material(NULL)
		// 4.19 VertexFactory需要用特征等级初始化.
		, VertexFactory(GetScene().GetFeatureLevel(),"FRayLineScenceProxy") 
		, DynamicData(NULL)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, MaxHitTimes(Component->MaxHitTimes)
		, RayWidth(Component->RayWidth)
	{

		//VertexBuffer.NumVerts = GetRequiredVertexCount(); 4.17

		//4.19
		//InitWithDummyData()函数将VertexFactory与VertexBuffers绑定，这一步会在VertexBuffers保存NumVertices和Stride
		VertexBuffers.InitWithDummyData(&VertexFactory, GetRequiredVertexCount());

		
		IndexBuffer.NumIndices = GetRequiredIndexCount();

		// 4.17 Init vertex factory
		// VertexFactory.Init(&VertexBuffer);

		// 4.17 Enqueue initialization of render resource
		//BeginInitResource(&VertexBuffer);

		// 调用过程 BeginInitResource(&Resource) -> Resource->InitResource() -> InitDynamicRHI(); && InitRHI();
		BeginInitResource(&IndexBuffer);
		//BeginInitResource(&VertexFactory);

		// Grab material
		Material = Component->GetMaterial(0);
		if (Material == NULL)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

	}

	virtual ~FRayLineMeshSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		
		//VertexBuffer.ReleaseResource(); 4.17
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();

		if (DynamicData != NULL)
			delete DynamicData;
	}

	int32 GetRequiredVertexCount() const
	{
		return (MaxHitTimes + 2) * 2;
	}

	int32 GetRequiredIndexCount() const
	{
		return (MaxHitTimes + 1) * 6;
	}

	//根据NewDynamicData->HitPointsPosition中的点，构建Mesh的顶点和Index顺序
	void BuildRayLineMesh(const TArray<FVector>& Points,TArray<FDynamicMeshVertex>& Vertices,TArray<int32>& Indices)
	{
		
		/*for (int32 i = 0; i < Points.Num(); i++)
		{
			FDynamicMeshVertex newvert0;
			newvert0.Position = Points[i] + FVector(-100, 100, 0);
			FDynamicMeshVertex newvert1;
			newvert1.Position = Points[i] + FVector(100, 100, 0);
			FDynamicMeshVertex newvert2;
			newvert2.Position = Points[i] + FVector(-100, -100, 0);
			FDynamicMeshVertex newvert3;
			newvert3.Position = Points[i] + FVector(100, -100, 0);

			Vertices.Add(newvert0);
			Vertices.Add(newvert1);
			Vertices.Add(newvert2);
			Vertices.Add(newvert3);

			Indices.Add(4 * i);
			Indices.Add(4 * i + 1);
			Indices.Add(4 * i + 2);
			Indices.Add(4 * i + 1);
			Indices.Add(4 * i + 3);
			Indices.Add(4 * i + 2);
		}*/

		for (int32 i = 0; i < Points.Num(); i++)
		{
			FDynamicMeshVertex newvert0;
			newvert0.Position = Points[i] + FVector(0, RayWidth, 0);
			newvert0.TextureCoordinate[0] = FVector2D(i / 10, 1);

			FDynamicMeshVertex newvert1;
			newvert1.Position = Points[i] + FVector(0, -RayWidth, 0);
			newvert1.TextureCoordinate[0] = FVector2D(i / 10, 0);

			Vertices.Add(newvert0);
			Vertices.Add(newvert1);
		}
		for (int32 i = 0; i < Points.Num() - 1; i++)
		{
			Indices.Add(2 * i);
			Indices.Add(2 * i + 2);
			Indices.Add(2 * i + 1);
			Indices.Add(2 * i + 2);
			Indices.Add(2 * i + 3);
			Indices.Add(2 * i + 1);
		}
	}

	//因为我们这个是动态的模型，所以需要有一个函数负责在渲染线程接收逻辑层发送过来的数据
	//在SendRenderDynamicData_Concurrent中调用，这个函数在 逻辑线程中
	/** Called on render thread to assign new dynamic data */
	void SetDynamicData_RenderThread(FRayLineDynamicData* NewDynamicData)
	{
		check(IsInRenderingThread());

		// Free existing data if present
		if (DynamicData)
		{
			delete DynamicData;
			DynamicData = NULL;
		}
		DynamicData = NewDynamicData;

		//构建RayLine Mesh的顶点和index
		TArray<FDynamicMeshVertex> Vertices;
		TArray<int32> Indices;
		
		BuildRayLineMesh(NewDynamicData->HitPointsPosition,Vertices,Indices);

		//确认顶点和index的数量
		check(Vertices.Num() == GetRequiredVertexCount());
		check(Indices.Num() == GetRequiredIndexCount());

		//将Vertices写入VertexBuffers中
		for (int i = 0; i < Vertices.Num(); i++)
		{
			const FDynamicMeshVertex& Vertex = Vertices[i];

			//将定点数据保存在VertexBuffers的PositionVertexBuffer的Data数据域中
			VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
			VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX, Vertex.GetTangentY(), Vertex.TangentZ);
			VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
			//将定点数据保存在VertexBuffers的PositionVertexBuffer的Data数据域中
			VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
		}

		//将VertexBuffers和IndexBufferData的数据通关内存复制的方法写入RHI中对应的位置，提供给系统API使用（win下就是提供给dx使用的数据）
		{
			auto& VertexBuffer = VertexBuffers.PositionVertexBuffer;
			//VertexBuffer.VertexBufferRHI 猜测：该是用来与dx交互的VertexBuffer
			//猜测：各种XXXRHI是用来抽象各种不同平台的,最终要DrawCall的使用必须要使用XXXRHI，这样这些XXXRHI会自动匹配不同的平台，调用相关图形库(dx,opengl等)
			//有关XXXRHI可以看一下这个：http://www.manew.com/thread-100777-1-1.html
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			//Dest,Src,count
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.ColorVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
			RHIUnlockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
			RHIUnlockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
		}

		void* IndexBufferData = RHILockIndexBuffer(IndexBuffer.IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(IndexBufferData, &Indices[0], Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBuffer.IndexBufferRHI);
	}


	//这个函数负责把模型数据加入到绘制队列Collector中
	/**
	* Gathers the primitive's dynamic mesh elements.  This will only be called if GetViewRelevance declares dynamic relevance. //该操作在GetViewRelevance中
	* This is called from the rendering thread for each set of views that might be rendered.
	* Game thread state like UObjects must have their properties mirrored on the proxy to avoid race conditions.  The rendering thread must not dereference UObjects.
	* The gathered mesh elements will be used multiple times, any memory referenced must last as long as the Collector (eg no stack memory should be referenced).
	* This function should not modify the proxy but simply collect a description of things to render.  Updates to the proxy need to be pushed from game thread or external events.
	*
	* @param Views - the array of views to consider.  These may not exist in the ViewFamily.
	* @param ViewFamily - the view family, for convenience
	* @param VisibilityMap - a bit representing this proxy's visibility in the Views array
	* @param Collector - gathers the mesh elements to be rendered and provides mechanisms for temporary allocations
	*/
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		//用于定义一个cpu时间统计的方法，详细可参考https://blog.csdn.net/leonwei/article/details/100099096
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRayLineMeshSceneProxy_GetDynamicMeshElements);

		//是否是线框模式
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		//创建相应的相框实例
		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
		);

		/** Add a material render proxy that will be cleaned up automatically */
		/** Material proxies that will be deleted at the end of the frame. */
		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		FMaterialRenderProxy* MaterialProxy = NULL;
		//判断绘制的是否是线框
		if (bWireframe)
		{
			MaterialProxy = WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy(IsSelected());
		}

		//将模型数据加入到Collector
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			//因为需要绘制所有视图，所有遍历所有存在的视图
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				// Draw the mesh.
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;
				BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = GetRequiredIndexCount() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = GetRequiredVertexCount(); //这里为什么是GetRequiredVertexCount()，不是GetRequiredVertexCount()-1？
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList; //注意这个使用的是TriangleList，所以没三个顶点组成一个面。对应的NumPrimitives要设置对
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}

	/**
	* Determines the relevance of this primitive's elements to the given view.
	* Called in the rendering thread.
	* @param View - The view to determine relevance for.
	* @return The relevance of the primitive's elements to the view.
	*/
	//确定绘制时需要哪些视图（可见），设置使用哪些渲染模型
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	/**
	* @return true if the proxy can be culled when occluded by other primitives
	*/
	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	/** Every derived class should override these functions */
	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }
	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

private:

	UMaterialInterface* Material;
	// FRayLineMeshVertexBuffer VertexBuffer; 4.17版本
	FStaticMeshVertexBuffers VertexBuffers; //4.19版本
	FRayLineMeshIndexBuffer IndexBuffer;
	// FCustomMeshVertexFactory VertexFactory; 4.17版本
	
	//一个顶点工厂，仅将显式顶点属性从本地空间转换为世界空间
	FLocalVertexFactory VertexFactory;

	FRayLineDynamicData* DynamicData;

	FMaterialRelevance MaterialRelevance;

protected:
	int32 MaxHitTimes;

	float RayWidth;
};

//////////////////////////////////////////////////////////////////////////
///
//URayBasicComponent 相当于是逻辑线程

URayBasicComponent::URayBasicComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//一定要把三个开关打开，这样才能调用ComponentTick这些函数来更新我们的组件。
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	DebugSec = 200.0f;
	
	MaxHitTimes = 10;
	RayDisappearDistance = 100000.0f;
	RayWidth = 100.0f;
}

//用于注册组件
void URayBasicComponent::OnRegister()
{
	Super::OnRegister();

	RayLineHitPoints.Reset();
	RayLineHitPoints.AddUninitialized(MaxHitTimes + 2);

	//这个函数会开启一个开关，让引擎每帧更新所有组件渲染状态的时候，会更新到我们的组件。
	MarkRenderDynamicDataDirty();
}

void URayBasicComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector RayDirection = this->RayDirection;
	FVector RayOrigin = GetOwner()->GetActorLocation();

	FHitResult OutHit;
	RayLineHitPoints[0].HitPosition = GetOwner()->GetTransform().Inverse().TransformPosition(RayOrigin);

	for (int32 i = 0; i < MaxHitTimes; i++)
	{
		bool bHit = RayTracingHit(RayOrigin, RayDirection, 2000.0f, OutHit);
		if (!bHit)
		{
			FVector LastPointLoc = RayLineHitPoints[i + 1 - 1].HitPosition;
			RayLineHitPoints[i + 1].HitPosition = LastPointLoc;
			continue;
		}
		FVector HitPointLoc = OutHit.Location + OutHit.ImpactNormal * 0.1f;
		FVector HitPointLocTransformed = GetOwner()->GetTransform().Inverse().TransformPosition(HitPointLoc);
		RayLineHitPoints[i+1].HitPosition = HitPointLocTransformed;

		RayOrigin = HitPointLoc;
		RayDirection = FMath::GetReflectionVector(RayDirection, OutHit.ImpactNormal);
	}

	RayLineHitPoints[MaxHitTimes + 1].HitPosition = RayLineHitPoints[MaxHitTimes].HitPosition + RayDirection * RayDisappearDistance;

	//Need to send new data render thread
	//这个函数会开启一个开关，让引擎每帧更新所有组件渲染状态的时候，会更新到我们的组件。
	//会自动调用CreateRenderState_Concurrent()，然后调用SendRenderDynamicData_Concurrent()
	MarkRenderDynamicDataDirty();

	//call this because bounds have changed
	//有使用到CalcBounds（）
	UpdateComponentToWorld();
}

bool URayBasicComponent::RayTracingHit(FVector RayOrigin, FVector RayDirection, float RayMarhingLength, FHitResult& OutHitResoult)
{
	const TArray<AActor*>IgActor;
	FVector startpos = RayOrigin;
	FVector endpos = RayOrigin + RayDirection * RayMarhingLength;
	return UKismetSystemLibrary::LineTraceSingle(
		GetOwner(),
		startpos,
		endpos,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		IgActor,
		EDrawDebugTrace::Type::ForOneFrame,
		OutHitResoult,
		true,
		FLinearColor::Blue,
		FLinearColor::Red,
		1.0f);
}

void URayBasicComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	SendRenderDynamicData_Concurrent();
}



void URayBasicComponent::SendRenderDynamicData_Concurrent()
{
	if (SceneProxy)
	{
		FRayLineDynamicData* NewDynamicData = new FRayLineDynamicData;
		NewDynamicData->HitPointsPosition.AddUninitialized(RayLineHitPoints.Num());
		for (int32 i = 0; i < RayLineHitPoints.Num(); i++)
		{
			NewDynamicData->HitPointsPosition[i] = RayLineHitPoints[i].HitPosition;
		}

		// Enqueue command to send to render thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendRayBasicComponentDynamicData,
			FRayLineMeshSceneProxy*, CableSceneProxy, (FRayLineMeshSceneProxy*)SceneProxy,
			FRayLineDynamicData*, NewDynamicData, NewDynamicData,
			{
				CableSceneProxy->SetDynamicData_RenderThread(NewDynamicData);
			});
	}
}

//创建场景代理，这样逻辑线程中就会把数据传入到渲染线程中的这个场景代理中
FPrimitiveSceneProxy* URayBasicComponent::CreateSceneProxy()
{
	return new FRayLineMeshSceneProxy(this);
}

//获取材质
int32 URayBasicComponent::GetNumMaterials() const
{
	return 1;
}

//构建包围盒的操作了
FBoxSphereBounds URayBasicComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}