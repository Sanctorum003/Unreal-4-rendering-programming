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
 *	��4.17��
 *		Vertex Buffer��Ҫ��FVectorBuffer�̳У�Ȼ���Զ���
 *  4.19��
 *		Vertex Bufferʹ��FStaticMeshVertexBuffers VertexBuffers;
 *		���������й�Vertex Buffer�Ĳ������������ı䣬�·��һ��ʶ����
 *			ע��һ�����FStaticMeshVertexBuffers��������Buffer���������ºܶ����Ҫ�ֱ�����������в�����������Ե��ȥ��
 *
 *  ����ο�CableComponent
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
// FRayLineMeshIndexBuffer��������SecneProxy�����ڽ��������߼��̵߳�����
// 4.17��Vertex Buffer��Index Buffer����
class FRayLineMeshIndexBuffer : public FIndexBuffer
{
public:

	//��ʼ������Դʹ�õ�RHI��Դ�� �ڽ�����Դ��RHI���ѳ�ʼ����״̬ʱ���á�������Ⱦ�̵߳��á�
	//������ڸ���NumIndices�Ĵ�С��������һ��������������������
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		//IndexBufferRHI��FIndexBuffer�ĳ�Ա���ǿ��Բ²�FIndexBufferͬһʱ��ֻ�ܸ�һ��Component�����
		//IndexBufferRHI���ڽ��������߼��̵߳�Index����,����Ⱦ�̵߳ĳ��������н�index���ݴ���DrawPolicy
		// Index���ݿ� ������ / �ܴ�С / �������� / ������Ϣ
		//BUF_Dynamic:The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed.
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), NumIndices * sizeof(int32), BUF_Dynamic, CreateInfo);
	}

	int32 NumIndices;
};

