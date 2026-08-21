#include "Html5Network.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "Logging/LogMacros.h"
#include "HtmlNetSubsystem.h"
#include "NetReplicationComponent.h"

#define LOCTEXT_NAMESPACE "FHtml5NetworkModule"

// Log category for this module
DEFINE_LOG_CATEGORY_STATIC(LogHtml5Network, Log, All);

void FHtml5NetworkModule::StartupModule()
{
    UE_LOG(LogHtml5Network, Warning, TEXT("========================================"));
    UE_LOG(LogHtml5Network, Warning, TEXT("Html5Network Module Started!"));
    UE_LOG(LogHtml5Network, Warning, TEXT("========================================"));
}

void FHtml5NetworkModule::ShutdownModule()
{
    UE_LOG(LogHtml5Network, Warning, TEXT("========================================"));
    UE_LOG(LogHtml5Network, Warning, TEXT("Html5Network Module Shutdown!"));
    UE_LOG(LogHtml5Network, Warning, TEXT("========================================"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHtml5NetworkModule, Html5Network)