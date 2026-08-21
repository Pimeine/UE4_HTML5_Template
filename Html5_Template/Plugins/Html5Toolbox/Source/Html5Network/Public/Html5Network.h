#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FHtml5NetworkModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static inline FHtml5NetworkModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FHtml5NetworkModule>("Html5Network");
    }

    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("Html5Network");
    }
};