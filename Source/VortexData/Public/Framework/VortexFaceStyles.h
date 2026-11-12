#pragma once

#include "CoreMinimal.h"

/** Um estilo de rosto do humano de blocos (Data/Looks/rostos.json; Tools/MakeBlockFace.py --estilo). A escolha fica em
 *  Look.Pieces["block_face"]; o "padrao" usa os olhos de sempre. Num cabeçalho à parte (não no VortexLook.h, incluído
 *  quase no jogo inteiro): mudar aqui recompila só o personagem e o menu. */
struct FVortexFaceStyle
{
    FName Id;
    FString Name;
    FName Pele;   // vazio = humano; "clara"/"escura" = os olhos de cada elfo
};

/** Os estilos na ordem do menu (o primeiro é o padrão). Lidos uma vez do arquivo. */
VORTEXDATA_API const TArray<FVortexFaceStyle>& VortexFaceStyles();
