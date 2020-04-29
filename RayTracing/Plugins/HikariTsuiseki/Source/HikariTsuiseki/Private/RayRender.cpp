#include "RayRender.h"
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


class FRayComputerShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FRayComputerShader, Global);
public:
	FRayComputerShader() {}
	FRayComputerShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FGlobalShader(Initializer)
	{
		OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
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
		bool bShaderHasOutDatedParams = FGlobalShader::Serialize(Ar);

		Ar << OutputSurface;

		return bShaderHasOutDatedParams;
	}

	void SetSurfaces(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputSurfaceUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutputSurface.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), OutputSurfaceUAV);
		}

	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutputSurface.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:
	FShaderResourceParameter OutputSurface;
};
IMPLEMENT_SHADER_TYPE(, FRayComputerShader, TEXT("/Plugin/HikariTsuiseki/Private/RayTracing.usf"), TEXT("MainCS"), SF_Compute);


void SaveArrayToTexture(TArray<FVector4>* BitmapRef, uint32 TextureSizeY, uint32 TextureSizeX)
{
	TArray<FColor>BitMapToBeSave;

	for (int32 pixelindex = 0; pixelindex < BitmapRef->Num(); ++pixelindex)
	{
		uint8 cr = 255.99f * (*BitmapRef)[pixelindex].X;
		uint8 cg = 255.99f * (*BitmapRef)[pixelindex].Y;
		uint8 cb = 255.99f * (*BitmapRef)[pixelindex].Z;
		uint8 ca = 255.99f * (*BitmapRef)[pixelindex].W;
		BitMapToBeSave.Add(FColor(cr, cg, cb, ca));
	}

	if (BitMapToBeSave.Num())
	{
		IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);

		const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("RenderOutputTexture"));

		uint32 ExtendXWithMSAA = BitMapToBeSave.Num() / TextureSizeY;

		FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, TextureSizeY, BitMapToBeSave.GetData());

		UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
	}
	else
	{
		UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP,format or texture type is not supported"));
	}
}

static void RayTracing_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel
)
{
	check(IsInRenderingThread());
	TArray<FVector4> Bitmap;

	TShaderMapRef<FRayComputerShader>ComputerShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputerShader->GetComputeShader());

	int32 SizeX = 256;
	int32 SizeY = 256;

	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef Texture = RHICreateTexture2D(SizeX, SizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);
	ComputerShader->SetSurfaces(RHICmdList, TextureUAV);
	DispatchComputeShader(RHICmdList, *ComputerShader, SizeX / 32, SizeY / 32, 1);
	ComputerShader->UnbindBuffers(RHICmdList);

	Bitmap.Init(FVector4(1.f, 0.f, 0.f, 1.f), SizeX * SizeY);

	uint32 LolStride = 0;
	uint8* TextureDataPtr = (uint8*)RHICmdList.LockTexture2D(Texture, 0, EResourceLockMode::RLM_ReadOnly, LolStride, false);
	uint8* ArraryData = (uint8*)Bitmap.GetData();
	FMemory::Memcpy(ArraryData, TextureDataPtr, GPixelFormats[PF_A32B32G32R32F].BlockBytes * SizeX * SizeY);
	RHICmdList.UnlockTexture2D(Texture, 0, false);

	SaveArrayToTexture(&Bitmap, SizeX, SizeY);
}


void ARayRender::MainRayRender()
{
	ERHIFeatureLevel::Type FeatureLevel = GetWorld()->Scene->GetFeatureLevel();
	ENQUEUE_RENDER_COMMAND(RayTracingCommand)
	(
  [FeatureLevel](FRHICommandListImmediate& RHICmdList)
		{
			RayTracing_RenderThread
			(
				RHICmdList,
				FeatureLevel
			);
		}
	);
}
