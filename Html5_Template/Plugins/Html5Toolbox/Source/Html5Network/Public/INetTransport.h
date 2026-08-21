#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INetTransport.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FString&);

UINTERFACE(BlueprintType)
class HTML5NETWORK_API UNetTransport : public UInterface
{
	GENERATED_BODY()
};

class HTML5NETWORK_API INetTransport
{
	GENERATED_BODY()

public:
	virtual void Connect(const FString& ServerURL, const FString& RoomId) = 0;
	virtual void SendMessage(const FString& JsonPayload) = 0;
	virtual void Disconnect() = 0;

	virtual FOnMessageReceived& GetOnMessageReceived() = 0;
};