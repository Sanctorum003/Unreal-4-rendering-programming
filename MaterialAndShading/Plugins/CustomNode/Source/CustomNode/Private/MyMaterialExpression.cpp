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
	//MenuCategories ���ڱ��ػ�,ֻ��ģ��д����
	MenuCategories.Add(ConstructorStatics.NAME_Math);
#endif
}


#if WITH_EDITOR
int32 UMyMaterialExpression::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 Result = INDEX_NONE;

	if (!Input1.GetTracedInput().Expression)
	{
		// �������û������ʱ���������������ڲ��ʱ༭���￴��
		return Compiler->Errorf(TEXT("�ۣ�������Ү������Ľڵ��һ����û���ã�����"));
	}
	if (!Input2.GetTracedInput().Expression)
	{
		// �������û������ʱ���������������ڲ��ʱ༭���￴��
		return Compiler->Errorf(TEXT("�ۣ�������Ү������Ľڵ�ڶ�����û���ã�����"));
	}
	if (!Input3.GetTracedInput().Expression)
	{
		// �������û������ʱ���������������ڲ��ʱ༭���￴��
		return Compiler->Errorf(TEXT("�ۣ�������Ү������Ľڵ��������û���ã�����"));
	}
	if (!Input4.GetTracedInput().Expression)
	{
		// �������û������ʱ���������������ڲ��ʱ༭���￴��
		return Compiler->Errorf(TEXT("�ۣ�������Ү������Ľڵ���ĸ���û���ã�����"));
	}
	if (!Input5.GetTracedInput().Expression)
	{
		// �������û������ʱ���������������ڲ��ʱ༭���￴��
		return Compiler->Errorf(TEXT("�ۣ�������Ү������Ľڵ�������û���ã�����"));
	}

	int32 newIndex = myIndex;

	if (newIndex > 5 || newIndex < 0)
	{
		return Compiler->Errorf(TEXT("indexָ�����ԣ�Ӧ����0��5֮��"));
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
