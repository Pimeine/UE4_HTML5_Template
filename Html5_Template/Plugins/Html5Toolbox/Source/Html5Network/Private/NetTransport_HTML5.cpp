#include "NetTransport_HTML5.h"

UNetTransport_HTML5::UNetTransport_HTML5()
{
	UE_LOG(LogTemp, Log, TEXT("UNetTransport_HTML5 constructed"));
}


#if PLATFORM_HTML5

#include "emscripten.h"
#include "Async/Async.h"

UNetTransport_HTML5* UNetTransport_HTML5::GInstance = nullptr;

void UNetTransport_HTML5::Connect(const FString& ServerURL, const FString& RoomId)
{
	GInstance = this;

	FTCHARToUTF8 UrlConverter(*ServerURL);
	const char* UrlCStr = UrlConverter.Get();

	EM_ASM(
		{
			var bridge = Module.WebsocketBridge;

			bridge.onMessageCallback = function(data) {
				if (typeof data == 'string' || data instanceof String) {
					var len = lengthBytesUTF8(data) + 1;
					var buf = _malloc(len);
					stringToUTF8(data, buf, len);
					Module.asm._OnWebSocketMessage(buf);
					_free(buf);
				}
			};

			bridge.onErrorCallback = function(error) {
				var msg = (typeof error == 'string') ? error : "WebSocket error";
				var len = lengthBytesUTF8(msg) + 1;
				var buf = _malloc(len);
				stringToUTF8(msg, buf, len);
				Module.asm._OnWebSocketError(buf);
				_free(buf);
			};

			bridge.onOpenCallback = function() {
				Module.asm._OnWebSocketOpen();
			};

			bridge.connect(UTF8ToString($0));
		},
		UrlCStr);
}

void UNetTransport_HTML5::SendMessage(const FString& JsonPayload)
{
	const char* JsonCStr = TCHAR_TO_UTF8(*JsonPayload);

	EM_ASM(
		{
			var bridge = Module.WebsocketBridge;
			bridge.send(UTF8ToString($0));
		},
		JsonCStr);
}

void UNetTransport_HTML5::Disconnect()
{
	EM_ASM(
		{
			var bridge = Module.WebsocketBridge;
			bridge.disconnect();
		});

	GInstance = nullptr;
}

extern "C"
{
	EMSCRIPTEN_KEEPALIVE
		void OnWebSocketMessage(const char* MessageData)
	{
		if (UNetTransport_HTML5::GInstance)
		{
			FString Message = UTF8_TO_TCHAR(MessageData);

			AsyncTask(ENamedThreads::GameThread, [Message]()
			{
				if (UNetTransport_HTML5::GInstance)
				{
					UNetTransport_HTML5::GInstance->OnMessageReceived.Broadcast(Message);
				}
			});
		}
	}

	EMSCRIPTEN_KEEPALIVE
		void OnWebSocketError(const char* ErrorMsg)
	{
		FString Error = UTF8_TO_TCHAR(ErrorMsg);
		UE_LOG(LogTemp, Error, TEXT("WebSocket Error: %s"), *Error);
	}

	EMSCRIPTEN_KEEPALIVE
		void OnWebSocketOpen()
	{
		UE_LOG(LogTemp, Log, TEXT("WebSocket Connected!"));

		if (UNetTransport_HTML5::GInstance)
		{
			AsyncTask(ENamedThreads::GameThread, []()
			{
				if (UNetTransport_HTML5::GInstance)
				{
					UNetTransport_HTML5::GInstance->OnMessageReceived.Broadcast(TEXT("{\"type\":\"TRANSPORT_CONNECTED\"}"));
				}
			});
	}
}
}

#else

UNetTransport_HTML5* UNetTransport_HTML5::GInstance = nullptr;

void UNetTransport_HTML5::Connect(const FString& ServerURL, const FString& RoomId)
{
	UE_LOG(LogTemp, Warning, TEXT("UNetTransport_HTML5::Connect available only on HTML5."));
}

void UNetTransport_HTML5::SendMessage(const FString& JsonPayload)
{
	UE_LOG(LogTemp, Warning, TEXT("UNetTransport_HTML5::SendMessage available only on HTML5."));
}

void UNetTransport_HTML5::Disconnect()
{
	UE_LOG(LogTemp, Warning, TEXT("UNetTransport_HTML5::Disconnect available only on HTML5."));
}

#endif