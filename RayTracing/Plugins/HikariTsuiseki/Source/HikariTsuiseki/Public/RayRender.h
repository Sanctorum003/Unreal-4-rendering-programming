#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/MeshComponent.h"
#include "RayRender.generated.h"

UCLASS()
//RayTracingPlugin
class  ARayRender : public AActor
{
	GENERATED_BODY()
public:

	ARayRender() {}

	UFUNCTION(BlueprintCallable, Category = RayRender)
		void MainRayRender();
};