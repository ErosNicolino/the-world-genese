// 16/11 (o usuário: "crie uma corda e sela quando domar ele e a corda com física"): os arreios do cavalo domado. A sela é de
// blocos (como o resto do jogo), presa ao osso "Saddle"; as rédeas são duas cordas de elos (Verlet, como o chicote em
// VortexWeaponParts.cpp) da boca até as mãos de quem monta, ou até o arção da sela com o cavalo solto. Só desenho: roda em
// cada máquina, nada vai pela rede. O estado fica aqui para não mexer no VortexMonster.h.
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Creatures/VortexMonster.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Framework/VortexCharacter.h"
#include "Materials/MaterialInterface.h"

namespace VortexHorseTack
{
    static constexpr int32 Links = 10;   // elos por rédea

    struct FTack
    {
        TWeakObjectPtr<UInstancedStaticMeshComponent> Leather, Cloth, Rope;
        FVector Points[2][Links + 1], Old[2][Links + 1];
        FVector MouthLocal = FVector::ZeroVector;   // a boca, no espaço do osso da cabeça
        float Rest[2] = { 0.f, 0.f };
        bool bStarted = false;
    };
    static TMap<TWeakObjectPtr<const AVortexMonster>, FTack> Tacks;

    // 16/11 (o usuário: "os traços do andar"): as pegadas. Um par de marcas escuras no chão a cada batida, alternando as
    // patas de um lado e do outro; as 60 últimas ficam (as mais velhas somem). Um lote só para todos os cavalos da máquina
    // shortcut: a marca é um disco deitado na altura do pé: em ladeira forte entra um pouco no chão; trocar por decalque
    // se incomodar
    void Hoof(AVortexMonster* Horse, const FVector& Feet, float Yaw)
    {
        static TWeakObjectPtr<UInstancedStaticMeshComponent> Marks;
        static TWeakObjectPtr<UWorld> MarksWorld;
        static int32 Next = 0, Side = 0;
        if (!Horse || Horse->GetNetMode() == NM_DedicatedServer) return;
        if (!Marks.IsValid() || MarksWorld.Get() != Horse->GetWorld())
        {
            AActor* Holder = Horse->GetWorld()->SpawnActor<AActor>();
            if (!Holder) return;
            UInstancedStaticMeshComponent* Made = NewObject<UInstancedStaticMeshComponent>(Holder);
            Made->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
            Made->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Weapons/Materials/MI_W_couro_escuro.MI_W_couro_escuro")));
            Made->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Made->SetCastShadow(false);
            Holder->SetRootComponent(Made);
            Made->RegisterComponent();
            Marks = Made;
            MarksWorld = Horse->GetWorld();
            Next = 0;
        }
        const FQuat Facing = FRotator(0.f, Yaw, 0.f).Quaternion();
        const float S = FMath::Max(.2f, Horse->SaddleHeight() / 112.f);
        Side = 1 - Side;
        // a pata da frente e a de trás do mesmo lado (no modelo: 18 cm do meio; 25 à frente e 65 atrás)
        for (const float Along : { 25.f, -65.f })
        {
            const FTransform Mark(Facing, Feet + Facing.RotateVector(FVector(Along, Side ? 18.f : -18.f, 0.f) * S) + FVector(0.f, 0.f, 1.2f), FVector(.13f * S, .1f * S, .012f));
            if (Marks->GetInstanceCount() < 60) Marks->AddInstance(Mark, true);
            else Marks->UpdateInstanceTransform(Next % 60, Mark, true, true);
            ++Next;
        }
    }

