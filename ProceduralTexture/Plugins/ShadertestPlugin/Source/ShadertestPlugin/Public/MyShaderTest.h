#pragma once  
  
#include "CoreMinimal.h"  
#include "UObject/ObjectMacros.h"  
#include "Classes/Kismet/BlueprintFunctionLibrary.h"  
#include "MyShaderTest.generated.h"  

class UTexture;
USTRUCT(BlueprintType)  
struct FMyShaderStructData  
{  
    GENERATED_USTRUCT_BODY()  
  
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)  
    FLinearColor ColorOne;  
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)  
    FLinearColor ColorTwo;  
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)  
    FLinearColor Colorthree;  
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)  
    FLinearColor ColorFour;  
};  
  
UCLASS(MinimalAPI,meta = (ScriptName = "TestShaderLibary"))  
class UTestShaderBlueprintLibrary : public UBlueprintFunctionLibrary  
{  
    GENERATED_UCLASS_BODY()  
  
    UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContextObject"))  
    static void DrawTestShaderRenderTarget(class UTextureRenderTarget2D* OutputRenderTarget, AActor* Ac, FLinearColor MyColor, UTexture* MyTexture);  
};  