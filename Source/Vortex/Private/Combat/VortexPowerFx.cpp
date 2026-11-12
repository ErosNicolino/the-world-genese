#include "Combat/VortexPowerFx.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    // as texturas animadas (Tools/VFX/MakeFlipbooks.py): o índice é o "tex" das receitas
    const TCHAR* const FlipbookNames[] = { TEXT("agua"), TEXT("poeira"), TEXT("brilho"), TEXT("faisca"), TEXT("fumaca"), TEXT("fogo"),
        TEXT("folhas"), TEXT("gelo"), TEXT("luz") };

    struct FDef
    {
        TArray<FLinearColor> Colors;
        TArray<TPair<FName, TMap<FName, float>>> Parts;
        float Hit = 0.f;
    };

    const TMap<FName, FDef>& Defs()
    {
        static TMap<FName, FDef> All = []()
        {
            TMap<FName, FDef> Out;
            FString Text;
            TSharedPtr<FJsonObject> Root;
            if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/power_fx.json"))) ||
                !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return Out;
            for (const auto& Pair : Root->GetObjectField(TEXT("fx"))->Values)
            {
                const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
                FDef Def;
                O->TryGetNumberField(TEXT("hit"), Def.Hit);
                for (const TSharedPtr<FJsonValue>& C : O->GetArrayField(TEXT("colors")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& RGB = C->AsArray();
                    Def.Colors.Add(FLinearColor(RGB[0]->AsNumber(), RGB[1]->AsNumber(), RGB[2]->AsNumber()));
                }
                for (const TSharedPtr<FJsonValue>& P : O->GetArrayField(TEXT("parts")))
                {
                    const TSharedPtr<FJsonObject> PO = P->AsObject();
                    TMap<FName, float> V;
                    FName Type;
                    for (const auto& F : PO->Values)
                    {
                        if (F.Key == TEXT("type")) Type = FName(*F.Value->AsString());
                        else if (F.Key == TEXT("at")) V.Add(FName(*(TEXT("at_") + F.Value->AsString())), 1.f);
                        else if (F.Key == TEXT("tex"))
                        {
                            for (int32 i = 0; i < UE_ARRAY_COUNT(FlipbookNames); ++i)
                                if (F.Value->AsString() == FlipbookNames[i]) V.Add(TEXT("tex"), float(i));
                        }
                        else V.Add(FName(*F.Key), float(F.Value->AsNumber()));
                    }
                    Def.Parts.Add({ Type, V });
                }
                Out.Add(FName(*Pair.Key), Def);
            }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_POWER_FX %d efeitos"), Out.Num());
            return Out;
        }();
        return All;
    }

    float Smooth(float T)
    {
        T = FMath::Clamp(T, 0.f, 1.f);
        return T * T * (3.f - 2.f * T);
    }

    /** Aparece rápido, fica e some no fim (o tamanho vai a zero: o bloco "apaga" encolhendo). */
    float Pop(float U)
    {
        return Smooth(U / .12f) * (1.f - Smooth((U - .6f) / .4f));
    }

    UStaticMesh* Shape(const TCHAR* Name)
    {
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
    }
}

AVortexPowerFx::AVortexPowerFx()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    SetReplicates(false);
}

bool AVortexPowerFx::Has(FName Key)
{
    return Defs().Contains(Key);
}

float AVortexPowerFx::HitTime(FName Key)
{
    const FDef* Def = Defs().Find(Key);
    return Def ? Def->Hit : 0.f;
}

AVortexPowerFx* AVortexPowerFx::Spawn(ACharacter* InHero, FName Item)
{
    const FDef* Def = Defs().Find(Item);
    if (!InHero || !Def || InHero->GetNetMode() == NM_DedicatedServer) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVortexPowerFx* Fx = InHero->GetWorld()->SpawnActor<AVortexPowerFx>(InHero->GetActorLocation(), FRotator::ZeroRotator, Params);
    if (!Fx) return nullptr;
    Fx->Hero = InHero;
    Fx->Origin = InHero->GetActorLocation();
    Fx->Forward = InHero->GetActorForwardVector().GetSafeNormal2D();
    Fx->Right = FVector::CrossProduct(FVector::UpVector, Fx->Forward);
    Fx->Ground = Fx->Origin.Z - 88.f;
    {
        FVector Eye;
        FRotator Look;
        InHero->GetActorEyesViewPoint(Eye, Look);
        Look = InHero->GetBaseAimRotation();
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexPowerFxAim), false, InHero);
        const FVector End = Eye + Look.Vector() * 2600.f;
        FVector At = InHero->GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Query) ? FVector(Hit.ImpactPoint) : End;
        // até o chão embaixo do ponto (mirando no ar, cai no chão lá na frente)
        if (InHero->GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 200), At - FVector(0, 0, 4000), ECC_WorldStatic, Query)) At = Hit.ImpactPoint;
        // atrás ou colado no herói (mirando para baixo): 7 m à frente, como antes
        if (FVector::DotProduct((At - Fx->Origin).GetSafeNormal2D(), Fx->Forward) < .2f || FVector::Dist2D(At, Fx->Origin) < 250.f)
            At = FVector(Fx->Origin.X, Fx->Origin.Y, Fx->Ground) + Fx->Forward * 700.f;
        Fx->AimAt = At;
    }
    Fx->Colors = Def->Colors;
    if (Fx->Colors.IsEmpty()) Fx->Colors.Add(FLinearColor::White);
    Fx->Lifetime = .3f;
    for (const TPair<FName, TMap<FName, float>>& P : Def->Parts)
    {
        FPart Part;
        Part.Type = P.Key;
        Part.V = P.Value;
        Part.Delay = Part.Value(TEXT("delay"));
        Part.Life = FMath::Max(.3f, Part.Value(TEXT("life"), 1.f));
        Fx->Lifetime = FMath::Max(Fx->Lifetime, Part.Delay + Part.Life + .2f);
        Fx->Parts.Add(MoveTemp(Part));
    }
    return Fx;
}

