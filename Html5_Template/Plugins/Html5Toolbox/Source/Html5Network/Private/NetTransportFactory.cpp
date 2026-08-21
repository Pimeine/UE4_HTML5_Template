#include "NetTransportFactory.h"
#include "NetTransport_HTML5.h"
#include "NetTransport_Native.h"

INetTransport* FNetTransportFactory::CreateTransport(UObject* Outer)
{
	INetTransport* Result = nullptr;

#if PLATFORM_HTML5
	UNetTransport_HTML5* TransportObj = NewObject<UNetTransport_HTML5>(Outer);
	if (TransportObj)
	{
		UE_LOG(LogTemp, Warning, TEXT("HTML5 Transport Created and cast"));
		Result = static_cast<INetTransport*>(TransportObj);
	}
#else
	UNetTransport_Native* TransportObj = NewObject<UNetTransport_Native>(Outer);
	if (TransportObj)
	{
		Result = static_cast<INetTransport*>(TransportObj);
	}
#endif

	return Result;
}