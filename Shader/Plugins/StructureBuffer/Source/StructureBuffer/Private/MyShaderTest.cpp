// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.  

#include "MyShaderTest.h"  

#include "Classes/Engine/TextureRenderTarget2D.h"  
#include "Classes/Engine/World.h"  
#include "Public/GlobalShader.h"  
#include "Public/PipelineStateCache.h"  
#include "Public/RHIStaticStates.h"  
#include "Public/SceneUtils.h"  
#include "Public/SceneInterface.h"  
#include "Public/ShaderParameterUtils.h"  
#include "Public/Logging/MessageLog.h"  
#include "Public/Internationalization/Internationalization.h"  
#include "Public/StaticBoundShaderState.h"
#include "HAL/FileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "RHICommandList.h"
#include "UniformBuffer.h"
#include "Engine/Texture2D.h"
#include "FileHelper.h"
#include "DynamicRHIResourceArray.h"

#define LOCTEXT_NAMESPACE "TestShader"

BEGIN_UNIFORM_BUFFER_STRUCT(FMyUniformStructData, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorOne)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorTwo)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorThree)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorFour)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ColorIndex)
END_UNIFORM_BUFFER_STRUCT(FMyUniformStructData)
//FMyUniformStructData用于C++文件，FMyUniform用于HLSL文件
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FMyUniformStructData, TEXT("FMyUniform"));

//typedef TUniformBufferRef<MyStructData> MyStructDataRef;  

UTestShaderBlueprintLibrary::UTestShaderBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

class FMyShaderTest : public FGlobalShader
{
public:

	FMyShaderTest() {}

	FMyShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
		TestTextureVal.Bind(Initializer.ParameterMap, TEXT("MyTexture"));
		TestTextureSampler.Bind(Initializer.ParameterMap, TEXT("MyTextureSampler"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);  
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
	}

	//
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const FLinearColor &MyColor,
		FTextureRHIParamRef &MyTexture,
		FMyShaderStructData &ShaderStructData
	)
	{
		SetShaderValue(RHICmdList, GetPixelShader(), SimpleColorVal, MyColor);
		SetTextureParameter(
			RHICmdList, 
			GetPixelShader(),
			TestTextureVal, 
			TestTextureSampler,
			TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(),
			MyTexture);

		FMyUniformStructData UniformData;
		UniformData.ColorOne = ShaderStructData.ColorOne;
		UniformData.ColorTwo = ShaderStructData.ColorTwo;
		UniformData.ColorThree = ShaderStructData.ColorThree;
		UniformData.ColorFour = ShaderStructData.ColorFour;
		UniformData.ColorIndex = ShaderStructData.ColorIndex;

		SetUniformBufferParameterImmediate(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FMyUniformStructData>(), UniformData);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SimpleColorVal << TestTextureVal;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderParameter SimpleColorVal;
	FShaderResourceParameter TestTextureVal;
	FShaderResourceParameter TestTextureSampler;

};

class FShaderTestVS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestVS, Global);

public:
	FShaderTestVS() {}

	FShaderTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};


class FShaderTestPS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestPS, Global);

public:
	FShaderTestPS() {}

	FShaderTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};

IMPLEMENT_SHADER_TYPE(, FShaderTestVS, TEXT("/Plugin/StructureBuffer/Private/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FShaderTestPS, TEXT("/Plugin/StructureBuffer/Private/MyShader.usf"), TEXT("MainPS"), SF_Pixel)

struct FMyTextureVertex
{
	FVector4 Position;
	FVector2D UV;
};

class FMyTextureVertexDeclaration:public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FMyTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyTextureVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyTextureVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI->Release();
	}
};

static void DrawTestShaderRenderTarget_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* OutputRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel,
	FName TextureRenderTargetName,
	FLinearColor MyColor,
	FTextureRHIParamRef MyTexture,
	FMyShaderStructData ShaderStructData
)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS  
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else  
	SCOPED_DRAW_EVENT(RHICmdList, DrawUVDisplacementToRenderTarget_RenderThread);
