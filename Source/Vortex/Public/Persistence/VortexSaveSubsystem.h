#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "VortexSaveSubsystem.generated.h"

class UVortexPlayerSave;
class UVortexWorldSave;
class USaveGame;

USTRUCT(BlueprintType)
struct FVortexHeroSummary
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Hero") FGuid PlayerId;
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Hero") FString HeroName;
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Hero") FName RaceId;
};

USTRUCT(BlueprintType)
struct FVortexWorldSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Save") FGuid WorldId;
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Save") FString DisplayName;
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Save") int32 Seed = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Save") FDateTime LastModified;
    UPROPERTY(BlueprintReadOnly, Category = "Vortex|Save") bool bMultiplayer = false;
};

// World data is written only by the host. Each machine writes its own local player profile.
UCLASS()
class VORTEX_API UVortexSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    UVortexWorldSave* CreateNewWorld(int32 Seed, int32 GeneratorVersion);

    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    bool SaveHostedWorld(UVortexWorldSave* WorldSave);

    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    UVortexWorldSave* LoadHostedWorld(FGuid WorldId);

    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    TArray<FVortexWorldSummary> ListHostedWorlds();

    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    UVortexPlayerSave* CreateLocalPlayer();

    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    bool SaveLocalPlayer(UVortexPlayerSave* PlayerSave);

    /**
     * O herói salvo. Lido do disco só na primeira vez e guardado em memória (gravar atualiza a cópia guardada).
     * Antes cada chamada lia e decodificava o arquivo inteiro: a HUD chamava duas vezes por quadro e, com o mapa
     * descoberto no save, isso custava ~70 ms por quadro (o jogo travava em ~13 FPS).
     */
    UFUNCTION(BlueprintCallable, Category = "Vortex|Save")
    UVortexPlayerSave* LoadLocalPlayer(FGuid PlayerId);
    /** Sempre do disco (os testes de save conferem o que foi gravado de verdade). */
    UVortexPlayerSave* LoadLocalPlayerFromDisk(FGuid PlayerId);

    TArray<FVortexHeroSummary> ListWorldHeroes(FGuid WorldId);
    bool DeleteWorldHero(FGuid WorldId, FGuid PlayerId);
    /** Apaga o mundo (e as cópias de segurança) e todos os heróis dele deste computador. */
    bool DeleteHostedWorld(FGuid WorldId);

private:
    UPROPERTY(Transient) TMap<FGuid, TObjectPtr<UVortexPlayerSave>> PlayerCache;
    bool CanWriteWorld() const;
    static FString SlotName(const TCHAR* Prefix, const FGuid& Id);
    static bool IsValidBytes(const TArray<uint8>& Bytes, UClass* ExpectedClass, const FGuid& ExpectedId);
    static USaveGame* LoadValidated(const FString& Slot, UClass* ExpectedClass, const FGuid& ExpectedId);
    static bool SaveValidated(USaveGame* Save, const FString& Slot, UClass* ExpectedClass, const FGuid& ExpectedId);
};
