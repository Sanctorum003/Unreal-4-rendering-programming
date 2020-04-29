#include "MyMaterialExpression.h"
#include "EditorSupportDelegates.h"
#include "MaterialCompiler.h"
#if WITH_EDITOR
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif //WITH_EDITOR
#define LOCTEXT_NAMESPACE "MaterialExpression"
UMyMaterialExpression::UMyMaterialExpression(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText NAME_Math;
		FConstructorStatics()
			: NAME_Math(LOCTEXT("MyMaterial", "MyMaterial"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	myIndex = 0.0f;

#if WITH_EDITORONLY_DATA
	//MenuCategories 用于本地化,只需模仿写即可
	MenuCategories.Add(ConstructorStatics.NAME_Math);
#endif
}


#if WITH_EDITOR
int32 UMyMaterialExpression::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 Result = INDEX_NONE;

	if (!Input1.GetTracedInput().Expression)
	{
		// 当输入口没有连线时报错，这个输出可以在材质编辑器里看到
		return Compiler->Errorf(TEXT("哇，报错了耶！！你的节点第一根线没连好！！！"));
	}
	if (!Input2.GetTracedInput().Expression)
	{
		// 当输入口没有连线时报错，这个输出可以在材质编辑器里看到
		return Compiler->Errorf(TEXT("哇，报错了耶！！你的节点第二根线没连好！！！"));
	}
	if (!Input3.GetTracedInput().Expression)
	{
		// 当输入口没有连线时报错，这个输出可以在材质编辑器里看到
		return Compiler->Errorf(TEXT("哇，报错了耶！！你的节点第三根线没连好！！！"));
	}
	if (!Input4.GetTracedInput().Expression)
	{
		// 当输入口没有连线时报错，这个输出可以在材质编辑器里看到
		return Compiler->Errorf(TEXT("哇，报错了耶！！你的节点第四根线没连好！！！"));
	}
	if (!Input5.GetTracedInput().Expression)
	{
		// 当输入口没有连线时报错，这个输出可以在材质编辑器里看到
		return Compiler->Errorf(TEXT("哇，报错了耶！！你的节点第五根线没连好！！！"));
	}

	int32 newIndex = myIndex;

	if (newIndex > 5 || newIndex < 0)
	{
		return Compiler->Errorf(TEXT("index指数不对，应该在0到5之间"));
	}

	switch (newIndex)
	{
	case 0:
		return Input1.Compile(Compiler);
	case 1:
		return Input2.Compile(Compiler);
	case 2:
		return Input3.Compile(Compiler);
	case 3:
		return Input4.Compile(Compiler);
	case 4:
		return Input5.Compile(Compiler);
	}

	return Result;
}

void UMyMaterialExpression::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("MyMaterialExpression"));
}
#endif