    static UInstancedStaticMeshComponent* Make(AVortexMonster* Horse, const TCHAR* Mesh, const TCHAR* Material)
    {
        UInstancedStaticMeshComponent* Part = NewObject<UInstancedStaticMeshComponent>(Horse);
        Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Mesh));
        Part->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Materials/%s.%s"), Material, Material)));
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetUsingAbsoluteLocation(true);
        Part->SetUsingAbsoluteRotation(true);
        Part->SetUsingAbsoluteScale(true);
        Part->SetupAttachment(Horse->GetRootComponent());
        Part->RegisterComponent();
        return Part;
    }

    void Tick(AVortexMonster* Horse, float Dt)
    {
        USkeletalMeshComponent* Body = Horse->GetMesh();
        const bool bWear = Horse->GetTamer() && !Horse->IsCreatureDead() && !Horse->IsHidden() && Body && Body->GetBoneIndex(TEXT("Saddle")) != INDEX_NONE;
        FTack* Found = Tacks.Find(Horse);
        if (!bWear)
        {
            if (Found)
            {
                for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& Part : { Found->Leather, Found->Cloth, Found->Rope }) if (Part.IsValid()) Part->DestroyComponent();
                Tacks.Remove(Horse);
            }
            return;
        }
        if (!Found)
        {
            for (auto It = Tacks.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
            Found = &Tacks.Add(Horse);
        }
        FTack& T = *Found;
        // o modelo (Data/Modelos/Cavalo/Horse_Skinned.glb) tem a sela a 1,12 m do chão: daí sai o tamanho de tudo (cm do
        // modelo -> cm do mundo)
        const float S = FMath::Max(.2f, Horse->SaddleHeight() / 112.f);
        const FQuat Facing = FRotator(0.f, Horse->GetActorRotation().Yaw, 0.f).Quaternion();
        const FVector Seat = Body->GetBoneLocation(TEXT("Saddle"));
        auto At = [&](float X, float Y, float Z) { return Seat + Facing.RotateVector(FVector(X, Y, Z) * S); };
        if (!T.Leather.IsValid())
        {
            T.Leather = Make(Horse, TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("MI_W_couro"));
            T.Cloth = Make(Horse, TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("MI_W_tecido_vermelho"));
            T.Rope = Make(Horse, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("MI_W_corda"));
            // a boca fica 27 cm à frente e 10 abaixo do osso da cabeça (Bone_028) no descanso; guardada no espaço do
            // osso, para descer junto quando ele pasta
            const FReferenceSkeleton& Ref = Body->GetSkeletalMeshAsset()->GetRefSkeleton();
            FTransform Pose = FTransform::Identity;
            for (int32 Bone = Ref.FindBoneIndex(TEXT("Bone_028")); Bone != INDEX_NONE; Bone = Ref.GetParentIndex(Bone)) Pose = Pose * Ref.GetRefBonePose()[Bone];
            const FVector InBody = Body->GetComponentTransform().InverseTransformVectorNoScale(Facing.RotateVector(FVector(27.f, 0.f, -10.f) * S));
            T.MouthLocal = Pose.InverseTransformVectorNoScale(InBody);
        }
        // ---- a sela: {meio (frente, lado, cima), tamanho} em cm do modelo. O lombo tem 52 de largura na sela e o
        // pescoço começa a subir 20 cm à frente dela (por isso a sela fica um pouco para trás do osso)
        struct FBlock { FVector At, Size; };
        static const FBlock LeatherBlocks[] = {
            { { -4.f, 0.f, 4.f }, { 44.f, 36.f, 7.f } },                                            // o assento
            { { 16.f, 0.f, 12.f }, { 6.f, 14.f, 12.f } },                                           // o arção (frente)
            { { -25.f, 0.f, 10.f }, { 6.f, 30.f, 10.f } },                                          // a patilha (trás)
            { { -2.f, 27.5f, -16.f }, { 28.f, 3.f, 36.f } }, { { -2.f, -27.5f, -16.f }, { 28.f, 3.f, 36.f } },   // as abas
            { { 0.f, 29.5f, -42.f }, { 3.f, 2.f, 22.f } }, { { 0.f, -29.5f, -42.f }, { 3.f, 2.f, 22.f } },       // os loros
            { { 0.f, 31.f, -55.f }, { 12.f, 9.f, 3.f } }, { { 0.f, -31.f, -55.f }, { 12.f, 9.f, 3.f } },         // os estribos
        };
        static const FBlock ClothBlocks[] = {
            { { -5.f, 0.f, 0.f }, { 56.f, 54.f, 3.f } },                                            // a manta
            { { -5.f, 26.5f, -9.f }, { 56.f, 2.f, 20.f } }, { { -5.f, -26.5f, -9.f }, { 56.f, 2.f, 20.f } },
        };
        auto Place = [&](UInstancedStaticMeshComponent* Part, TArrayView<const FBlock> Blocks)
        {
            TArray<FTransform> Out;
            for (const FBlock& B : Blocks) Out.Add(FTransform(Facing, At(B.At.X, B.At.Y, B.At.Z), B.Size * S / 100.f));
            if (Part->GetInstanceCount() != Out.Num()) { Part->ClearInstances(); Part->AddInstances(Out, false, true); }
            else Part->BatchUpdateInstancesTransforms(0, Out, true, true);
        };
        Place(T.Leather.Get(), LeatherBlocks);
        Place(T.Cloth.Get(), ClothBlocks);
        // ---- as rédeas: da boca (um lado e o outro) até as mãos do cavaleiro ou até o arção
        const FTransform Head = Body->GetSocketTransform(TEXT("Bone_028"));
        const FVector Mouth = Head.GetLocation() + Head.TransformVectorNoScale(T.MouthLocal);
        const AVortexCharacter* Rider = Horse->GetRider();
        const float Feet = Horse->GetActorLocation().Z - Horse->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        Dt = FMath::Min(Dt, 1.f / 30.f);
        TArray<FTransform> Segments;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const float Sign = Side == 0 ? 1.f : -1.f;
            const FVector From = Mouth + Facing.RotateVector(FVector(0.f, 8.f * Sign * S, 0.f));
            const FName Hand = Side == 0 ? TEXT("Hand_R") : TEXT("Hand_L");
            const FVector To = Rider && Rider->GetMesh()->DoesSocketExist(Hand) ? Rider->GetMesh()->GetSocketLocation(Hand) : At(16.f, 5.f * Sign, 19.f);
            FVector* P = T.Points[Side];
            FVector* Old = T.Old[Side];
            // de início, ou depois de um salto (assobio de longe, montar): a corda nasce reta, sem chicotear
            if (!T.bStarted || FVector::DistSquared(P[0], From) > FMath::Square(250.f))
                for (int32 i = 0; i <= Links; ++i) Old[i] = P[i] = FMath::Lerp(From, To, float(i) / Links);
            // a folga: 12% a mais que a reta, e a corda estica se a mão for mais longe (não segura o cavaleiro)
            T.Rest[Side] = FMath::Max(FVector::Dist(From, To) * 1.02f, FMath::FInterpTo(T.Rest[Side], FVector::Dist(From, To) * 1.12f, Dt, 4.f));
            const float Link = T.Rest[Side] / Links;
            for (int32 i = 1; i < Links; ++i)
            {
                const FVector V = (P[i] - Old[i]) * .93f;
                Old[i] = P[i];
                P[i] += V + FVector(0.f, 0.f, -980.f) * Dt * Dt;
                P[i].Z = FMath::Max(P[i].Z, Feet + 2.f);
            }
            for (int32 Pass = 0; Pass < 10; ++Pass)
            {
                P[0] = From;
                P[Links] = To;
                for (int32 i = 0; i < Links; ++i)
                {
                    const FVector Delta = P[i + 1] - P[i];
                    const float Len = FMath::Max(Delta.Size(), .01f);
                    const FVector Fix = Delta * ((Len - Link) / Len) * .5f;
                    if (i > 0) P[i] += Fix;
                    if (i + 1 < Links) P[i + 1] -= Fix;
                }
            }
            for (int32 i = 0; i < Links; ++i)
            {
                const FVector Dir = (P[i + 1] - P[i]).GetSafeNormal();
                Segments.Add(FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Dir.IsNearlyZero() ? FVector::UpVector : Dir), (P[i] + P[i + 1]) * .5f,
                    FVector(1.6f * S / 100.f, 1.6f * S / 100.f, FVector::Dist(P[i], P[i + 1]) * 1.05f / 100.f)));
            }
        }
        T.bStarted = true;
        // a focinheira: uma volta de corda em torno do focinho, onde as rédeas prendem
        Segments.Add(FTransform(Facing * FRotator(0.f, 0.f, 90.f).Quaternion(), Mouth, FVector(1.6f * S / 100.f, 1.6f * S / 100.f, 19.f * S / 100.f)));
        UInstancedStaticMeshComponent* Rope = T.Rope.Get();
        if (Rope->GetInstanceCount() != Segments.Num()) { Rope->ClearInstances(); Rope->AddInstances(Segments, false, true); }
        else Rope->BatchUpdateInstancesTransforms(0, Segments, true, true);
    }
}
