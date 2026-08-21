// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Html5Toolbox.h"

#define LOCTEXT_NAMESPACE "FHtml5ToolboxModule"

void FHtml5ToolboxModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Html5Toolbox module started successfully!"));
}

void FHtml5ToolboxModule::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Html5Toolbox module shutdown!"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHtml5ToolboxModule, Html5Toolbox)