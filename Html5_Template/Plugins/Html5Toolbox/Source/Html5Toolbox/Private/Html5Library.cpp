// Fill out your copyright notice in the Description page of Project Settings.

#include "Html5Library.h"

DEFINE_LOG_CATEGORY(NielsTools);

// Project Settings

void UHtml5Library::GetProjectVersion(FString& version)
{
	version = GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}

// Misc

FVector UHtml5Library::GetBlackbody(float Temperature)
{
	FVector VectorRGB;
	float TempK = FMath::Clamp(Temperature, 1000.0f, 40000.0f) / 100.0f;

	if (TempK <= 66.0f)
	{
		VectorRGB.X = 1.0f;
		VectorRGB.Y = FMath::Clamp(0.39008157876901960784f * logf(TempK) - 0.63184144378862745098f, 0.0f, 1.0f);
	}
	else
	{
		float NewTempK = TempK - 60.0f;
		VectorRGB.X = FMath::Clamp(1.29293618606274509804f * powf(NewTempK, -0.1332047592), 0.0f, 1.0f);
		VectorRGB.Y = FMath::Clamp(1.12989086089529411765f * powf(NewTempK, -0.0755148492), 0.0f, 1.0f);
	}

	if (TempK >= 66.0f)
	{
		VectorRGB.Z = 1.0f;
	}
	else if (TempK <= 19.0f)
	{
		VectorRGB.Z = 0.0f;
	}
	else
	{
		VectorRGB.Z = FMath::Clamp(0.54320678911019607843f * logf(TempK - 10.0f) - 1.19625408914f, 0.0f, 1.0f);
	}

	return VectorRGB;
}

FString UHtml5Library::RepeatString(int32 Count, const FString& StringToRepeat)
{
	TArray<FString> StringArray;
	for (int32 i = 0; i < Count; ++i) {
		StringArray.Add(StringToRepeat);
	}
	return FString::Join(StringArray, TEXT(""));
}

UWorld* UHtml5Library::GetCurrentWorldObject(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		UE_LOG(NielsTools, Error, TEXT("WorldContextObject is null!"));
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(NielsTools, Error, TEXT("Failed to get UWorld from WorldContextObject!"));
		return nullptr;
	}

	return World;
}

// Discord

void UHtml5Library::SendDiscordMessage(const FString& WebhookUrl, const FString& MessageToSend)
{

	FHttpModule& http = FHttpModule::Get();

	if (!http.IsHttpEnabled()) {
		UE_LOG(NielsTools, Error, TEXT("Http requests are disabled"));
		return;
	}

	if (MessageToSend.IsEmpty()) {
		UE_LOG(NielsTools, Error, TEXT("Message To Send is empty"));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> Request = http.CreateRequest();

	Request->SetVerb("POST");
	Request->SetURL(WebhookUrl);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("content", MessageToSend);

	FString Payload = "";
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(Payload);
	Request->ProcessRequest();

}

// UE5

void UHtml5Library::OpenLevelBySoftObjectPtr(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld> Level, bool bAbsolute, FString Options)
{
	const FName LevelName = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));
	UGameplayStatics::OpenLevel(WorldContextObject, LevelName, bAbsolute, Options);
}

float UHtml5Library::GetDistanceAlongSplineAtInputKey(USplineComponent * Spline, float InKey)
{
	if (!Spline) return 0.0f;
	
	int32 NumPoints = Spline->GetNumberOfSplinePoints();
	int32 NumSegments = Spline->IsClosedLoop() ? NumPoints : NumPoints - 1;

	if ((InKey >= 0) && (InKey < NumSegments))
	{
		int32 PointIndex = FMath::FloorToInt(InKey);
		float Fraction = InKey - PointIndex;
		int32 ReparamPointIndex = PointIndex * Spline->ReparamStepsPerSegment;

		auto& ReparamTable = Spline->SplineCurves.ReparamTable.Points;

		if (!ReparamTable.IsValidIndex(ReparamPointIndex))
		{
			return 0.0f;
		}

		float BaseDistance = Spline->SplineCurves.ReparamTable.Points[ReparamPointIndex].InVal;

		FVector PosA = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::Local);
		FVector PosB = Spline->GetLocationAtSplinePoint(PointIndex + 1, ESplineCoordinateSpace::Local);
		float SegmentLength = FVector::Distance(PosA, PosB) * Fraction;

		return BaseDistance + SegmentLength;
	}
	else if (InKey >= NumSegments)
	{
		return Spline->GetSplineLength();
	}
	else
	{
		return 0.0f;
	}
}

// Google Sheet

void UHtml5Library::SendToSheet(const FString& SheetUrl, const TMap<FString, FString>& DataMap)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	for (const auto& Elem : DataMap)
	{
		JsonObject->SetStringField(Elem.Key, Elem.Value);
	}

	FString JsonPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonPayload);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SheetUrl);
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("Content-Type"), TEXT("text/plain"));
	Request->SetContentAsString(JsonPayload);
	Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (bWasSuccessful && Response.IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("Data sent successfully: %s"), *Response->GetContentAsString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to send data: %s"), *Request->GetURL());
			}
		});
	Request->ProcessRequest();
}

void UHtml5Library::GetSheetCellData(const FString& SheetUrl, const FString& CellReference, FOnGoogleSheetDataReceived OnCompleted)
{
	FString FormattedUrl = FString::Printf(TEXT("%s?cell=%s"), *SheetUrl, *CellReference);

	TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(FormattedUrl);
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([OnCompleted](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			FGoogleSheetCellData ParsedValue;

			if (bWasSuccessful && Response.IsValid())
			{
				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
				{
					//FString Value = JsonObject->GetStringField("value");
					//UE_LOG(LogTemp, Log, TEXT("Cell Value: %s"), *Value);

					FString RawValue;
					if (JsonObject->TryGetStringField("value", RawValue))
					{
						ParsedValue.StringValue = RawValue;
						ParsedValue.FloatValue = FCString::Atof(*RawValue);
						ParsedValue.IntValue = FCString::Atoi(*RawValue);
						ParsedValue.BoolValue = RawValue.ToLower() == "true" || RawValue == "1";

						OnCompleted.ExecuteIfBound(ParsedValue);
						return;
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to send data: %s"), *Request->GetURL());
			}
		});

	Request->ProcessRequest();
}

void UHtml5Library::UpdateSheetCellData(const FString& SheetUrl, const FString& CellReference, const FString& NewValue)
{
	//FString FormattedUrl = FString::Printf(TEXT("%s?cell=%s"), *SheetUrl, *CellReference);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("cell", CellReference);
	JsonObject->SetStringField("value", NewValue);

	FString JsonPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonPayload);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::NotThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SheetUrl);
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("Content-Type"), TEXT("text/plain"));
	Request->SetContentAsString(JsonPayload);


	Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (bWasSuccessful && Response.IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("Cell Updated: %s"), *Response->GetContentAsString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to update cell: %s"), *Request->GetURL());
			}
		});
	Request->ProcessRequest();
}