FVector AVortexPowerFx::Aim() const
{
    return AimAt.IsZero() ? FVector(Origin.X, Origin.Y, Ground) + Forward * 700.f : AimAt;
}

void AVortexPowerFx::SetHeroHidden(bool bHide)
{
    if (bHide)
    {
        ACharacter* H = Hero.Get();
        if (!H || !H->GetMesh() || !HiddenParts.IsEmpty()) return;
        TArray<USceneComponent*> All;
        H->GetMesh()->GetChildrenComponents(true, All);
        All.Add(H->GetMesh());
        for (USceneComponent* C : All)
            if (C && C->IsVisible()) { HiddenParts.Add(C); C->SetVisibility(false, false); }
        return;
    }
    for (const TWeakObjectPtr<USceneComponent>& C : HiddenParts)
        if (C.IsValid()) C->SetVisibility(true, false);
    HiddenParts.Reset();
}

void AVortexPowerFx::EndPlay(const EEndPlayReason::Type Reason)
{
    SetHeroHidden(false);
    Super::EndPlay(Reason);
}

void AVortexPowerFx::StartPart(FPart& Part)
{
    Part.bStarted = true;
    FRandomStream Rng(int32(Part.Value(TEXT("seed"), 7.f)));
    const FLinearColor Color = Colors[FMath::Clamp(int32(Part.Value(TEXT("color"))), 0, Colors.Num() - 1)];
    const int32 Count = FMath::Clamp(int32(Part.Value(TEXT("count"), 12.f)), 1, 80);
    const float Radius = Part.Value(TEXT("radius"), 220.f), Size = Part.Value(TEXT("size"), 1.f);
    const int32 Style = int32(Part.Value(TEXT("style")));
    // "lit": peça sólida (pedra, água, madeira) com luz, sombra e a textura de blocos (M_PowerSolid); sem ele, a energia que
    // soma luz (M_PowerFx, aditivo: 07/11, antes era cor chapada e as colunas viravam blocos cinza). As cores bem escuras
    // (sombra, vazio) também vão sólidas: somar quase preto não aparece
    const bool bLit = Part.Value(TEXT("lit")) > 0.f || Color.GetLuminance() < .12f;
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bLit ? TEXT("/Game/Vortex/Combat/M_PowerSolid.M_PowerSolid") : TEXT("/Game/Vortex/Combat/M_PowerFx.M_PowerFx"));
    if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor"));
    Part.Look = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
    if (Part.Look)
    {
        Part.Look->SetVectorParameterValue(TEXT("Color"), Color);
        // 07/11: o brilho baixo (a exposição do dia estourava as cores em branco): a cor da arma aparece
        Part.Look->SetScalarParameterValue(TEXT("Glow"), Style == 2 && Part.Type == TEXT("orbs") ? .35f : .7f + Rng.FRandRange(0.f, .35f));
    }
    if (Part.Type == TEXT("vanish")) return;
    if (Part.Type == TEXT("slash"))
    {
        // o corte no ar: um leque como o rastro do golpe, na cor da arma
        Part.Arc = NewObject<UProceduralMeshComponent>(this);
        Part.Arc->SetupAttachment(RootComponent);
        Part.Arc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part.Arc->SetCastShadow(false);
        Part.Arc->RegisterComponent();
        const float Sweep = Part.Value(TEXT("sweep"), 160.f);
        const float R = FMath::Clamp(Radius, 120.f, 420.f), Width = R * .4f;
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FVector2D> UVs;
        constexpr int32 Segments = 32, Rows = 3;
        for (int32 i = 0; i <= Segments; ++i)
        {
            const float U = float(i) / Segments;
            const float Theta = FMath::DegreesToRadians(FMath::Lerp(-Sweep * .5f, Sweep * .5f, U));
            for (int32 j = 0; j <= Rows; ++j)
            {
                const float V = float(j) / Rows;
                const float Rr = R - Width * FMath::Sin(U * UE_PI) * V;
                Vertices.Add(FVector(FMath::Cos(Theta) * Rr, FMath::Sin(Theta) * Rr, 0.f));
                Normals.Add(FVector::UpVector);
                UVs.Add(FVector2D(U, V));
            }
        }
        for (int32 i = 0; i < Segments; ++i)
            for (int32 j = 0; j < Rows; ++j)
            {
                const int32 A = i * (Rows + 1) + j, B = A + Rows + 1;
                Triangles.Append({ A, B, A + 1, A + 1, B, B + 1, A, A + 1, B, A + 1, B + 1, B });
            }
        Part.Arc->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
        if (UMaterialInterface* Slash = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SlashArc.M_SlashArc")))
        {
            UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Slash, this);
            Look->SetScalarParameterValue(TEXT("Seed"), Rng.FRand());
            Look->SetScalarParameterValue(TEXT("Glow"), .9f);
            Look->SetScalarParameterValue(TEXT("Sharp"), 3.f + Style);
            Look->SetVectorParameterValue(TEXT("Color"), Color);
            Part.Arc->SetMaterial(0, Look);
            Part.Look = Look;
        }
        return;
    }
    // o resto: blocos em lote (cubos, esferas, cilindros ou cones)
    const TCHAR* Mesh = Part.Type == TEXT("orbs") ? TEXT("Sphere") : Part.Type == TEXT("pillar") ? TEXT("Cylinder")
        : Part.Type == TEXT("spikes") ? TEXT("Cone") : TEXT("Cube");
    Part.Ism = NewObject<UInstancedStaticMeshComponent>(this);
    Part.Ism->SetupAttachment(RootComponent);
    Part.Ism->SetUsingAbsoluteLocation(true);
    Part.Ism->SetUsingAbsoluteRotation(true);
    Part.Ism->SetStaticMesh(Shape(Mesh));
    Part.Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part.Ism->SetCastShadow(false);
    if (Part.Look) Part.Ism->SetMaterial(0, Part.Look);
    Part.Ism->RegisterComponent();
    const FVector Up = FVector::UpVector;
    auto Add = [&](const FVector& Pos, const FVector& Vel, float S, float Delay = 0.f, float Phase = 0.f)
    {
        FBit Bit;
        Bit.Pos = Pos;
        Bit.Vel = Vel;
        Bit.Size = S;
        Bit.Delay = Delay;
        Bit.Phase = Phase;
        Bit.Rot = FRotator(Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180));
        Bit.Spin = FRotator(Rng.FRandRange(-400, 400), Rng.FRandRange(-400, 400), Rng.FRandRange(-400, 400));
        Part.Bits.Add(Bit);
    };
    // "front" (cm): o ponto à frente do herói (onde a espada bate), junto com qualquer arranjo (anel, espalhado)
    const FVector Center = Part.Value(TEXT("front")) > 0.f ? FVector(Origin.X, Origin.Y, Ground) + Forward * Part.Value(TEXT("front"))
        : Part.Value(TEXT("at_aim")) > 0.f ? Aim() : Part.Value(TEXT("at_front")) > 0.f ? FVector(Origin.X, Origin.Y, Ground) + Forward * 140.f
        : Part.Value(TEXT("at_end")) > 0.f ? FVector(Origin.X, Origin.Y, Ground) + Forward * Radius * 2.5f : FVector(Origin.X, Origin.Y, Ground);
    if (Part.Type == TEXT("ring") || Part.Type == TEXT("glyph"))
    {
        const int32 N = FMath::Max(10, Count * 2);
        for (int32 k = 0; k < N; ++k) Add(Center, FVector::ZeroVector, Size, 0.f, float(k) / N);
        if (Part.Type == TEXT("glyph"))
        {
            for (int32 k = 0; k < N / 2; ++k) Add(Center, FVector(1, 0, 0), Size * .8f, 0.f, float(k) / (N / 2));     // o círculo de dentro
            for (int32 s = 0; s < 4 + Style; ++s)
                for (int32 k = 1; k < 6; ++k) Add(Center, FVector(2, 0, 0), Size * .7f, 0.f, float(s) / (4 + Style) + k * .0001f);   // os raios
        }
        if (Style == 1) for (int32 k = 0; k < N; ++k) Add(Center, FVector(3, 0, 0), Size * .8f, .12f, float(k) / N);   // a segunda onda
    }
    else if (Part.Type == TEXT("pillar"))
    {
        const bool bWall = Part.Value(TEXT("at_wall")) > 0.f || Part.Value(TEXT("wall")) > 0.f;
        const int32 N = bWall ? FMath::Clamp(Count, 3, 15) : Part.Value(TEXT("at_ring")) > 0.f ? FMath::Clamp(Count / 2, 3, 8) : Style == 2 ? 3 : 1;
        const FVector WallAt = Part.Value(TEXT("at_wall")) > 0.f ? FVector(Origin.X, Origin.Y, Ground) + Forward * 450.f : Center;
        for (int32 k = 0; k < N; ++k)
        {
            FVector P = Center;
            if (bWall) P = WallAt + Right * (float(k) / FMath::Max(1, N - 1) - .5f) * Radius * 2.f;
            else if (N > 1) P = Center + FRotator(0, 360.f * k / N, 0).RotateVector(Forward) * Radius * .6f;
            Add(P, FVector::ZeroVector, Size * (Style == 1 ? .7f : 1.f) * (bWall ? Rng.FRandRange(.7f, 1.2f) : 1.f), bWall ? FMath::Abs(k - (N - 1) * .5f) * .04f : k * .05f);
        }
    }
    else if (Part.Type == TEXT("spikes"))
    {
        for (int32 k = 0; k < Count; ++k)
        {
            FVector P;
            if (Part.Value(TEXT("at_line")) > 0.f) P = FVector(Origin.X, Origin.Y, Ground) + Forward * (120.f + k * Radius * 2.6f / Count) + Right * Rng.FRandRange(-40, 40);
            else if (Part.Value(TEXT("at_ring")) > 0.f) P = Center + FRotator(0, 360.f * k / Count, 0).RotateVector(Forward) * Radius;
            else P = Center + FRotator(0, Rng.FRandRange(0, 360), 0).RotateVector(Forward) * Rng.FRandRange(60.f, Radius);
            Add(P, FVector::ZeroVector, Size * Rng.FRandRange(.7f, 1.3f) * (Style == 2 ? 1.5f : 1.f), Part.Value(TEXT("at_line")) > 0.f ? k * .035f : Rng.FRandRange(0, .2f));
        }
    }
    else if (Part.Type == TEXT("shards"))
    {
        const FVector From = Center + Up * 70.f;
        for (int32 k = 0; k < Count + 6; ++k)
        {
            const FVector Dir = (Rng.GetUnitVector() + Up * .9f).GetSafeNormal();
            Add(From, Dir * Rng.FRandRange(250.f, 650.f) * Size, Size * Rng.FRandRange(.5f, 1.1f) * (Part.Value(TEXT("small")) > 0.f ? .6f : 1.f));
        }
    }
    else if (Part.Type == TEXT("orbs"))
    {
        for (int32 k = 0; k < FMath::Clamp(Count / 2, 3, 12); ++k) Add(Center, FVector::ZeroVector, Size * Rng.FRandRange(.8f, 1.2f), 0.f, float(k) / FMath::Clamp(Count / 2, 3, 12));
    }
    else if (Part.Type == TEXT("rain"))
    {
        for (int32 k = 0; k < Count * 2; ++k)
        {
            const FVector P = Center + FRotator(0, Rng.FRandRange(0, 360), 0).RotateVector(Forward) * Rng.FRandRange(0.f, Radius);
            Add(P, FVector::ZeroVector, Size * Rng.FRandRange(.6f, 1.f), Rng.FRandRange(0.f, .6f), Rng.FRand());
        }
    }
    else if (Part.Type == TEXT("spiral"))
    {
        for (int32 k = 0; k < Count * 2; ++k) Add(Center, FVector::ZeroVector, Size * .8f, 0.f, float(k) / (Count * 2));
    }
    else if (Part.Type == TEXT("beam"))
    {
        const int32 N = FMath::Clamp(int32(Part.Value(TEXT("count"), 1.f)), 1, 9);
        for (int32 k = 0; k < N; ++k) Add(Center, FVector::ZeroVector, Size, Part.Value(TEXT("fan")) > 0.f ? 0.f : k * Part.Value(TEXT("gap"), .12f), N > 1 ? float(k) / (N - 1) - .5f : 0.f);
    }
    else if (Part.Type == TEXT("feathers"))
    {
        for (int32 k = 0; k < Count; ++k)
        {
            const FVector P = Center + Up * Rng.FRandRange(160.f, 300.f) + FRotator(0, Rng.FRandRange(0, 360), 0).RotateVector(Forward) * Rng.FRandRange(0.f, Radius * .8f);
            Add(P, FVector(0, 0, -Rng.FRandRange(60.f, 120.f)), Size, Rng.FRandRange(0.f, .3f), Rng.FRand() * 6.f);
        }
    }
    else if (Part.Type == TEXT("bolt"))
    {
        const bool bPlaced = Part.Value(TEXT("front")) > 0.f || Part.Value(TEXT("at_front")) > 0.f || Part.Value(TEXT("at_end")) > 0.f || Part.Value(TEXT("hero")) > 0.f;
        const FVector End = Part.Value(TEXT("hero")) > 0.f ? FVector(Origin.X, Origin.Y, Ground + 120.f) : bPlaced ? Center : Aim();
        const FVector Top = End + Up * Part.Value(TEXT("height"), 700.f);
        FVector P = Top;
        constexpr int32 Segs = 9;
        for (int32 k = 0; k < Segs; ++k)
        {
            const FVector Next = FMath::Lerp(Top, End, float(k + 1) / Segs) + (k < Segs - 1 ? Right * Rng.FRandRange(-60, 60) + Forward * Rng.FRandRange(-40, 40) : FVector::ZeroVector);
            Add((P + Next) * .5f, Next - P, Size);
            P = Next;
        }
    }
    else if (Part.Type == TEXT("sprites"))
    {
        // as texturas animadas: em volta do ponto, em anel ou espalhadas, cada uma num instante
        const int32 Tex = int32(Part.Value(TEXT("tex")));
        for (int32 k = 0; k < Count; ++k)
        {
            const float Ang = Part.Value(TEXT("at_ring")) > 0.f ? 360.f * k / Count : Rng.FRandRange(0.f, 360.f);
            const float R = Part.Value(TEXT("at_ring")) > 0.f ? Radius : Rng.FRandRange(0.f, Radius);
            FVector P = Center + FRotator(0, Ang, 0).RotateVector(Forward) * R + Up * Part.Value(TEXT("height"), 40.f);
            float Delay = Part.Value(TEXT("spread"), .2f) * Rng.FRand();
            const float Along = (k + .5f) / Count;
            if (Part.Value(TEXT("at_line")) > 0.f)
            {
                // em fila na linha da frente, cada uma na sua vez (o caminho do avanço, a trilha da flecha)
                P = FVector(Origin.X, Origin.Y, Ground) + Forward * (Along * Radius) + Right * Rng.FRandRange(-40.f, 40.f) + Up * Part.Value(TEXT("height"), 40.f);
                Delay = Along * Part.Value(TEXT("spread"), .2f);
            }
            else if (Part.Value(TEXT("wall")) > 0.f)
                P = Center + Right * (Along - .5f) * Radius * 2.f + Up * Part.Value(TEXT("height"), 40.f);
            const FVector Vel = FRotator(0, Ang, 0).RotateVector(Forward) * Part.Value(TEXT("out"), 0.f) + Forward * Part.Value(TEXT("ahead"), 0.f)
                + Up * Part.Value(TEXT("rise"), 30.f);
            AddSprite(Tex, P, Vel, 100.f * Size * Rng.FRandRange(.7f, 1.3f), Part.Life * Rng.FRandRange(.7f, 1.f), Delay, Part.Value(TEXT("gravity")));
        }
    }
    else if (Part.Type == TEXT("wave"))
    {
        // a onda: 3 lotes de blocos (a água funda, a clara e a espuma da crista)
        for (int32 k = 1; k <= 2; ++k)
        {
            UInstancedStaticMeshComponent* More = NewObject<UInstancedStaticMeshComponent>(this);
            More->SetupAttachment(RootComponent);
            More->SetUsingAbsoluteLocation(true);
            More->SetUsingAbsoluteRotation(true);
            More->SetStaticMesh(Shape(TEXT("Cube")));
            More->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            More->SetCastShadow(false);
            // a espuma brilha um pouco (M_PowerFx); a água clara tem luz e sombra como a funda
            UMaterialInterface* Mat = k == 2 ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_PowerFx.M_PowerFx")) : Base;
            if (Mat)
            {
                UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Mat, this);
                Look->SetVectorParameterValue(TEXT("Color"), Colors[FMath::Min(k, Colors.Num() - 1)]);
                Look->SetScalarParameterValue(TEXT("Glow"), k == 2 ? .9f : .8f);
                More->SetMaterial(0, Look);
            }
            More->RegisterComponent();
            Part.Extra.Add(More);
        }
        if (Part.Look && !bLit) Part.Look->SetScalarParameterValue(TEXT("Glow"), .55f);
    }
    else if (Part.Type == TEXT("cracks"))
    {
        // as rachaduras: linhas tortas saindo do ponto (cada uma de blocos chatos), que crescem até o raio
        for (int32 l = 0; l < Count; ++l)
        {
            FVector P = Center;
            float Ang = 360.f * l / Count + Rng.FRandRange(-12.f, 12.f);
            const int32 Segs = 9;
            for (int32 k = 0; k < Segs; ++k)
            {
                Ang += Rng.FRandRange(-25.f, 25.f);
                const FVector Next = P + FRotator(0, Ang, 0).RotateVector(Forward) * Radius / Segs * Rng.FRandRange(.7f, 1.3f);
                Add((P + Next) * .5f + Up * 9.f, Next - P, Size * (1.f - k / float(Segs + 2)), float(k) / Segs, Ang);
                P = Next;
            }
        }
    }
    // o rastro (trail) não tem blocos de saída: nasce da ponta da arma a cada quadro
}

