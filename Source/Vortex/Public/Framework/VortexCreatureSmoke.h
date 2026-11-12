#pragma once

#include "CoreMinimal.h"

class AGameModeBase;
class APlayerController;

#if !UE_BUILD_SHIPPING
/** Teste das criaturas do Godot e das missões (-VortexCreatureSmoke[=Espécie,...]). */
namespace VortexCreatureSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Novidades do combate e da água com fotos (-VortexFeatureShots). */
namespace VortexFeatureShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** O Despertar (cartas) e as habilidades de classe com fotos (-VortexClassShots). */
namespace VortexClassShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** O chefe da Caldeira: céu, fases, lava e queimadura com fotos (-VortexBossShots). */
namespace VortexBossShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Coletáveis, domar o cavalo com frutas, montar, galopar e o assobio, com fotos (-VortexMountShots). */
namespace VortexMountShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** O continente: a emenda com o vale e as nove regiões, com fotos (-VortexWorldShots). */
namespace VortexWorldShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** 08/11: a rotina dos moradores em quatro horas do dia, com fotos (-VortexNpcShots). */
namespace VortexNpcShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Jogador com jogador: grupo, duelo, troca e amizade, anfitrião + cliente (-VortexSocialSmoke). */
namespace VortexSocialSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** O "Aplicar" das opções: pendente, tela de carregamento, o chão remonta parado e o DLSS (-VortexApplySmoke). */
namespace VortexApplySmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Passeio de desempenho da cidade para o continente, com tudo ligado (-VortexRoamBench). */
namespace VortexRoamBench
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Bandidos (-VortexBanditSmoke): os 4 tipos nascem vestidos e armados, atacam (flecha de verdade),
 *  o escudo segura pela frente, vencer dá XP; fotos de perto e de um acampamento. */
namespace VortexBanditSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Lojas (-VortexShopSmoke): compra, venda, estoque que acaba, falta de moedas e fotos da janela. */
namespace VortexShopSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Cada arma de perto (-VortexWeaponCloseUp[=tipo,...]): pose, golpe e especial. */
namespace VortexWeaponCloseUp
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Pontos de interesse e portal (-VortexPoiSmoke): visita um de cada tipo e o portal, fotografa e confere o
 *  santuário (bênção), a torre (topo), os baús guardados da mina e do cemitério. */
namespace VortexPoiSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Fauna e flora (-VortexWildlifeSmoke): um lago perto da cidade, o pântano de Lodonegro e Verdalume, com fotos
 *  sem e com os bichos; pássaros no chão e no céu, peixes, vitórias-régias e algas; o bando foge de quem chega;
 *  esporos do cogumelo; a flor que se fecha quando o herói pisa nela. */
class FVortexTerrainData;
namespace VortexWildlifeSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
    /** Água funda perto de Center e um chão seco a até 20 m dela (para pôr o herói na margem). */
    bool FindSite(const FVortexTerrainData& Data, const FVector2D& Center, float MaxR, FVector& Water, FVector& Shore);
}
/** HUD cartoon, minimapa e mapa (-VortexMapShots): fotos da HUD num lago, do mapa nos 3 zooms (o de perto com o detalhe
 *  por bloco) e do marcador no mapa e no minimapa. */
namespace VortexMapShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** A HQ de abertura (-VortexIntroShots): abre, fotografa a página chegando e pintando, um quadro com legenda, o
 *  impacto da estrela, a página virando e o "Acorde", e confere que ela fecha e devolve o controle. */
namespace VortexIntroShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Os heróis novos de frente, parados e golpeando, uma foto por raça (-VortexHeroShots). */
namespace VortexHeroShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
/** Golpe de perto de cada arma contra criaturas paradas em volta do herói (-VortexMeleeSweep). */
namespace VortexMeleeSweep
{
    void Begin(AGameModeBase* Mode, APlayerController* Player);
}
#endif