#endif  

	//设置渲染目标  
	SetRenderTarget(
		RHICmdList,
		OutputRenderTargetResource->GetRenderTargetTexture(),
		FTextureRHIRef(),
		ESimpleRenderTargetMode::EUninitializedColorAndDepth,
		FExclusiveDepthStencil::DepthNop_StencilNop
	);

	//设置视口  
	//FIntPoint DrawTargetResolution(OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY());  
	//RHICmdList.SetViewport(0, 0, 0.0f, DrawTargetResolution.X, DrawTargetResolution.Y, 1.0f);  

	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FShaderTestVS> VertexShader(GlobalShaderMap);
	TShaderMapRef<FShaderTestPS> PixelShader(GlobalShaderMap);

	FMyTextureVertexDeclaration VertexDec;
	VertexDec.InitRHI();

	// Set the graphic pipeline state.  
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDec.VertexDeclarationRHI;
	//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	//RHICmdList.SetViewport(0, 0, 0.0f, DrawTargetResolution.X, DrawTargetResolution.Y, 1.0f);  
	PixelShader->SetParameters(RHICmdList, MyColor,MyTexture,ShaderStructData);

	// Draw grid.  
	//uint32 PrimitiveCount = 2;  
	//RHICmdList.DrawPrimitive(PT_TriangleList, 0, PrimitiveCount, 1);  
	FMyTextureVertex Vertices[4];
	Vertices[0].Position.Set(-1.0f, 1.0f, 0, 1.0f);
	Vertices[1].Position.Set(1.0f, 1.0f, 0, 1.0f);
	Vertices[2].Position.Set(-1.0f, -1.0f, 0, 1.0f);
	Vertices[3].Position.Set(1.0f, -1.0f, 0, 1.0f);
	Vertices[0].UV = FVector2D(0.0f, 1.0f);
	Vertices[1].UV = FVector2D(1.0f, 1.0f);
	Vertices[2].UV = FVector2D(0.0f, 0.0f);
	Vertices[3].UV = FVector2D(1.0f, 0.0f);
	static const uint16 Indices[6] =
	{
		0, 1, 2,
		2, 1, 3
	};
	//DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]));  
	DrawIndexedPrimitiveUP(
		RHICmdList,
		PT_TriangleList,
		0,
		ARRAY_COUNT(Vertices),
		2,
		Indices,
		sizeof(Indices[0]),
		Vertices,
		sizeof(Vertices[0])
	);

	// Resolve render target.  
	RHICmdList.CopyToResolveTarget(
		OutputRenderTargetResource->GetRenderTargetTexture(),
		OutputRenderTargetResource->TextureRHI,
		false, FResolveParams());
}

void UTestShaderBlueprintLibrary::DrawTestShaderRenderTarget(
	UTextureRenderTarget2D* OutputRenderTarget,
	AActor* Ac,
	FLinearColor MyColor,
	UTexture* MyTexture,
	FMyShaderStructData ShaderStructData
)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	FTextureRHIParamRef MyTextureRHI = MyTexture->TextureReference.TextureReferenceRHI;
	UWorld* World = Ac->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName,MyTextureRHI,ShaderStructData](FRHICommandListImmediate& RHICmdList)
	{
		DrawTestShaderRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, MyColor,MyTextureRHI,ShaderStructData);
	}
	);

}

static void TextureWriting_RenderThread(FRHICommandListImmediate& RHICmdList,ERHIFeatureLevel::Type FeatureLevel, UTexture2D* Texture)
{
	check(IsInRenderingThread());
	if (Texture == nullptr) return;

	//将bmp格式解压为颜色信息并存储在Bitmap中
	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
	const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("satori.bmp"));
	TArray<uint8> OutArray;
	const TArray<uint8>* Bitmap = nullptr;
	if (FFileHelper::LoadFileToArray(OutArray, *ScreenFileName))
	{
		if (imageWrapper.IsValid() &&
			imageWrapper->SetCompressed(OutArray.GetData(), OutArray.Num()))
		{
			imageWrapper->GetRaw(ERGBFormat::RGBA, 8, Bitmap);
		}
	}

	//从UTexture2D转换为FTexture2D
	FTextureReferenceRHIRef MyTextureRHI = Texture->TextureReference.TextureReferenceRHI;
	FRHITexture* TexRef = MyTextureRHI->GetTextureReference()->GetReferencedTexture();
	FRHITexture2D* TexRef2D = (FRHITexture2D*)TexRef;

	//To access our resource we do a custom read using lockrecrt
	uint32 LolStride = 0;
	//uint8* ColorInfo = Bitmap->GetData();
	const TArray<uint8> ColorInfo = *Bitmap;
	uint32 ColorInfoCnt = 0;
	char* TextureDataPtr = (char*)RHICmdList.LockTexture2D(TexRef2D, 0, EResourceLockMode::RLM_WriteOnly, LolStride, false);

	for (uint32 Row = 0; Row < TexRef2D->GetSizeY(); ++Row)
	{
		uint32* PixelPtr = (uint32*)TextureDataPtr;
		//since we are using our custom UINT format,we need to unpack it here to access the actual colors
		for (uint32 Col = 0; Col < TexRef2D->GetSizeX(); ++Col)
		{
			uint8 r = ColorInfo[ColorInfoCnt];
			ColorInfoCnt++;
			uint8 g = ColorInfo[ColorInfoCnt];
			ColorInfoCnt++;
			uint8 b = ColorInfo[ColorInfoCnt];
			ColorInfoCnt++;
			uint8 a = ColorInfo[ColorInfoCnt];
			ColorInfoCnt++;
			*PixelPtr = r | (g << 8) | (b << 16) | (a << 24);
			PixelPtr++;
		}
		//move to next row;
		TextureDataPtr += LolStride;
	}

	RHICmdList.UnlockTexture2D(TexRef2D, 0, false);
}

