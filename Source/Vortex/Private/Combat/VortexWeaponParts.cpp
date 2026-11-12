#include "Combat/VortexWeaponParts.h"

#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponKit.h"
#include "Combat/VortexSwords.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "StaticMeshResources.h"

namespace
{
    // 08/11: o que fica guardado em "static" precisa de AddToRoot: senão a limpeza de memória apaga ao trocar de mapa e a
    // próxima partida usa lixo (crash ao recomeçar, "Pure virtual function being called")
    template <typename T> T* Keep(T* Object) { if (Object) Object->AddToRoot(); return Object; }

    UStaticMesh* LoadMesh(const TCHAR* Path) { return LoadObject<UStaticMesh>(nullptr, Path); }

    /** Peça de arma importada (Tools/ImportVortexWeapons.py): na pasta dos modelos ou na subpasta que o importador cria. */
    UStaticMesh* LoadPart(const TCHAR* Name)
    {
        if (UStaticMesh* Flat = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s.%s"), Name, Name))) return Flat;
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s/StaticMeshes/%s.%s"), Name, Name, Name));
    }

    /** Eixo de giro de uma peça do livro (no espaço da arma): a dobradiça sobra para fora da peça,
     *  então fica no lado do eixo médio que vai mais longe; o eixo comprido é o da lombada. */
    void Hinge(const UStaticMesh* Mesh, FVector& Pivot, FVector& Axis)
    {
        const FBox Box = Mesh->GetBoundingBox();
        const FVector Size = Box.GetSize();
        int32 Long = 0, Mid = 0, Thin = 0;
        for (int32 A = 0; A < 3; ++A) { if (Size[A] > Size[Long]) Long = A; if (Size[A] < Size[Thin]) Thin = A; }
        Mid = 3 - Long - Thin;
        Pivot = Box.GetCenter();
        Pivot[Mid] = FMath::Abs(Box.Max[Mid]) > FMath::Abs(Box.Min[Mid]) ? Box.Max[Mid] : Box.Min[Mid];
        Axis = FVector::ZeroVector;
        Axis[Long] = 1.f;
    }

    FTransform Around(const FVector& Pivot, const FVector& Axis, float Degrees)
    {
        const FQuat Q(Axis, FMath::DegreesToRadians(Degrees));
        return FTransform(Q, Pivot - Q.RotateVector(Pivot));
    }
}

UVortexWeaponParts::UVortexWeaponParts()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;   // depois da pose: a corda sai da mão onde ela está
}

float UVortexWeaponParts::StrikeAlpha(const AVortexCharacter* Hero) const
{
    const UVortexWeaponComponent* Weapon = Hero ? Hero->GetWeapon() : nullptr;
    const UVortexWeaponDefinition* D = Weapon ? Weapon->GetDefinition() : nullptr;
    if (!D) return 0.f;
    TArray<float> Hits;
    if (D->Swings.IsValidIndex(LastSwing)) Hits.Add(D->Swings[LastSwing].ActiveStart / FMath::Max(.1f, Weapon->GetAttackSpeed()));
    else if (LastSwing == VortexSpecialPose::Release)
    {
        // corrente: 3 golpes rápidos; chicote: o gancho
        if (const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(D->WeaponId); Kit && Kit->Family == TEXT("Chain")) Hits = { .15f, .42f, .69f };
        else Hits = { .3f };
    }
    float Best = 0.f;
    for (const float T : Hits)
    {
        const float Rise = FMath::Clamp((SwingAge - (T - .12f)) / .12f, 0.f, 1.f);
        const float Fall = 1.f - FMath::Clamp((SwingAge - (T + .08f)) / .22f, 0.f, 1.f);
        Best = FMath::Max(Best, Rise * Fall);
    }
    return Best;
}

