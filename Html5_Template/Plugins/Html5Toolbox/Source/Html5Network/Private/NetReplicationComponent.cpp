#include "NetReplicationComponent.h"
#include "HtmlNetSubsystem.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/ActorComponent.h"

UNetReplicationComponent::UNetReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
}

	void UNetReplicationComponent::BeginPlay()
	{
		Super::BeginPlay();

		if (NetId.IsEmpty())
		{
			NetId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
		}

		if (UGameInstance* GI = GetWorld()->GetGameInstance())
		{
			if (UHtmlNetSubsystem* Subsystem = GI->GetSubsystem<UHtmlNetSubsystem>())
			{
				Subsystem->RegisterActor(NetId, this);
			}
		}

		TargetTransform = GetOwner()->GetActorTransform();
		LastSentTransform = TargetTransform;


		if (bIsOwner)
		{
			for (const FReplicatedProperty& Prop : CustomProperties)
			{
				switch (Prop.Type)
				{
				case ENetPropertyType::Float:
					if (const float* Val = FloatValues.Find(Prop.PropertyName))
						LastSentFloatValues.Add(Prop.PropertyName, *Val - 999999.f); // Force diff
					break;
				case ENetPropertyType::Int:
					if (const int32* Val = IntValues.Find(Prop.PropertyName))
						LastSentIntValues.Add(Prop.PropertyName, *Val - 999999);
					break;
				case ENetPropertyType::Bool:
					if (const bool* Val = BoolValues.Find(Prop.PropertyName))
						LastSentBoolValues.Add(Prop.PropertyName, !*Val);
					break;
				case ENetPropertyType::String:
					if (const FString* Val = StringValues.Find(Prop.PropertyName))
						LastSentStringValues.Add(Prop.PropertyName, *Val + "___FORCE_SEND");
					break;
				default: break;
				}
			}
		}
	}

void UNetReplicationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UHtmlNetSubsystem* Subsystem = GI->GetSubsystem<UHtmlNetSubsystem>())
		{
			Subsystem->UnregisterActor(NetId);

			if (bIsOwner && EndPlayReason != EEndPlayReason::EndPlayInEditor)
			{
				Subsystem->RequestDespawn(NetId);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UNetReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsOwner)
	{
		AccumulatedTime += DeltaTime;
		if (AccumulatedTime >= ReplicationRate)
		{
			AccumulatedTime = 0.f;
			SendCurrentState();
		}
	}
	else
	{
		if (bReplicateTransform && bHasReceivedFirstState)
		{
			FTransform Current = GetOwner()->GetActorTransform();

			FVector NewLoc = FMath::VInterpTo(
				Current.GetLocation(), TargetTransform.GetLocation(),
				DeltaTime, InterpolationSpeed
			);

			FTransform NewTransform(Current.GetRotation(), NewLoc, Current.GetScale3D());

			if (bReplicateRotation)
			{
				FQuat NewRot = FQuat::Slerp(
					Current.GetRotation(), TargetTransform.GetRotation(),
					FMath::Clamp(DeltaTime * InterpolationSpeed, 0.f, 1.f)
				);
				NewTransform.SetRotation(NewRot);
			}

			GetOwner()->SetActorTransform(NewTransform);
		}
	}
}

void UNetReplicationComponent::SetFloatValue(const FString& PropName, float Value)
{
	FloatValues.Add(PropName, Value);
}

void UNetReplicationComponent::SetIntValue(const FString& PropName, int32 Value)
{
	IntValues.Add(PropName, Value);
}

void UNetReplicationComponent::SetBoolValue(const FString& PropName, bool Value)
{
	BoolValues.Add(PropName, Value);
}

void UNetReplicationComponent::SetStringValue(const FString& PropName, const FString& Value)
{
	StringValues.Add(PropName, Value);
}

float UNetReplicationComponent::GetFloatValue(const FString& PropName, float DefaultValue) const
{
	if (const float* Found = FloatValues.Find(PropName)) return *Found;
	return DefaultValue;
}

int32 UNetReplicationComponent::GetIntValue(const FString& PropName, int32 DefaultValue) const
{
	if (const int32* Found = IntValues.Find(PropName)) return *Found;
	return DefaultValue;
}

bool UNetReplicationComponent::GetBoolValue(const FString& PropName, bool DefaultValue) const
{
	if (const bool* Found = BoolValues.Find(PropName)) return *Found;
	return DefaultValue;
}

