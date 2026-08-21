#pragma once

#include "CoreMinimal.h"
#include "INetTransport.h"

class FNetTransportFactory
{
public:
	static INetTransport* CreateTransport(UObject* Outer);
};