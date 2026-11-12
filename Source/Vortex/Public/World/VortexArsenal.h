#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VortexArsenal.generated.h"

class AVortexCharacter;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UStaticMeshComponent;

/**
 * O Arsenal (07/11, o usuário: "um modo de jogo à parte, mais leve, com todas as armas enfileiradas para eu testar uma
 * por uma, de armas a armaduras e arcos"): o mapa L_Arsenal (botão "Arsenal" no menu), só o herói, sem o vale.
 *
 * Montado aqui a partir dos catálogos (o que entrar nos dados aparece sozinho): as espadas e os arcos da tabela em
 * grades por raridade, as outras armas (uma de cada tipo), os conjuntos de armadura vestidos em manequins, a placa de
 * tirar a armadura e o campo de treino com bonecos a 5, 10 e 20 m. Perto de um pedestal, E equipa (AVortexCharacter::
 * Interact). As placas da frente mudam o nível do herói e o das armas (o item trava se o herói não tem o nível).
 * Mana infinita e poder sem recarga. Nada vai para o save (UVortexSaveSubsystem e a ficha conferem IsArsenal).
 */
UCLASS()
class VORTEX_API AVortexArsenal : public AActor
{
    GENERATED_BODY()

public:
    AVortexArsenal();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Está no mapa do Arsenal (o save não grava, o Estádio não abre). */
    static bool IsArsenal(const UWorld* World);
    static AVortexArsenal* Find(const UWorld* World);
    /** E perto de um pedestal: equipa o que está nele; false se não há pedestal perto. */
    bool TryUse(AVortexCharacter* Hero);

    struct FStand
    {
        FVector Where = FVector::ZeroVector;
        FString Label;
        int32 Slot = 0;             // VortexRpg::EEquip (armadura: o conjunto inteiro, Slot = -1)
        TArray<FName> Items;        // o que veste (as 4 peças no conjunto); vazio = tirar a armadura
        UStaticMeshComponent* Show = nullptr;   // o modelo que gira em cima
        UTextRenderComponent* Text = nullptr;
    };

private:
    void Build();
    void AddStand(const FVector& Where, const FString& Label, const FString& Rarity, int32 Slot, const TArray<FName>& Items, UStaticMesh* Mesh, float MeshScale);
    void AddMannequin(const FVector& Where, const FString& Label, const FString& Rarity, const TArray<FName>& Pieces);
    UTextRenderComponent* AddText(const FVector& Where, const FString& Text, float Size, const FColor& Color);
    void AddBlock(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, bool bGround = false);

    TArray<FStand> Stands;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Blocks;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Prompt;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Status;
    /** O nível das armas e armaduras que o E dá (0 = o natural de cada raridade). */
    int32 GearLevel = 0;
    int32 Near = INDEX_NONE;
};
