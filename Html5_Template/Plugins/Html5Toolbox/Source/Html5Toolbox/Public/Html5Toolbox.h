// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FHtml5ToolboxModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FHtml5ToolboxModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FHtml5ToolboxModule>("Html5Toolbox");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("Html5Toolbox");
	}
};