void UVortexWeaponParts::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    if (!Hero || GetNetMode() == NM_DedicatedServer) return;
    const UVortexWeaponComponent* Weapon = Hero->GetWeapon();
    if (Weapon)
    {
        const FVortexAttackState& Attack = Weapon->GetAttackState();
        if (Attack.Serial != LastSerial)
        {
            LastSerial = Attack.Serial;
            if (Attack.SwingIndex != INDEX_NONE) { LastSwing = Attack.SwingIndex; SwingAge = 0.f; }
        }
    }
    SwingAge += DeltaTime;
    // a arma no corpo e a da primeira pessoa
    UStaticMeshComponent* Wanted[2] = { Hero->GetWeaponVisual(), Hero->GetViewWeapon() };
    Holders.SetNum(2);
    for (int32 i = 0; i < 2; ++i)
    {
        if (Holders[i].Weapon.Get() != Wanted[i]) { Clear(Holders[i]); Holders[i].Weapon = Wanted[i]; }
        if (Wanted[i]) Tick(Holders[i], Hero, DeltaTime);
    }
}

void UVortexWeaponParts::Clear(FHolder& H)
{
    if (H.Cover.IsValid()) H.Cover->DestroyComponent();
    for (TWeakObjectPtr<UStaticMeshComponent>& P : H.Pages) if (P.IsValid()) P->DestroyComponent();
    if (H.Rope.IsValid()) H.Rope->DestroyComponent();
    if (H.Tip.IsValid()) H.Tip->DestroyComponent();
    H = FHolder();
}

void UVortexWeaponParts::Tick(FHolder& H, AVortexCharacter* Hero, float DeltaTime)
{
    const UVortexWeaponComponent* Weapon = Hero->GetWeapon();
    const UVortexWeaponDefinition* D = Weapon ? Weapon->GetDefinition() : nullptr;
    const VortexWeaponKit::FKit* Kit = D ? VortexWeaponKit::Find(D->WeaponId) : nullptr;
    const FName Family = Kit ? Kit->Family : NAME_None;
    if (Family != H.Family)
    {
        TWeakObjectPtr<UStaticMeshComponent> Keep = H.Weapon;
        Clear(H);
        H.Weapon = Keep;
        H.Family = Family;
    }
    if (Family == TEXT("Book")) TickBook(H, DeltaTime);
    else if (Family == TEXT("Whip") || Family == TEXT("Chain")) TickRope(H, Hero, DeltaTime);
    else if (Family == TEXT("Bow")) TickBowString(H, Hero, DeltaTime);
}

void UVortexWeaponParts::TickBook(FHolder& H, float DeltaTime)
{
    UStaticMeshComponent* Book = H.Weapon.Get();
    if (!Book) return;
    auto Make = [&](UStaticMesh* Mesh)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetOwner());
        Part->SetupAttachment(Book);
        Part->SetStaticMesh(Mesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->RegisterComponent();
        return Part;
    };
    if (!H.Cover.IsValid())
    {
        UStaticMesh* Cover = LoadPart(TEXT("W_livro_da_aurora_capa"));
        UStaticMesh* Page = LoadPart(TEXT("W_livro_da_aurora_pagina"));
        if (!Cover || !Page) return;
        H.Cover = Make(Cover);
        for (int32 i = 0; i < 3; ++i) H.Pages.Add(Make(Page));
    }
    // a capa e as folhas copiam quem vê a arma (corpo ou primeira pessoa)
    auto Same = [&](UStaticMeshComponent* Part)
    {
        Part->SetVisibility(Book->IsVisible());
        Part->SetOnlyOwnerSee(Book->bOnlyOwnerSee);
        Part->SetOwnerNoSee(Book->bOwnerNoSee);
        Part->SetCastShadow(Book->CastShadow);
    };
    // abre ao usar (e fica aberto um pouco), fecha parado
    const bool bUsing = SwingAge < 1.6f && LastSwing != INDEX_NONE;
    Open = FMath::FInterpTo(Open, bUsing ? 1.f : 0.f, DeltaTime, bUsing ? 14.f : 5.f);
    UStaticMeshComponent* Cover = H.Cover.Get();
    FVector Pivot, Axis;
    Hinge(Cover->GetStaticMesh(), Pivot, Axis);
    // para que lado abre: a capa sai para longe do miolo do livro
    const FVector Center = Cover->GetStaticMesh()->GetBoundingBox().GetCenter();
    const FVector Core = Book->GetStaticMesh() ? Book->GetStaticMesh()->GetBoundingBox().GetCenter() : FVector::ZeroVector;
    const FVector Out = (Center - Core);
    const FVector Test = Around(Pivot, Axis, 10.f).TransformPosition(Center);
    const float Sign = FVector::DotProduct(Test - Center, Out) >= 0.f ? 1.f : -1.f;
    Same(Cover);
    Cover->SetRelativeTransform(Around(Pivot, Axis, Sign * 165.f * Open));
