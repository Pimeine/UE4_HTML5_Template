#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/Guid.h"
#include "INetTransport.h"
#include "NetTransport_Native.generated.h"

#if !PLATFORM_HTML5
#include "IWebSocket.h"
#include "WebSocketsModule.h"
#endif

UCLASS()
class HTML5NETWORK_API UNetTransport_Native : public UObject, public INetTransport
{
	GENERATED_BODY()

#if !PLATFORM_HTML5
		TSharedPtr<IWebSocket> WebSocket;
#endif

public:
	virtual void Connect(const FString& ServerURL, const FString& RoomId) override;
	virtual void SendMessage(const FString& JsonPayload) override;
	virtual void Disconnect() override;
	virtual FOnMessageReceived& GetOnMessageReceived() override { return OnMessageReceived; }

	FOnMessageReceived OnMessageReceived;
};