FString UNetReplicationComponent::GetStringValue(const FString& PropName, const FString& DefaultValue) const
{
	if (const FString* Found = StringValues.Find(PropName)) return *Found;
	return DefaultValue;
}

void UNetReplicationComponent::SendCustomRPC(const FString& EventName, const FString& PayloadJson, bool bIncludeSelf)
{
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UHtmlNetSubsystem* Subsystem = GI->GetSubsystem<UHtmlNetSubsystem>())
		{
			Subsystem->SendRPC(NetId, EventName, PayloadJson, bIncludeSelf);
		}
	}
}

void UNetReplicationComponent::HandleRawRPC(const FString& EventName, const TSharedPtr<FJsonObject>& Payload)
{
	FString PayloadString;
	if (Payload.IsValid())
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
		FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);
	}
	OnRPCReceived.Broadcast(EventName, PayloadString);
}

bool UNetReplicationComponent::HasTransformChangedSignificantly() const
{
	FTransform Current = GetOwner()->GetActorTransform();
	float DistSq = FVector::DistSquared(Current.GetLocation(), LastSentTransform.GetLocation());
	if (DistSq > 1.f) return true;

	if (bReplicateRotation)
	{
		float AngleDiff = Current.GetRotation().AngularDistance(LastSentTransform.GetRotation());
		if (AngleDiff > 0.01f) return true;
	}
	return false;
}

void UNetReplicationComponent::SendCurrentState()
{
	TSharedPtr<FJsonObject> StateJson = MakeShared<FJsonObject>();
	bool bHasAnyChange = false;

	if (bReplicateTransform && HasTransformChangedSignificantly())
	{
		FTransform T = GetOwner()->GetActorTransform();
		TSharedPtr<FJsonObject> TransformJson = MakeShared<FJsonObject>();
		TransformJson->SetNumberField("x", T.GetLocation().X);
		TransformJson->SetNumberField("y", T.GetLocation().Y);
		TransformJson->SetNumberField("z", T.GetLocation().Z);

		if (bReplicateRotation)
		{
			FRotator R = T.GetRotation().Rotator();
			TransformJson->SetNumberField("pitch", R.Pitch);
			TransformJson->SetNumberField("yaw", R.Yaw);
			TransformJson->SetNumberField("roll", R.Roll);
		}

		StateJson->SetObjectField("transform", TransformJson);
		LastSentTransform = T;
		bHasAnyChange = true;
	}

	for (const FReplicatedProperty& Prop : CustomProperties)
	{
		switch (Prop.Type)
		{
		case ENetPropertyType::Float:
		{
			if (const float* Val = FloatValues.Find(Prop.PropertyName))
			{
				const float* LastVal = LastSentFloatValues.Find(Prop.PropertyName);
				if (!LastVal || FMath::Abs(*Val - *LastVal) > Prop.ChangeThreshold)
				{
					StateJson->SetNumberField(Prop.PropertyName, *Val);
					LastSentFloatValues.Add(Prop.PropertyName, *Val);
					bHasAnyChange = true;
				}
			}
			break;
		}
		case ENetPropertyType::Int:
		{
			if (const int32* Val = IntValues.Find(Prop.PropertyName))
			{
				const int32* LastVal = LastSentIntValues.Find(Prop.PropertyName);
				if (!LastVal || *Val != *LastVal)
				{
					StateJson->SetNumberField(Prop.PropertyName, *Val);
					LastSentIntValues.Add(Prop.PropertyName, *Val);
					bHasAnyChange = true;
				}
			}
			break;
		}
		case ENetPropertyType::Bool:
		{
			if (const bool* Val = BoolValues.Find(Prop.PropertyName))
			{
				const bool* LastVal = LastSentBoolValues.Find(Prop.PropertyName);
				if (!LastVal || *Val != *LastVal)
				{
					StateJson->SetBoolField(Prop.PropertyName, *Val);
					LastSentBoolValues.Add(Prop.PropertyName, *Val);
					bHasAnyChange = true;
				}
			}
			break;
		}
		case ENetPropertyType::String:
		{
			if (const FString* Val = StringValues.Find(Prop.PropertyName))
			{
				const FString* LastVal = LastSentStringValues.Find(Prop.PropertyName);
				if (!LastVal || *Val != *LastVal)
				{
					StateJson->SetStringField(Prop.PropertyName, *Val);
					LastSentStringValues.Add(Prop.PropertyName, *Val);
					bHasAnyChange = true;
				}
			}
			break;
		}
		default: break;
		}
	}

	if (!bHasAnyChange) return;

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(StateJson.ToSharedRef(), Writer);

	FString FullMsg = FString::Printf(
		TEXT("{\"type\":\"UPDATE\",\"netId\":\"%s\",\"state\":%s}"),
		*NetId, *OutputString
	);

	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UHtmlNetSubsystem* Subsystem = GI->GetSubsystem<UHtmlNetSubsystem>())
		{
			Subsystem->GetTransport()->SendMessage(FullMsg);
		}
	}
}