void UTestShaderBlueprintLibrary::TextureWriting(UTexture2D* TextureToBeWrite, AActor* selfref)
{
	//check(IsInGameThread());

	//if (selfref == nullptr && TextureToBeWrite == nullptr) return;
	////压缩设置为RGBA(8)
	//TextureToBeWrite->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	//TextureToBeWrite->SRGB = 0;
	////获取材质的mipmap信息，并锁定数据块得到一个指向它的指针
	//FTexture2DMipMap& mipmap = TextureToBeWrite->PlatformData->Mips[0];
	//void* Data = mipmap.BulkData.Lock(LOCK_READ_WRITE);
	////获取材质的长宽
	//int32 textureX = TextureToBeWrite->PlatformData->SizeX;
	//int32 textureY = TextureToBeWrite->PlatformData->SizeY;

	////向colors中填充TextureToBeWrite需要的颜色信息
	//TArray<FColor> colors;
	//for(int32 x = 0; x < textureX*textureY ;++x)
	//{
	//	colors.Add(FColor::Blue);
	//}
	////每个颜色信息的大小
	//int32 stride = (int32)(sizeof(uint8) * 4);

	////复制数据到数据内存位置。(数据块指针，待用数据数组首指针，待用数组总大小）
	//FMemory::Memcpy(Data, colors.GetData(), textureX*textureY*stride);
	////解锁数据块
	//mipmap.BulkData.Unlock();
	////更新材质数据
	//TextureToBeWrite->UpdateResource();

	UWorld* World = selfref->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[FeatureLevel,TextureToBeWrite](FRHICommandListImmediate& RHICmdList)
	{
		TextureWriting_RenderThread
		(
			RHICmdList,
			FeatureLevel,
			TextureToBeWrite
		);
	}
	);
	
}

struct TestStruct
{
	FVector TestPosition;
};


class FMyComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyComputeShader, Global);
public:
	FMyComputeShader(){}
	FMyComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FGlobalShader(Initializer)
	{
		OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
		TestStructureBufferSurface.Bind(Initializer.ParameterMap, TEXT("TestStructureBuffer"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);
		Ar << OutputSurface << TestStructureBufferSurface;
		return bShaderHasOutdatedParams;
	}

	void SetSurfaces(
		FRHICommandList& RHICmdList,
		FUnorderedAccessViewRHIRef OutputSurfaceUAV,
		FUnorderedAccessViewRHIRef TestStructrueBuffUAV,
		FMyShaderStructData& ShaderStructData)
	{
		if (OutputSurface.IsBound())
			RHICmdList.SetUAVParameter(GetComputeShader(), OutputSurface.GetBaseIndex(), OutputSurfaceUAV);
		if (TestStructureBufferSurface.IsBound())
			RHICmdList.SetUAVParameter(GetComputeShader(), TestStructureBufferSurface.GetBaseIndex(), TestStructrueBuffUAV);

		FMyUniformStructData UniformData;
		UniformData.ColorOne = ShaderStructData.ColorOne;
		UniformData.ColorTwo = ShaderStructData.ColorTwo;
		UniformData.ColorThree = ShaderStructData.ColorThree;
		UniformData.ColorFour = ShaderStructData.ColorFour;
		UniformData.ColorIndex = ShaderStructData.ColorIndex;

		SetUniformBufferParameterImmediate(RHICmdList, GetComputeShader(), GetUniformBufferParameter<FMyUniformStructData>(), UniformData);
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		if (OutputSurface.IsBound())
			RHICmdList.SetUAVParameter(GetComputeShader(), OutputSurface.GetBaseIndex(), FUnorderedAccessViewRHIRef());
		if (TestStructureBufferSurface.IsBound())
			RHICmdList.SetUAVParameter(GetComputeShader(), TestStructureBufferSurface.GetBaseIndex(), FUnorderedAccessViewRHIRef());
	}
private:
	FShaderResourceParameter OutputSurface;
	FShaderResourceParameter TestStructureBufferSurface;
};

