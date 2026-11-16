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

/** 14/11: as raças no corpo de blocos do humano (Data/Looks/racas.json): a pele, o traço (orelhas, chifres, barba...) e a
 *  largura. As escolhas ficam em Look.Pieces["raca_pele"] e ["raca_traco"]; "peca_orelha" é a malha do traço. */
struct FVortexRaceChoice
{
    FName Id;
    FString Name;
    FName Faces;   // na pele: o grupo de olhos (FVortexFaceStyle::Pele)
    FName Face;    // na pele: os olhos padrão
};

struct FVortexRaceLook
{
    FName Race;
    float Width = 1.f;
    TArray<FVortexRaceChoice> Skins, Traits;   // a primeira é a padrão
    FString TraitTitle, TraitPiece;            // a malha: "P_chifre_{op}_{pele}"
    FName FemaleTrait;                         // o traço padrão do corpo feminino (vazio = o primeiro)
};

/** A raça no arquivo, ou nulo (o humano não tem escolhas). */
VORTEXDATA_API const FVortexRaceLook* VortexRaceLook(FName Race);