void AVortexPowerFx::AddSprite(int32 Tex, const FVector& Pos, const FVector& Vel, float Size, float Life, float Delay, float Gravity)
{
    FSprite S;
    S.Tex = FMath::Clamp(Tex, 0, int32(UE_ARRAY_COUNT(FlipbookNames)) - 1);
    S.Pos = Pos;
    S.Vel = Vel;
    S.Size = Size;
    S.Start = Age + Delay;
    S.Life = FMath::Max(.15f, Life);
    S.Gravity = Gravity;
    Sprites.Add(S);
}

void AVortexPowerFx::TickSprites()
{
    if (Sprites.IsEmpty()) return;
    // viradas para a câmera de quem joga (só visual)
    FVector Cam = Origin + FVector(0, 0, 300);
    if (const APlayerController* Pc = GetWorld()->GetFirstPlayerController(); Pc && Pc->PlayerCameraManager) Cam = Pc->PlayerCameraManager->GetCameraLocation();
    TMap<int32, TArray<TPair<FTransform, float>>> ByTex;
    for (const FSprite& S : Sprites)
    {
        const float T = Age - S.Start;
        if (T < 0.f || T > S.Life) continue;
        const FVector P = S.Pos + S.Vel * T + FVector(0, 0, -.5f * S.Gravity * T * T);
        const FRotator Face = FRotationMatrix::MakeFromZ((Cam - P).GetSafeNormal()).Rotator();
        ByTex.FindOrAdd(S.Tex).Add({ FTransform(Face, P, FVector(S.Size / 100.f)), FMath::Clamp(T / S.Life, 0.f, .999f) });
    }
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/VFX/M_PowerFlipbook.M_PowerFlipbook"));
    for (int32 Tex = 0; Tex < UE_ARRAY_COUNT(FlipbookNames); ++Tex)
    {
        TArray<TPair<FTransform, float>>* List = ByTex.Find(Tex);
        UInstancedStaticMeshComponent** Found = SpriteMeshes.Find(Tex);
        if (!List && !Found) continue;
        UInstancedStaticMeshComponent* Ism = Found ? *Found : nullptr;
        if (!Ism)
        {
            Ism = NewObject<UInstancedStaticMeshComponent>(this);
            Ism->SetupAttachment(RootComponent);
            Ism->SetUsingAbsoluteLocation(true);
            Ism->SetUsingAbsoluteRotation(true);
            Ism->SetStaticMesh(Shape(TEXT("Plane")));
            Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Ism->SetCastShadow(false);
            Ism->NumCustomDataFloats = 1;
            const FString Name = FString::Printf(TEXT("T_FB_%s"), FlipbookNames[Tex]);
            UTexture* Atlas = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Vortex/VFX/Flipbooks/%s.%s"), *Name, *Name));
            if (Base)
            {
                UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Base, this);
                if (Atlas) Look->SetTextureParameterValue(TEXT("Atlas"), Atlas);
                Ism->SetMaterial(0, Look);
            }
            Ism->RegisterComponent();
            SpriteMeshes.Add(Tex, Ism);
        }
        Ism->ClearInstances();
        if (!List) continue;
        for (const TPair<FTransform, float>& Item : *List)
        {
            const int32 I = Ism->AddInstance(Item.Key, true);
            Ism->SetCustomDataValue(I, 0, Item.Value, false);
        }
        Ism->MarkRenderStateDirty();
    }
}

