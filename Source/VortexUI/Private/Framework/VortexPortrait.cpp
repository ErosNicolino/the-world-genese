#include "Framework/VortexPortrait.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/VortexCharacter.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"

namespace VortexPortrait
{
    FString PathFor(const FGuid& PlayerId)
    {
        return FPaths::ProjectSavedDir() / TEXT("Portraits") / (PlayerId.ToString(EGuidFormats::Digits) + TEXT("_v3.png"));   // _v3 (07/11): de frente, com folga; os antigos são refeitos
    }

    /** A foto de frente (256 x 256) de qualquer personagem de blocos com o osso Head: o herói (Stature da raça) ou um
     *  morador (07/11, o vendedor na loja). false se a malha não está pronta. */
    static bool Shoot(ACharacter* Who, float Stature, TArray<FColor>& Pixels)
    {
        if (!Who || !Who->GetWorld() || Who->GetNetMode() == NM_DedicatedServer) return false;
        USkeletalMeshComponent* Mesh = Who->GetMesh();
        if (!Mesh || Mesh->GetBoneIndex(TEXT("Head")) == INDEX_NONE) return false;
        // 07/11: só com o corpo, a roupa e o cabelo prontos (logo depois de importar, a Unreal ainda monta os modelos:
        // a foto saía só com os olhos)
        TInlineComponentArray<UMeshComponent*> Parts(Who);
        for (const UMeshComponent* Part : Parts)
        {
            if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Part))
                if (Skinned->GetSkinnedAsset() && (Skinned->GetSkinnedAsset()->IsCompiling() || !Skinned->IsRenderStateCreated())) return false;
            if (const UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(Part))
                if (Static->GetStaticMesh() && Static->GetStaticMesh()->IsCompiling()) return false;
        }
        const FVector Head = Mesh->GetBoneLocation(TEXT("Head"));
        // 07/11 (o usuário: "a fotinho bem estranha; algo bonito, tirado de frente, estilo retrato, com sobra em cima do
        // cabelo e embaixo do queixo"): de frente, o centro no meio da cabeça com o cabelo (o osso fica na base dela),
        // longe o bastante para a cabeça de bloco (grande) ocupar ~55% da altura
        const FVector Face = Head + FVector(0.f, 0.f, 25.f * Stature);
        const FVector Forward = Who->GetActorForwardVector().GetSafeNormal2D();
        const FVector Eye = Face + Forward * 215.f * FMath::Max(.8f, Stature) + FVector(0.f, 0.f, 6.f);

        UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
        Target->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
        Target->InitCustomFormat(256, 256, PF_B8G8R8A8, false);
        USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(Who);
        Capture->bCaptureEveryFrame = false;
        Capture->bCaptureOnMovement = false;
        Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        // só a cor do modelo (sem luz da cena): igual de dia e de noite, como o retrato do Godot
        Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
        Capture->ShowFlags.SetLighting(false);
        Capture->ShowFlags.SetAtmosphere(false);
        Capture->ShowFlags.SetFog(false);
        Capture->ShowFlags.SetBloom(false);
        Capture->ShowFlags.SetEyeAdaptation(false);
        Capture->FOVAngle = 30.f;
        Capture->TextureTarget = Target;
        Capture->ShowOnlyActors.Add(Who);
        Capture->RegisterComponentWithWorld(Who->GetWorld());
        Capture->SetWorldLocationAndRotation(Eye, (Face - Eye).Rotation());
        Capture->CaptureScene();
        FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
        const bool bRead = Resource && Resource->ReadPixels(Pixels) && Pixels.Num() == 256 * 256;
        Capture->DestroyComponent();
        if (!bRead) return false;
        // o fundo (o que não é o personagem sai preto puro): o azul-noite da HUD, mais claro atrás da cabeça
        for (int32 Y = 0; Y < 256; ++Y)
            for (int32 X = 0; X < 256; ++X)
            {
                FColor& C = Pixels[Y * 256 + X];
                if (C.R + C.G + C.B <= 3)
                {
                    const float D = FMath::Clamp(FVector2D(X - 128.f, Y - 100.f).Size() / 190.f, 0.f, 1.f);
                    const FLinearColor Mid(.30f, .38f, .58f), Edge(.06f, .08f, .15f);
                    C = FMath::Lerp(Mid, Edge, D * D).ToFColor(false);
                }
                C.A = 255;
            }
        return true;
    }

    bool CaptureAndSave(AVortexCharacter* Hero, const FGuid& PlayerId)
    {
        if (!Hero || !PlayerId.IsValid()) return false;
        TArray<FColor> Pixels;
        if (!Shoot(Hero, Hero->GetBodyShape().Stature() * Hero->GetBodyShape().Head, Pixels)) return false;
        TArray64<uint8> Png;
        FImageUtils::PNGCompressImageArray(256, 256, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(PathFor(PlayerId)), true);
        const bool bSaved = FFileHelper::SaveArrayToFile(Png, *PathFor(PlayerId));
        UE_LOG(LogTemp, Display, TEXT("VORTEX_PORTRAIT %s %s"), bSaved ? TEXT("salvo") : TEXT("FALHOU"), *PathFor(PlayerId));
        return bSaved;
    }

    UTexture2D* CaptureFace(ACharacter* Who)
    {
        static TMap<FString, TStrongObjectPtr<UTexture2D>> Cache;
        if (!Who) return nullptr;
        if (TStrongObjectPtr<UTexture2D>* Found = Cache.Find(Who->GetName())) return Found->Get();
        TArray<FColor> Pixels;
        if (!Shoot(Who, 1.f, Pixels)) return nullptr;
        UTexture2D* Texture = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);
        if (!Texture) return nullptr;
        Texture->SRGB = true;
        void* Data = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
        Texture->UpdateResource();
        Cache.Add(Who->GetName(), TStrongObjectPtr<UTexture2D>(Texture));
        return Texture;
    }

    UTexture2D* Load(const FGuid& PlayerId)
    {
        static TMap<FGuid, TStrongObjectPtr<UTexture2D>> Cache;
        if (TStrongObjectPtr<UTexture2D>* Found = Cache.Find(PlayerId)) return Found->Get();
        const FString Path = PathFor(PlayerId);
        if (!PlayerId.IsValid() || !FPaths::FileExists(Path)) return nullptr;
        UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Path);
        if (Texture) Cache.Add(PlayerId, TStrongObjectPtr<UTexture2D>(Texture));
        return Texture;
    }
}
