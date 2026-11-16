#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace VortexJson
{
    /**
     * Um JSON de dados do jogo (caminho a partir da pasta do projeto: "Data/Rpg/balance.json"). Nulo, com um aviso no
     * log, se o arquivo falta ou está quebrado. 16/11: eram as mesmas quatro linhas copiadas em 44 lugares, cada um
     * tratando o erro de um jeito (a maioria calada).
     */
    VORTEXDATA_API TSharedPtr<FJsonObject> Load(const FString& ProjectPath);
}
