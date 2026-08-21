#include "HtmlNetSubsystem.h"

#include "NetTransportFactory.h"
#include "NetReplicationComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "NetTransport_HTML5.h"
#include "NetTransport_Native.h"
#include "GameFramework/Actor.h"

void UHtmlNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if PLATFORM_HTML5
	UNetTransport_HTML5* Obj = NewObject<UNetTransport_HTML5>(this);
	TransportObject = Obj;
	Transport = static_cast<INetTransport*>(Obj);
#else
	UNetTransport_Native* Obj = NewObject<UNetTransport_Native>(this);
	TransportObject = Obj;
	Transport = static_cast<INetTransport*>(Obj);
#endif

	UE_LOG(LogTemp, Error, TEXT("Transport: %p"), Transport);
	UE_LOG(LogTemp, Error, TEXT("Transport valid: %s"), Transport ? TEXT("true") : TEXT("false"));

	if (!Transport)
	{
		UE_LOG(LogTemp, Fatal, TEXT("TRANSPORT CREATION FAILED!"));
		return;
	}

	Transport->GetOnMessageReceived().AddUObject(this, &UHtmlNetSubsystem::HandleMessage);
	LocalClientId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
}

void UHtmlNetSubsystem::Deinitialize()
{
	if (Transport)
	{
		Transport->Disconnect();
	}
	Super::Deinitialize();
}

void UHtmlNetSubsystem::ConnectToServer(const FString& ServerURL, const FString& RoomId, int32 MaxPlayers)
{
	if (ConnectionState == ENetConnectionState::Connecting ||
		ConnectionState == ENetConnectionState::Connected)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ConnectToServer] Already connecting or connected!"));
		return;
	}

	CurrentRoomId = RoomId;
	PendingMaxPlayers = MaxPlayers;
	ConnectionState = ENetConnectionState::Connecting;
	OnConnectionStateChanged.Broadcast(ConnectionState);

	Transport->Connect(ServerURL, RoomId);
}

void UHtmlNetSubsystem::DisconnectFromServer()
{
	Transport->Disconnect();
	ConnectionState = ENetConnectionState::Disconnected;
	bIsConnected = false;
	OnConnectionStateChanged.Broadcast(ConnectionState);
}

void UHtmlNetSubsystem::HandleTransportConnected(const TSharedPtr<FJsonObject>& JsonObject)
{
	UE_LOG(LogTemp, Log, TEXT("[TRANSPORT] WebSocket connected, sending JOIN_ROOM for room: %s"), *CurrentRoomId);

	JoinRoom(CurrentRoomId, PendingMaxPlayers);
}

void UHtmlNetSubsystem::RequestRoomList()
{
	if (!Transport) return;
	Transport->SendMessage(TEXT("{\"type\":\"LIST_ROOMS\"}"));
}

void UHtmlNetSubsystem::JoinRoom(const FString& RoomId, int32 MaxPlayers)
{
	if (!Transport) return;

	CurrentRoomId = RoomId;

	FString Msg = FString::Printf(
		TEXT("{\"type\":\"JOIN_ROOM\",\"roomId\":\"%s\",\"maxPlayers\":%d,\"clientId\":\"%s\"}"),
		*RoomId, MaxPlayers, *LocalClientId
	);
	Transport->SendMessage(Msg);
}