/* Vertex Factory */
/**
 *	��4.17��
 *		Vertex Factory��Ҫ��FLocalVertexFactory�̳У�Ȼ���Զ���
 *  4.19��
 *		Vertex Factoryֱ��ʹ��FLocalVertexFactory VertexFactory;
 *		���������й�Vertex Factory�Ĳ������������ı䣬�·��һ��ʶ����
 *
 * 	����ο�CableComponent
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

//FRayLineDynamicData�ṹ�壬��������һ�����ݰ����������Ǵ��߼���������Ǵ��һ���͵���Ⱦ�߳�
//��SendRenderDynamicData_Concurrent�н��߼��߳��е����ݴ�����͵���Ⱦ�߳�
struct FRayLineDynamicData
{
	TArray<FVector> HitPointsPosition;
	//You can also define some other data to send
};

/** Scene proxy */
// * Encapsulates the data which is mirrored to render a UPrimitiveComponent parallel to the game thread.
// * This is intended to be subclassed to support different primitive types.
// ����������FPrimitiveSceneProxy���������������������ʱ����Ҫ�̳�FPrimitiveSceneProxy���Զ���
// ��Ⱦ�̲߳��� 
class FRayLineMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	/** Return a type (or subtype) specific hash for sorting purposes */
	// 4.17�в�����д��4.19����Ҫ����ȫ�ճ����ɣ����岻��
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	//Constructor Func
	FRayLineMeshSceneProxy(URayBasicComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, Material(NULL)
		// 4.19 VertexFactory��Ҫ�������ȼ���ʼ��.
		, VertexFactory(GetScene().GetFeatureLevel(),"FRayLineScenceProxy") 
		, DynamicData(NULL)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, MaxHitTimes(Component->MaxHitTimes)
		, RayWidth(Component->RayWidth)
	{

		//VertexBuffer.NumVerts = GetRequiredVertexCount(); 4.17

		//4.19
		//InitWithDummyData()������VertexFactory��VertexBuffers�󶨣���һ������VertexBuffers����NumVertices��Stride
		VertexBuffers.InitWithDummyData(&VertexFactory, GetRequiredVertexCount());

		
		IndexBuffer.NumIndices = GetRequiredIndexCount();

		// 4.17 Init vertex factory
		// VertexFactory.Init(&VertexBuffer);

		// 4.17 Enqueue initialization of render resource
		//BeginInitResource(&VertexBuffer);

		// ���ù��� BeginInitResource(&Resource) -> Resource->InitResource() -> InitDynamicRHI(); && InitRHI();
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

	//����NewDynamicData->HitPointsPosition�еĵ㣬����Mesh�Ķ����Index˳��
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

	//��Ϊ��������Ƕ�̬��ģ�ͣ�������Ҫ��һ��������������Ⱦ�߳̽����߼��㷢�͹���������
	//��SendRenderDynamicData_Concurrent�е��ã���������� �߼��߳���
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

		//����RayLine Mesh�Ķ����index
		TArray<FDynamicMeshVertex> Vertices;
		TArray<int32> Indices;
		
		BuildRayLineMesh(NewDynamicData->HitPointsPosition,Vertices,Indices);

		//ȷ�϶����index������
		check(Vertices.Num() == GetRequiredVertexCount());
		check(Indices.Num() == GetRequiredIndexCount());

		//��Verticesд��VertexBuffers��
		for (int i = 0; i < Vertices.Num(); i++)
		{
			const FDynamicMeshVertex& Vertex = Vertices[i];

			//���������ݱ�����VertexBuffers��PositionVertexBuffer��Data��������
			VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
			VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX, Vertex.GetTangentY(), Vertex.TangentZ);
			VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
			//���������ݱ�����VertexBuffers��PositionVertexBuffer��Data��������
			VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
		}

		//��VertexBuffers��IndexBufferData������ͨ���ڴ渴�Ƶķ���д��RHI�ж�Ӧ��λ�ã��ṩ��ϵͳAPIʹ�ã�win�¾����ṩ��dxʹ�õ����ݣ�
		{
			auto& VertexBuffer = VertexBuffers.PositionVertexBuffer;
			//VertexBuffer.VertexBufferRHI �²⣺����������dx������VertexBuffer
			//�²⣺����XXXRHI������������ֲ�ͬƽ̨��,����ҪDrawCall��ʹ�ñ���Ҫʹ��XXXRHI��������ЩXXXRHI���Զ�ƥ�䲻ͬ��ƽ̨���������ͼ�ο�(dx,opengl��)
			//�й�XXXRHI���Կ�һ�������http://www.manew.com/thread-100777-1-1.html
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


	//������������ģ�����ݼ��뵽���ƶ���Collector��
	/**
	* Gathers the primitive's dynamic mesh elements.  This will only be called if GetViewRelevance declares dynamic relevance. //�ò�����GetViewRelevance��
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
		//���ڶ���һ��cpuʱ��ͳ�Ƶķ�������ϸ�ɲο�https://blog.csdn.net/leonwei/article/details/100099096
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRayLineMeshSceneProxy_GetDynamicMeshElements);

		//�Ƿ����߿�ģʽ
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		//������Ӧ�����ʵ��
		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
		);

		/** Add a material render proxy that will be cleaned up automatically */
		/** Material proxies that will be deleted at the end of the frame. */
		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		FMaterialRenderProxy* MaterialProxy = NULL;
		//�жϻ��Ƶ��Ƿ����߿�
		if (bWireframe)
		{
			MaterialProxy = WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy(IsSelected());
		}

		//��ģ�����ݼ��뵽Collector
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			//��Ϊ��Ҫ����������ͼ�����б������д��ڵ���ͼ
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
				BatchElement.MaxVertexIndex = GetRequiredVertexCount(); //����Ϊʲô��GetRequiredVertexCount()������GetRequiredVertexCount()-1��
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList; //ע�����ʹ�õ���TriangleList������û�����������һ���档��Ӧ��NumPrimitivesҪ���ö�
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
	//ȷ������ʱ��Ҫ��Щ��ͼ���ɼ���������ʹ����Щ��Ⱦģ��
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
	// FRayLineMeshVertexBuffer VertexBuffer; 4.17�汾
	FStaticMeshVertexBuffers VertexBuffers; //4.19�汾
	FRayLineMeshIndexBuffer IndexBuffer;
	// FCustomMeshVertexFactory VertexFactory; 4.17�汾
	
	//һ�����㹤����������ʽ�������Դӱ��ؿռ�ת��Ϊ����ռ�
	FLocalVertexFactory VertexFactory;

	FRayLineDynamicData* DynamicData;

	FMaterialRelevance MaterialRelevance;

protected:
	int32 MaxHitTimes;

	float RayWidth;
};

//////////////////////////////////////////////////////////////////////////
///
//URayBasicComponent �൱�����߼��߳�

URayBasicComponent::URayBasicComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//һ��Ҫ���������ش򿪣��������ܵ���ComponentTick��Щ�������������ǵ������
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	DebugSec = 200.0f;
	
	MaxHitTimes = 10;
	RayDisappearDistance = 100000.0f;
	RayWidth = 100.0f;
}

//����ע�����
void URayBasicComponent::OnRegister()
{
	Super::OnRegister();

	RayLineHitPoints.Reset();
	RayLineHitPoints.AddUninitialized(MaxHitTimes + 2);

	//��������Ὺ��һ�����أ�������ÿ֡�������������Ⱦ״̬��ʱ�򣬻���µ����ǵ������
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
	//��������Ὺ��һ�����أ�������ÿ֡�������������Ⱦ״̬��ʱ�򣬻���µ����ǵ������
	//���Զ�����CreateRenderState_Concurrent()��Ȼ�����SendRenderDynamicData_Concurrent()
	MarkRenderDynamicDataDirty();

	//call this because bounds have changed
	//��ʹ�õ�CalcBounds����
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

//�����������������߼��߳��оͻ�����ݴ��뵽��Ⱦ�߳��е��������������
FPrimitiveSceneProxy* URayBasicComponent::CreateSceneProxy()
{
	return new FRayLineMeshSceneProxy(this);
}

//��ȡ����
int32 URayBasicComponent::GetNumMaterials() const
{
	return 1;
}

//������Χ�еĲ�����
FBoxSphereBounds URayBasicComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}