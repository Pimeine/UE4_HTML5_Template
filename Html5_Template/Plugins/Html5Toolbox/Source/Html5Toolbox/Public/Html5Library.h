// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "HttpModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "GeneralProjectSettings.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/Object.h"
#include "Engine/World.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Components/SplineComponent.h"

#include "Html5Library.generated.h"

USTRUCT(BlueprintType)
struct FGoogleSheetCellData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Html5 Library|Google Sheet")
	FString StringValue;

	UPROPERTY(BlueprintReadOnly, Category = "Html5 Library|Google Sheet")
	float FloatValue;

	UPROPERTY(BlueprintReadOnly, Category = "Html5 Library|Google Sheet")
	int32 IntValue;

	UPROPERTY(BlueprintReadOnly, Category = "Html5 Library|Google Sheet")
	bool BoolValue;

	FGoogleSheetCellData()
		: StringValue(TEXT("")), FloatValue(0.f), IntValue(0), BoolValue(false)
	{
	}
};

DECLARE_LOG_CATEGORY_EXTERN(NielsTools, Log, All);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGoogleSheetDataReceived, FGoogleSheetCellData, CellData);

/**
 * 
 */
UCLASS()
class HTML5TOOLBOX_API UHtml5Library : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	// Project Settings

	UFUNCTION(BlueprintPure, Category = "Html5 Library|General Settings", meta = (DisplayName = "Get Project's Version", ToolTip = "Get the current project version from the Project's Settings."))
		static void GetProjectVersion(FString& version);

	// Misc

	UFUNCTION(BlueprintPure, Category = "Html5 Library|Utility", meta = (DisplayName = "Get Blackbody", ToolTip = "Handle Temperatures between 1000K and 12000K."))
		static FVector GetBlackbody(float Temperature);

	UFUNCTION(BlueprintPure, Category = "Html5 Library|Utility", meta = (DisplayName = "Repeat String", ToolTip = "Repeat a string x times."))
		static FString RepeatString(int32 Count, const FString& StringToRepeat);

	UFUNCTION(BlueprintPure, Category = "Html5 Library|Game", meta = (DisplayName = "Get World", ToolTip = "Get Current World UWorld Reference.", WorldContext = "WorldContextObject"))
		static UWorld* GetCurrentWorldObject(UObject* WorldContextObject);

	// Discord

	UFUNCTION(BlueprintCallable, Category = "Html5 Library|Discord", meta = (DisplayName = "Send Message to Discord", ToolTip = "Send message to a Discord Webhook"))
		static void SendDiscordMessage(const FString& WebhookUrl, const FString& MessageToSend);


	// UE5

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "2", DisplayName = "Open Level (by Object Reference)"), Category = "Html5 Library|Game")
	static void OpenLevelBySoftObjectPtr(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld> Level, bool bAbsolute = true, FString Options = FString(TEXT("")));

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Distance Along Spline At Location"), Category = "Html5 Library|Spline")
	static float GetDistanceAlongSplineAtInputKey(USplineComponent* Spline, float InKey);


	// Google Sheet
	UFUNCTION(BlueprintCallable, Category = "Google Sheets")
	static void SendToSheet(const FString& SheetUrl, const TMap<FString, FString>& DataMap);

	UFUNCTION(BlueprintCallable, Category = "Google Sheets")
	static void GetSheetCellData(const FString& SheetUrl, const FString& CellReference, FOnGoogleSheetDataReceived OnCompleted);

	UFUNCTION(BlueprintCallable, Category = "Google Sheets")
	static void UpdateSheetCellData(const FString& SheetUrl, const FString& CellReference, const FString& NewValue);
};
