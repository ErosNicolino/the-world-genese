#include "Framework/VortexJson.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TSharedPtr<FJsonObject> VortexJson::Load(const FString& ProjectPath)
{
    FString Text;
    TSharedPtr<FJsonObject> Json;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / ProjectPath)))
    {
        UE_LOG(LogTemp, Warning, TEXT("VortexJson: não achei %s"), *ProjectPath);
        return nullptr;
    }
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("VortexJson: %s está quebrado (JSON inválido)"), *ProjectPath);
        return nullptr;
    }
    return Json;
}
