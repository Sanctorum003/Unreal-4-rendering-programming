// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "GeneratedCppIncludes.h"
#include "Public/VCubeComponent.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeVCubeComponent() {}
// Cross Module References
	VCUBE_API UClass* Z_Construct_UClass_UVCubeComponent_NoRegister();
	VCUBE_API UClass* Z_Construct_UClass_UVCubeComponent();
	ENGINE_API UClass* Z_Construct_UClass_UMeshComponent();
	UPackage* Z_Construct_UPackage__Script_VCube();
	ENGINE_API UClass* Z_Construct_UClass_UStaticMesh_NoRegister();
// End Cross Module References
	void UVCubeComponent::StaticRegisterNativesUVCubeComponent()
	{
	}
	UClass* Z_Construct_UClass_UVCubeComponent_NoRegister()
	{
		return UVCubeComponent::StaticClass();
	}
	UClass* Z_Construct_UClass_UVCubeComponent()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			static UObject* (*const DependentSingletons[])() = {
				(UObject* (*)())Z_Construct_UClass_UMeshComponent,
				(UObject* (*)())Z_Construct_UPackage__Script_VCube,
			};
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
				{ "BlueprintSpawnableComponent", "" },
				{ "ClassGroupNames", "Rendering" },
				{ "DisplayName", "VCubeComponent" },
				{ "HideCategories", "Object LOD Physics Collision Mobility Trigger" },
				{ "IncludePath", "VCubeComponent.h" },
				{ "ModuleRelativePath", "Public/VCubeComponent.h" },
				{ "ToolTip", "This is a mesh effect component" },
			};
#endif
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_StaticMeshAsset_MetaData[] = {
				{ "Category", "VCubeComponent" },
				{ "ModuleRelativePath", "Public/VCubeComponent.h" },
				{ "ToolTip", "Mesh asset for this component" },
			};
#endif
			static const UE4CodeGen_Private::FObjectPropertyParams NewProp_StaticMeshAsset = { UE4CodeGen_Private::EPropertyClass::Object, "StaticMeshAsset", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000010005, 1, nullptr, STRUCT_OFFSET(UVCubeComponent, StaticMeshAsset), Z_Construct_UClass_UStaticMesh_NoRegister, METADATA_PARAMS(NewProp_StaticMeshAsset_MetaData, ARRAY_COUNT(NewProp_StaticMeshAsset_MetaData)) };
			static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[] = {
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_StaticMeshAsset,
			};
			static const FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
				TCppClassTypeTraits<UVCubeComponent>::IsAbstract,
			};
			static const UE4CodeGen_Private::FClassParams ClassParams = {
				&UVCubeComponent::StaticClass,
				DependentSingletons, ARRAY_COUNT(DependentSingletons),
				0x00B01080u,
				nullptr, 0,
				PropPointers, ARRAY_COUNT(PropPointers),
				nullptr,
				&StaticCppClassTypeInfo,
				nullptr, 0,
				METADATA_PARAMS(Class_MetaDataParams, ARRAY_COUNT(Class_MetaDataParams))
			};
			UE4CodeGen_Private::ConstructUClass(OuterClass, ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UVCubeComponent, 3578945084);
	static FCompiledInDefer Z_CompiledInDefer_UClass_UVCubeComponent(Z_Construct_UClass_UVCubeComponent, &UVCubeComponent::StaticClass, TEXT("/Script/VCube"), TEXT("UVCubeComponent"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UVCubeComponent);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
