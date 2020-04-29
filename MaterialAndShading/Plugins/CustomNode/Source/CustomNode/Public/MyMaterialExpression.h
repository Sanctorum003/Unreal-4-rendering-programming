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

	//���ʽڵ������
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

	// WITH_EDITOR��
	// https://forums.unrealengine.com/development-discussion/c-gameplay-programming/20454-if-with_editor
	// https://forums.unrealengine.com/development-discussion/c-gameplay-programming/1392262-with_editor-vs-with_editoronly_data-need-clarification
#if WITH_EDITOR
	//����HLSLTranlator�ĺ���������HLSL����
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	//���ʽڵ������
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif

};