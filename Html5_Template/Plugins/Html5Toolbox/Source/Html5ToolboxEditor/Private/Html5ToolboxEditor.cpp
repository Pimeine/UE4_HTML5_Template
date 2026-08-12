// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Html5ToolboxEditor.h"
#include "Html5ToolboxSettings.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityObject.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "EditorStyleSet.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "UnrealEdMisc.h"


#define LOCTEXT_NAMESPACE "FHtml5ToolboxEditorModule"

void FHtml5ToolboxEditorModule::StartupModule()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	MenuExtender = MakeShareable(new FExtender);
	MenuExtender->AddMenuBarExtension(
		"Help",
		EExtensionHook::After,
		nullptr,
		FMenuBarExtensionDelegate::CreateRaw(this, &FHtml5ToolboxEditorModule::AddMenuBarButton)
	);

	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

	OnPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FHtml5ToolboxEditorModule::OnSettingsChanged);
	
}

void FHtml5ToolboxEditorModule::ShutdownModule()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);

	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);
	}
}

void FHtml5ToolboxEditorModule::OnSettingsChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object && Object->IsA<UHtml5ToolboxSettings>())
	{
		RefreshMenuExtender();
	}
}

void FHtml5ToolboxEditorModule::RefreshMenuExtender()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	if (MenuExtender.IsValid())
	{
		LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);
	}

	MenuExtender = MakeShareable(new FExtender);
	MenuExtender->AddMenuBarExtension(
		"Help",
		EExtensionHook::After,
		nullptr,
		FMenuBarExtensionDelegate::CreateRaw(this, &FHtml5ToolboxEditorModule::AddMenuBarButton)
	);

	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

	FNotificationInfo Info(LOCTEXT("RestartRequired", "Some changes require an editor restart to take effect"));
	Info.bFireAndForget = true;
	Info.FadeOutDuration = 1.0f;
	Info.ExpireDuration = 8.0f;
	Info.bUseThrobber = false;

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("RestartNow", "Restart Now"),
		LOCTEXT("RestartNowTooltip", "Restart the editor now"),
		FSimpleDelegate::CreateLambda([]()
	{
		FUnrealEdMisc::Get().RestartEditor(false);
	})
	));

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("Dismiss", "Dismiss"),
		LOCTEXT("DismissTooltip", "Close this notification"),
		FSimpleDelegate::CreateLambda([WeakNotification = TWeakPtr<SNotificationItem>()]() {}),
		SNotificationItem::CS_None
	));

	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FHtml5ToolboxEditorModule::AddMenuBarButton(FMenuBarBuilder& Builder)
{
	const UHtml5ToolboxSettings* Settings = GetDefault<UHtml5ToolboxSettings>();

	if (Settings->Menus.Num() == 0)
	{
		Builder.AddPullDownMenu(
			LOCTEXT("Html5ToolboxDefaultLabel", "Html5 Toolbox"),
			LOCTEXT("Html5ToolboxDefaulTooltip", "Configure your menus in editor preferences"),
			FNewMenuDelegate::CreateLambda([](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("NoEntries", "No setup menu"),
				LOCTEXT("NoEntriesTooltip", "Add menus in Editor Preferences / Html5 Toolbox"),
				FSlateIcon(),
				FUIAction()
			);
		})
		);
		return;
	}

	for (const FToolboxMenu& MenuData : Settings->Menus)
	{
		FToolboxMenu MenuDataCopy = MenuData;

		Builder.AddPullDownMenu(
			MenuDataCopy.MenuLabel,
			MenuDataCopy.MenuTooltip,
			FNewMenuDelegate::CreateRaw(this, &FHtml5ToolboxEditorModule::GenerateMenu, MenuDataCopy)
		);
	}
}

void FHtml5ToolboxEditorModule::GenerateMenu(FMenuBuilder& MenuBuilder, FToolboxMenu MenuData)
{
	const UHtml5ToolboxSettings* Settings = GetDefault<UHtml5ToolboxSettings>();

	if (MenuData.MenuEntries.Num() == 0)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("NoEntries", "No setup tool"),
			LOCTEXT("NoEntriesTooltip", "Add entries in Editor Preferences / Html5 Toolbox"),
			FSlateIcon(),
			FUIAction()
		);
	}
	else
	{
		TMap<FString, TArray<FToolboxMenuEntry>> SectionMap;

		for (const FToolboxMenuEntry& Entry : MenuData.MenuEntries)
		{
			FString Section = Entry.SectionName.IsEmpty() ? TEXT("General") : Entry.SectionName.ToString();
			SectionMap.FindOrAdd(Section).Add(Entry);
		}

		bool bFirstSection = true;
		for (const auto& Pair : SectionMap)
		{
			if (!bFirstSection)
			{
				MenuBuilder.AddMenuSeparator();
			}
			bFirstSection = false;

			TSharedRef<SWidget> SectionWidget = SNew(SBox)
				.Padding(FMargin(-16.0f, 0.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Pair.Key))
					.TextStyle(FEditorStyle::Get(), "SmallText")
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
				];

			MenuBuilder.AddWidget(SectionWidget, FText::GetEmpty(), false, false);

			for (const FToolboxMenuEntry& Entry : Pair.Value)
			{
				TSharedRef<SWidget> EntryWidget = SNew(SBox)
					.MinDesiredWidth(200.0f)
					.Padding(FMargin(8.0f, 2.0f))
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(8.0f, 0.0f, 2.0f, 0.0f))
					[
						SNew(SImage).Image(FEditorStyle::GetBrush(*Entry.IconName))
					]

				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(Entry.DisplayName)
						.ToolTipText(Entry.Tooltip)
					]

					];

				MenuBuilder.AddMenuEntry(
					FUIAction(FExecuteAction::CreateRaw(this, &FHtml5ToolboxEditorModule::ExecuteBlueprintFunction, Entry)),
					EntryWidget
				);
			}
		}
	}
}

void FHtml5ToolboxEditorModule::ExecuteBlueprintFunction(FToolboxMenuEntry Entry)
{
	UObject* LoadedAsset = Entry.UtilityBlueprint.TryLoad();
	if (!LoadedAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot load the Blueprint: %s"), *Entry.UtilityBlueprint.ToString());
		return;
	}

	UEditorUtilityBlueprint* UtilityBP = Cast<UEditorUtilityBlueprint>(LoadedAsset);
	if (!UtilityBP)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to load something else than an EditorUtilityBlueprint"));
		return;
	}

	//Temp BP instance
	UClass* GeneratedClass = UtilityBP->GeneratedClass;
	if (!GeneratedClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to find the GeneratedClass"));
		return;
	}

	UObject* Instance = NewObject<UObject>(GetTransientPackage(), GeneratedClass);
	if (!Instance)
	{
		return;
	}

	UFunction* Function = Instance->FindFunction(Entry.FunctionName);
	if (Function)
	{
		Instance->ProcessEvent(Function, nullptr);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to find function '%s' in %s"), *Entry.FunctionName.ToString(), *GeneratedClass->GetName());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHtml5ToolboxEditorModule, Html5ToolboxEditor)