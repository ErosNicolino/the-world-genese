#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "VortexMakeClothCommandlet.generated.h"

/**
 * Cria o Chaos Cloth das saias (09/11). O Python da Unreal não expõe isso, por isso é um comando em C++:
 * UnrealEditor-Cmd.exe Vortex.uproject -run=VortexMakeCloth -unattended -nullrhi
 * Para cada malha *_tecido de humano/Pecas e humano/Roupas (o tubo de Tools/Blender/MakeBlockOutfit.py): prende a cintura, solta a
 * barra aos poucos e cria as cápsulas de colisão do quadril e das pernas.
 */
UCLASS()
class UVortexMakeClothCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    virtual int32 Main(const FString& Params) override;
};
