#pragma once

#include "CoreMinimal.h"

struct FVortexItemDef;

/**
 * Lojas, como as do Godot (Main.Shop.cs e Main.Vendors.cs):
 * - Ducci, o mercador mago ambulante: 8 armas de raridade sorteada, 3 armaduras ou acessórios e as
 *   poções de sempre; compra tudo da mochila por 30%.
 * - Comerciantes de ofício da Cidade do Santuário, cada um vende e compra do seu jeito: Bram (forja),
 *   Garrick (armaria), Rosa (alfaiataria), Ilsa (ervas e poções) e Marta (taverna).
 * - Mercadores regionais (caravana das trilhas, Sahil nas dunas, Tavian na mata): só o básico, sem
 *   acabar, e compram os achados como o Ducci.
 * O estoque é sorteado por dia do mundo (mesma semente para todos no mesmo mundo e no mesmo dia) e
 * acaba: o que o herói comprou some até o dia seguinte. Tudo aqui é cálculo puro; quem compra e
 * vende de verdade é o servidor (UVortexHeroSheet::ServerShopBuy/ServerShopSell).
 */
namespace VortexShop
{
    struct FOffer
    {
        FName Item;
        int32 Price = 0;
        bool bUnlimited = false;
        int32 Qty = 1;           // quantas o comerciante tem hoje
    };

    struct FVendorInfo
    {
        FName Id;
        FString Seller, Title, Greeting, BuyLine, SellHelp;
        bool bRestocksDaily = true;   // regionais: suprimentos todos os dias, sem acabar
    };

    /** Comerciante conhecido? (ducci, bram, garrick, rosa, ilsa, marta, caravana, solcanion, verdalume) */
    VORTEX_API bool IsVendor(FName Vendor);
    VORTEX_API FVendorInfo Info(FName Vendor);
    /** NPC que vende (a loja só abre perto dele): "solcanion" é o Sahil, "verdalume" o Tavian. */
    VORTEX_API FName NpcOf(FName Vendor);
    /** Estoque do dia (sempre igual para o mesmo mundo, comerciante e dia). */
    /** 07/11: HeroLevel > 0 = o equipamento à venda vem no nível do herói (e o preço junto) e o que se usa (poções,
     *  comida) custa mais nos níveis altos, como cura mais. */
    VORTEX_API TArray<FOffer> Stock(FName Vendor, const FString& WorldId, int32 Day, int32 HeroLevel = 0);
    /** 07/11: a forja do Bram: subir o item 1 nível (até o nível do herói): moedas e o material da raridade (sucata de
     *  ferro, obsidiana ou fragmento de colosso). false se já está no nível do herói. */
    VORTEX_API TArray<FOffer> StockBase(FName Vendor, const FString& WorldId, int32 Day);
    VORTEX_API bool ForgeCost(const FVortexItemDef& Item, int32 HeroLevel, int32& OutCoins, FName& OutMaterial, int32& OutCount, int32& OutLevel);
    /** Preço de compra de um item (o preço do arsenal ou a tabela do Godot por raridade). */
    VORTEX_API int32 BuyPrice(const FVortexItemDef& Def);
    /** Quanto o comerciante paga por 1 unidade (0 = não compra). */
    VORTEX_API int32 Pays(FName Vendor, const FVortexItemDef& Def);
    /** Chave do que o herói comprou hoje (repetida uma vez por unidade). */
    VORTEX_API FName BoughtKey(FName Vendor, int32 Index);
}
