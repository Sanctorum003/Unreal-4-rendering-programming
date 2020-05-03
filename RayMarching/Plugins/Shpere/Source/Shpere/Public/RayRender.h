#pragma once  

#include "CoreMinimal.h"  
#include "UObject/ObjectMacros.h"  
#include "Classes/Kismet/BlueprintFunctionLibrary.h"  
#include "RayRender.generated.h" 


UCLASS(MinimalAPI, meta = (ScriptName = "RayRenderLibary"))
class URayRenderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	URayRenderBlueprintLibrary() {}
public:
		
		UFUNCTION(BlueprintCallable, Category = "RayRenderPlugin", meta = (WorldContext = "WorldContextObject"))
		static void DrawRayMarchingTarget(class UTextureRenderTarget2D* OutputRenderTarget, AActor* Ac, FVector RayOrigin,FVector RayDirection);
};