IMPLEMENT_SHADER_TYPE(, FMyComputeShader, TEXT("/Plugin/StructureBuffer/Private/MyShader.usf"), TEXT("MainCS"), SF_Compute);

static void UseComputeShader_RenderThread(
	FRHICommandListImmediate& RHICmdList, 
	FTextureRenderTargetResource* OutputRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel, 
	FName TextureRenderTargetName,
	FMyShaderStructData ShaderStructData
)
{
	check(IsInRenderingThread());

	TShaderMapRef<FMyComputeShader> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	/**
	*Sets the current compute shader.  Mostly for compliance with platforms
	*that require shader setting before resource binding.
	*/
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	int32 SizeX = OutputRenderTargetResource->GetSizeX();
	int32 SizeY = OutputRenderTargetResource->GetSizeY();
	FRHIResourceCreateInfo CreateInfo;
	/**
	* Creates a 2D RHI texture resource
	* @param SizeX - width of the texture to create
	* @param SizeY - height of the texture to create
	* @param Format - EPixelFormat texture format
	* @param NumMips - number of mips to generate or 0 for full mip pyramid
	* @param NumSamples - number of MSAA samples, usually 1
	* @param Flags - ETextureCreateFlags creation flags
	*/
	FTexture2DRHIRef Texture = RHICreateTexture2D(SizeX, SizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);
	//********************StructureBuffer***********************//
	FStructuredBufferRHIRef TestStructureBuffer;
	FUnorderedAccessViewRHIRef TestStructureBufferUAV;
	TestStruct TestElement;
	TestElement.TestPosition = FVector(1.f, 1.f, 1.f);
	TResourceArray<TestStruct> bufferData;
	bufferData.Reset();
	bufferData.Add(TestElement);
	bufferData.SetAllowCPUAccess(true);

	FRHIResourceCreateInfo TestCreateInfo;
	TestCreateInfo.ResourceArray = &bufferData;

	TestStructureBuffer = RHICreateStructuredBuffer(sizeof(TestStruct), sizeof(TestStruct) * 1, BUF_UnorderedAccess | BUF_ShaderResource, TestCreateInfo);
	TestStructureBufferUAV = RHICreateUnorderedAccessView(TestStructureBuffer, true, false);
	//**********************************************************//
	ComputeShader->SetSurfaces(RHICmdList, TextureUAV,TestStructureBufferUAV,ShaderStructData);
	DispatchComputeShader(RHICmdList, *ComputeShader, SizeX / 32, SizeY / 32, 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	TArray<FVector> Data;
	Data.Reset();
	FVector TestEle(1.f, 1.f, 1.f);
	Data.Add(TestEle);
	
	FVector* srcptr = (FVector*)RHILockStructuredBuffer(TestStructureBuffer.GetReference(), 0, sizeof(FVector), EResourceLockMode::RLM_ReadOnly);
	FMemory::Memcpy(Data.GetData(), srcptr, sizeof(FVector));
	RHIUnlockStructuredBuffer(TestStructureBuffer.GetReference());
	
	DrawTestShaderRenderTarget_RenderThread(RHICmdList, OutputRenderTargetResource, FeatureLevel, TextureRenderTargetName, FLinearColor(),Texture, FMyShaderStructData());
}

void UTestShaderBlueprintLibrary::UseComputeShader(
	UTextureRenderTarget2D* OutputRenderTarget, 
	AActor* Ac, 
	FMyShaderStructData ShaderStructData)
{
	check(IsInGameThread());

	if (OutputRenderTarget==nullptr && Ac == nullptr)
	{
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	UWorld* World = Ac->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, ShaderStructData](FRHICommandListImmediate& RHICmdList)
	{
		UseComputeShader_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, ShaderStructData);
	}
	);
}

#undef LOCTEXT_NAMESPACE  
