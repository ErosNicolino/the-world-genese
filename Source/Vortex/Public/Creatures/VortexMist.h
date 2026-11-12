#pragma once

#include "NiagaraComponent.h"

namespace VortexMist
{
    /** A aura "Soul" do Free_Spells virada em névoa roxa de corrupção: só a fumaça e umas faíscas, todas roxas
     *  (os raios, a espiral, a onda e a rachadura saem; a cor de cada parte é um parâmetro próprio do efeito). */
    inline void MakePurpleMist(UNiagaraComponent* Mist)
    {
        if (!Mist) return;
        const FLinearColor Purple(1.4f, .25f, 3.f);
        for (const TCHAR* Part : { TEXT("Smoke"), TEXT("Sparks1"), TEXT("Ray"), TEXT("Spiral1"), TEXT("Wave"), TEXT("Crack") })
            Mist->SetVariableLinearColor(FName(*FString::Printf(TEXT("Color_%s"), Part)), Purple);
        for (const TCHAR* Part : { TEXT("Ray"), TEXT("Spiral1"), TEXT("Wave"), TEXT("Crack") })
            Mist->SetVariableFloat(FName(*FString::Printf(TEXT("Scale_%s"), Part)), 0.f);
        Mist->SetVariableFloat(TEXT("Scale_Sparks1"), .5f);
    }
}
