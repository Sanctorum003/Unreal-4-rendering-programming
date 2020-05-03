#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Classes/Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture.h"
#include "MyShaderTest.generated.h"

//将此结构公开为蓝图中变量的类型
USTRUCT(BlueprintType)
struct FMyShaderStructData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorOne;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorTwo;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorThree;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorFour;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	int32 ColorIndex;
};

UCLASS(MinimalAPI, meta = (ScriptName = "TestShaderLibrary"))
class UTestShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContextObject"))
	static void DrawTestShaderRenderTarget(
		class UTextureRenderTarget2D* OutputRenderTarget, 
		AActor* AC, 
		FLinearColor MyColor,
		UTexture* MyTexutre,
		FMyShaderStructData ShaderStructData
	);

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContextObject"))
		static void TextureWriting(UTexture2D* TextureToBeWrite, AActor* selfref);

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContextObject"))
		static void UseComputeShader(
			UTextureRenderTarget2D* OutputRenderTarget,
			AActor* Ac,
			FMyShaderStructData ShaderStructData
		);
};
