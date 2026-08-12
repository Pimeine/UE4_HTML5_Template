// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Modules/ModuleManager.h"
#include "Html5ToolboxSettings.h"

class FHtml5ToolboxEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	void AddMenuBarButton(FMenuBarBuilder& Builder);
	void GenerateMenu(FMenuBuilder& MenuBuilder, FToolboxMenu MeduData);
	void ExecuteBlueprintFunction(FToolboxMenuEntry Entry);

	void RefreshMenuExtender();
	void OnSettingsChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	TSharedPtr<FExtender> MenuExtender;
	FDelegateHandle OnPropertyChangedHandle;
};