void UHtmlNetSubsystem::HandleRoomList(const TSharedPtr<FJsonObject>& JsonObject)
{
	TArray<FRoomInfo> RoomList;

	const TArray<TSharedPtr<FJsonValue>>* RoomsArray = nullptr;
	if (JsonObject->TryGetArrayField("rooms", RoomsArray))
	{
		for (const auto& RoomValue : *RoomsArray)
		{
			if (!RoomValue.IsValid()) continue;

			const TSharedPtr<FJsonObject>* RoomObj = nullptr;
			if (RoomValue->TryGetObject(RoomObj) && RoomObj->IsValid())
			{
				FRoomInfo Info;
				(*RoomObj)->TryGetStringField("roomId", Info.RoomId);

				int32 PlayerCount = 0, MaxPlayers = 0;
				(*RoomObj)->TryGetNumberField("playerCount", PlayerCount);
				(*RoomObj)->TryGetNumberField("maxPlayers", MaxPlayers);

				Info.PlayerCount = PlayerCount;
				Info.MaxPlayers = MaxPlayers;

				RoomList.Add(Info);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ROOM_LIST] Received %d rooms"), RoomList.Num());
	OnRoomListReceived.Broadcast(RoomList);
}

void UHtmlNetSubsystem::HandleJoinRefused(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString Reason;
	JsonObject->TryGetStringField("reason", Reason);

	FString RoomId;
	JsonObject->TryGetStringField("roomId", RoomId);

	UE_LOG(LogTemp, Warning, TEXT("[JOIN_REFUSED] Room=%s Reason=%s"), *RoomId, *Reason);

	ConnectionState = ENetConnectionState::Failed;
	OnConnectionStateChanged.Broadcast(ConnectionState);
	OnJoinRefused.Broadcast(Reason);
}

void UHtmlNetSubsystem::RegisterActor(const FString& NetId, UNetReplicationComponent* Comp)
{
	RegisteredActors.Add(NetId, Comp);

	if (TArray<TSharedPtr<FJsonObject>>* Pending = PendingMessagesForUnregisteredActors.Find(NetId))
	{
		for (const auto& State : *Pending)
		{
			Comp->ApplyReplicatedState(State);
		}
		PendingMessagesForUnregisteredActors.Remove(NetId);
	}
}

void UHtmlNetSubsystem::UnregisterActor(const FString& NetId)
{
	RegisteredActors.Remove(NetId);
}

void UHtmlNetSubsystem::SendRPC(const FString& NetId, const FString& EventName, const FString& PayloadJson, bool bIncludeSelf)
{
	if (ConnectionState != ENetConnectionState::Connected) return;

	FString Msg = FString::Printf(
		TEXT("{\"type\":\"RPC\",\"netId\":\"%s\",\"eventName\":\"%s\",\"payload\":%s,\"includeSelf\":%s}"),
		*NetId, *EventName, *PayloadJson, bIncludeSelf ? TEXT("true") : TEXT("false")
	);
	Transport->SendMessage(Msg);
}

void UHtmlNetSubsystem::RequestSpawn(const FString& NetId, const FString& InitialStateJson, const FString& ActorType)
{
	FString Msg = FString::Printf(
		TEXT("{\"type\":\"SPAWN\",\"netId\":\"%s\",\"actorType\":\"%s\",\"state\":%s}"),
		*NetId, *ActorType, *InitialStateJson
	);
	Transport->SendMessage(Msg);
}

void UHtmlNetSubsystem::RequestDespawn(const FString& NetId)
{
	FString Msg = FString::Printf(TEXT("{\"type\":\"DESPAWN\",\"netId\":\"%s\"}"), *NetId);
	Transport->SendMessage(Msg);
}

void UHtmlNetSubsystem::HandleInitPlayers(const TSharedPtr<FJsonObject>& JsonObject)
{
	const TArray<TSharedPtr<FJsonValue>>* ExistingActorsArray = nullptr;
	if (JsonObject->TryGetArrayField("existingActors", ExistingActorsArray))
	{
		for (const auto& ActorValue : *ExistingActorsArray)
		{
			if (!ActorValue.IsValid()) continue;

			const TSharedPtr<FJsonObject>* ActorObj = nullptr;
			if (ActorValue->TryGetObject(ActorObj) && ActorObj->IsValid())
			{
				FString ActorNetId;
				(*ActorObj)->TryGetStringField("netId", ActorNetId);

				FString ActorTypeStr;
				(*ActorObj)->TryGetStringField("actorType", ActorTypeStr);

				const TSharedPtr<FJsonObject>* StateObj;
				bool bHasState = (*ActorObj)->TryGetObjectField("state", StateObj);

				if (TWeakObjectPtr<UNetReplicationComponent>* CompPtr = RegisteredActors.Find(ActorNetId))
				{
					if (CompPtr->IsValid())
					{
						UE_LOG(LogTemp, Log, TEXT("[INIT] Actor already registered, applying state directly: %s"), *ActorNetId);
						if (bHasState)
						{
							(*CompPtr)->ApplyReplicatedState(*StateObj);
						}
						continue;
					}
					else
					{
						RegisteredActors.Remove(ActorNetId);
					}
				}

				if (bHasState)
				{
					PendingMessagesForUnregisteredActors.FindOrAdd(ActorNetId).Add(*StateObj);
				}

				OnRemoteActorSpawnRequestedWithType.Broadcast(ActorNetId, ActorTypeStr);

				UE_LOG(LogTemp, Log, TEXT("[INIT] Remote actor pending: %s with type: %s"), *ActorNetId, *ActorTypeStr);
			}
		}
	}
}

void UHtmlNetSubsystem::HandleMessage(const FString& JsonMsg)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonMsg);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid JSON Message : %s"), *JsonMsg);
		return;
	}

	FString Type;
	if (!JsonObject->TryGetStringField("type", Type)) return;

	if (Type == "INIT_PLAYERS") HandleInitPlayers(JsonObject);
	else if (Type == "JOINED")       HandleJoin(JsonObject);
	else if (Type == "UPDATE" || Type == "ACTOR_STATE" || Type == "UPDATE_PLAYER_TRANSFORM") HandleUpdate(JsonObject);
	else if (Type == "SPAWN")   HandleSpawn(JsonObject);
	else if (Type == "DESPAWN") HandleDespawn(JsonObject);
	else if (Type == "RPC")     HandleRPC(JsonObject);
	else if (Type == "CLIENT_LEFT") HandleClientLeft(JsonObject);
	else if (Type == "TRANSPORT_CONNECTED") HandleTransportConnected(JsonObject);
	else if (Type == "ROOM_LIST")           HandleRoomList(JsonObject);
	else if (Type == "JOIN_REFUSED")        HandleJoinRefused(JsonObject);
}