void AVortexPowerFx::TickWave(FPart& Part, float T)
{
    // a onda: a frente anda na linha da mira (a mesma conta do poder: VortexWeaponSpecials "tidewave"); cada coluna da
    // largura é uma pilha de blocos (funda embaixo, clara no meio, espuma em cima) com a crista enrolando para a frente
    const float Length = Part.Value(TEXT("length"), 1400.f), Width = Part.Value(TEXT("width"), 320.f), Speed = Part.Value(TEXT("speed"), 1500.f);
    const float Travel = Length / FMath::Max(1.f, Speed);
    const float Front = FMath::Min(Length, Speed * T);
    const float Collapse = 1.f - Smooth((T - Travel) / .35f);
    const float Rise = Smooth(T / .15f);
    const float HeightCm = Part.Value(TEXT("height"), 150.f) * Rise * Collapse;
    const float Cell = 26.f;
    const FVector Start = FVector(Origin.X, Origin.Y, Ground) + Forward * 80.f;
    TArray<FTransform> Deep, Light, Foam;
    const int32 Cols = FMath::Max(3, int32(Width / Cell));
    for (int32 c = 0; c < Cols; ++c)
    {
        const float Lat = -Width * .5f + (c + .5f) * Width / Cols;
        const float Edge = 1.f - FMath::Square(FMath::Abs(Lat) / (Width * .55f));
        for (int32 r = 0; r < 5; ++r)
        {
            // de trás (r = 4) para a frente (r = 0): a frente é a mais alta
            const float Wobble = .85f + .3f * FMath::Sin(c * .9f + T * 9.f + r);
            const float H = HeightCm * Edge * Wobble * (1.f - r * .19f);
            const int32 N = FMath::Max(0, FMath::RoundToInt(H / Cell));
            const FVector Base = Start + Forward * (Front - r * Cell * 1.1f) + Right * Lat;
            for (int32 k = 0; k < N; ++k)
            {
                const bool bTop = k == N - 1;
                const float Curl = r == 0 && k >= N - 2 ? Cell * (k - (N - 3)) * .55f : 0.f;   // a crista se enrola para a frente
                const FTransform X(FRotator(0, Forward.Rotation().Yaw, 0), Base + Forward * Curl + FVector(0, 0, Cell * (k + .5f)), FVector(Cell / 100.f * .95f));
                (bTop && r <= 1 ? Foam : k < N / 2 ? Deep : Light).Add(X);
            }
        }
    }
    auto Put = [](UInstancedStaticMeshComponent* Ism, const TArray<FTransform>& List)
    {
        if (!Ism) return;
        if (Ism->GetInstanceCount() != List.Num()) { Ism->ClearInstances(); Ism->AddInstances(List, false, true); }
        else Ism->BatchUpdateInstancesTransforms(0, List, true, true);
    };
    Put(Part.Ism, Deep);
    Put(Part.Extra.IsValidIndex(0) ? Part.Extra[0] : nullptr, Light);
    Put(Part.Extra.IsValidIndex(1) ? Part.Extra[1] : nullptr, Foam);
    // os respingos na crista, a espuma no rastro e as gotas
    if (T < Travel && Age >= Part.NextSpawn)
    {
        Part.NextSpawn = Age + .045f;
        FRandomStream Rng(int32(Age * 1000.f));
        const FVector Crest = Start + Forward * Front + FVector(0, 0, HeightCm * .8f);
        for (int32 k = 0; k < 2; ++k)
            AddSprite(0, Crest + Right * Rng.FRandRange(-Width * .5f, Width * .5f), Forward * Speed * .35f + FVector(0, 0, 160.f), Rng.FRandRange(240.f, 340.f), .5f, 0.f, 600.f);
        AddSprite(0, Start + Forward * (Front - 90.f) + Right * Rng.FRandRange(-Width * .4f, Width * .4f) + FVector(0, 0, 30.f), FVector::ZeroVector, Rng.FRandRange(180.f, 240.f), .7f);
    }
    if (T >= Travel && Part.NextSpawn < 1.e6f)
    {
        // a onda quebra no fim: um estouro de respingos
        Part.NextSpawn = 1.e7f;
        FRandomStream Rng(77);
        const FVector End = Start + Forward * Length + FVector(0, 0, 60.f);
        for (int32 k = 0; k < 10; ++k)
            AddSprite(0, End + Right * Rng.FRandRange(-Width * .5f, Width * .5f), Rng.GetUnitVector() * 200.f + FVector(0, 0, 260.f), Rng.FRandRange(280.f, 400.f), .7f, Rng.FRand() * .1f, 700.f);
    }
}

