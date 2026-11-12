#pragma once

#include "CoreMinimal.h"

class ACharacter;
class AVortexCharacter;
class UTexture2D;

/**
 * Retrato do herói: uma foto parada do rosto, de frente, tirada quando o herói é criado (no menu,
 * parado na pose de vitrine) e guardada em Saved/Portraits/<PlayerId>.png. A HUD só mostra a foto.
 * O enquadramento segue o osso da cabeça e o tamanho da raça (anão e bárbaro saem iguais).
 * Heróis antigos, sem foto: a HUD tira uma vez no jogo, com o herói parado.
 */
namespace VortexPortrait
{
    /** Fotografa o rosto e grava o arquivo; false se não deu (sem malha, sem mundo). */
    VORTEXUI_API bool CaptureAndSave(AVortexCharacter* Hero, const FGuid& PlayerId);
    /** A foto guardada (carregada uma vez e mantida), ou nada. */
    VORTEXUI_API UTexture2D* Load(const FGuid& PlayerId);
    VORTEXUI_API FString PathFor(const FGuid& PlayerId);
    /** 07/11: a mesma foto de um morador (o vendedor na loja), guardada na memória pelo nome; nada se não deu. */
    VORTEXUI_API UTexture2D* CaptureFace(ACharacter* Who);
}
