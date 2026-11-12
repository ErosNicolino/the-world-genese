#pragma once

#include "CoreMinimal.h"

/** Os 6 espaços de roupa separada do humano de blocos. A escolha fica em Look.Pieces[Key] (ausente = nada nesse espaço),
 *  o mapa que já é salvo e vai pela rede: heróis antigos seguem iguais. Num cabeçalho à parte (não no VortexLook.h). */
struct FVortexWearSlot
{
    FName Key;
    FString Label;
};

/** Uma peça de Data/Looks/pecas.json (Tools/Blender/piece_designs.py e Tools/ImportBlockArmor.py com VORTEX_PECAS=1). */
struct FVortexWearPiece
{
    FName Id, Slot;       // Slot = FVortexWearSlot::Key
    FName Body;           // "male" ou "female"
    FString Name, Mesh;
    bool bHidesHair = false;
    FName Race;           // vazio = todas as raças; "elfo" = só no menu do elfo
    FName HairMode;       // "tudo", "franja" (corta o cabelo acima da aba) ou "mostra"; vazio = pelo bHidesHair
    bool bCloth = false;  // tem a malha <Mesh>_tecido (Chaos Cloth: o comando VortexMakeCloth)
};

VORTEXDATA_API const TArray<FVortexWearSlot>& VortexWearSlots();
/** Na ordem do arquivo. */
VORTEXDATA_API const TArray<FVortexWearPiece>& VortexWearPieces();
VORTEXDATA_API const FVortexWearPiece* VortexWearPiece(FName Id);
/** Altura (cm, no modelo em repouso) em que os chapéus "franja" cortam o cabelo. */
VORTEXDATA_API float VortexWearFranjaZ();