void UHtmlNetSubsystem::HandleJoin(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString ServerClientId;
	JsonObject->TryGetStringField("clientId", ServerClientId);
	LocalClientId = ServerClientId;

	FString MyNetId;
	JsonObject->TryGetStringField("netId", MyNetId);
	LocalNetId = MyNetId;

	ConnectionState = ENetConnectionState::Connected;
	bIsConnected = true;
	OnConnectionStateChanged.Broadcast(ConnectionState);
}

void UHtmlNetSubsystem::HandleUpdate(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString NetId;
	if (!JsonObject->TryGetStringField("netId", NetId)) return;

	const TSharedPtr<FJsonObject>* StateObj;
	if (!JsonObject->TryGetObjectField("state", StateObj)) return;

	if (auto* CompPtr = RegisteredActors.Find(NetId))
	{
		if (CompPtr->IsValid())
		{
			(*CompPtr)->ApplyReplicatedState(*StateObj);
			return;
		}
	}

	PendingMessagesForUnregisteredActors.FindOrAdd(NetId).Add(*StateObj);
}

void UHtmlNetSubsystem::HandleSpawn(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString NetId, OwnerId, ActorType;

	JsonObject->TryGetStringField("netId", NetId);
	JsonObject->TryGetStringField("ownerId", OwnerId);
	JsonObject->TryGetStringField("actorType", ActorType);

	const TSharedPtr<FJsonObject>* StateObj;
	bool bHasState = JsonObject->TryGetObjectField("state", StateObj);

	if (TWeakObjectPtr<UNetReplicationComponent>* CompPtr = RegisteredActors.Find(NetId))
	{
		if (CompPtr->IsValid())
		{
			if (bHasState)
			{
				(*CompPtr)->ApplyReplicatedState(*StateObj);
			}
			return;
		}
		RegisteredActors.Remove(NetId);
	}

	OnRemoteActorSpawnRequested.Broadcast(NetId);
	OnRemoteActorSpawnRequestedWithType.Broadcast(NetId, ActorType);

	if (bHasState)
	{
		PendingMessagesForUnregisteredActors.FindOrAdd(NetId).Add(*StateObj);
	}
}

void UHtmlNetSubsystem::HandleDespawn(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString NetId;
	if (!JsonObject->TryGetStringField("netId", NetId)) return;

	if (auto* CompPtr = RegisteredActors.Find(NetId))
	{
		if (CompPtr->IsValid())
		{
			CompPtr->Get()->GetOwner()->Destroy();
		}
	}
	RegisteredActors.Remove(NetId);
}

void UHtmlNetSubsystem::HandleRPC(
	const TSharedPtr<FJsonObject>& JsonObject)
{
	FString NetId;
	FString EventName;

	JsonObject->TryGetStringField(TEXT("netId"), NetId);
	JsonObject->TryGetStringField(TEXT("eventName"), EventName);

	FString PayloadJson = TEXT("{}");

	const TSharedPtr<FJsonObject>* PayloadObj = nullptr;

	if (JsonObject->TryGetObjectField(TEXT("payload"), PayloadObj)
		&& PayloadObj != nullptr
		&& PayloadObj->IsValid())
	{
		TSharedRef<TJsonWriter<>> Writer =
			TJsonWriterFactory<>::Create(&PayloadJson);

		FJsonSerializer::Serialize(
			PayloadObj->ToSharedRef(),
			Writer
		);
	}

	if (TWeakObjectPtr<UNetReplicationComponent>* CompPtr =
		RegisteredActors.Find(NetId))
	{
		if (CompPtr->IsValid())
		{
			(*CompPtr)->OnRPCReceived.Broadcast(
				EventName,
				PayloadJson
			);
		}
	}
}

void UHtmlNetSubsystem::HandleClientLeft(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString ClientId;
	JsonObject->TryGetStringField("clientId", ClientId);

	if (ClientId == LocalClientId)
	{
		bIsConnected = false;
		ConnectionState = ENetConnectionState::Disconnected;
		OnConnectionStateChanged.Broadcast(ConnectionState);
	}
	UE_LOG(LogTemp, Log, TEXT("CLient Disconnected : %s"), *ClientId);
}

void UHtmlNetSubsystem::OnRemoteActorSpawned(const FString& NetId, class UNetReplicationComponent* Comp)
{
	if (!Comp) return;

	Comp->NetId = NetId;
	RegisterActor(NetId, Comp);

	UE_LOG(LogTemp, Warning, TEXT("[OnRemoteActorSpawned] Registered remote actor: %s"), *NetId);
}