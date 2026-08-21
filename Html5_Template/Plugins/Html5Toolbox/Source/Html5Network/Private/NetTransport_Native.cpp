#include "NetTransport_Native.h"

#if !PLATFORM_HTML5
void UNetTransport_Native::Connect(const FString& ServerURL, const FString& RoomId)
{
	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}

	FWebSocketsModule* WebSocketModule =
		&FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));

	WebSocket = WebSocketModule->CreateWebSocket(ServerURL);

	WebSocket->OnConnected().AddLambda([this, RoomId]()
	{
		OnMessageReceived.Broadcast(TEXT("{\"type\":\"TRANSPORT_CONNECTED\"}"));
	});

	WebSocket->OnMessage().AddLambda([this](const FString& Message)
	{
		OnMessageReceived.Broadcast(Message);
	});

	WebSocket->OnConnectionError().AddLambda([](const FString& Error)
	{
		UE_LOG(LogTemp, Error, TEXT("Connection Error WS: %s"), *Error);
	});

	WebSocket->Connect();
}

void UNetTransport_Native::SendMessage(const FString& JsonPayload)
{
	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		WebSocket->Send(JsonPayload);
	}
}

void UNetTransport_Native::Disconnect()
{
	if (WebSocket.IsValid())
	{
		WebSocket->Close();
	}
}
#else

void UNetTransport_Native::Connect(
	const FString& ServerURL,
	const FString& RoomId)
{
}

void UNetTransport_Native::SendMessage(
	const FString& JsonPayload)
{
}

void UNetTransport_Native::Disconnect()
{
}

#endif