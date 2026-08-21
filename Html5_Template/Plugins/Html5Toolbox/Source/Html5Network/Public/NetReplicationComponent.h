#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dom/JsonObject.h"
#include "NetReplicationComponent.generated.h"

UENUM(BlueprintType)
enum class ENetPropertyType : uint8
{
	Float,
	Int,
	Bool,
	String,
	Transform
};

USTRUCT(BlueprintType)
struct FReplicatedProperty
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication")
		FString PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication")
		ENetPropertyType Type = ENetPropertyType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication", meta = (EditCondition = "Type==ENetPropertyType::Float"))
		float ChangeThreshold = 0.01f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRPCReceivedDelegate, FString, EventName, const FString&, PayloadJson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStateUpdatedDelegate);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HTML5NETWORK_API UNetReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNetReplicationComponent();

	UPROPERTY(EditAnywhere, Category = "Replication")
		bool bReplicateTransform = true;

	UPROPERTY(EditAnywhere, Category = "Replication", meta = (EditCondition = "bReplicateTransform"))
		bool bReplicateRotation = true;

	UPROPERTY(EditAnywhere, Category = "Replication")
		TArray<FReplicatedProperty> CustomProperties;

	UPROPERTY(EditAnywhere, Category = "Replication", meta = (ClampMin = "0.02", ClampMax = "2.0"))
		float ReplicationRate = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Replication")
		float InterpolationSpeed = 10.f;

	UPROPERTY(BlueprintReadWrite, Category = "Replication")
		bool bIsOwner = false;

	UPROPERTY(BlueprintReadWrite, Category = "Replication")
		FString NetId;

	UPROPERTY(BlueprintAssignable, Category = "Replication")
		FOnRPCReceivedDelegate OnRPCReceived;

	UPROPERTY(BlueprintAssignable, Category = "Replication")
		FOnStateUpdatedDelegate OnStateUpdated;

	UFUNCTION(BlueprintCallable, Category = "Replication")
		void SetFloatValue(const FString& PropName, float Value);

	UFUNCTION(BlueprintCallable, Category = "Replication")
		void SetIntValue(const FString& PropName, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Replication")
		void SetBoolValue(const FString& PropName, bool Value);

	UFUNCTION(BlueprintCallable, Category = "Replication")
		void SetStringValue(const FString& PropName, const FString& Value);

	UFUNCTION(BlueprintPure, Category = "Replication")
		float GetFloatValue(const FString& PropName, float DefaultValue = 0.f) const;

	UFUNCTION(BlueprintPure, Category = "Replication")
		int32 GetIntValue(const FString& PropName, int32 DefaultValue = 0) const;

	UFUNCTION(BlueprintPure, Category = "Replication")
		bool GetBoolValue(const FString& PropName, bool DefaultValue = false) const;

	UFUNCTION(BlueprintPure, Category = "Replication")
		FString GetStringValue(const FString& PropName, const FString& DefaultValue = "") const;

	UFUNCTION(BlueprintCallable, Category = "Replication")
		void SendCustomRPC(const FString& EventName, const FString& PayloadJson, bool bIncludeSelf = false);

	UFUNCTION(BlueprintCallable, Category = "Replication")
		FString GenerateCurrentStateJson() const;

	void ApplyReplicatedState(const TSharedPtr<FJsonObject>& State);

	void HandleRawRPC(const FString& EventName, const TSharedPtr<FJsonObject>& Payload);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TMap<FString, float> FloatValues;
	TMap<FString, int32> IntValues;
	TMap<FString, bool> BoolValues;
	TMap<FString, FString> StringValues;

	TMap<FString, float> LastSentFloatValues;
	TMap<FString, int32> LastSentIntValues;
	TMap<FString, bool> LastSentBoolValues;
	TMap<FString, FString> LastSentStringValues;

	FTransform TargetTransform;
	FTransform LastSentTransform;
	bool bHasReceivedFirstState = false;

	float AccumulatedTime = 0.f;

	void SendCurrentState();
	bool HasTransformChangedSignificantly() const;
	ENetPropertyType GetConfiguredType(const FString& PropName) const;
};