#if !UE_BUILD_SHIPPING
    static int32 Logged = 0;
    if (Logged < 3 && Open > .5f)
    {
        ++Logged;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_BOOK pivot=%s axis=%s sign=%.0f cover=%s core=%s box=%s"), *Pivot.ToCompactString(), *Axis.ToCompactString(), Sign,
            *Center.ToCompactString(), *Core.ToCompactString(), *Cover->GetStaticMesh()->GetBoundingBox().ToString());
    }
#endif
    for (int32 i = 0; i < H.Pages.Num(); ++i)
    {
        UStaticMeshComponent* Page = H.Pages[i].Get();
        if (!Page) continue;
        Same(Page);
        FVector PagePivot, PageAxis;
        Hinge(Page->GetStaticMesh(), PagePivot, PageAxis);
        // as folhas passam rapidinho uma depois da outra, logo depois da capa abrir
        const float Flip = bUsing ? FMath::Clamp((SwingAge - .12f - i * .09f) / .12f, 0.f, 1.f) : Open;
        Page->SetRelativeTransform(Around(PagePivot, PageAxis, Sign * (160.f - i * 4.f) * FMath::Min(Flip, Open)));
    }
}

void UVortexWeaponParts::TickRope(FHolder& H, AVortexCharacter* Hero, float DeltaTime)
{
    UStaticMeshComponent* Handle = H.Weapon.Get();
    if (!Handle || !Handle->GetStaticMesh()) return;
    const bool bChain = H.Family == TEXT("Chain");
    const int32 N = bChain ? 16 : 18;
    const float Length = bChain ? 190.f : 250.f;
    if (!H.Rope.IsValid())
    {
        UInstancedStaticMeshComponent* Rope = NewObject<UInstancedStaticMeshComponent>(GetOwner());
        Rope->SetStaticMesh(LoadMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Rope->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, bChain ? TEXT("/Game/Vortex/Weapons/Materials/MI_W_aco_escuro.MI_W_aco_escuro")
            : TEXT("/Game/Vortex/Weapons/Materials/MI_W_couro.MI_W_couro")));
        Rope->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rope->SetUsingAbsoluteLocation(true);
        Rope->SetUsingAbsoluteRotation(true);
        Rope->SetUsingAbsoluteScale(true);
        Rope->SetupAttachment(GetOwner()->GetRootComponent());
        Rope->RegisterComponent();
        H.Rope = Rope;
        UStaticMeshComponent* Tip = NewObject<UStaticMeshComponent>(GetOwner());
        Tip->SetStaticMesh(bChain ? LoadPart(TEXT("W_correntes_lamina")) : LoadMesh(TEXT("/Engine/BasicShapes/Cone.Cone")));
        if (!bChain) { Tip->SetMaterial(0, Rope->GetMaterial(0)); }
        Tip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Tip->SetUsingAbsoluteLocation(true);
        Tip->SetUsingAbsoluteRotation(true);
        Tip->SetUsingAbsoluteScale(true);
        Tip->SetupAttachment(GetOwner()->GetRootComponent());
        Tip->RegisterComponent();
        H.Tip = Tip;
    }
    UInstancedStaticMeshComponent* Rope = H.Rope.Get();
    UStaticMeshComponent* Tip = H.Tip.Get();
    for (UPrimitiveComponent* P : { (UPrimitiveComponent*)Rope, (UPrimitiveComponent*)Tip })
    {
        P->SetVisibility(Handle->IsVisible());
        P->SetOnlyOwnerSee(Handle->bOnlyOwnerSee);
        P->SetOwnerNoSee(Handle->bOwnerNoSee);
        P->SetCastShadow(!Handle->bOnlyOwnerSee);
    }
    // a corda sai da ponta do cabo (o lado mais longe da pegada)
    const FBox Box = Handle->GetStaticMesh()->GetBoundingBox();
    int32 Axis = 0;
    for (int32 A = 1; A < 3; ++A) if (FMath::Max(FMath::Abs(Box.Min[A]), FMath::Abs(Box.Max[A])) > FMath::Max(FMath::Abs(Box.Min[Axis]), FMath::Abs(Box.Max[Axis]))) Axis = A;
    FVector Local = Box.GetCenter();
    Local[Axis] = FMath::Abs(Box.Max[Axis]) >= FMath::Abs(Box.Min[Axis]) ? Box.Max[Axis] : Box.Min[Axis];
    const FVector Anchor = Handle->GetComponentTransform().TransformPosition(Local);
    if (H.Points.Num() != N + 1)
    {
        H.Points.SetNum(N + 1);
        for (int32 i = 0; i <= N; ++i) H.Points[i] = Anchor - FVector(0.f, 0.f, Length * i / N);
        H.Old = H.Points;
    }
    // no golpe: a ponta vai até o alcance na frente (o chicote estala, a corrente se estica até 5 m)
    const UVortexWeaponComponent* Weapon = Hero->GetWeapon();
    const UVortexWeaponDefinition* D = Weapon ? Weapon->GetDefinition() : nullptr;
    const float Reach = D && D->Swings.Num() ? D->Swings[0].ReachCm : 400.f;
    const float Strike = StrikeAlpha(Hero);
    const float Rest = FMath::Lerp(Length, FMath::Max(Length, Reach), Strike) / N;
    const float Side = LastSwing == 1 ? 1.f : LastSwing == 2 ? -.6f : 0.f;
    const FVector Forward = Hero->GetActorForwardVector().GetSafeNormal2D();
    const FVector Goal = Hero->GetActorLocation() + Forward * Reach + Hero->GetActorRightVector() * Side * Reach * .3f * (1.f - Strike) + FVector(0.f, 0.f, 10.f);
    const float Dt = FMath::Min(DeltaTime, 1.f / 30.f);
    const float Feet = Hero->GetActorLocation().Z - 68.f;
    H.Points[0] = Anchor;
    for (int32 i = 1; i <= N; ++i)
    {
        FVector& P = H.Points[i];
        const FVector V = (P - H.Old[i]) * .975f;
        H.Old[i] = P;
        P += V + FVector(0.f, 0.f, -980.f) * Dt * Dt * (1.f - Strike);
        if (Strike > 0.f)
        {
            // puxada: cada elo vai para a reta entre a mão e o alvo (mais forte na ponta)
            const FVector Line = FMath::Lerp(Anchor, Goal, float(i) / N);
            P = FMath::Lerp(P, Line, FMath::Clamp(Strike * (.25f + .6f * i / N), 0.f, 1.f));
        }
        P.Z = FMath::Max(P.Z, Feet + 1.5f);
    }
    for (int32 Pass = 0; Pass < 8; ++Pass)
    {
        H.Points[0] = Anchor;
        for (int32 i = 0; i < N; ++i)
        {
            FVector& A = H.Points[i];
            FVector& B = H.Points[i + 1];
            const FVector Delta = B - A;
            const float Len = FMath::Max(Delta.Size(), .01f);
            const FVector Fix = Delta * ((Len - Rest) / Len) * .5f;
            if (i > 0) { A += Fix; B -= Fix; }
            else B -= Fix * 2.f;   // o primeiro ponto fica preso na mão
        }
    }
    // desenho: um cilindro por trecho (o chicote afina até a ponta; a corrente alterna os elos)
    TArray<FTransform> Segments;
    Segments.Reserve(N);
    for (int32 i = 0; i < N; ++i)
    {
        const FVector A = H.Points[i], B = H.Points[i + 1];
        const FVector Dir = (B - A).GetSafeNormal();
        const float Len = FVector::Dist(A, B);
        const float R = bChain ? 1.6f : FMath::Lerp(1.8f, .45f, float(i) / N);
        FQuat Q = FQuat::FindBetweenNormals(FVector::UpVector, Dir.IsNearlyZero() ? FVector::UpVector : Dir);
        if (bChain) Q = Q * FQuat(FVector::UpVector, (i % 2) ? HALF_PI : 0.f);
        const FVector Scale = bChain ? FVector(R * 2.2f / 100.f, R * .9f / 100.f, Len * 1.1f / 100.f) : FVector(R / 50.f, R / 50.f, Len / 100.f);
        Segments.Add(FTransform(Q, (A + B) * .5f, Scale));
    }
    if (Rope->GetInstanceCount() != Segments.Num())
    {
        Rope->ClearInstances();
        Rope->AddInstances(Segments, false, true);
    }
    else Rope->BatchUpdateInstancesTransforms(0, Segments, true, true);
    const FVector End = H.Points[N];
    const FVector Last = (H.Points[N] - H.Points[N - 1]).GetSafeNormal();
    if (bChain) Tip->SetWorldTransform(FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Last), End, Handle->GetComponentScale()));
    else Tip->SetWorldTransform(FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Last), End, FVector(.03f, .03f, .06f)));
}

