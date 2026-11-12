#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VortexAudio.generated.h"

class UAudioComponent;

/**
 * Sons do jogo (mixados por Tools/BuildSfxMaster.py e importados por Tools/ImportSfxMaster.py em /Game/Vortex/Audio/Sfx).
 * Só tocam onde há tela (nunca no servidor dedicado); com posição, somem com a distância conforme o tipo (passos ~16 m,
 * a maioria ~30 m, chefes ~70 m).
 * - Gravações com variações (Sfx/Recorded/<nome>_v1.._v4, as gravações reais do Godot): cada vez toca uma diferente,
 *   nunca a mesma duas vezes seguidas.
 * - O mesmo som não empilha: no máximo 4 cópias a cada 0,12 s (um golpe de área em 10 bichos não estoura).
 */
namespace VortexAudio
{
    VORTEX_API void Play(const UObject* Context, FName Sound, const FVector& At, float Volume = 1.f, float PitchJitter = .06f);
    VORTEX_API void Play2D(const UObject* Context, FName Sound, float Volume = 1.f);
    /** O som existe (em /Game/Vortex/Audio/Sfx)? */
    VORTEX_API bool Has(FName Sound);
    /** Os sons de interface ("ui_*") já tocados e a hora do último (o botão de borracha do VortexUiJuice compara). */
    VORTEX_API uint64& UiSoundCount();
    VORTEX_API double& LastUiSoundAt();
    /**
     * Som da arma na mão do herói: wfx_<família>_<Kind>[_<Variant>] (Tools/BuildWeaponSfx.py), ou Fallback
     * se a família não tiver esse som. Kind: swing, hit, shot.
     */
    VORTEX_API FName WeaponSound(const AActor* Hero, const TCHAR* Kind, int32 Variant, FName Fallback);
    /** Passo conforme o chão do vale embaixo do ponto (grama, pedra, areia, lama, neve; água rasa por cima). */
    VORTEX_API FName StepSound(const FVector& Feet, int32 Variant);
    /** A voz da espécie para o evento ("hit", "death"): wolf_hit, rabbit_death... (NAME_None se ela não tiver). */
    VORTEX_API FName CreatureVoice(FName Species, const TCHAR* Event);
    /**
     * Volumes das opções (geral, música, efeitos, ambiente, interface) no mix SM_VortexVolumes: cada
     * som do Vortex tem a sua categoria (Tools/ImportSoundsAndClasses.py). Muda na hora.
     */
    VORTEX_API void ApplyVolumes(const UObject* Context);
    /** Cena (a HQ de abertura): sem ambiente e com os efeitos do mundo quase mudos; false devolve os volumes das opções. */
    VORTEX_API void SetCutscene(const UObject* Context, bool bOn);
    /** Carrega todos os efeitos e sons da interface em segundo plano (uma vez). */
    VORTEX_API void Preload();
}

/**
 * Música do jogo, como o GameMusic.cs do Godot: exploração (o tema) de dia, "vale adormecido" à
 * noite, "praça do santuário" perto da vila e "fúria de Azhur" quando um lobo está lutando perto.
 * Troca com fade de 2,5 s. Não toca no menu (o palco do menu tem a própria música).
 */
UCLASS()
class VORTEX_API UVortexMusicSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }
    FName CurrentTrack() const { return Track; }
    float MusicVolume = .38f;
    /** A HQ de abertura (e outras cenas com a própria música) abaixa a do jogo enquanto toca. */
    void SetDucked(bool bOn);

private:
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Playing;
    FName Track;
    float CheckTime = 0.f, CombatHold = 0.f;
    bool bPreloaded = false;
    void Switch(FName NewTrack);
};
