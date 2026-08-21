// NetTransport_HTML5.h
#pragma once

#include "CoreMinimal.h"
#include "INetTransport.h"
#include "NetTransport_HTML5.generated.h"

extern "C"
{
#if PLATFORM_HTML5
	EMSCRIPTEN_KEEPALIVE
#endif
	void OnWebSocketMessage(const char* MessageData);
	void OnWebSocketError(const char* ErrorMsg);
	void OnWebSocketOpen();
}

UCLASS()
class HTML5NETWORK_API UNetTransport_HTML5 : public UObject, public INetTransport
{
	GENERATED_BODY()

public:
	UNetTransport_HTML5();

	virtual void Connect(const FString& ServerURL, const FString& RoomId) override;
	virtual void SendMessage(const FString& JsonPayload) override;
	virtual void Disconnect() override;
	virtual FOnMessageReceived& GetOnMessageReceived() override { return OnMessageReceived; }

	FOnMessageReceived OnMessageReceived;
	static UNetTransport_HTML5* GInstance;
};