void UVortexWeaponParts::TickBowString(FHolder& H, AVortexCharacter* Hero, float DeltaTime)
{
    UStaticMeshComponent* Bow = H.Weapon.Get();
    UStaticMesh* Mesh = Bow ? Bow->GetStaticMesh() : nullptr;
    if (!Mesh) return;
    if (!H.bTipsFound)
    {
        // as pontas: a média dos vértices nos 4% de cada extremo do eixo comprido (Tools/ImportNewWeapons.py liga o
        // acesso da CPU aos vértices do arco); sem os vértices, o meio das faces da caixa
        const FBox Box = Mesh->GetBoundingBox();
        int32 Axis = 0;
        for (int32 A = 1; A < 3; ++A) if (Box.GetSize()[A] > Box.GetSize()[Axis]) Axis = A;
        H.TipA = H.TipB = Box.GetCenter();
        H.TipA[Axis] = Box.Max[Axis];
        H.TipB[Axis] = Box.Min[Axis];
        const FStaticMeshRenderData* Render = Mesh->GetRenderData();
        if (Mesh->bAllowCPUAccess && Render && Render->LODResources.Num())
        {
            const FPositionVertexBuffer& Positions = Render->LODResources[0].VertexBuffers.PositionVertexBuffer;
            const float Band = Box.GetSize()[Axis] * .04f;
            FVector SumA = FVector::ZeroVector, SumB = FVector::ZeroVector;
            int32 CountA = 0, CountB = 0;
            for (uint32 i = 0; i < Positions.GetNumVertices(); ++i)
            {
                const FVector P = FVector(Positions.VertexPosition(i));
                if (P[Axis] > Box.Max[Axis] - Band) { SumA += P; ++CountA; }
                else if (P[Axis] < Box.Min[Axis] + Band) { SumB += P; ++CountB; }
            }
            if (CountA) H.TipA = SumA / CountA;
            if (CountB) H.TipB = SumB / CountB;
            // a corda passa um pouco para dentro das pontas
            H.TipA[Axis] -= Band * .5f;
            H.TipB[Axis] += Band * .5f;
        }
        H.bTipsFound = true;
    }
    if (!H.Rope.IsValid())
    {
        UInstancedStaticMeshComponent* Rope = NewObject<UInstancedStaticMeshComponent>(GetOwner());
        Rope->SetStaticMesh(LoadMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        UMaterialInterface* Cord = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Weapons/Materials/Novas/M_BowString.M_BowString"));
        Rope->SetMaterial(0, Cord ? Cord : LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Weapons/Materials/MI_W_couro.MI_W_couro")));
        Rope->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rope->SetUsingAbsoluteLocation(true);
        Rope->SetUsingAbsoluteRotation(true);
        Rope->SetUsingAbsoluteScale(true);
        Rope->SetupAttachment(GetOwner()->GetRootComponent());
        Rope->RegisterComponent();
        H.Rope = Rope;
        H.Nock = FVector::ZeroVector;
        H.NockSpeed = FVector::ZeroVector;
    }
    UInstancedStaticMeshComponent* Rope = H.Rope.Get();
    Rope->SetVisibility(Bow->IsVisible());
    Rope->SetOnlyOwnerSee(Bow->bOnlyOwnerSee);
    Rope->SetOwnerNoSee(Bow->bOwnerNoSee);
    Rope->SetCastShadow(!Bow->bOnlyOwnerSee);
    // 08/11 (o usuário: "fazer igual o minecraft"): na primeira pessoa o arco fica preso à câmera, sem braço: embaixo à
    // direita descansando; puxando vem para perto da mira, de perfil, com a flecha apontando reto para a frente
    const bool bOnCamera = Bow->bOnlyOwnerSee && Bow->GetAttachParent() && !Cast<USkeletalMeshComponent>(Bow->GetAttachParent());
    FVector ViewFront = FVector::ZeroVector;
    float ViewDepth = 0.f;
    const float Charge = Hero->GetWeapon() ? Hero->GetWeapon()->DrawCharge() : 0.f;
    if (bOnCamera)
    {
        const bool bPulling = LastSwing == VortexSpecialPose::Hold && SwingAge < 3.2f;
        H.ViewDraw = FMath::FInterpTo(H.ViewDraw, bPulling ? 1.f : 0.f, DeltaTime, 14.f);
        const FBox Box = Mesh->GetBoundingBox();
        const FVector Mid0 = (H.TipA + H.TipB) * .5f, Long0 = (H.TipA - H.TipB).GetSafeNormal();
        // o eixo da curva da madeira: o mais comprido depois do arco; "frente" é da corda para a madeira
        int32 LongAxis = 0;
        for (int32 Ax = 1; Ax < 3; ++Ax) if (Box.GetSize()[Ax] > Box.GetSize()[LongAxis]) LongAxis = Ax;
        int32 Curve = LongAxis == 0 ? 1 : 0;
        for (int32 Ax = 0; Ax < 3; ++Ax) if (Ax != LongAxis && Box.GetSize()[Ax] > Box.GetSize()[Curve]) Curve = Ax;
        FVector Front0 = FVector::ZeroVector;
        Front0[Curve] = Box.GetCenter()[Curve] >= Mid0[Curve] ? 1.f : -1.f;
        ViewDepth = FMath::Max(Box.Max[Curve] - Mid0[Curve], Mid0[Curve] - Box.Min[Curve]);
        const float D = FMath::SmoothStep(0.f, 1.f, H.ViewDraw);
        const FQuat Mesh0 = FRotationMatrix::MakeFromXZ(Front0, Long0).ToQuat();
        const FQuat Want = FRotationMatrix::MakeFromXZ(FMath::Lerp(FVector(.92f, -.2f, .12f), FVector(.9f, -.38f, .16f), D), FMath::Lerp(FVector(.05f, -.22f, .97f), FVector(.05f, -.12f, .99f), D)).ToQuat();
        const FQuat Turn = Want * Mesh0.Inverse();
        const float Size = 58.f / FMath::Max(1.f, FVector::Dist(H.TipA, H.TipB));
        // puxada cheia: treme de leve
        const float Shake = Charge >= 1.f && bPulling ? FMath::Sin(SwingAge * 70.f) * .18f : 0.f;
        const FVector At = FMath::Lerp(FVector(40.f, 28.f, -17.f), FVector(38.f, 19.f, -11.f), D) + FVector(0.f, Shake, Shake * .6f);
        Bow->SetRelativeTransform(FTransform(Turn, At - Turn.RotateVector(Mid0 * Size), FVector(Size)));
        ViewFront = Bow->GetAttachParent()->GetComponentTransform().TransformVectorNoScale(Want.GetAxisX());
        ViewDepth *= Size;
    }
    const FTransform& World = Bow->GetComponentTransform();
    const FVector A = World.TransformPosition(H.TipA);
    const FVector B = World.TransformPosition(H.TipB);
    const FVector Rest = (A + B) * .5f;
    // puxando: a corda vai até a mão que não segura o arco (no máximo 75% do comprimento do arco para trás)
    FVector Goal = Rest;
    if (bOnCamera) { if (LastSwing == VortexSpecialPose::Hold && SwingAge < 3.2f) Goal = Rest - ViewFront * FVector::Dist(A, B) * (.1f + .25f * Charge); }
    else if (LastSwing == VortexSpecialPose::Hold && SwingAge < 3.2f)
    {
        USkeletalMeshComponent* Body = Cast<USkeletalMeshComponent>(Bow->GetAttachParent());
        if (Body)
        {
            const FVector R = Body->GetSocketLocation(TEXT("Hand_R")), L = Body->GetSocketLocation(TEXT("Hand_L"));
            const FVector Grip = Bow->GetComponentLocation();
            const FVector Hand = FVector::DistSquared(R, Grip) > FVector::DistSquared(L, Grip) ? R : L;
            const FVector Pull = Hand - Rest;
            const float Max = FVector::Dist(A, B) * .75f;
            // a corda estica junto da animação de puxar (0,25 s até a mão chegar)
            Goal = Rest + Pull.GetClampedToMaxSize(Max) * FMath::Clamp(SwingAge / .25f, 0.f, 1.f);
        }
    }
    // solta: mola rápida e pouco amortecida (a corda vibra), presa no ponto da mão enquanto puxa
    const FVector Offset = H.Nock;
    if (Goal != Rest) { H.Nock = Goal - Rest; H.NockSpeed = FVector::ZeroVector; }
    else if (bOnCamera) { H.Nock = FMath::VInterpTo(H.Nock, FVector::ZeroVector, DeltaTime, 18.f); H.NockSpeed = FVector::ZeroVector; }   // perto da câmera a mola jogava a corda pela tela
    else
    {
        const float Dt = FMath::Min(DeltaTime, 1.f / 30.f);
        H.NockSpeed += (-Offset * 2600.f - H.NockSpeed * 9.f) * Dt;
        H.Nock += H.NockSpeed * Dt;
        if (H.Nock.SizeSquared() < .0004f && H.NockSpeed.SizeSquared() < .01f) { H.Nock = FVector::ZeroVector; H.NockSpeed = FVector::ZeroVector; }
    }
    const FVector Mid = Rest + H.Nock;
    // 03/11: a flecha encaixada no meio da corda, apontando para o cabo (e daí para a mira), enquanto puxa; some ao soltar
    // (a flecha de verdade sai do arco: AVortexProjectile)
    {
        const bool bNocked = LastSwing == VortexSpecialPose::Hold && SwingAge < 3.2f && SwingAge > .06f;
        if (!H.Nocked.IsValid())
        {
            UStaticMeshComponent* Arrow = NewObject<UStaticMeshComponent>(GetOwner());
            Arrow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Weapons/Models/W_flecha/StaticMeshes/W_flecha.W_flecha")));
            Arrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Arrow->SetUsingAbsoluteLocation(true);
            Arrow->SetUsingAbsoluteRotation(true);
            Arrow->SetUsingAbsoluteScale(true);
            Arrow->SetupAttachment(GetOwner()->GetRootComponent());
            Arrow->RegisterComponent();
            H.Nocked = Arrow;
        }
        UStaticMeshComponent* Arrow = H.Nocked.Get();
        // 07/11: a flecha do arco equipado (cada arco tem a sua)
        if (bNocked)
        {
            static UStaticMesh* Plain = Keep(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Weapons/Models/W_flecha/StaticMeshes/W_flecha.W_flecha")));
            UStaticMesh* Own = VortexSwords::ArrowMesh(GetOwner());
            if (UStaticMesh* Want = Own ? Own : Plain; Want && Arrow->GetStaticMesh() != Want) Arrow->SetStaticMesh(Want);
        }
        Arrow->SetVisibility(Bow->IsVisible() && bNocked);
        Arrow->SetOnlyOwnerSee(Bow->bOnlyOwnerSee);
        Arrow->SetOwnerNoSee(Bow->bOwnerNoSee);
        Arrow->SetCastShadow(!Bow->bOnlyOwnerSee);
        if (bNocked && Arrow->GetStaticMesh())
        {
            // 05/11: do tamanho de verdade (antes ficava pequena: supunha 80 cm e ainda encolhia pela escala do arco).
            // O fim encosta na corda (Mid) e a ponta passa da madeira do cabo (12% do comprimento do arco, no mínimo
            // 8 cm). A malha aponta para +X, com a ponta no X maior; a grossura acompanha, sem passar de 1,3x.
            const FVector Grip = bOnCamera ? Rest + ViewFront * ViewDepth : Bow->GetComponentLocation();
            const FVector Dir = (Grip - Mid).GetSafeNormal();
            const FBox Box = Arrow->GetStaticMesh()->GetBoundingBox();
            const float MeshLen = Box.GetSize().X;
            if (!Dir.IsNearlyZero() && MeshLen > 1.f)
            {
                const float Past = FMath::Max(8.f, FVector::Dist(A, B) * .12f);
                const float Len = FVector::Dist(Grip, Mid) + Past;
                const float Long = Len / MeshLen;
                const float Thick = bOnCamera ? .6f : FMath::Clamp(Long, .8f, 1.3f);   // perto da câmera a pena tomava a tela
                const FVector Tip = Grip + Dir * Past;
                Arrow->SetWorldTransform(FTransform(Dir.Rotation(), Tip - Dir * (Box.Max.X * Long), FVector(Long, Thick, Thick)));
            }
        }
    }
    // dois trechos: ponta de cima -> meio -> ponta de baixo (fios de ~0,5 cm)
    TArray<FTransform> Segments;
    for (const TPair<FVector, FVector>& S : { TPair<FVector, FVector>(A, Mid), TPair<FVector, FVector>(Mid, B) })
    {
        const FVector Dir = (S.Value - S.Key).GetSafeNormal();
        const float Len = FVector::Dist(S.Key, S.Value);
        const float Thick = .5f * Bow->GetComponentScale().X;
        Segments.Add(FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Dir.IsNearlyZero() ? FVector::UpVector : Dir), (S.Key + S.Value) * .5f,
            FVector(Thick / 50.f, Thick / 50.f, Len / 100.f)));
    }
    if (Rope->GetInstanceCount() != Segments.Num())
    {
        Rope->ClearInstances();
        Rope->AddInstances(Segments, false, true);
    }
    else Rope->BatchUpdateInstancesTransforms(0, Segments, true, true);
}
