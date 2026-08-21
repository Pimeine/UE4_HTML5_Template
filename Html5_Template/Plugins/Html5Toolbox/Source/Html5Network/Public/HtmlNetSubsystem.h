#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "INetTransport.h"
#include "Dom/JsonObject.h"
#include "HtmlNetSubsystem.generated.h"

UENUM(BlueprintType)
enum class ENetConnectionState : uint8
{
	Disconnected,
	Connecting,
	Connected,
	Failed
};

USTRUCT(BlueprintType)
struct FRoomInfo
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadOnly, Category = "Network")
		FString RoomId;

	UPROPERTY(BlueprintReadOnly, Category = "Network")
		int32 PlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Network")
		int32 MaxPlayers = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStateChanged, ENetConnectionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorSpawnRequested, const FString, NetId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoteActorSpawnRequestedWithType, const FString&, NetId, const FString&, ActorType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomListReceived, const TArray<FRoomInfo>&, Rooms);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinRefused, const FString&, Reason);

UCLASS()
class HTML5NETWORK_API UHtmlNetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, Category = "Network")
		bool bIsConnected = false;

	UFUNCTION(BlueprintCallable, Category = "Network")
		void ConnectToServer(const FString& ServerURL, const FString& RoomId, int32 MaxPlayers = 8);

	UFUNCTION(BlueprintCallable, Category = "Network")
		void DisconnectFromServer();

	UFUNCTION(BlueprintPure, Category = "Network")
		ENetConnectionState GetConnectionState() const { return ConnectionState; }

	UFUNCTION(BlueprintPure, Category = "Network")
		FString GetLocalClientId() const { return LocalClientId; }

	UFUNCTION(BlueprintPure, Category = "Network")
		FString GetLocalNetId() const { return LocalNetId; }


	void RegisterActor(const FString& NetId, class UNetReplicationComponent* Comp);
	void UnregisterActor(const FString& NetId);

	UFUNCTION(BlueprintCallable, Category = "Network")
		void SendRPC(const FString& NetId, const FString& EventName, const FString& PayloadJson, bool bIncludeSelf = false);

	UFUNCTION(BlueprintCallable, Category = "Network")
		void RequestSpawn(const FString& NetId, const FString& InitialStateJson, const FString& ActorType = TEXT("ACTOR"));

	UFUNCTION(BlueprintCallable, Category = "Network")
		void RequestDespawn(const FString& NetId);

	INetTransport* GetTransport() const { return Transport; }

	UPROPERTY(BlueprintAssignable, Category = "Network")
		FOnConnectionStateChanged OnConnectionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Network")
		FOnActorSpawnRequested OnRemoteActorSpawnRequested;

	UPROPERTY(BlueprintAssignable, Category = "Network")
		FOnRemoteActorSpawnRequestedWithType OnRemoteActorSpawnRequestedWithType;

	UFUNCTION(BlueprintCallable, Category = "Network")
		void OnRemoteActorSpawned(const FString& NetId, class UNetReplicationComponent* Comp);

	UFUNCTION(BlueprintCallable, Category = "Network")
		void RequestRoomList();

	UFUNCTION(BlueprintCallable, Category = "Network")
		void JoinRoom(const FString& RoomId, int32 MaxPlayers = 8);

	UPROPERTY(BlueprintAssignable, Category = "Network")
		FOnRoomListReceived OnRoomListReceived;

	UPROPERTY(BlueprintAssignable, Category = "Network")
		FOnJoinRefused OnJoinRefused;

private:
	UPROPERTY()
	UObject* TransportObject = nullptr;

	UPROPERTY()
	FString LocalNetId;

	INetTransport* Transport = nullptr;
	TMap<FString, TWeakObjectPtr<class UNetReplicationComponent>> RegisteredActors;

	ENetConnectionState ConnectionState = ENetConnectionState::Disconnected;
	FString LocalClientId;
	FString CurrentRoomId;
	int32 PendingMaxPlayers = 8;

	TMap<FString, TArray<TSharedPtr<FJsonObject>>> PendingMessagesForUnregisteredActors;

	void HandleInitPlayers(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleMessage(const FString& JsonMsg);
	void HandleJoin(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleUpdate(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleSpawn(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleDespawn(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleRPC(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleClientLeft(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleTransportConnected(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleRoomList(const TSharedPtr<FJsonObject>& JsonObject);
	void HandleJoinRefused(const TSharedPtr<FJsonObject>& JsonObject);
};