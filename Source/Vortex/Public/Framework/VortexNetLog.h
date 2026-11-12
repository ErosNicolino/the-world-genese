#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/**
 * Diário da rede (sala da Epic, conexão, viagens e quedas) em Saved/Logs/VortexRede.log.
 * O jogo empacotado (Shipping) não grava o log da Unreal; este grava sempre, para o anfitrião e o
 * convidado mandarem o arquivo quando a sala der problema. Acima de 2 MB vira VortexRede.anterior.log.
 */
namespace VortexNetLog
{
    inline void Write(const FString& Message)
    {
        const FString Path = FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("VortexRede.log");
        IFileManager& Files = IFileManager::Get();
        Files.MakeDirectory(*FPaths::GetPath(Path), true);
        // 09/11: acima de 2 MB guarda o anterior, em vez de apagar justamente o histórico da queda
        if (Files.FileSize(*Path) > 2 * 1024 * 1024) Files.Move(*(FPaths::GetPath(Path) / TEXT("VortexRede.anterior.log")), *Path, true);
        const FString Line = FString::Printf(TEXT("[%s] %s\n"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")), *Message);
        FFileHelper::SaveStringToFile(Line, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &Files, FILEWRITE_Append);
        UE_LOG(LogTemp, Display, TEXT("VORTEX_REDE %s"), *Message);
    }
}