ENetPropertyType UNetReplicationComponent::GetConfiguredType(const FString& PropName) const
{
	for (const FReplicatedProperty& Prop : CustomProperties)
	{
		if (Prop.PropertyName == PropName) return Prop.Type;
	}
	return ENetPropertyType::Float;
}

void UNetReplicationComponent::ApplyReplicatedState(const TSharedPtr<FJsonObject>& State)
{
	if (!State.IsValid()) return;

	if (State->HasField("transform"))
	{
		const TSharedPtr<FJsonObject>* TransformObj;
		if (State->TryGetObjectField("transform", TransformObj))
		{
			FVector Loc(
				(*TransformObj)->GetNumberField("x"),
				(*TransformObj)->GetNumberField("y"),
				(*TransformObj)->GetNumberField("z")
			);

			FRotator Rot = TargetTransform.GetRotation().Rotator();
			if ((*TransformObj)->HasField("yaw"))
			{
				Rot = FRotator(
					(*TransformObj)->GetNumberField("pitch"),
					(*TransformObj)->GetNumberField("yaw"),
					(*TransformObj)->GetNumberField("roll")
				);
			}

			TargetTransform = FTransform(Rot, Loc);

			if (!bHasReceivedFirstState)
			{
				GetOwner()->SetActorTransform(TargetTransform);
			}
		}
	}

	for (const FReplicatedProperty& Prop : CustomProperties)
	{
		if (!State->HasField(Prop.PropertyName)) continue;

		switch (Prop.Type)
		{
		case ENetPropertyType::Float:
		{
			float Value = (float)State->GetNumberField(Prop.PropertyName);
			if (FloatValues.Contains(Prop.PropertyName))
			{
				FloatValues[Prop.PropertyName] = Value;
			}
			else
			{
				FloatValues.Add(Prop.PropertyName, Value);
			}
			break;
		}
		case ENetPropertyType::Int:
		{
			int32 Value = (int32)State->GetNumberField(Prop.PropertyName);
			if (IntValues.Contains(Prop.PropertyName))
			{
				IntValues[Prop.PropertyName] = Value;
			}
			else
			{
				IntValues.Add(Prop.PropertyName, Value);
			}
			break;
		}
		case ENetPropertyType::Bool:
		{
			bool Value = State->GetBoolField(Prop.PropertyName);
			if (BoolValues.Contains(Prop.PropertyName))
			{
				BoolValues[Prop.PropertyName] = Value;
			}
			else
			{
				BoolValues.Add(Prop.PropertyName, Value);
			}
			break;
		}
		case ENetPropertyType::String:
		{
			FString Value = State->GetStringField(Prop.PropertyName);
			if (StringValues.Contains(Prop.PropertyName))
			{
				StringValues[Prop.PropertyName] = Value;
			}
			else
			{
				StringValues.Add(Prop.PropertyName, Value);
			}
			break;
		}
		default: break;
		}
	}

	bHasReceivedFirstState = true;
	OnStateUpdated.Broadcast();
}

FString UNetReplicationComponent::GenerateCurrentStateJson() const
{
	TSharedPtr<FJsonObject> StateJson = MakeShared<FJsonObject>();

	if (bReplicateTransform)
	{
		FTransform T = GetOwner()->GetActorTransform();
		TSharedPtr<FJsonObject> TransformJson = MakeShared<FJsonObject>();
		TransformJson->SetNumberField("x", T.GetLocation().X);
		TransformJson->SetNumberField("y", T.GetLocation().Y);
		TransformJson->SetNumberField("z", T.GetLocation().Z);
		TransformJson->SetNumberField("yaw", T.Rotator().Yaw);

		StateJson->SetObjectField("transform", TransformJson);
	}

	FString Result;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
	FJsonSerializer::Serialize(StateJson.ToSharedRef(), Writer);

	return Result;
}