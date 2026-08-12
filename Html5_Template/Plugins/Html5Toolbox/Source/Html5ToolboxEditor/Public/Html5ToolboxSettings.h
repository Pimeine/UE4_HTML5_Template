// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Html5ToolboxSettings.generated.h"

USTRUCT(BlueprintType)
struct FToolboxMenuEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget")
	FText DisplayName;
	
	UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget")
	FText Tooltip;

	UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget")
	FText SectionName;

	UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget", meta = (Tooltip = "Icon Name (e.g.: LevelEditor.OpenContentBrowser) - https://github.com/EpicKiwi/unreal-engine-editor-icons"))
	FString IconName;
	
	UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget", meta = (AllowedClasses = "EditorUtilityBlueprint"))
	FSoftObjectPath UtilityBlueprint;
	
	UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget", meta = (Tooltip = "Exact Name of the Function or Event from your Editor Utility Blueprint!"))
	FName FunctionName;
};

USTRUCT(BlueprintType)
struct FToolboxMenu
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget")
		FText MenuLabel;

		UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget")
		FText MenuTooltip;

		UPROPERTY(EditAnywhere, Category = "Html5 Settings|Editor Utility Widget")
		TArray<FToolboxMenuEntry> MenuEntries;
};

UCLASS(config = EditorPerProjectUserSettings, defaultconfig, meta = (DisplayName = "Html5 Toolbox"))
class HTML5TOOLBOXEDITOR_API UHtml5ToolboxSettings : public UDeveloperSettings
{

	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, config, Category = "Html5 Settings|Editor Utility Widget")
	TArray<FToolboxMenu> Menus;
	

};
