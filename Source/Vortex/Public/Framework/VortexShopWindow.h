#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;

/**
 * Janela da loja (abre pela conversa com o comerciante: ação open_shop do diálogo), no estilo da
 * janela do herói. À esquerda o estoque do dia (preço, quantas restam, comprar); à direita o que
 * o comerciante compra da mochila e por quanto (vender 1 ou tudo). Só pede ao servidor
 * (UVortexHeroSheet::ServerShopBuy/ServerShopSell) e se redesenha quando a ficha muda.
 */
namespace VortexShopWindow
{
    VORTEX_API void Open(AVortexCharacter* Hero, FName Vendor);
    VORTEX_API void Close();
    VORTEX_API bool IsOpen();
}