void AVortexPowerFx::TickCracks(FPart& Part, float T)
{
    // cada pedaço da rachadura aparece quando o crescimento chega nele (Phase = a ordem dele na linha)
    const float U = T / Part.Life;
    TArray<FTransform> Out;
    for (const FBit& Bit : Part.Bits)
    {
        const float Show = Smooth((U * 3.f - Bit.Delay) / .25f) * (1.f - Smooth((U - .75f) / .25f));
        const float Len = Bit.Vel.Size();
        Out.Add(FTransform(FRotator(0, Bit.Phase, 0), Bit.Pos, FVector(Len / 100.f * Show, .1f * Bit.Size * Show + .001f, .1f * Show + .001f)));
    }
    if (Part.Ism->GetInstanceCount() != Out.Num()) { Part.Ism->ClearInstances(); Part.Ism->AddInstances(Out, false, true); }
    else Part.Ism->BatchUpdateInstancesTransforms(0, Out, true, true);
}

void AVortexPowerFx::TickPart(FPart& Part, float T)
{
    const float U = T / Part.Life;
    const float Radius = Part.Value(TEXT("radius"), 220.f), Size = Part.Value(TEXT("size"), 1.f), SpinDeg = Part.Value(TEXT("spin"));
    const int32 Style = int32(Part.Value(TEXT("style")));
    const FVector Up = FVector::UpVector;
    const ACharacter* H = Hero.Get();
    const FVector Now = H ? H->GetActorLocation() : Origin;
    if (Part.Arc)
    {
        // o corte: varre no sentido do golpe, inclinado, e apaga
        const int32 N = FMath::Max(1, int32(Part.Value(TEXT("count"), 1.f)));
        const int32 k = FMath::Min(N - 1, int32(U * N));
        const float Local = FMath::Clamp(U * N - k, 0.f, 1.f);
        const float Tilt = Part.Value(TEXT("tilt")) * ((k % 2) ? -1.f : 1.f);
        const float Height = Part.Value(TEXT("low")) > 0.f ? 25.f : 95.f;
        const FQuat Facing = Forward.Rotation().Quaternion();
        const FQuat Sweep(Up, FMath::DegreesToRadians(((k % 2) ? -1.f : 1.f) * FMath::Lerp(-40.f, 40.f, Local) + (Part.Value(TEXT("sweep")) >= 360.f ? 360.f * Local : 0.f)));
        Part.Arc->SetWorldLocationAndRotation(Now + FVector(0, 0, Height - 88.f), Facing * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Tilt)) * Sweep);
        Part.Arc->SetWorldScale3D(FVector(Part.Value(TEXT("small")) > 0.f ? .6f : 1.f));
        if (Part.Look) Part.Look->SetScalarParameterValue(TEXT("Fade"), FMath::Square(1.f - Local) * (U < 1.f ? 1.f : 0.f));
        Part.Arc->SetVisibility(U < 1.f);
        return;
    }
    if (Part.Type == TEXT("vanish")) { SetHeroHidden(U < 1.f); return; }
    if (!Part.Ism) return;
    if (Part.Type == TEXT("wave")) { TickWave(Part, T); return; }
    if (Part.Type == TEXT("cracks")) { TickCracks(Part, T); return; }
    if (Part.Type == TEXT("sprites")) return;   // as texturas andam no TickSprites
    TArray<FTransform> Out;
    Out.Reserve(Part.Bits.Num());
    const float Dt = GetWorld()->GetDeltaSeconds();
    if (Part.Type != TEXT("streak")) for (FBit& Bit : Part.Bits)
    {
        const float B = FMath::Clamp((T - Bit.Delay) / FMath::Max(.1f, Part.Life - Bit.Delay), 0.f, 1.f);
        const float Pop = (T < Bit.Delay) ? 0.f : ::Pop(B);
        FVector Pos = Bit.Pos;
        FRotator Rot = Bit.Rot;
        FVector Scale(.2f * Bit.Size * Pop);
        if (Part.Type == TEXT("ring") || Part.Type == TEXT("glyph"))
        {
            const bool bInner = Bit.Vel.X == 1.f, bSpoke = Bit.Vel.X == 2.f;
            const float R = Part.Type == TEXT("ring") ? Radius * (Style == 2 ? 2.f : 1.f) * (.15f + .85f * Smooth(B)) : Radius * (bInner ? .6f : 1.f);
            const float Ang = (Bit.Phase + (Part.Type == TEXT("glyph") ? SpinDeg / 360.f * U : 0.f)) * 2.f * UE_PI;
            const float Rr = bSpoke ? Radius * .6f * (FMath::Frac(Bit.Phase * 10000.f) * 10.f / 6.f + .1f) : R;
            Pos = Bit.Pos + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * Rr + Up * (8.f + (Part.Type == TEXT("ring") ? 25.f * FMath::Sin(B * UE_PI) : 2.f));
            Rot = FRotator(0, FMath::RadiansToDegrees(Ang), 0);
            Scale = Part.Type == TEXT("glyph") ? FVector(.22f, .22f, .04f) * Bit.Size * Pop : FVector(.35f, .2f, .3f) * Bit.Size * Pop;
        }
        else if (Part.Type == TEXT("pillar"))
        {
            const float Rise = Smooth(B / .25f), Thin = 1.f - Smooth((B - .55f) / .45f);
            const float HeightCm = 240.f * Size * (Style == 2 ? 1.4f : 1.f);
            Scale = FVector(.32f * Bit.Size * Thin, .32f * Bit.Size * Thin, HeightCm / 100.f * Rise);
            Pos = Bit.Pos + Up * (HeightCm * .5f * Rise);
            Rot = FRotator(0, SpinDeg * U, 0);
        }
        else if (Part.Type == TEXT("spikes"))
        {
            const float Out_ = Smooth(B / .18f) * (1.f - Smooth((B - .7f) / .3f));
            const float HeightCm = 75.f * Bit.Size;
            Pos = Bit.Pos + Up * (HeightCm * (Out_ - .5f));
            Scale = FVector(.28f * Bit.Size, .28f * Bit.Size, HeightCm / 100.f) * FMath::Max(.01f, Out_);
            Rot = FRotator(Style == 1 ? 8.f : 0.f, Bit.Phase * 360.f, 0);
        }
        else if (Part.Type == TEXT("shards"))
        {
            const float Tt = FMath::Max(0.f, T - Bit.Delay);
            Pos = Bit.Pos + Bit.Vel * Tt + FVector(0, 0, -490.f * Tt * Tt);
            Pos.Z = FMath::Max(Pos.Z, Ground + 5.f);
            Rot = Bit.Rot + Bit.Spin * Tt;
            Scale = (Style == 1 ? FVector(.12f, .12f, .4f) : Style == 3 ? FVector(.3f, .3f, .03f) : FVector(.15f)) * Bit.Size * (1.f - Smooth((B - .5f) / .5f));
        }
        else if (Part.Type == TEXT("orbs"))
        {
            const bool bFollow = Part.Value(TEXT("follow")) > 0.f, bConverge = Part.Value(TEXT("converge")) > 0.f;
            const FVector C = bFollow ? Now : Origin;
            const float Ang = (Bit.Phase + U * 1.5f * (SpinDeg >= 0.f ? 1.f : -1.f)) * 2.f * UE_PI;
            float R = Radius * .6f;
            if (bConverge) R *= 1.f - Smooth(B);
            if (!bFollow && !bConverge) R *= .4f + B * 1.2f;   // abrem para fora
            Pos = C + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * R + Up * (20.f + 30.f * FMath::Sin(B * 6.f + Bit.Phase * 6.f) + (Style == 1 ? 80.f * B : 0.f));
            Scale = FVector(.18f * Bit.Size * Pop);
        }
        else if (Part.Type == TEXT("rain"))
        {
            const bool bUp = Part.Value(TEXT("up")) > 0.f;
            const float Fall = FMath::Frac(B * 1.6f + Bit.Phase);
            Pos = Bit.Pos + Up * (bUp ? 20.f + 260.f * Fall : 620.f * (1.f - Fall));
            Scale = FVector(.06f, .06f, .32f) * Bit.Size * Pop;
        }
        else if (Part.Type == TEXT("spiral"))
        {
            const float Ang = (Bit.Phase * 4.f + U * 2.5f * (SpinDeg >= 0.f ? 1.f : -1.f)) * 2.f * UE_PI;
            const bool bPlaced = Part.Value(TEXT("front")) > 0.f || Part.Value(TEXT("at_aim")) > 0.f || Part.Value(TEXT("at_front")) > 0.f;
            const FVector C = bPlaced ? Bit.Pos + Up * 88.f : Now;
            const float Wide = Part.Value(TEXT("wide"), 70.f) * (1.f + Part.Value(TEXT("flare")) * Bit.Phase);
            Pos = C + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * (Wide + 40.f * Bit.Phase) + Up * (-80.f + Part.Value(TEXT("height"), 240.f) * Bit.Phase * Smooth(B / .5f));
            Scale = FVector(.1f * Bit.Size * Pop);
            Rot = FRotator(0, FMath::RadiansToDegrees(Ang), 45.f);
        }
        else if (Part.Type == TEXT("beam"))
        {
            const float Len = Radius * (Part.Value(TEXT("long")) > 0.f ? 8.f : 4.5f);
            const FVector Dir = FRotator(Part.Value(TEXT("pitch")), Forward.Rotation().Yaw + Bit.Phase * Part.Value(TEXT("spread"), 50.f), 0).Vector();
            const float Flash = (T < Bit.Delay) ? 0.f : 1.f - Smooth((B - .2f) / .8f);
            Pos = Origin + Up * 45.f + Dir * Len * .5f;
            Rot = Dir.Rotation();
            Scale = FVector(Len / 100.f * Smooth(B / .1f), .09f * Bit.Size * Flash, .09f * Bit.Size * Flash);
        }
        else if (Part.Type == TEXT("feathers"))
        {
            const float Tt = FMath::Max(0.f, T - Bit.Delay);
            Pos = Bit.Pos + Bit.Vel * Tt + Right * 30.f * FMath::Sin(Tt * 4.f + Bit.Phase) + Forward * 20.f * FMath::Cos(Tt * 3.f + Bit.Phase);
            Rot = FRotator(25.f * FMath::Sin(Tt * 5.f + Bit.Phase), Bit.Phase * 60.f + Tt * 90.f, 15.f);
            Scale = FVector(.32f, .12f, .02f) * Bit.Size * Pop;
        }
        else if (Part.Type == TEXT("bolt"))
        {
            const bool bOn = B < 1.f && (int32(T * 24.f) % 3 != 0);
            Rot = Bit.Vel.Rotation();
            Scale = bOn ? FVector(Bit.Vel.Size() / 100.f, .07f * Bit.Size, .07f * Bit.Size) : FVector(.001f);
        }
        Out.Add(FTransform(Rot, Pos, Scale.ComponentMax(FVector(.001f))));
    }
    if (Part.Type == TEXT("streak") && H)
    {
        FRandomStream Rng(Part.Bits.Num() * 31 + 7);
        const FVector Body = Now + FVector(0, 0, Part.Value(TEXT("height"), -20.f));
        const FVector Last = Part.Bits.Num() ? Part.Bits.Last().Vel : Body;
        if (U < Part.Value(TEXT("until"), .6f))
            for (float D = 0.f, L = FVector::Dist(Last, Body); D <= L; D += 28.f)
            {
                FBit Bit;
                Bit.Vel = FMath::Lerp(Last, Body, L > 0.f ? D / L : 1.f);
                Bit.Pos = Bit.Vel + Rng.GetUnitVector() * 35.f * Part.Value(TEXT("spray"), 1.f);
                Bit.Delay = T;
                Bit.Rot = FRotator(Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180), 0);
                Bit.Size = Size * Rng.FRandRange(.6f, 1.2f);
                Part.Bits.Add(Bit);
                if (L <= 0.f) break;
            }
        for (const FBit& Bit : Part.Bits)
        {
            const float Age_ = T - Bit.Delay, S = FMath::Max(0.f, 1.f - Age_ / Part.Value(TEXT("fade"), .45f));
            Out.Add(FTransform(Bit.Rot + FRotator(0, Age_ * 200.f, 0), Bit.Pos + FVector(0, 0, Age_ * Part.Value(TEXT("rise"), 40.f)), FVector(.14f * Bit.Size * S + .001f)));
        }
    }
    // o rastro: um bloco novo na ponta da arma a cada quadro, encolhendo
    if (Part.Type == TEXT("trail") && H && U < 1.f)
    {
        if (const AVortexCharacter* VH = Cast<AVortexCharacter>(H))
            if (const UStaticMeshComponent* Blade = VH->GetWeaponVisual())
            {
                const FBoxSphereBounds B = Blade->Bounds;
                const FVector Tip = B.Origin + (B.Origin - Blade->GetComponentLocation()).GetSafeNormal() * B.SphereRadius * .8f;
                TrailPoints.Add(Tip);
                TrailAges.Add(0.f);
            }
        for (int32 i = 0; i < TrailPoints.Num(); ++i)
        {
            TrailAges[i] += Dt;
            const float S = FMath::Max(0.f, 1.f - TrailAges[i] / .35f);
            Out.Add(FTransform(FRotator(TrailAges[i] * 300.f, i * 37.f, 0), TrailPoints[i], FVector(.09f * Size * S)));
        }
    }
    if (Part.Ism->GetInstanceCount() != Out.Num())
    {
        Part.Ism->ClearInstances();
        Part.Ism->AddInstances(Out, false, true);
    }
    else Part.Ism->BatchUpdateInstancesTransforms(0, Out, true, true);
}

void AVortexPowerFx::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    for (FPart& Part : Parts)
    {
        const float T = Age - Part.Delay;
        if (T < 0.f) continue;
        if (!Part.bStarted) StartPart(Part);
        TickPart(Part, FMath::Min(T, Part.Life));
    }
    TickSprites();
    float SpritesEnd = 0.f;
    for (const FSprite& S : Sprites) SpritesEnd = FMath::Max(SpritesEnd, S.Start + S.Life);
    if (Age > FMath::Max(Lifetime, SpritesEnd + .05f)) Destroy();
}
