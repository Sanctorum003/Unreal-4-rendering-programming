#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionIO.h"
#include "Materials/MaterialExpression.h"
#include "MyMaterialExpression.generated.h"

/**
 *
 */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMyMaterialExpression : public UMaterialExpression
{

	GENERATED_UCLASS_BODY()

	//材质节点的输入
	UPROPERTY()
		FExpressionInput Input1;
	UPROPERTY()
		FExpressionInput Input2;
	UPROPERTY()
		FExpressionInput Input3;
	UPROPERTY()
		FExpressionInput Input4;
	UPROPERTY()
		FExpressionInput Input5;

	UPROPERTY(EditAnywhere, Category = "MyMaterial")
		float myIndex;

	// WITH_EDITOR宏
	// https://forums.unrealengine.com/development-discussion/c-gameplay-programming/20454-if-with_editor
	// https://forums.unrealengine.com/development-discussion/c-gameplay-programming/1392262-with_editor-vs-with_editoronly_data-need-clarification
#if WITH_EDITOR
	//调用HLSLTranlator的函数来翻译HLSL代码
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	//材质节点的名字
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif

};