#include "Framework/VortexAimHUD.h"
#include "Creatures/VortexNpcRoutine.h"

#include "Framework/VortexProbe.h"
#include "AbilitySystemComponent.h"
#include "Combat/VortexClassSkills.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "CanvasItem.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Framework/VortexPortrait.h"
#include "Framework/VortexAudio.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Creatures/VortexNpc.h"
#include "Creatures/VortexWolf.h"
#include "Engine/Canvas.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Font.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Framework/VortexGameState.h"
#include "Framework/VortexGameInstance.h"
#include "Persistence/VortexPlayerSave.h"
#include "Rpg/VortexHeroSheet.h"
#include "Online/VortexOnline.h"
#include "World/VortexAwakeningStadium.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Framework/VortexPlayerSettings.h"
#include "Framework/VortexIntroComic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Containers/Ticker.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/Parse.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexCreatureSubsystem.h"
#include "PipelineStateCache.h"
#include "ShaderPipelineCache.h"
#include "World/VortexTown.h"
#include "World/VortexForage.h"
#include "World/VortexWorldFeatures.h"
#include "World/VortexPointsOfInterest.h"
#include "World/VortexPortal.h"
#include "World/VortexDungeons.h"
#include "World/VortexLandmarks.h"
#include "Rpg/VortexSocial.h"
#include "Framework/VortexSocialUi.h"
#include "World/VortexTerrainData.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "World/VortexFountain.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexMonster.h"
#include "Framework/VortexDialogue.h"
#include "Framework/VortexGameMode.h"
#include "Rpg/VortexQuestLog.h"
#include "Rpg/VortexStory.h"
#include "CollisionQueryParams.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    // Medidas das molduras em pixels da textura (ui/art/hud/layout.json do Godot).
    struct FRectPx { float X0, Y0, X1, Y1; };
    const FVector2D PlayerSize(1100, 342), BossSize(1100, 282), MapFrameSize(1100, 1116), BarSize(1100, 247);
    const FVector3f PlayerPortrait(164, 169, 109), BossPortrait(119, 129, 73), MapCircle(552, 551, 426);
    const FRectPx PlayerName{ 299, 54, 700, 98 }, PlayerSub{ 299, 105, 700, 142 }, PlayerHp{ 278, 155, 1086, 219 },
        PlayerMp{ 283, 219, 1044, 276 }, PlayerXp{ 249, 283, 1031, 313 };
    const FRectPx BossName{ 229, 62, 600, 110 }, BossHp{ 181, 113, 1076, 172 }, BossCast{ 294, 189, 966, 253 },
        BossCastText{ 307, 203, 946, 237 };
    const FRectPx MapTitle{ 401, 63, 704, 139 }, MapArea{ 178, 964, 713, 1044 }, MapClock{ 761, 964, 960, 1044 };
    const FRectPx Slots[] = { { 128, 81, 232, 180 }, { 263, 81, 370, 180 }, { 414, 81, 535, 180 },
        { 565, 81, 667, 180 }, { 700, 81, 807, 180 }, { 846, 81, 964, 180 } };
    const FRectPx Keys[] = { { 138, 181, 222, 217 }, { 270, 181, 362, 217 }, { 438, 181, 508, 217 },
        { 581, 181, 651, 217 }, { 719, 181, 789, 217 }, { 872, 181, 943, 217 } };

    const FLinearColor HudCream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor HudGold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));
    const FLinearColor HudMuted = FLinearColor::FromSRGBColor(FColor(0xD8, 0xC0, 0x8A));
    const FLinearColor Navy = FLinearColor::FromSRGBColor(FColor(0x0B, 0x14, 0x28));

    // peça da HUD: origem na tela e escala (pixels da tela por pixel da textura)
    struct FPiece
    {
        float X, Y, S;
        float PX(float V) const { return X + V * S; }
        float PY(float V) const { return Y + V * S; }
    };

    const TCHAR* WolfDoing(EVortexWolfState State)
    {
        switch (State)
        {
        case EVortexWolfState::Observe: return TEXT("Observando");
        case EVortexWolfState::Howl: return TEXT("Uivando");
        case EVortexWolfState::Chase: return TEXT("Caçando");
        case EVortexWolfState::Circle: return TEXT("Rondando");
        case EVortexWolfState::Bite: return TEXT("Mordida!");
        case EVortexWolfState::Pounce: return TEXT("Bote!");
        case EVortexWolfState::Retreat: return TEXT("Recuando");
        case EVortexWolfState::ReturnHome: return TEXT("Voltando à toca");
        case EVortexWolfState::Hit: return TEXT("Atordoado");
        case EVortexWolfState::Dead: return TEXT("Derrotado");
        default: return TEXT("Calmo");
        }
    }

    FString Thousands(int32 Value)
    {
        // 2.984 (milhar com ponto, como no Godot em pt-BR)
        FString Digits = FString::FromInt(FMath::Abs(Value)), Out;
        for (int32 i = 0; i < Digits.Len(); ++i)
        {
            if (i > 0 && (Digits.Len() - i) % 3 == 0) Out.AppendChar(TEXT('.'));
            Out.AppendChar(Digits[i]);
        }
        return Value < 0 ? TEXT("-") + Out : Out;
    }

    FVector FaceOf(const AActor* Actor, float FallbackForward, float FallbackUp)
    {
        if (const ACharacter* Character = Cast<ACharacter>(Actor))
        {
            if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
            {
                for (const TCHAR* Bone : { TEXT("head"), TEXT("Head"), TEXT("mixamorig:Head"), TEXT("Bip001-Head"), TEXT("neck"), TEXT("Neck") })
                {
                    if (Mesh->GetBoneIndex(Bone) != INDEX_NONE) return Mesh->GetBoneLocation(Bone);
                }
            }
        }
        return Actor->GetActorLocation() + Actor->GetActorForwardVector() * FallbackForward + FVector(0, 0, FallbackUp);
    }
}

AVortexAimHUD::AVortexAimHUD()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AVortexAimHUD::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    // teste da HQ em rede (-VortexIntroNet=host|client, Tools/RunIntroNetworkSmoke.ps1): com os dois jogadores
    // conectados, o anfitrião abre a HQ, o cliente abre a dele 5 s depois, cada um conta onde está, o anfitrião fecha
    // e o cliente confere que a dele continua: cada máquina tem a sua, com o próprio relógio e o próprio som
    FString NetRole;
    if (FParse::Value(FCommandLine::Get(), TEXT("VortexIntroNet="), NetRole))
    {
        TWeakObjectPtr<AVortexAimHUD> Weak(this);
        const bool bHost = NetRole == TEXT("host");
        TSharedRef<double> Start = MakeShared<double>(0.0);
        TSharedRef<int32> Step = MakeShared<int32>(0);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, bHost, Start, Step, NetRole](float) -> bool
        {
            AVortexAimHUD* Hud = Weak.Get();
            if (!Hud || !Hud->GetWorld()) return false;
            const AGameStateBase* State = Hud->GetWorld()->GetGameState();
            const int32 Players = State ? State->PlayerArray.Num() : 0;
            if (*Start <= 0.0)
            {
                if (Players < 2 || !Hud->GetOwningPawn()) return true;
                *Start = FPlatformTime::Seconds();
                UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_NET papel=%s conectados jogadores=%d"), *NetRole, Players);
            }
            const double T = FPlatformTime::Seconds() - *Start;
            auto Log = [&](const TCHAR* What) { UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_NET papel=%s %s aberta=%d jogadores=%d estado=%s"), *NetRole, What, VortexIntroComic::IsOpen() ? 1 : 0, Players, *VortexIntroComic::DebugState()); };
            const double OpenAt = bHost ? 3.0 : 8.0;
            if (*Step == 0 && T >= OpenAt) { VortexIntroComic::Open(Hud->GetOwningPlayerController()); Log(TEXT("abriu")); ++*Step; }
            else if (*Step == 1 && T >= 12.0) { Log(TEXT("no-meio")); ++*Step; }
            else if (*Step == 2 && T >= 15.0) { if (bHost) { VortexIntroComic::Close(); Log(TEXT("fechou")); } ++*Step; }
            else if (*Step == 3 && T >= 19.0) { Log(bHost ? TEXT("depois") : TEXT("depois-do-anfitriao-fechar")); ++*Step; }
            else if (*Step == 4 && T >= 21.0) { if (!bHost) { VortexIntroComic::Close(); Log(TEXT("fechou")); } ++*Step; }
            // o anfitrião espera o cliente terminar (ele começa a contar depois)
            else if (*Step == 5 && T >= (bHost ? 60.0 : 24.0)) { UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_NET papel=%s fim"), *NetRole); FPlatformMisc::RequestExit(false); return false; }
            return true;
        }));
    }
#endif
    if (UMaterialInterface* Portrait = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/UI/Hud/M_HudPortrait.M_HudPortrait")))
    {
        HeroPortrait = UMaterialInstanceDynamic::Create(Portrait, this);
        TargetPortrait = UMaterialInstanceDynamic::Create(Portrait, this);
    }
    if (UMaterialInterface* Map = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/UI/Hud/M_HudMinimap.M_HudMinimap")))
    {
        Minimap = UMaterialInstanceDynamic::Create(Map, this);
    }
    // retratos: uma câmera só, que fotografa o rosto numa textura
    auto MakeTarget = [this]()
    {
        UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(this);
        Target->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
        Target->InitCustomFormat(256, 256, PF_B8G8R8A8, false);
        return Target;
    };
    HeroTarget = MakeTarget();
    WolfTarget = MakeTarget();
    Capture = NewObject<USceneCaptureComponent2D>(this, TEXT("PortraitCapture"));
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->bAlwaysPersistRenderingState = false;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    // só a cor do modelo (sem luz da cena): o retrato fica igual de dia e de noite; o fundo vazio
    // sai preto e o material troca por azul-noite
    Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
    Capture->ShowFlags.SetLighting(false);
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetVolumetricFog(false);
    Capture->ShowFlags.SetCloud(false);
    Capture->ShowFlags.SetBloom(false);
    Capture->ShowFlags.SetMotionBlur(false);
    Capture->ShowFlags.SetEyeAdaptation(false);
    Capture->ShowFlags.SetLightShafts(false);
    Capture->ShowFlags.SetLensFlares(false);
    Capture->RegisterComponentWithWorld(GetWorld());
}

void AVortexAimHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if (MapJob.IsValid()) MapJob.Wait();
    if (FogJob.IsValid()) FogJob.Wait();
    if (bSeenUnsaved) SaveDiscovery();
    Super::EndPlay(Reason);
}

UTexture2D* AVortexAimHUD::Tex(FName Name)
{
    if (TObjectPtr<UTexture2D>* Found = Textures.Find(Name)) return *Found;
    const FString Path = FString::Printf(TEXT("/Game/Vortex/UI/Hud/T_%s.T_%s"), *Name.ToString(), *Name.ToString());
    UTexture2D* Loaded = LoadObject<UTexture2D>(nullptr, *Path);
    Textures.Add(Name, Loaded);
    return Loaded;
}

// Textura inteira, ou só a parte esquerda (barras: Fill de 0 a 1).
void AVortexAimHUD::Image(FName Name, float X, float Y, float W, float H, float Fill, const FLinearColor& Tint)
{
    UTexture2D* Texture = Tex(Name);
    if (!Texture || Fill <= 0.f) return;
    Fill = FMath::Min(Fill, 1.f);
    FCanvasTileItem Tile(FVector2D(X, Y), Texture->GetResource(), FVector2D(W * Fill, H), FVector2D(0, 0), FVector2D(Fill, 1), Tint);
    Tile.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Tile);
}

FVector2D AVortexAimHUD::Measure(const FString& Text, const FSlateFontInfo& Font) const
{
    if (!FSlateApplication::IsInitialized()) return FVector2D(Text.Len() * Font.Size * .55f, Font.Size * 1.3f);
    return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
}

// Texto com contorno escuro e sombra, centrado na altura da caixa. Size = altura em pixels da tela.
void AVortexAimHUD::Label(const FString& Text, float X, float Y, float W, float H, float Size, const FLinearColor& Color,
    int32 Align, bool bBold)
{
    if (Text.IsEmpty() || Color.A <= .01f) return;
    // a fonte da Slate mede em pontos (96 ppp): 0,75 ponto por pixel
    FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FMath::Max(6, FMath::RoundToInt(Size * .75f)));
    Font.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(Size / 14.f));
    Font.OutlineSettings.OutlineColor = FLinearColor(.06f, .04f, .02f, .95f * Color.A);
    const FVector2D Extent = Measure(Text, Font);
    const float TX = Align == 1 ? X + (W - Extent.X) * .5f : Align == 2 ? X + W - Extent.X : X;
    const float TY = Y + (H - Extent.Y) * .5f;
    FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(TX), FMath::RoundToFloat(TY)), FText::FromString(Text), Font, Color);
    // o Canvas só desenha texto com uma UFont de cache em tempo real; a forma vem do FSlateFontInfo
    if (!TextFont) TextFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    Item.Font = TextFont;
    // sombra só nos textos grandes: o contorno já separa do fundo, e a sombra desenhava cada texto duas vezes
    // (a HUD custava ~1,3 ms por quadro, boa parte no texto)
    if (Size >= 30.f) Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, .7f * Color.A), FVector2D(0.f, FMath::Max(1.f, Size / 14.f)));
    Canvas->DrawItem(Item);
}

void AVortexAimHUD::ExportPortraits(const FString& Prefix) const
{
    for (const auto& [Target, Name] : { TPair<UTextureRenderTarget2D*, const TCHAR*>(HeroTarget, TEXT("heroi")), TPair<UTextureRenderTarget2D*, const TCHAR*>(WolfTarget, TEXT("lobo")) })
    {
        FImage Image;
        if (Target && FImageUtils::GetRenderTargetImage(Target, Image))
        {
            FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots") / (Prefix + Name + TEXT(".png"))), Image);
        }
    }
}

void AVortexAimHUD::Disc(float CX, float CY, float Radius, const FLinearColor& Fill, const FLinearColor& Ring)
{
    FCanvasNGonItem Outer(FVector2D(CX, CY), FVector2D(Radius, Radius), 32, Ring);
    Canvas->DrawItem(Outer);
    FCanvasNGonItem Inner(FVector2D(CX, CY), FVector2D(Radius * .86f, Radius * .86f), 32, Fill);
    Canvas->DrawItem(Inner);
}

void AVortexAimHUD::DrawHUD()
{
    VORTEX_PROBE("HUD");
    Super::DrawHUD();
#if !UE_BUILD_SHIPPING
    // -VortexProbeLog: o tempo de cada parte (VortexProbe) no log a cada 10 s, para medir o jogo de verdade
    static const bool bProbeLog = FParse::Param(FCommandLine::Get(), TEXT("VortexProbeLog"));
    static double NextProbeLog = 0.0;
    if (bProbeLog && FPlatformTime::Seconds() > NextProbeLog)
    {
        NextProbeLog = FPlatformTime::Seconds() + 10.0;
        TArray<TPair<FString, VortexProbe::FEntry>> Rows;
        for (const TPair<FString, VortexProbe::FEntry>& Pair : VortexProbe::Entries()) Rows.Add(Pair);
        Rows.Sort([](const auto& A, const auto& B) { return A.Value.TotalMs > B.Value.TotalMs; });
        for (const auto& Row : Rows)
            UE_LOG(LogTemp, Display, TEXT("VORTEX_PROBE %-22s total=%8.1fms media=%6.2fms max=%6.1fms vezes=%d"), *Row.Key, Row.Value.TotalMs, Row.Value.TotalMs / FMath::Max(1, Row.Value.Calls), Row.Value.MaxMs, Row.Value.Calls);
        VortexProbe::Entries().Reset();
    }
#endif
    AVortexCharacter* Hero = PlayerOwner ? Cast<AVortexCharacter>(PlayerOwner->GetPawn()) : nullptr;
    // nas medições de desempenho a HUD some, a menos que -VortexBenchHud (para medir o custo dela)
    static const bool bBenchHud = FParse::Param(FCommandLine::Get(), TEXT("VortexBenchHud"));
    if (!Canvas || !Hero || (Hero->IsGodotBenchCamera() && !bBenchHud)) return;
    const float Now = GetWorld()->GetTimeSeconds();
    // Abra antes de desenhar qualquer HUD para que a cutscene jÃ¡ cubra o primeiro quadro do herÃ³i.
    const UVortexHeroSheet* IntroSheet = Hero->GetSheet();
    const UVortexGameInstance* IntroGame = Cast<UVortexGameInstance>(GetGameInstance());
    const FString HeroId = IntroGame && IntroGame->GetSelectedHeroId().IsValid() ? IntroGame->GetSelectedHeroId().ToString() : TEXT("local");
    const FString Line = FCommandLine::Get();
    // testes automáticos (todos passam -Vortex...): a HQ nunca abre sozinha; os testes dela a abrem quando querem
    const bool bIntroTest = Line.Contains(TEXT("-Vortex"));
    static TSet<FString> AskedForIntro;
    // a primeira vez que cada herói entra no mundo (no Estádio ou já no vale), a HQ abre sozinha
    if (!bIntroTest && IntroSheet && IntroSheet->IsLoaded() && !VortexIntroComic::IsOpen()
        && !UVortexPlayerSettings::Get()->HasWatchedIntro(HeroId) && !AskedForIntro.Contains(HeroId) && Now > .1f)
    {
        AskedForIntro.Add(HeroId);
        VortexIntroComic::Open(GetOwningPlayerController(), [HeroId]() { UVortexPlayerSettings::Get()->MarkIntroSeen(HeroId); });
    }
    if (VortexIntroComic::IsOpen()) return;
    const float DeltaSeconds = LastDrawTime < 0.f ? 0.f : FMath::Clamp(Now - LastDrawTime, 0.f, .25f);
    LastDrawTime = Now;
    Scale = Canvas->ClipY / 1080.f;
    if (BoundHero.Get() != Hero)
    {
        if (UVortexCombatComponent* Combat = Hero->GetCombat()) Combat->OnConfirmedHit.AddUniqueDynamic(this, &AVortexAimHUD::OnHeroHit);
        BoundHero = Hero;
        LastHeroHealth = -1.f;
    }
    // mira só para armas de longe (ou segurando F): de perto o herói bate no que está à frente
    if (!bWorldMap)
    {
        const AVortexCharacter* Aimer = Cast<AVortexCharacter>(GetOwningPawn());
        if (!Aimer || !Aimer->IsMeleeWeapon() || Aimer->IsAimHeld() || Aimer->IsFirstPerson()) DrawCrosshair();
        DrawWeaponAim(Hero);
    }
    if (UVortexPlayerSettings::Get()->bShowFps && DeltaSeconds > 0.f)
    {
        // média curta para o número não pular
        FpsShown = FpsShown <= 0.f ? 1.f / DeltaSeconds : FMath::Lerp(FpsShown, 1.f / DeltaSeconds, .08f);
        Label(FString::Printf(TEXT("%d FPS  •  %s ms"), FMath::RoundToInt(FpsShown), *FString::Printf(TEXT("%.1f"), 1000.f / FpsShown).Replace(TEXT("."), TEXT(","))),
            Canvas->ClipX - 330.f * Scale, Canvas->ClipY - 40.f * Scale, 310.f * Scale, 30.f * Scale, 22.f * Scale, HudCream, 2);
    }
    // ao sair do Estádio do Despertar: a tela de carregamento cobre até o mundo montar
    if (const UVortexHeroSheet* Sheet = Hero->GetSheet())
    {
        if (bWasAwakening && !Sheet->NeedsAwakening()) StartLoading(true);
        bWasAwakening = Sheet->NeedsAwakening();
    }
    // 08/11 (o usuário: "aparece o mundo e depois a tela de carregamento"): a tela cobre na hora ao entrar no mundo e a cada
    // salto grande do herói (a saída do Despertar, o renascer, os portais), antes do primeiro quadro do lugar novo. Os
    // testes automáticos (-Vortex...) ficam como antes
    if (!bIntroTest)
    {
        const FVector HeroAt = Hero->GetActorLocation();
        if (!bEnteredWorld) { bEnteredWorld = true; StartLoading(true); }
        else if (FVector::Dist2D(HeroAt, LastHeroAt) > 4000.f) StartLoading(true);
        LastHeroAt = HeroAt;
    }
    if (DrawLoading(Hero, DeltaSeconds)) return;
    if (!UVortexPlayerSettings::Get()->bShowHud) { { VORTEX_PROBE("HUD.DrawDeath"); DrawDeath(Hero); } return; }
    RequestMinimap();
    { VORTEX_PROBE("HUD.UpdateDiscovery"); UpdateDiscovery(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.UpdateSpotted"); UpdateSpotted(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.MapaLocal"); UpdateLocalMaps(Hero); }
    // mapa aberto: tela cheia, só ele (e a tela de morte por cima)
    if (bWorldMap && !AVortexAwakeningStadium::Contains(Hero->GetActorLocation()))
    {
        { VORTEX_PROBE("HUD.DrawWorldMap"); DrawWorldMap(Hero); }
        FlushToon();
        { VORTEX_PROBE("HUD.DrawDeath"); DrawDeath(Hero); }
        return;
    }
    { VORTEX_PROBE("HUD.UpdatePortraits"); UpdatePortraits(Hero); }
    { VORTEX_PROBE("HUD.DrawNpcTags"); DrawNpcTags(Hero); }
    { VORTEX_PROBE("HUD.DrawCreaturePlates"); DrawCreaturePlates(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.DrawPlayer"); DrawPlayer(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.DrawBuffs"); DrawBuffs(Hero); }
    { VORTEX_PROBE("HUD.DrawParty"); DrawParty(Hero); }
    // no Estádio do Despertar não há mapa (o vale fica longe)
    const bool bStadium = AVortexAwakeningStadium::Contains(Hero->GetActorLocation());
    if (!bStadium) { VORTEX_PROBE("HUD.DrawMinimap"); DrawMinimap(Hero); }
    if (!bStadium) { VORTEX_PROBE("HUD.DrawQuestTracker"); DrawQuestTracker(Hero); }
    { VORTEX_PROBE("HUD.DrawPings"); DrawPings(Hero); }
    if (!bStadium) { VORTEX_PROBE("HUD.DrawInteractables"); DrawInteractables(Hero); }
    if (!bStadium && !bWorldMap) { VORTEX_PROBE("HUD.DrawCompass"); DrawCompass(Hero); }
    if (!bStadium && !bWorldMap) { VORTEX_PROBE("HUD.DrawPlaceBanner"); DrawPlaceBanner(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.DrawFocus"); DrawFocus(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.DrawTarget"); DrawTarget(Hero, DeltaSeconds); }
    { VORTEX_PROBE("HUD.DrawHotbar"); DrawHotbar(Hero); }
    { VORTEX_PROBE("HUD.DrawDamage"); DrawDamage(); }
    { VORTEX_PROBE("HUD.DrawNotices"); DrawNotices(Hero); }
    { VORTEX_PROBE("HUD.DrawStadium"); DrawStadium(Hero); }
    // 03/11: o tutorial de comandos (F1 abre e fecha; no Estádio aparece sozinho)
    if (const APlayerController* HelpInput = GetOwningPlayerController(); HelpInput && HelpInput->WasInputKeyJustPressed(EKeys::F1)) bShowControls = !bShowControls;
    // 07/11: no Estádio (herói novo) o tutorial passo a passo (Alpha -1); F1 abre a lista completa por cima
    // (começado no Estádio, segue fora dele até o último passo ou o Backspace)
    static bool bTutorialStarted = false;
    bTutorialStarted |= Hero->GetSheet() && Hero->GetSheet()->NeedsAwakening();
    if (bShowControls) DrawControls(Hero, 1.f);
    else if (bTutorialStarted) DrawControls(Hero, -1.f);
    if (Hero->IsEmoteWheelOpen()) { VORTEX_PROBE("HUD.DrawEmoteWheel"); DrawEmoteWheel(Hero); }
    { VORTEX_PROBE("HUD.Textos"); FlushToon(); }
    { VORTEX_PROBE("HUD.DrawDeath"); DrawDeath(Hero); }
}

void AVortexAimHUD::DrawEmoteWheel(AVortexCharacter* Hero)
{
    const TArray<AVortexCharacter::FEmote>& Page = AVortexCharacter::EmotePage(Hero->GetEmotePage());
    const int32 Choice = Hero->GetEmoteChoice();
    const float CX = Canvas->ClipX * .5f, CY = Canvas->ClipY * .5f, Radius = 250.f * Scale, Slot = 58.f * Scale;
    const FLinearColor Ink(.07f, .05f, .04f, .78f);
    Disc(CX, CY, 92.f * Scale, Ink, HudGold);
    Label(Hero->GetEmotePage() == 0 ? TEXT("Danças") : TEXT("Gestos"), CX - 90.f * Scale, CY - 34.f * Scale, 180.f * Scale, 40.f * Scale, 30.f * Scale, HudGold, 1);
    Label(TEXT("rodinha: trocar"), CX - 90.f * Scale, CY + 6.f * Scale, 180.f * Scale, 26.f * Scale, 17.f * Scale, HudCream, 1, false);
    for (int32 i = 0; i < Page.Num(); ++i)
    {
        // fatia 0 em cima, no sentido do relógio (como AVortexCharacter::GetEmoteChoice)
        const float Angle = FMath::DegreesToRadians(360.f * i / Page.Num());
        const float X = CX + FMath::Sin(Angle) * Radius, Y = CY - FMath::Cos(Angle) * Radius;
        const bool bHas = Hero->HasEmote(Page[i].Clip), bPicked = i == Choice;
        Disc(X, Y, bPicked ? Slot * 1.15f : Slot, bPicked ? FLinearColor(.36f, .24f, .08f, .92f) : Ink,
            bPicked ? HudGold : FLinearColor(HudCream.R, HudCream.G, HudCream.B, bHas ? .7f : .25f));
        Label(Page[i].Label, X - 70.f * Scale, Y - 15.f * Scale, 140.f * Scale, 30.f * Scale, (bPicked ? 21.f : 18.f) * Scale,
            bHas ? (bPicked ? HudGold : HudCream) : FLinearColor(.6f, .6f, .6f, .5f), 1);
    }
}

// ---- marcações e bússola --------------------------------------------------------------------------

// (VortexPings::All/Add: no jogo, Framework/VortexPings.cpp)

void AVortexAimHUD::DrawInteractables(AVortexCharacter* Hero)
{
    if (bWorldMap || VortexDialogue::IsOpen()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    UVortexForageSubsystem* Forage = GetWorld()->GetSubsystem<UVortexForageSubsystem>();
    // brilhos: estrelinhas verdes e douradas que piscam sobre cada coletável a até 14 m (Main.Forage)
    const double Clock = FPlatformTime::Seconds();
    if (Forage && !Hero->IsMounted())
    {
        // a lista dos coletáveis perto é refeita 4 vezes por segundo (o herói anda pouco nesse tempo)
        if (Clock - SparkleCheckAt > .25)
        {
            SparkleCheckAt = Clock;
            Sparkles.Reset();
            TArray<const FVortexForageSpot*> Near;
            Forage->Nearby(Hero->GetActorLocation(), 1400.f, Near);
            for (const FVortexForageSpot* Spot : Near)
                Sparkles.Add({ Spot->At, float(GetTypeHash(Spot->Id) % 1000) * .01f, Spot->Kind == TEXT("bush_berry") ? 75.f : 35.f });
        }
        for (const FSparkle& Spot : Sparkles)
        {
            const float Seed = Spot.Seed;
            const float Rise = Spot.Rise;
            const float Fade = FMath::Clamp((1400.f - FVector::Dist(Spot.At, Hero->GetActorLocation())) / 400.f, 0.f, 1.f);
            for (int32 k = 0; k < 3; ++k)
            {
                const float T = Now * (1.3f + .25f * k) + Seed + k * 2.1f;
                const float Twinkle = FMath::Max(0.f, FMath::Sin(T * 2.f));
                if (Twinkle < .05f) continue;
                const FVector At = Spot.At + FVector(FMath::Cos(T * .7f + k) * 30.f, FMath::Sin(T * .9f + k) * 30.f, Rise + 18.f * FMath::Sin(T * .5f) + k * 10.f);
                FVector2D Screen;
                if (!PlayerOwner || !PlayerOwner->ProjectWorldLocationToScreen(At, Screen)) continue;
                const float R = (4.f + 4.f * Twinkle) * Scale;
                const FLinearColor Color = k == 1 ? FLinearColor(1.f, .9f, .45f, Twinkle * Fade) : FLinearColor(.65f, 1.f, .55f, Twinkle * Fade);
                // estrela de quatro pontas: dois losangos finos cruzados
                const FVector2D C = Screen;
                const FVector2D Arms[2][4] = { { {0, -2.2f}, {.45f, 0}, {0, 2.2f}, {-.45f, 0} }, { {-2.2f, 0}, {0, .45f}, {2.2f, 0}, {0, -.45f} } };
                for (const auto& Arm : Arms)
                {
                    FCanvasTriangleItem A(C + Arm[0] * R, C + Arm[1] * R, C + Arm[3] * R, GWhiteTexture);
                    FCanvasTriangleItem B(C + Arm[2] * R, C + Arm[3] * R, C + Arm[1] * R, GWhiteTexture);
                    A.SetColor(Color); B.SetColor(Color);
                    A.BlendMode = B.BlendMode = SE_BLEND_Additive;
                    Canvas->DrawItem(A); Canvas->DrawItem(B);
                }
            }
        }
    }
    // o aviso do E (um por vez; falar com um morador tem a vez, e ele é desenhado nas etiquetas); as buscas
    // rodam ~8 vezes por segundo e o texto guardado é desenhado em todo quadro
    if (Clock - InteractCheckAt < .12)
    {
        if (!bInteractTalk && !InteractPrompt.IsEmpty())
            Label(InteractPrompt, 0.f, Canvas->ClipY * .66f, Canvas->ClipX, 36.f * Scale, 26.f * Scale, InteractColor, 1);
        return;
    }
    InteractCheckAt = Clock;
    InteractPrompt.Reset();
    bInteractTalk = VortexDialogue::FindTalkable(Hero) != nullptr;
    if (bInteractTalk) return;
    FString& Prompt = InteractPrompt;
    FLinearColor& Color = InteractColor;
    Color = HudGold;
    const AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(GetWorld());
    const AVortexPointsOfInterest* Pois = Hero->IsMounted() ? nullptr : AVortexPointsOfInterest::Find(GetWorld());
    const AVortexWorldFeatures::FChest* Chest = Features && !Hero->IsMounted() ? Features->NearestChest(Hero->GetActorLocation(), 240.f, Hero->GetQuestLog()) : nullptr;
    if (const AVortexCharacter* Other = VortexSocialUi::FindLookedAt(Hero); Other && !Hero->IsMounted())
        Prompt = FString::Printf(TEXT("[E]  Interagir com %s"), Other->GetSocial() ? *Other->GetSocial()->GetHeroName() : TEXT("o jogador"));
    else if (Hero->IsMounted()) Prompt = TEXT("[E]  Desmontar     •     [Shift]  galopar");
    else if (Chest && Chest->bGuarded && AVortexWorldFeatures::GuardsAlive(GetWorld(), Chest->Location) > 0) { Prompt = TEXT("Baú guardado: vença os guardas para abrir"); Color = HudCream; }
    else if (Chest) Prompt = Chest->bGuarded ? TEXT("[E]  Abrir o saque") : Chest->bRare ? TEXT("[E]  Abrir o baú raro") : TEXT("[E]  Abrir o baú");
    else if (const AVortexPortal* Portal = AVortexPortal::FindNear(GetWorld(), Hero->GetActorLocation(), 320.f))
    {
        Prompt = Portal->IsNoEscape() ? TEXT("Portal VERMELHO  •  ranque S  •  EM CONSTRUÇÃO")
            : FString::Printf(TEXT("Portal  •  ranque %s  •  EM CONSTRUÇÃO"), *Portal->Rank);
        Color = AVortexPortal::RankColor(Portal->Rank);
    }
    else if (const FVortexPoi* Shrine = Pois ? Pois->NearestShrine(Hero->GetActorLocation(), 350.f) : nullptr)
        Prompt = FString::Printf(TEXT("[E]  Orar no santuário  •  %s por 5 minutos"), *AVortexPointsOfInterest::BlessingName(Shrine->Variant));
    else if (Pois && Pois->NearestLore(Hero->GetActorLocation(), 260.f)) Prompt = TEXT("[E]  Ler a inscrição");
    else if (const AVortexMonster* Horse = Hero->FindHorseNear(340.f))
    {
        if (Horse->GetTamer() == Hero) Prompt = TEXT("[E]  Montar");
        else
        {
            const UVortexHeroSheet* Sheet = Hero->GetSheet();
            const int32 Fruits = Sheet ? Sheet->CountOf(TEXT("frutas_silvestres")) : 0;
            if (Fruits > 0)
                Prompt = Horse->GetTameFed() > 0 ? FString::Printf(TEXT("[E]  Dar frutas silvestres  (%d de %d)  •  você tem %d"), Horse->GetTameFed(), Horse->GetTameNeed(), Fruits)
                    : FString::Printf(TEXT("[E]  Dar frutas silvestres para domar  •  você tem %d"), Fruits);
            else { Prompt = TEXT("Cavalo selvagem: dome com frutas silvestres (colha nas moitas)"); Color = HudCream; }
        }
    }
    else if (bool bDoorOpen = false; AVortexTown::FindDoor(GetWorld(), Hero->GetActorLocation(), 230.f, bDoorOpen))
        Prompt = bDoorOpen ? TEXT("[E]  Fechar a porta") : TEXT("[E]  Abrir a porta");
    else if (const FVortexForageSpot* Spot = Forage ? Forage->Nearest(Hero->GetActorLocation(), UVortexForageSubsystem::Reach) : nullptr)
        Prompt = FString::Printf(TEXT("[E]  Colher %s"), *Spot->Name);
    if (Prompt.IsEmpty()) return;
    Label(Prompt, 0.f, Canvas->ClipY * .66f, Canvas->ClipX, 36.f * Scale, 26.f * Scale, Color, 1);
}

void AVortexAimHUD::DrawPings(AVortexCharacter* Hero)
{
    const float Now = GetWorld()->GetTimeSeconds();
    for (const FVortexPing& Ping : VortexPings::All())
    {
        const float Age = Now - Ping.Time;
        if (Age < 0.f || Age > VortexPings::Life) continue;
        FVector At = Ping.At;
        if (AActor* Target = Ping.Target.Get()) At = Target->GetActorLocation() + FVector(0.f, 0.f, Target->GetSimpleCollisionHalfHeight() + 60.f);
        const float Fade = FMath::Clamp((VortexPings::Life - Age) / 1.f, 0.f, 1.f);
        // 03/11: na cor de quem marcou; o perigo continua vermelho, com o nome de quem avisou
        const FLinearColor Color = Ping.bDanger ? FLinearColor(1.f, .25f, .2f, Fade) : FLinearColor(Ping.Color.R, Ping.Color.G, Ping.Color.B, Fade);
        const float Meters = FVector::Dist(At, Hero->GetActorLocation()) / 100.f;
        FVector2D Screen;
        const bool bOn = PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(At, Screen) && Screen.X > 0.f && Screen.Y > 0.f && Screen.X < Canvas->ClipX && Screen.Y < Canvas->ClipY;
        if (!bOn)
        {
            // fora da tela: na borda, para o lado certo
            const FRotator View = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation() : FRotator::ZeroRotator;
            const float Rel = FMath::DegreesToRadians(FRotator::NormalizeAxis((At - Hero->GetActorLocation()).Rotation().Yaw - View.Yaw));
            Screen = FVector2D(Canvas->ClipX * .5f + FMath::Sin(Rel) * Canvas->ClipX * .42f, Canvas->ClipY * .5f - FMath::Cos(Rel) * Canvas->ClipY * .4f);
        }
        // losango que pulsa, o aviso e a distância
        const float Pulse = 1.f + .2f * FMath::Sin(Age * 9.f);
        const float R = 13.f * Scale * Pulse;
        const FVector2D C(Screen.X, Screen.Y);
        for (int32 Pass = 0; Pass < 2; ++Pass)
        {
            const float RR = Pass == 0 ? R + 3.f * Scale : R;
            const FLinearColor Tint = Pass == 0 ? FLinearColor(0.f, 0.f, 0.f, .7f * Fade) : Color;
            FCanvasTriangleItem Top(C + FVector2D(0, -RR), C + FVector2D(RR, 0), C + FVector2D(-RR, 0), GWhiteTexture);
            FCanvasTriangleItem Bottom(C + FVector2D(0, RR), C + FVector2D(-RR, 0), C + FVector2D(RR, 0), GWhiteTexture);
            Top.SetColor(Tint); Bottom.SetColor(Tint);
            Canvas->DrawItem(Top); Canvas->DrawItem(Bottom);
        }
        Label(Ping.bDanger ? TEXT("!") : TEXT(""), C.X - R, C.Y - R, 2.f * R, 2.f * R, 20.f * Scale, FLinearColor(1, 1, 1, Fade), 1);
        const FString Text = Ping.bDanger ? (Ping.Who.IsEmpty() ? FString::Printf(TEXT("PERIGO  •  %.0f m"), Meters) : FString::Printf(TEXT("PERIGO (%s)  •  %.0f m"), *Ping.Who, Meters))
            : FString::Printf(TEXT("%s  •  %.0f m"), Ping.Who.IsEmpty() ? TEXT("Aqui") : *Ping.Who, Meters);
        Label(Text, C.X - 200.f * Scale, C.Y + R + 2.f * Scale, 400.f * Scale, 24.f * Scale, 19.f * Scale, Color, 1);
    }
}

const AVortexWolf* AVortexAimHUD::GetTargetWolf() const { return Cast<AVortexWolf>(TargetCreature.Get()); }

// ---- o que o herói já viu ------------------------------------------------------------------------

// Uma criatura entra no mapa quando o herói a vê: está na tela, sem morro ou árvore no caminho (ou
// já está brigando com ele, ou bem perto). Some do mapa 4 s depois de sair da vista.
void AVortexAimHUD::UpdateSpotted(AVortexCharacter* Hero, float DeltaSeconds)
{
    SpotClock -= DeltaSeconds;
    if (SpotClock > 0.f || !PlayerOwner) return;
    SpotClock = .2f;
    const float Now = GetWorld()->GetTimeSeconds();
    FVector Eye;
    FRotator View;
    PlayerOwner->GetPlayerViewPoint(Eye, View);
    const FVector Here = Hero->GetActorLocation();
    for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
        if (!Creature || Creature->IsCreatureDead() || It->IsHidden()) continue;
        const float Distance = FVector::Dist(It->GetActorLocation(), Here);
        if (Distance > 9000.f) continue;
        bool bSeen = Distance < 1200.f || (Creature->IsEngaged() && Distance < 4000.f);
        if (!bSeen)
        {
            const FVector Screen = Project(It->GetActorLocation(), false);
            const bool bOnScreen = Screen.Z > 0.f && Screen.X > 0.f && Screen.Y > 0.f && Screen.X < Canvas->ClipX && Screen.Y < Canvas->ClipY;
            if (bOnScreen)
            {
                FHitResult Block;
                FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexSpot), false, *It);
                Query.AddIgnoredActor(Hero);
                const FVector Body = It->GetActorLocation() + FVector(0.f, 0.f, 40.f);
                bSeen = !GetWorld()->LineTraceSingleByChannel(Block, Eye, Body, ECC_Visibility, Query);
            }
        }
        if (bSeen) SeenAt.Add(*It, Now);
    }
    for (auto Entry = SeenAt.CreateIterator(); Entry; ++Entry) if (!Entry.Key().IsValid()) Entry.RemoveCurrent();
}

bool AVortexAimHUD::WasSeen(const AActor* Actor) const
{
    const float* When = SeenAt.Find(const_cast<AActor*>(Actor));
    return When && GetWorld()->GetTimeSeconds() - *When < 4.f;
}

void AVortexAimHUD::MapMark(const FVector2D& At, float Radius, const FLinearColor& Color, int32 Shape)
{
    if (Shape == 1)
    {
        // losango (objetivo de missão)
        const FVector2D Up(0.f, -Radius), Right(Radius, 0.f);
        for (int32 Pass = 0; Pass < 2; ++Pass)
        {
            const float K = Pass == 0 ? 1.35f : 1.f;
            const FLinearColor C = Pass == 0 ? FLinearColor(0, 0, 0, .85f) : Color;
            FCanvasTriangleItem Top(At + Up * K, At + Right * K, At - Right * K, GWhiteTexture);
            FCanvasTriangleItem Bottom(At - Up * K, At - Right * K, At + Right * K, GWhiteTexture);
            Top.SetColor(C); Bottom.SetColor(C);
            Canvas->DrawItem(Top); Canvas->DrawItem(Bottom);
        }
        return;
    }
    if (Shape == 2)
    {
        // quadrado (cidade, construções)
        FCanvasTileItem Edge(At - FVector2D(Radius + 2.f), FVector2D(2.f * Radius + 4.f), FLinearColor(0, 0, 0, .85f));
        Canvas->DrawItem(Edge);
        FCanvasTileItem Fill(At - FVector2D(Radius), FVector2D(2.f * Radius), Color);
        Canvas->DrawItem(Fill);
        return;
    }
    FCanvasNGonItem Ring(At, FVector2D(Radius + 1.5f, Radius + 1.5f), 12, FLinearColor(0, 0, 0, .8f));
    Canvas->DrawItem(Ring);
    FCanvasNGonItem Fill(At, FVector2D(Radius, Radius), 12, Color);
    Canvas->DrawItem(Fill);
}

// ---- moradores: nome, título, marca de missão, falas e o convite para conversar --------------------

void AVortexAimHUD::DrawNpcTags(AVortexCharacter* Hero)
{
    if (bWorldMap) return;
    const float Now = GetWorld()->GetTimeSeconds();
    const UVortexQuestLog* Log = Hero->GetQuestLog();
    const FVortexStoryCatalog& Story = FVortexStoryCatalog::Get();
    const AVortexNpc* Talkable = VortexDialogue::IsOpen() ? nullptr : VortexDialogue::FindTalkable(Hero);
    for (TActorIterator<AVortexNpc> It(GetWorld()); It; ++It)
    {
        if (It->IsHidden()) continue;
        const float Distance = FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation());
        if (Distance > 2500.f) continue;
        const FVector Head = It->GetActorLocation() + FVector(0.f, 0.f, 125.f);
        const FVector Screen = Project(Head, false);
        if (Screen.Z <= 0.f) continue;
        const float Fade = FMath::Clamp((2500.f - Distance) / 600.f, 0.f, 1.f);
        const float Size = FMath::Clamp(1.f - Distance / 4000.f, .55f, 1.f) * Scale;
        const FVortexNpcInfo* Info = Story.Npc(It->GetNpcId());
        FLinearColor NameColor = HudCream; NameColor.A = Fade;
        FLinearColor TitleColor = HudMuted; TitleColor.A = Fade * .9f;
        Label(Info ? Info->Name : It->GetDisplayName(), Screen.X - 200.f, Screen.Y - 30.f * Size, 400.f, 30.f * Size, 26.f * Size, NameColor, 1);
        if (!Info && VortexNpcRoutine::HasRoutine(It->GetNpcId()))
            Label(It->IsChild() ? TEXT("Criança da vila") : TEXT("Morador"), Screen.X - 200.f, Screen.Y - 4.f * Size, 400.f, 22.f * Size, 18.f * Size, TitleColor, 1, false);
        if (Info && !Info->Title.IsEmpty()) Label(Info->Title, Screen.X - 200.f, Screen.Y - 4.f * Size, 400.f, 22.f * Size, 18.f * Size, TitleColor, 1, false);
        // "!" tem missão, "?" pode entregar (como o Godot)
        const FString Mark = Log ? Log->MarkFor(It->GetNpcId()) : FString();
        if (!Mark.IsEmpty())
        {
            const float Bob = FMath::Sin(Now * 3.f) * 4.f * Scale;
            FLinearColor MarkColor = Mark == TEXT("?") ? FLinearColor(.55f, 1.f, .5f) : HudGold;
            MarkColor.A = Fade;
            Label(Mark, Screen.X - 50.f, Screen.Y - 92.f * Size + Bob, 100.f, 60.f * Size, 64.f * Size, MarkColor, 1);
        }
        // falas curtas quando o herói passa perto: uma a cada 9 s, por 4,5 s (08/11: mais as da hora do dia, da rotina;
        // os passantes só têm essas)
        TArray<FString> Barks = Info ? Info->Barks : TArray<FString>();
        Barks.Append(VortexNpcRoutine::Barks(It->GetNpcId(), VortexNpcRoutine::HourOf(GetWorld())));
        // 08/11: o balão de fala (o que ele diz a outro morador, ou a fala curta quando o herói passa perto), no estilo das janelas
        FString Said = It->GetBubble();
        if (Said.IsEmpty() && Barks.Num() && Distance < 700.f && !VortexDialogue::IsOpen() && !It->IsAsleep())
        {
            const float Phase = Now + (GetTypeHash(It->GetNpcId()) % 97) * .1f;
            if (FMath::Fmod(Phase, 9.f) < 4.5f) Said = Barks[FMath::FloorToInt(Phase / 9.f) % Barks.Num()];
        }
        if (!Said.IsEmpty() && Distance < 1800.f && !VortexDialogue::IsOpen())
        {
            // duas linhas quando é comprida, cortando no espaço mais perto do meio
            FString First = Said, Second;
            if (Said.Len() > 30)
                for (int32 Off = 0; Off < Said.Len() / 2; ++Off)
                {
                    const int32 Cut = Said[Said.Len() / 2 + Off] == TEXT(' ') ? Said.Len() / 2 + Off : Said[Said.Len() / 2 - Off] == TEXT(' ') ? Said.Len() / 2 - Off : INDEX_NONE;
                    if (Cut != INDEX_NONE) { First = Said.Left(Cut); Second = Said.Mid(Cut + 1); break; }
                }
            const float Fs = FMath::Max(15.f, 21.f * Size);
            const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", FMath::RoundToInt(Fs * .75f));
            const float TextW = FMath::Max(Measure(First, Font).X, Second.IsEmpty() ? 0.f : Measure(Second, Font).X);
            const float Lines = Second.IsEmpty() ? 1.f : 2.f;
            const float BoxW = TextW + 34.f * Size, BoxH = Lines * Fs * 1.25f + 14.f * Size;
            const float BoxX = Screen.X - BoxW * .5f, BoxY = Screen.Y - (Mark.IsEmpty() ? 50.f : 108.f) * Size - BoxH;
            const float Line = FMath::Max(2.f, 2.f * Size), Step = FMath::Max(4.f, 6.f * Size);
            const float A = FMath::Min(Fade, .96f);
            auto Tile = [&](float X, float Y, float W, float H, const FLinearColor& C)
            {
                FCanvasTileItem Item(FVector2D(X, Y), FVector2D(W, H), C);
                Item.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(Item);
            };
            const FLinearColor Dark(.012f, .022f, .05f, A), Gold(HudGold.R, HudGold.G, HudGold.B, A);
            // a moldura dourada de canto cortado (pixel) e o rabinho em degraus, como o resto da interface
            Tile(BoxX + Line, BoxY, BoxW - 2.f * Line, BoxH, Gold);
            Tile(BoxX, BoxY + Line, BoxW, BoxH - 2.f * Line, Gold);
            Tile(BoxX + Line, BoxY + Line, BoxW - 2.f * Line, BoxH - 2.f * Line, Dark);
            for (int32 K = 0; K < 3; ++K)
            {
                const float W = Step * (3 - K);
                Tile(Screen.X - W * .5f - Line, BoxY + BoxH + K * Step * .7f - Line, W + 2.f * Line, Step * .7f + Line, Gold);
                Tile(Screen.X - W * .5f, BoxY + BoxH + K * Step * .7f - Line, W, Step * .7f, Dark);
            }
            FLinearColor TextColor = HudCream; TextColor.A = Fade;
            const float Row = Fs * 1.25f, Top = BoxY + 7.f * Size;
            Label(First, BoxX, Top, BoxW, Row, Fs, TextColor, 1, false);
            if (!Second.IsEmpty()) Label(Second, BoxX, Top + Row, BoxW, Row, Fs, TextColor, 1, false);
        }
    }
    if (Talkable)
    {
        const FVortexNpcInfo* Info = Story.Npc(Talkable->GetNpcId());
        Label(FString::Printf(TEXT("[E]  Falar com %s"), Info ? *Info->Name : *Talkable->GetDisplayName()), 0.f, Canvas->ClipY * .66f,
            Canvas->ClipX, 36.f * Scale, 28.f * Scale, HudGold, 1);
    }
}

// ---- morte e carregamento ---------------------------------------------------------------------------

void AVortexAimHUD::DrawDeath(AVortexCharacter* Hero)
{
    const float T = Hero->GetDeadTime();
    if (const AActor* Foe = TargetCreature.Get(); Foe && T <= 0.f)
        if (const IVortexCreature* Creature = Cast<IVortexCreature>(Foe)) LastKiller = FText::FromString(Creature->GetCreatureTitle()).ToString();
    if (T <= 0.f) return;
    const float A = FMath::Clamp(T / .8f, 0.f, 1.f);
    const float W = Canvas->ClipX, H = Canvas->ClipY;
    auto Tile = [&](float X, float Y, float Tw, float Th, const FLinearColor& Color)
    {
        FCanvasTileItem Item(FVector2D(X, Y), FVector2D(Tw, Th), Color);
        Item.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Item);
    };
    // véu vermelho-escuro e as faixas de cinema que entram por cima e por baixo
    Tile(0.f, 0.f, W, H, FLinearColor(.09f, 0.f, 0.f, .45f * A));
    const float Band = H * .14f * FMath::SmoothStep(0.f, 1.f, FMath::Clamp(T / 1.f, 0.f, 1.f));
    Tile(0.f, 0.f, W, Band, FLinearColor(0.f, 0.f, 0.f, .95f));
    Tile(0.f, H - Band, W, Band, FLinearColor(0.f, 0.f, 0.f, .95f));
    // título que chega crescendo, com filetes dourados dos lados
    const float In = FMath::Clamp((T - .3f) / .7f, 0.f, 1.f);
    if (In <= 0.f) return;
    const float Grow = 1.25f - .25f * FMath::SmoothStep(0.f, 1.f, In);
    const float Size = 78.f * Scale * Grow, CY = H * .4f;
    Label(TEXT("VOCÊ CAIU"), 4.f, CY - Size * .6f + 4.f, W, Size * 1.2f, Size, FLinearColor(0.f, 0.f, 0.f, .8f * In), 1);
    Label(TEXT("VOCÊ CAIU"), 0.f, CY - Size * .6f, W, Size * 1.2f, Size, FLinearColor(.95f, .72f, .3f, In), 1);
    const float Line = 260.f * Scale * In;
    for (const float Side : { -1.f, 1.f })
    {
        const float X0 = W * .5f + Side * 230.f * Scale;
        DrawLine(X0, CY + Size * .62f, X0 + Side * Line, CY + Size * .62f, FLinearColor(.85f, .62f, .25f, In), 2.f);
        FCanvasTriangleItem Gem(FVector2D(X0 + Side * Line, CY + Size * .62f - 6.f * Scale), FVector2D(X0 + Side * (Line + 8.f * Scale), CY + Size * .62f),
            FVector2D(X0 + Side * Line, CY + Size * .62f + 6.f * Scale), GWhiteTexture);
        Gem.SetColor(FLinearColor(.95f, .72f, .3f, In));
        Canvas->DrawItem(Gem);
    }
    const float Sub = FMath::Clamp((T - .9f) / .6f, 0.f, 1.f);
    if (!LastKiller.IsEmpty())
        Label(FString::Printf(TEXT("Derrotado por %s"), *LastKiller), 0.f, CY + Size * .75f, W, 34.f * Scale, 26.f * Scale, FLinearColor(.92f, .86f, .76f, Sub), 1, false);
    // contagem para renascer na Fonte do Santuário, e o Espaço depois de 2 s
    const int32 Left = FMath::Max(0, FMath::CeilToInt(5.f - T));
    if (Hero->GetSheet() && !Hero->GetSheet()->NeedsAwakening())
    {
        Label(Left > 0 ? FString::Printf(TEXT("Renascendo na Fonte do Santuário em %d"), Left) : FString(TEXT("Renascendo...")), 0.f, H - Band - 90.f * Scale, W, 30.f * Scale, 24.f * Scale, FLinearColor(.8f, .75f, .68f, Sub), 1, false);
        if (T > 2.f) Label(TEXT("[Espaço]  renascer agora"), 0.f, H - Band - 55.f * Scale, W, 30.f * Scale, 22.f * Scale, FLinearColor(.95f, .72f, .3f, .55f + .45f * FMath::Sin(T * 4.f)), 1);
    }
}

void AVortexAimHUD::StartLoading(bool bSnap)
{
    if (!bLoading) { LoadingAge = 0.f; LoadingPeak = 1; LoadingTip = FMath::RandRange(0, 9); LoadingArt = nullptr; }
    bLoading = true;
    if (bSnap) LoadingFade = 1.f;
}

bool AVortexAimHUD::DrawLoading(AVortexCharacter* Hero, float DeltaSeconds)
{
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { Streamer = *It; }
    if (bLoading)
    {
        LoadingAge += DeltaSeconds;
        LoadingPeak = FMath::Max(LoadingPeak, Streamer ? Streamer->PendingChunks() : 0);
        float Ground = 0.f;
        const FVector At = Hero->GetActorLocation();
        const UVortexCreatureSubsystem* Creatures = GetWorld()->GetSubsystem<UVortexCreatureSubsystem>();
        const bool bReady = Streamer && Streamer->GroundReadyAt(At.X, At.Y, Ground) && Streamer->PendingChunks() == 0
            && Hero->GetCharacterMovement()->MovementMode != MOVE_None && (!Creatures || Creatures->IsPreloadDone())
            // 11/11: e a fila de carregamento em segundo plano vazia: com ela cheia (1.200+ pacotes), qualquer carregamento
            // "na hora" no jogo (a fada, a roupa de um morador, um ícone) esperava a fila inteira e travava 100 a 500 ms
            && GetNumAsyncPackages() == 0
            // os shaders sendo preparados (só no jogo empacotado; até 15 s)
            && (LoadingAge > 15.f || (PipelineStateCache::GetNumActivePipelinePrecompileTasks() == 0 && FShaderPipelineCache::NumPrecompilesRemaining() == 0));
        if ((bReady && LoadingAge > (bApplyingOptions ? .8f : 1.6f)) || LoadingAge > 25.f) bLoading = bApplyingOptions = false;
    }
    LoadingFade = FMath::FInterpConstantTo(LoadingFade, bLoading ? 1.f : 0.f, DeltaSeconds, bLoading ? 6.f : 1.6f);
    if (LoadingFade <= 0.f) return false;
    const float A = LoadingFade, W = Canvas->ClipX, H = Canvas->ClipY;
    auto Tile = [&](float X, float Y, float Tw, float Th, const FLinearColor& Color)
    {
        FCanvasTileItem Item(FVector2D(X, Y), FVector2D(Tw, Th), Color);
        Item.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Item);
    };
    // 08/11: o fundo é uma das cenas pintadas da abertura (a da região para onde o herói vai), com um zoom lento
    const TSharedPtr<FVortexTerrainData> RegionData = AVortexTerrainStreamer::SharedData();
    if (!LoadingArt)
    {
        static const TCHAR* ByRegion[][2] = { { TEXT("C19b"), TEXT("C20b") }, { TEXT("C02"), TEXT("C04") }, { TEXT("C12"), TEXT("C11") }, { TEXT("C10b"), TEXT("C14") },
            { TEXT("C10a"), TEXT("C09") }, { TEXT("C08a"), TEXT("C01") }, { TEXT("C18b"), TEXT("C04") }, { TEXT("C15"), TEXT("C22") }, { TEXT("C22"), TEXT("C07") }, { TEXT("C03"), TEXT("C02") } };
        const int32 Region = RegionData && RegionData->HasWorld() ? FMath::Clamp(RegionData->RegionAt(Hero->GetActorLocation().X, Hero->GetActorLocation().Y), 0, 9) : 1;
        const TCHAR* Name = ByRegion[Region][LoadingTip % 2];
        LoadingArt = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Abertura/Cinema/T_%s.T_%s"), Name, Name));
    }
    Tile(0.f, 0.f, W, H, FLinearColor(.012f, .014f, .028f, A));
    if (LoadingArt && LoadingArt->GetResource())
    {
        const float TexAspect = float(LoadingArt->GetSizeX()) / FMath::Max(1, LoadingArt->GetSizeY()), ScreenAspect = W / FMath::Max(1.f, H);
        const float Zoom = 1.06f + .08f * FMath::Min(LoadingAge / 20.f, 1.f);
        const float USpan = (ScreenAspect > TexAspect ? 1.f : ScreenAspect / TexAspect) / Zoom, VSpan = (ScreenAspect > TexAspect ? TexAspect / ScreenAspect : 1.f) / Zoom;
        const float Pan = FMath::Min(LoadingAge / 20.f, 1.f) * (1.f - USpan) * .35f;
        const FVector2D UV0(.5f - USpan * .5f - Pan, .5f - VSpan * .5f), UV1 = UV0 + FVector2D(USpan, VSpan);
        FCanvasTileItem Art(FVector2D::ZeroVector, LoadingArt->GetResource(), FVector2D(W, H), UV0, UV1, FLinearColor(1.f, 1.f, 1.f, A));
        Art.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Art);
    }
    // escurece embaixo (onde vão o nome e a barra) e de leve em cima e nos lados
    // (faixas em pixels inteiros, sem sobrepor: a sobra de 1 px escurecia o dobro e desenhava listras)
    auto Band = [&](float From, float To, bool bRows, float Alpha)
    {
        const float P0 = FMath::RoundToFloat(From), P1 = FMath::RoundToFloat(To);
        if (P1 > P0) bRows ? Tile(0.f, P0, W, P1 - P0, FLinearColor(0.f, 0.f, 0.f, Alpha)) : Tile(P0, 0.f, P1 - P0, H, FLinearColor(0.f, 0.f, 0.f, Alpha));
    };
    for (int32 i = 0; i < 16; ++i) Band(H * (.5f + i / 32.f), H * (.5f + (i + 1) / 32.f), true, .055f * (i + 1) * A);
    for (int32 i = 0; i < 8; ++i) Band(H * i / 32.f, H * (i + 1) / 32.f, true, .06f * (8 - i) * A);
    for (int32 i = 0; i < 6; ++i) { Band(W * i / 48.f, W * (i + 1) / 48.f, false, .07f * (6 - i) * A); Band(W - W * (i + 1) / 48.f, W - W * i / 48.f, false, .07f * (6 - i) * A); }
    // moldura dourada fina
    const FLinearColor Gold(.95f, .72f, .3f, A), GoldDim(.85f, .62f, .25f, .55f * A);
    const float Pad = 22.f * Scale;
    Tile(Pad, Pad, W - Pad * 2.f, 2.f, GoldDim); Tile(Pad, H - Pad - 2.f, W - Pad * 2.f, 2.f, GoldDim);
    Tile(Pad, Pad, 2.f, H - Pad * 2.f, GoldDim); Tile(W - Pad - 2.f, Pad, 2.f, H - Pad * 2.f, GoldDim);
    // o anel de runas pequeno, no canto (gira enquanto carrega)
    const float CX = W - 110.f * Scale, CY = H - 120.f * Scale;
    // anel de runas girando (o portal do Despertar)
    const float Ring = 46.f * Scale, Spin = LoadingAge * .9f;
    for (int32 k = 0; k < 24; ++k)
    {
        const float Ang = Spin + k * UE_TWO_PI / 24.f;
        const float Glow = .35f + .65f * FMath::Max(0.f, FMath::Sin(LoadingAge * 3.f - k * .5f));
        const FVector2D P(CX + FMath::Cos(Ang) * Ring, CY + FMath::Sin(Ang) * Ring);
        const float R = (k % 3 == 0 ? 4.f : 2.5f) * Scale;
        FCanvasTriangleItem Top(P + FVector2D(0, -R), P + FVector2D(R, 0), P + FVector2D(-R, 0), GWhiteTexture);
        FCanvasTriangleItem Bottom(P + FVector2D(0, R), P + FVector2D(-R, 0), P + FVector2D(R, 0), GWhiteTexture);
        const FLinearColor Color(.95f * Glow, .7f * Glow, .28f * Glow, A);
        Top.SetColor(Color); Bottom.SetColor(Color);
        Canvas->DrawItem(Top); Canvas->DrawItem(Bottom);
    }
    Disc(CX, CY, Ring * .55f, FLinearColor(.03f, .03f, .06f, .9f * A), FLinearColor(.85f, .62f, .25f, A));
    Label(TEXT("G"), CX - 30.f * Scale, CY - 20.f * Scale, 60.f * Scale, 40.f * Scale, 30.f * Scale, Gold, 1);
    // o nome do lugar, grande, com a linha de enfeite e o losango, e o subtítulo
    const FString Place = RegionData && RegionData->HasWorld() ? RegionData->RegionName(RegionData->RegionAt(Hero->GetActorLocation().X, Hero->GetActorLocation().Y)) : FString(TEXT("Vale do Santuário"));
    const float MidX = W * .5f, TitleY = H * .70f;
    Label(FText::FromString(Place).ToUpper().ToString(), 0.f, TitleY + 3.f * Scale, W, 70.f * Scale, 60.f * Scale, FLinearColor(0.f, 0.f, 0.f, .6f * A), 1);
    Label(FText::FromString(Place).ToUpper().ToString(), 0.f, TitleY, W, 70.f * Scale, 60.f * Scale, Gold, 1);
    const float LineW = 260.f * Scale, LineY = TitleY + 80.f * Scale;
    Tile(MidX - LineW - 14.f * Scale, LineY, LineW, 2.f, GoldDim);
    Tile(MidX + 14.f * Scale, LineY, LineW, 2.f, GoldDim);
    {
        const float R = 7.f * Scale;
        const FVector2D P(MidX, LineY + 1.f);
        FCanvasTriangleItem Top(P + FVector2D(0, -R), P + FVector2D(R, 0), P + FVector2D(-R, 0), GWhiteTexture);
        FCanvasTriangleItem Bottom(P + FVector2D(0, R), P + FVector2D(-R, 0), P + FVector2D(R, 0), GWhiteTexture);
        Top.SetColor(Gold); Bottom.SetColor(Gold);
        Canvas->DrawItem(Top); Canvas->DrawItem(Bottom);
    }
    Label(TEXT("Preparando o mundo..."), 0.f, LineY + 14.f * Scale, W, 30.f * Scale, 22.f * Scale, FLinearColor(.85f, .82f, .76f, A), 1, false);
    // a barra fina dourada, com um brilho que corre por ela
    const float Progress = Streamer ? FMath::Clamp(1.f - float(Streamer->PendingChunks()) / FMath::Max(1, LoadingPeak), 0.f, 1.f) : 0.f;
    const float Shown = FMath::Max(Progress, FMath::Min(LoadingAge / 12.f, .15f));
    const float BarW = 520.f * Scale, BarH = 6.f * Scale, BarY = LineY + 56.f * Scale;
    Tile(MidX - BarW * .5f - 1.f, BarY - 1.f, BarW + 2.f, BarH + 2.f, GoldDim);
    Tile(MidX - BarW * .5f, BarY, BarW, BarH, FLinearColor(.02f, .02f, .03f, .85f * A));
    Tile(MidX - BarW * .5f, BarY, BarW * Shown, BarH, FLinearColor(.95f, .62f, .18f, A));
    const float Shine = FMath::Fmod(LoadingAge * .6f, 1.f) * BarW * Shown;
    Tile(MidX - BarW * .5f + FMath::Max(0.f, Shine - 30.f * Scale), BarY, FMath::Min(30.f * Scale, Shine), BarH, FLinearColor(1.f, .95f, .7f, .55f * A));
    // dica
    static const TCHAR* Tips[] = {
        TEXT("Moitas com brilho dão frutas silvestres: elas curam e domam cavalos."),
        TEXT("Aperte Z para marcar um ponto para o grupo. Numa criatura, o aviso é de perigo."),
        TEXT("O nome da criatura diz o perigo: verde é do seu nível, vermelho é muito forte, preto mata com um golpe."),
        TEXT("Quanto mais longe da vila inicial, mais fortes as criaturas."),
        TEXT("Na Fonte do Santuário a vida e a mana voltam depressa."),
        TEXT("Montado no cavalo, Shift galopa e X chama ele de volta."),
        TEXT("Cabanas abandonadas guardam baús. Aperte E perto do baú."),
        TEXT("No nível 10 as seis cartas do Despertar revelam a sua classe."),
        TEXT("Segure F para se defender; no momento certo, a defesa vira contra-ataque."),
        TEXT("O mapa (M) se revela por onde você passa. A roda do mouse dá zoom."),
    };
    Label(TEXT("DICA"), 0.f, H - 104.f * Scale, W, 24.f * Scale, 18.f * Scale, Gold, 1);
    Label(Tips[LoadingTip % UE_ARRAY_COUNT(Tips)], 0.f, H - 80.f * Scale, W, 30.f * Scale, 22.f * Scale, FLinearColor(.88f, .85f, .8f, A), 1, false);
    return LoadingFade >= .99f;
}

// ---- placas das criaturas ------------------------------------------------------------------------

void AVortexAimHUD::DrawCreaturePlates(AVortexCharacter* Hero, float DeltaSeconds)
{
    VORTEX_PROBE("HUD.Placas");
    if (bWorldMap || !PlayerOwner) return;
    const float Now = GetWorld()->GetTimeSeconds();
    const int32 HeroLevel = Hero->GetSheet() ? Hero->GetSheet()->GetLevel() : 1;
    const FVector Eye = PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraLocation() : Hero->GetActorLocation();
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        const IVortexCreature* Creature = Cast<IVortexCreature>(Actor);
        if (!Creature || Creature->IsBoss()) continue;
        FPlateState& State = Plates.FindOrAdd(Actor);
        const float Distance = FVector::Dist(Actor->GetActorLocation(), Hero->GetActorLocation());
        const float Fraction = FMath::Clamp(Creature->GetHealthFraction(), 0.f, 1.f);
        // aparece de perto, em luta, ferida ou como alvo; some devagar
        const bool bWanted = !Creature->IsCreatureDead() && Distance < 3200.f && (Distance < 1400.f || Creature->IsEngaged() || Fraction < .999f || Actor == TargetCreature.Get());
        State.Alpha = FMath::FInterpTo(State.Alpha, bWanted ? 1.f : 0.f, DeltaSeconds, bWanted ? 8.f : 4.f);
        // a vida desce rápido; o rastro claro do dano espera um instante e escorre atrás
        if (Fraction < State.LastFraction - .001f) State.HitAt = Now;
        State.LastFraction = Fraction;
        State.Shown = FMath::FInterpTo(State.Shown, Fraction, DeltaSeconds, 14.f);
        if (Fraction > State.Chip) State.Chip = Fraction;
        else if (Now - State.HitAt > .35f) State.Chip = FMath::FInterpConstantTo(State.Chip, State.Shown, DeltaSeconds, .7f);
        // o "pulo" de quando aparece (e volta a zero quando some)
        State.Pop = State.Alpha < .05f ? 0.f : FMath::FInterpConstantTo(State.Pop, 1.f, DeltaSeconds, 4.f);
        if (State.Alpha < .02f) continue;
        const float Half = Actor->GetSimpleCollisionHalfHeight();
        FVector2D Screen;
        if (!PlayerOwner->ProjectWorldLocationToScreen(Actor->GetActorLocation() + FVector(0.f, 0.f, Half + 45.f), Screen)) continue;
        if (FVector::DotProduct((Actor->GetActorLocation() - Eye).GetSafeNormal(), PlayerOwner->PlayerCameraManager->GetCameraRotation().Vector()) < .2f) continue;
        // segue o monstro sem tremer (a projeção pula um pouco com a câmera e o passo do bicho)
        State.Pos = State.Pos.IsZero() || FVector2D::Distance(State.Pos, Screen) > 300.f ? Screen : FMath::Vector2DInterpTo(State.Pos, Screen, DeltaSeconds, 22.f);
        Screen = State.Pos;
        // aparece com um pulo (passa um pouco do tamanho e volta)
        const float PopT = State.Pop;
        const float Spring = 1.f + .12f * FMath::Sin(PopT * UE_PI) * (1.f - PopT * .3f);
        const float K = Scale * FMath::Clamp(1.25f - FVector::Dist(Eye, Actor->GetActorLocation()) / 3500.f, .62f, 1.1f) * FMath::Lerp(.8f, 1.f, PopT) * Spring;
        const float A = State.Alpha;
        // o golpe: a placa treme e o preenchimento pisca em branco por um instante
        const float HitT = Now - State.HitAt;
        const float Shake = HitT < .25f ? FMath::Sin(HitT * 70.f) * 3.5f * K * (1.f - HitT / .25f) : 0.f;
        Screen.X += Shake;
        const int32 Tier = VortexRpg::Difficulty(HeroLevel, Creature->GetCreatureLevel());
        const FLinearColor Tint = VortexRpg::DifficultyColor(Tier);
        const bool bPassive = Creature->IsPassive();
        const float W = 132.f * K, H = 11.f * K;
        const float X = Screen.X - W * .5f + 10.f * K, Y = Screen.Y;
        auto Tile = [&](float Tx, float Ty, float Tw, float Th, const FLinearColor& Color)
        {
            FCanvasTileItem Item(FVector2D(Tx, Ty), FVector2D(Tw, Th), Color);
            Item.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Item);
        };
        // a placa inteira (cápsulas, riscos e o selo) vira uma lista de triângulos desenhada de uma vez: cada
        // cápsula e cada ponta redonda como item separado davam ~25 desenhos por placa e custavam ~20 FPS
        TArray<FCanvasUVTri> Tris;
        auto Tri = [&Tris](const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FLinearColor& Color)
        {
            FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
            T.V0_Pos = P0; T.V1_Pos = P1; T.V2_Pos = P2;
            T.V0_Color = T.V1_Color = T.V2_Color = Color;
        };
        auto Quad = [&Tri](float Qx, float Qy, float Qw, float Qh, const FLinearColor& Color)
        {
            Tri(FVector2D(Qx, Qy), FVector2D(Qx + Qw, Qy), FVector2D(Qx + Qw, Qy + Qh), Color);
            Tri(FVector2D(Qx, Qy), FVector2D(Qx + Qw, Qy + Qh), FVector2D(Qx, Qy + Qh), Color);
        };
        // cápsula: o meio é um retângulo e as pontas são meios-círculos (8 fatias cada)
        auto Pill = [&](float Px, float Py, float Pw, float Ph, const FLinearColor& Color)
        {
            if (Pw <= .5f || Color.A <= .01f) return;
            const float R = FMath::Min(Ph, Pw) * .5f;
            const FVector2D L(Px + R, Py + Ph * .5f), Rt(Px + Pw - R, Py + Ph * .5f);
            if (Pw > 2.f * R) Quad(Px + R, Py, Pw - 2.f * R, Ph, Color);
            constexpr int32 Slices = 8;
            for (int32 i = 0; i < Slices; ++i)
            {
                const float A0 = UE_HALF_PI + UE_PI * i / Slices, A1 = UE_HALF_PI + UE_PI * (i + 1) / Slices;
                const FVector2D D0(FMath::Cos(A0) * R, -FMath::Sin(A0) * Ph * .5f), D1(FMath::Cos(A1) * R, -FMath::Sin(A1) * Ph * .5f);
                Tri(L, L + D0, L + D1, Color);
                Tri(Rt, Rt - D0, Rt - D1, Color);
            }
        };
        // moldura: sombra, aro dourado, fundo escuro
        Pill(X - 3.5f * K, Y - 3.5f * K + 1.5f * K, W + 7.f * K, H + 7.f * K, FLinearColor(0.f, 0.f, 0.f, .5f * A));
        Pill(X - 2.f * K, Y - 2.f * K, W + 4.f * K, H + 4.f * K, FLinearColor(HudGold.R, HudGold.G, HudGold.B, .9f * A));
        Pill(X, Y, W, H, FLinearColor(.06f, .035f, .03f, .95f * A));
        // rastro do dano (claro) e a vida: vermelho nas hostis, âmbar nos animais; mais clara em cima;
        // com pouca vida pulsa
        Pill(X, Y, W * State.Chip, H, FLinearColor(1.f, .86f, .55f, .85f * A));
        const float Low = State.Shown < .3f ? .5f + .5f * FMath::Sin(Now * 9.f) : 0.f;
        const FLinearColor Deep = bPassive ? FLinearColor(.5f, .22f, .03f, A) : FLinearColor(.42f + .25f * Low, .02f, .015f, A);
        const FLinearColor Bright = bPassive ? FLinearColor(.95f, .55f, .12f, A) : FLinearColor(.95f + .05f * Low, .16f + .1f * Low, .1f, A);
        Pill(X, Y, W * State.Shown, H, Deep);
        Pill(X + H * .15f, Y + H * .1f, FMath::Max(0.f, W * State.Shown - H * .3f), H * .55f, Bright);
        Pill(X + H * .3f, Y + H * .16f, FMath::Max(0.f, W * State.Shown - H * .6f), H * .2f, FLinearColor(1.f, 1.f, 1.f, .28f * A));
        // brilho do golpe
        if (HitT < .18f) Pill(X, Y, W * FMath::Max(State.Shown, State.Chip), H, FLinearColor(1.f, 1.f, 1.f, .55f * A * (1.f - HitT / .18f)));
        for (int32 Notch = 1; Notch < 4; ++Notch) Quad(X + W * Notch / 4.f, Y + H * .2f, 1.f, H * .6f, FLinearColor(0.f, 0.f, 0.f, .35f * A));
        // selo do nível (losango na cor da dificuldade) à esquerda
        const FVector2D C(X - 4.f * K, Y + H * .5f);
        for (int32 Pass = 0; Pass < 3; ++Pass)
        {
            const float R = (Pass == 0 ? 17.f : Pass == 1 ? 15.f : 12.5f) * K;
            const FLinearColor Color = Pass == 0 ? FLinearColor(0.f, 0.f, 0.f, .7f * A) : Pass == 1 ? FLinearColor(HudGold.R, HudGold.G, HudGold.B, A) : FLinearColor(Tint.R, Tint.G, Tint.B, A);
            Tri(C + FVector2D(0, -R), C + FVector2D(R, 0), C + FVector2D(-R, 0), Color);
            Tri(C + FVector2D(0, R), C + FVector2D(-R, 0), C + FVector2D(R, 0), Color);
        }
        {
            FCanvasTriangleItem Plate(Tris, GWhiteTexture);
            Plate.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Plate);
        }
        const bool bDarkBadge = Tier == 2 || Tier == 3 || Tier == 1 || Tier == 0;
        Label(FString::FromInt(Creature->GetCreatureLevel()), C.X - 20.f * K, C.Y - 11.f * K, 40.f * K, 22.f * K, 17.f * K,
            bDarkBadge ? FLinearColor(.05f, .04f, .03f, A) : FLinearColor(1.f, .97f, .9f, A), 1);
        // nome na cor da dificuldade (o preto ganha caveira e contorno claro para aparecer)
        FString Name = FText::FromString(Creature->GetCreatureTitle()).ToString();
        const float NameSize = 17.f * K;
        // mata com um golpe: caveira desenhada antes do nome
        if (Tier == 6)
        {
            const FVector2D Measured = Measure(Name, FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(NameSize * .75f)));
            const FVector2D Skull(Screen.X + 10.f * K - Measured.X * .5f - 13.f * K, Y - 14.f * K);
            Disc(Skull.X, Skull.Y, 7.f * K, FLinearColor(.92f, .9f, .85f, A), FLinearColor(.6f, .05f, .05f, A));
            Tile(Skull.X - 4.f * K, Skull.Y + 4.f * K, 8.f * K, 4.f * K, FLinearColor(.92f, .9f, .85f, A));
            Tile(Skull.X - 4.f * K, Skull.Y - 2.f * K, 3.f * K, 3.f * K, FLinearColor(0.f, 0.f, 0.f, A));
            Tile(Skull.X + 1.f * K, Skull.Y - 2.f * K, 3.f * K, 3.f * K, FLinearColor(0.f, 0.f, 0.f, A));
        }
        if (Tier == 6)
            for (const FVector2D O : { FVector2D(-1, 0), FVector2D(1, 0), FVector2D(0, -1), FVector2D(0, 1) })
                Label(Name, X - 60.f * K + O.X, Y - 24.f * K + O.Y, W + 120.f * K, 20.f * K, NameSize, FLinearColor(.95f, .25f, .25f, .9f * A), 1);
        Label(Name, X - 60.f * K, Y - 24.f * K, W + 120.f * K, 20.f * K, NameSize, FLinearColor(Tint.R, Tint.G, Tint.B, A), 1);
    }
    // esquece as que sumiram
    for (auto It = Plates.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
}

// ---- missão acompanhada (embaixo do minimapa) -----------------------------------------------------

// ---- efeitos ativos -------------------------------------------------------------------------------

// ---- grupo: quadros à esquerda e o nome/seta de cada companheiro no mundo --------------------------

void AVortexAimHUD::DrawParty(AVortexCharacter* Hero)
{
    if (bWorldMap) return;
    const UVortexSocial* Social = Hero->GetSocial();
    auto Tile = [&](float X, float Y, float W, float H, const FLinearColor& Color)
    {
        FCanvasTileItem Item(FVector2D(X, Y), FVector2D(W, H), Color);
        Item.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Item);
    };
    auto NameOf = [](const AVortexCharacter* Mate)
    {
        const UVortexSocial* S = Mate->GetSocial();
        return S ? S->GetHeroName() : FString(TEXT("Aventureiro"));
    };
    // ---- o grupo: cartões pequenos embaixo do seu quadro (até 7 companheiros)
    const TArray<AVortexCharacter*> Mates = Social ? Social->PartyMates() : TArray<AVortexCharacter*>();
    float Y = 238.f * Scale;   // embaixo do cartão do herói e das pílulas de efeito
    if (Mates.Num())
    {
        Label(FString::Printf(TEXT("GRUPO  %d/%d"), Mates.Num() + 1, UVortexSocial::PartyLimit), 30.f * Scale, Y - 4.f * Scale, 200.f * Scale, 18.f * Scale, 14.f * Scale, FLinearColor(HudGold.R, HudGold.G, HudGold.B, .8f), 0);
        Y += 16.f * Scale;
    }
    for (AVortexCharacter* Mate : Mates)
    {
        const UVortexAttributes* Vitals = Mate->GetVitals();
        const float Health = Vitals ? FMath::Clamp(Vitals->GetHealth() / FMath::Max(1.f, Vitals->GetMaxHealth()), 0.f, 1.f) : 1.f;
        const float Mana = Vitals ? FMath::Clamp(Vitals->GetMana() / FMath::Max(1.f, Vitals->GetMaxMana()), 0.f, 1.f) : 1.f;
        const int32 Level = Mate->GetSheet() ? Mate->GetSheet()->GetLevel() : 1;
        const bool bDead = Mate->IsDead();
        const float X = 30.f * Scale, W = 236.f * Scale, Hh = 44.f * Scale;
        // fundo, aro e a faixa dourada à esquerda
        Tile(X - 2.f, Y - 2.f, W + 4.f, Hh + 4.f, FLinearColor(0.f, 0.f, 0.f, .5f));
        Tile(X, Y, W, Hh, FLinearColor(.015f, .02f, .035f, .82f));
        Tile(X, Y, 3.f * Scale, Hh, FLinearColor(HudGold.R, HudGold.G, HudGold.B, .9f));
        // selo do nível
        const FVector2D C(X + 22.f * Scale, Y + Hh * .5f);
        for (int32 Pass = 0; Pass < 2; ++Pass)
        {
            const float R = (Pass == 0 ? 15.f : 12.5f) * Scale;
            FCanvasTriangleItem Top(C + FVector2D(0, -R), C + FVector2D(R, 0), C + FVector2D(-R, 0), GWhiteTexture);
            FCanvasTriangleItem Bottom(C + FVector2D(0, R), C + FVector2D(-R, 0), C + FVector2D(R, 0), GWhiteTexture);
            const FLinearColor Color = Pass == 0 ? HudGold : FLinearColor(.06f, .1f, .08f);
            Top.SetColor(Color); Bottom.SetColor(Color);
            Canvas->DrawItem(Top); Canvas->DrawItem(Bottom);
        }
        Label(FString::FromInt(Level), C.X - 18.f * Scale, C.Y - 10.f * Scale, 36.f * Scale, 20.f * Scale, 15.f * Scale, HudCream, 1);
        // nome, distância e as barras
        const float Meters = FVector::Dist(Mate->GetActorLocation(), Hero->GetActorLocation()) / 100.f;
        Label(NameOf(Mate), X + 44.f * Scale, Y + 3.f * Scale, W - 100.f * Scale, 18.f * Scale, 16.f * Scale, bDead ? HudMuted : FLinearColor(.55f, 1.f, .65f), 0);
        Label(bDead ? FString(TEXT("caído")) : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meters)), X + W - 64.f * Scale, Y + 3.f * Scale, 58.f * Scale, 18.f * Scale, 13.f * Scale, bDead ? FLinearColor(1.f, .35f, .3f) : HudMuted, 2, false);
        const float BarX = X + 44.f * Scale, BarW = W - 52.f * Scale;
        Tile(BarX, Y + 24.f * Scale, BarW, 8.f * Scale, FLinearColor(.12f, .01f, .01f, .9f));
        Tile(BarX, Y + 24.f * Scale, BarW * Health, 8.f * Scale, Health < .3f ? FLinearColor(.75f, .05f, .03f) : FLinearColor(.5f, .02f, .02f));
        Tile(BarX, Y + 24.f * Scale, BarW * Health, 3.f * Scale, FLinearColor(1.f, 1.f, 1.f, .15f));
        Tile(BarX, Y + 34.f * Scale, BarW, 4.f * Scale, FLinearColor(.01f, .02f, .08f, .9f));
        Tile(BarX, Y + 34.f * Scale, BarW * Mana, 4.f * Scale, FLinearColor(.05f, .2f, .75f));
        if (bDead)
        {
            Disc(C.X + W - 30.f * Scale, Y + Hh * .5f + 6.f * Scale, 6.f * Scale, FLinearColor(.92f, .9f, .85f), FLinearColor(.5f, .05f, .05f));
        }
        Y += Hh + 8.f * Scale;
    }
    // ---- no mundo: nome de cada jogador (verde no grupo, vermelho no duelo); seta na borda só para o grupo
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
    {
        AVortexCharacter* Mate = *It;
        if (Mate == Hero || !Mate->IsPlayerControlled()) continue;
        const bool bParty = Social && Social->SameParty(Mate);
        const bool bDuel = Social && Social->GetDuelOpponent() == Mate;
        const FLinearColor Color = bDuel ? FLinearColor(1.f, .35f, .2f) : bParty ? FLinearColor(.55f, 1.f, .65f) : FLinearColor(.6f, .8f, 1.f);
        const FString Name = NameOf(Mate);
        const int32 Level = Mate->GetSheet() ? Mate->GetSheet()->GetLevel() : 1;
        const FVector Head = Mate->GetActorLocation() + FVector(0.f, 0.f, 120.f);
        FVector Screen = Project(Head, false);
        const float Meters = FVector::Dist(Mate->GetActorLocation(), Hero->GetActorLocation()) / 100.f;
        const bool bOnScreen = Screen.Z > 0.f && Screen.X > 20.f && Screen.Y > 20.f && Screen.X < Canvas->ClipX - 20.f && Screen.Y < Canvas->ClipY - 20.f;
        if (bOnScreen && Meters < 80.f)
        {
            // 02/11: a caveira (matou alguém fora do grupo, 3 dias do jogo): o nome em vermelho e a caveira em cima
            const bool bOutlaw = Mate->GetSheet() && Mate->GetSheet()->IsOutlaw();
            Label(FString::Printf(TEXT("%s%s  ·  %d"), bDuel ? TEXT("DUELO  ·  ") : TEXT(""), *Name, Level), Screen.X - 200.f, Screen.Y - 46.f * Scale, 400.f, 24.f * Scale, 19.f * Scale,
                bOutlaw ? FLinearColor(1.f, .25f, .2f) : Color, 1);
            if (bOutlaw)
            {
                static TWeakObjectPtr<UTexture2D> Skull;
                if (!Skull.IsValid()) Skull = LoadObject<UTexture2D>(nullptr, TEXT("/Game/FreeParticle_SoftTofu/Texture/T_SkullHead.T_SkullHead"));
                const float S = 40.f * Scale;
                if (Skull.IsValid()) DrawTexture(Skull.Get(), Screen.X - S * .5f, Screen.Y - 46.f * Scale - S - 2.f * Scale, S, S, 0.f, 0.f, 1.f, 1.f, FLinearColor(1.f, .9f, .85f), BLEND_Additive);
                else Label(TEXT("CAVEIRA"), Screen.X - 100.f, Screen.Y - 70.f * Scale, 200.f, 20.f * Scale, 16.f * Scale, FLinearColor(1.f, .25f, .2f), 1);
            }
            if (bParty || bDuel)
            {
                // a vida do companheiro (ou do adversário) em cima da cabeça
                const UVortexAttributes* Vitals = Mate->GetVitals();
                const float Health = Vitals ? FMath::Clamp(Vitals->GetHealth() / FMath::Max(1.f, Vitals->GetMaxHealth()), 0.f, 1.f) : 1.f;
                const float W = 90.f * Scale;
                Tile(Screen.X - W * .5f - 1.f, Screen.Y - 22.f * Scale - 1.f, W + 2.f, 7.f * Scale + 2.f, FLinearColor(0.f, 0.f, 0.f, .6f));
                Tile(Screen.X - W * .5f, Screen.Y - 22.f * Scale, W * Health, 7.f * Scale, bDuel ? FLinearColor(.6f, .08f, .02f) : FLinearColor(.08f, .45f, .12f));
            }
        }
        else if (bParty)
        {
            FVector2D Dir(Screen.X - Canvas->ClipX * .5f, Screen.Y - Canvas->ClipY * .5f);
            if (Screen.Z <= 0.f) Dir = -Dir;
            if (Dir.IsNearlyZero()) Dir = FVector2D(0.f, -1.f);
            Dir.Normalize();
            const FVector2D Half(Canvas->ClipX * .5f - 60.f * Scale, Canvas->ClipY * .5f - 60.f * Scale);
            const float K = FMath::Min(Half.X / FMath::Max(.001f, FMath::Abs(Dir.X)), Half.Y / FMath::Max(.001f, FMath::Abs(Dir.Y)));
            const FVector2D At = FVector2D(Canvas->ClipX * .5f, Canvas->ClipY * .5f) + Dir * K;
            const FVector2D Side(-Dir.Y, Dir.X);
            const float A = 14.f * Scale;
            FCanvasTriangleItem Arrow(At + Dir * A, At - Dir * A * .6f + Side * A * .8f, At - Dir * A * .6f - Side * A * .8f, GWhiteTexture);
            Arrow.SetColor(FLinearColor(Color.R, Color.G, Color.B, .95f));
            Canvas->DrawItem(Arrow);
            Label(FString::Printf(TEXT("%s  %d m"), *Name, FMath::RoundToInt(Meters)), At.X - 150.f, At.Y - Dir.Y * 26.f * Scale - 12.f * Scale - (Dir.Y > 0 ? 24.f * Scale : 0.f),
                300.f, 24.f * Scale, 19.f * Scale, Color, 1);
        }
    }
    // ---- duelo: a contagem grande e, na luta, a faixa com as duas vidas
    if (Social && Social->GetDuelPhase() != 0 && Social->GetDuelOpponent())
    {
        const AVortexCharacter* Foe = Social->GetDuelOpponent();
        if (Social->GetDuelPhase() == 1)
        {
            const float Left = Social->DuelCountdown();
            const int32 Number = FMath::CeilToInt(Left);
            const float Pop = 1.f + .35f * (1.f - FMath::Frac(Left));
            if (Number > 0)
            {
                const float Size = 120.f * Scale * Pop;
                Label(FString::FromInt(Number), 4.f, Canvas->ClipY * .3f + 4.f, Canvas->ClipX, Size, Size, FLinearColor(0.f, 0.f, 0.f, .7f), 1);
                Label(FString::FromInt(Number), 0.f, Canvas->ClipY * .3f, Canvas->ClipX, Size, Size, FLinearColor(1.f, .4f, .2f), 1);
            }
            Label(FString::Printf(TEXT("Duelo contra %s"), *NameOf(Foe)), 0.f, Canvas->ClipY * .3f - 40.f * Scale, Canvas->ClipX, 34.f * Scale, 28.f * Scale, HudGold, 1);
        }
        else
        {
            const float W = 520.f * Scale, X = (Canvas->ClipX - W) * .5f, Yb = 92.f * Scale;
            Tile(X - 3.f, Yb - 3.f, W + 6.f, 46.f * Scale + 6.f, FLinearColor(0.f, 0.f, 0.f, .55f));
            Tile(X, Yb, W, 46.f * Scale, FLinearColor(.03f, .015f, .015f, .85f));
            Label(FString::Printf(TEXT("DUELO  ·  %s  x  %s"), *Social->GetHeroName(), *NameOf(Foe)), X, Yb + 2.f * Scale, W, 20.f * Scale, 16.f * Scale, HudGold, 1);
            auto Bar = [&](const AVortexCharacter* Who, float Bx, bool bRight)
            {
                const UVortexAttributes* V = Who->GetVitals();
                const float F = V ? FMath::Clamp(V->GetHealth() / FMath::Max(1.f, V->GetMaxHealth()), 0.f, 1.f) : 1.f;
                const float Bw = W * .46f;
                Tile(Bx, Yb + 26.f * Scale, Bw, 12.f * Scale, FLinearColor(.1f, .01f, .01f));
                Tile(bRight ? Bx + Bw * (1.f - F) : Bx, Yb + 26.f * Scale, Bw * F, 12.f * Scale, FLinearColor(.6f, .06f, .02f));
                // a linha dos 15%: quem chegar nela perde
                const float Mark = bRight ? Bx + Bw * (1.f - UVortexSocial::DuelLoseAt) : Bx + Bw * UVortexSocial::DuelLoseAt;
                Tile(Mark, Yb + 24.f * Scale, 2.f, 16.f * Scale, HudGold);
            };
            Bar(Hero, X + W * .03f, false);
            Bar(Foe, X + W * .51f, true);
        }
    }
}

// ---- mapa do vale (M) -----------------------------------------------------------------------------

// ---- mapa descoberto aos poucos ----------------------------------------------------------------------

namespace
{
    /** Ruído suave (0..1) pelo lugar: as nuvens da névoa mudam de forma devagar, sempre iguais no mesmo ponto. */
    float SoftNoise(float X, float Y, float Cell, uint32 Salt)
    {
        const float Fx = X / Cell, Fy = Y / Cell;
        const int32 Ix = FMath::FloorToInt(Fx), Iy = FMath::FloorToInt(Fy);
        const float Tx = FMath::SmoothStep(0.f, 1.f, Fx - Ix), Ty = FMath::SmoothStep(0.f, 1.f, Fy - Iy);
        auto H = [Salt](int32 A, int32 B) { return (HashCombineFast(HashCombineFast(uint32(A) * 73856093u, uint32(B) * 19349663u), Salt) % 1000u) / 999.f; };
        return FMath::Lerp(FMath::Lerp(H(Ix, Iy), H(Ix + 1, Iy), Tx), FMath::Lerp(H(Ix, Iy + 1), H(Ix + 1, Iy + 1), Tx), Ty);
    }

    /** A cor da névoa num ponto: nuvem pintada (clara em cima, sombreada embaixo), deixando ver de leve o contorno do
     *  que está por baixo; F = quanto o ponto já foi visto (0..1), Base = a cor do mapa ali. */
    FColor PaintCloud(const FColor& Base, float F, float X, float Y, float Cell)
    {
        if (F >= .999f) return Base;
        const float N = SoftNoise(X, Y, Cell * 9.f, 7u) * .6f + SoftNoise(X, Y, Cell * 3.f, 13u) * .3f + SoftNoise(X, Y, Cell, 29u) * .1f;
        // a borda das nuvens ondula com o ruído (não é um degrau reto entre pedaços)
        const float V = FMath::SmoothStep(.18f, .55f, F + (N - .5f) * .45f);
        FLinearColor Cloud = FMath::Lerp(FLinearColor::FromSRGBColor(FColor(214, 222, 238)), FLinearColor::FromSRGBColor(FColor(246, 248, 252)), N);
        // por baixo, só o contorno do mundo (bem apagado)
        Cloud = FMath::Lerp(Cloud, FLinearColor::FromSRGBColor(Base), .1f);
        FLinearColor C = FMath::Lerp(Cloud, FLinearColor::FromSRGBColor(Base), V);
        // sombra azulada na beira da nuvem
        C *= 1.f - .22f * FMath::Sin(FMath::Clamp(V, 0.f, 1.f) * UE_PI) * (1.f - V);
        return C.ToFColor(true);
    }

    /** O mapa com a névoa: pedaços não vistos ficam sob nuvens pintadas, com a borda macia e ondulada. */
    TArray<FColor> FogMapPixels(const TArray<FColor>& Base, const TArray<uint8>& Seen, FIntPoint SeenSize, int32 CellsPerChunk, FIntPoint Size, const FIntRect& Rect)
    {
        TArray<FColor> Out;
        Out.SetNumUninitialized(Rect.Area());
        auto SeenAt = [&](int32 X, int32 Y) { return Seen[FMath::Clamp(Y, 0, SeenSize.Y - 1) * SeenSize.X + FMath::Clamp(X, 0, SeenSize.X - 1)] / 255.f; };
        for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
        {
            const float Fy = (Y + .5f) / CellsPerChunk - .5f;
            const int32 Cy = FMath::FloorToInt(Fy);
            const float Ty = Fy - Cy;
            for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
            {
                const float Fx = (X + .5f) / CellsPerChunk - .5f;
                const int32 Cx = FMath::FloorToInt(Fx);
                const float Tx = Fx - Cx;
                const float F = FMath::Lerp(FMath::Lerp(SeenAt(Cx, Cy), SeenAt(Cx + 1, Cy), Tx), FMath::Lerp(SeenAt(Cx, Cy + 1), SeenAt(Cx + 1, Cy + 1), Tx), Ty);
                const int32 I = Y * Size.X + X, O = (Y - Rect.Min.Y) * Rect.Width() + X - Rect.Min.X;
                Out[O] = PaintCloud(Base[I], F, float(X), float(Y), float(CellsPerChunk));
            }
        }
        return Out;
    }
}

FColor VortexMapPaint::CloudOver(const FColor& Base, float F, float X, float Y, float Cell) { return PaintCloud(Base, F, X, Y, Cell); }

bool AVortexAimHUD::IsDiscovered(const FVector& At) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!bSeenReady || !Data) return true;
    const int32 X = FMath::FloorToInt(At.X / Data->ChunkSize()) - SeenOrigin.X, Y = FMath::FloorToInt(At.Y / Data->ChunkSize()) - SeenOrigin.Y;
    return X >= 0 && Y >= 0 && X < SeenSize.X && Y < SeenSize.Y && Seen[Y * SeenSize.X + X] > 0;
}

void AVortexAimHUD::UploadMap(const TArray<FColor>& Pixels)
{
    VORTEX_PROBE("HUD.EnviaMapa");
    if (!MapTexture || Pixels.Num() != MapSize.X * MapSize.Y) return;
    // cópia própria: a placa de vídeo lê depois, e a memória é solta no fim
    uint8* Copy = static_cast<uint8*>(FMemory::Malloc(Pixels.Num() * sizeof(FColor)));
    FMemory::Memcpy(Copy, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, MapSize.X, MapSize.Y);
    MapTexture->UpdateTextureRegions(0, 1, Region, MapSize.X * sizeof(FColor), sizeof(FColor), Copy,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
}

void AVortexAimHUD::UploadMapRect(const FIntRect& Rect, const TArray<FColor>& Pixels)
{
    VORTEX_PROBE("HUD.EnviaMapa");
    if (!MapTexture || Rect.Area() <= 0 || Pixels.Num() != Rect.Area()) return;
    uint8* Copy = static_cast<uint8*>(FMemory::Malloc(Pixels.Num() * sizeof(FColor)));
    FMemory::Memcpy(Copy, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(Rect.Min.X, Rect.Min.Y, 0, 0, Rect.Width(), Rect.Height());
    MapTexture->UpdateTextureRegions(0, 1, Region, Rect.Width() * sizeof(FColor), sizeof(FColor), Copy,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
}

void AVortexAimHUD::MarkFog(const FIntPoint& Cell)
{
    // a borda suave mistura o pedaço com os vizinhos: refaz um pedaço a mais em volta
    const FIntRect Around(Cell - FIntPoint(1, 1), Cell + FIntPoint(2, 2));
    if (FogDirty.Area() <= 0) FogDirty = Around;
    else FogDirty.Union(Around);
    bFogDirty = true;
}

void AVortexAimHUD::UpdateDiscovery(AVortexCharacter* Hero, float DeltaSeconds)
{
    VORTEX_PROBE("HUD.Nevoa");
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!MapTexture || !MapBase || !Data) return;
    UVortexGameInstance* Game = Cast<UVortexGameInstance>(GetGameInstance());
    UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr;
    if (!bSeenReady)
    {
        bSeenReady = true;
        SeenOrigin = Data->MapChunkMin();
        SeenSize = Data->MapChunkCount();
        Seen.Init(0, SeenSize.X * SeenSize.Y);
        SeenTotal = MapLand.IsValid() && *MapLand > 0 ? *MapLand : SeenSize.X * SeenSize.Y;
        // o que este herói já viu (o save guarda a origem: se o mundo crescer, cada pedaço volta ao lugar)
        if (Save && Save->MapSeen.Num() == Save->MapSeenSize.X * Save->MapSeenSize.Y)
            for (int32 Y = 0; Y < Save->MapSeenSize.Y; ++Y) for (int32 X = 0; X < Save->MapSeenSize.X; ++X)
            {
                const FIntPoint C = Save->MapSeenOrigin + FIntPoint(X, Y) - SeenOrigin;
                if (Save->MapSeen[Y * Save->MapSeenSize.X + X] && C.X >= 0 && C.Y >= 0 && C.X < SeenSize.X && C.Y < SeenSize.Y && !Seen[C.Y * SeenSize.X + C.X])
                {
                    Seen[C.Y * SeenSize.X + C.X] = 255;
                    ++SeenCount;
                }
            }
        // pontos de interesse do Godot (menires, arcos, estátuas, altares, pilares): aparecem quando descobertos
        static const TMap<FString, FString> PoiNames = { { TEXT("poi_menhir"), TEXT("Menir") }, { TEXT("poi_arch"), TEXT("Arco antigo") },
            { TEXT("poi_statue"), TEXT("Estátua") }, { TEXT("poi_altar"), TEXT("Altar") }, { TEXT("poi_pillar"), TEXT("Pilares") } };
        for (const FVortexTerrainData::FFeature& Feature : Data->Features())
            MapPois.Add({ Feature.At, Feature.bBridge ? TEXT("Ponte") : TEXT("Cabana abandonada") });
        // ruínas, torres, santuários, minas, cemitérios, caravanas e cais (o mesmo sorteio que monta cada um)
        if (Data->HasWorld())
        {
            const FBox2D Bounds = Data->MapBounds();
            const float Cell = AVortexPointsOfInterest::CellSize;
            for (int32 CY = FMath::FloorToInt(Bounds.Min.Y / Cell); CY <= FMath::FloorToInt(Bounds.Max.Y / Cell); ++CY)
                for (int32 CX = FMath::FloorToInt(Bounds.Min.X / Cell); CX <= FMath::FloorToInt(Bounds.Max.X / Cell); ++CX)
                {
                    FVortexPoi Poi;
                    if (AVortexPointsOfInterest::PoiAt(FIntPoint(CX, CY), Poi)) MapPois.Add({ Poi.At, AVortexPointsOfInterest::KindName(Poi.Kind) });
                    FVector2D GateAt; FString GateRank; float GateYaw = 0.f;
                    if (AVortexPointsOfInterest::GateAt(FIntPoint(CX, CY), GateAt, GateRank, GateYaw)) MapPois.Add({ GateAt, GateRank == TEXT("S") ? FString(TEXT("Portal VERMELHO")) : TEXT("Portal ") + GateRank });
                }
        }
        for (int32 Y = 0; Y < Data->ChunkCountY; ++Y) for (int32 X = 0; X < Data->ChunkCountX; ++X)
            if (const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(FIntPoint(Data->ChunkMinX + X, Data->ChunkMinY + Y)))
                for (const FVortexTerrainProp& Prop : *Props)
                {
                    const FString* Name = Data->Kinds.IsValidIndex(Prop.Kind) ? PoiNames.Find(Data->Kinds[Prop.Kind]) : nullptr;
                    if (!Name) continue;
                    bool bNear = false;   // um nome por grupo (pilares e menires vêm em roda)
                    for (const FMapPoi& Poi : MapPois) bNear |= Poi.Name == *Name && FVector2D::Distance(Poi.At, FVector2D(Prop.Location)) < 3000.f;
                    if (!bNear) MapPois.Add({ FVector2D(Prop.Location), *Name });
                }
        FogDirty = FIntRect(FIntPoint::ZeroValue, SeenSize);
        bFogDirty = true;
        FogClock = 10.f;
    }
    // descobre em volta do herói (70 m no vale; 110 m com o continente, que é bem maior)
    DiscoverClock += DeltaSeconds;
    if (DiscoverClock >= .25f)
    {
        DiscoverClock = 0.f;
        // no topo de uma torre de vigia (pontos de interesse) a vista vai a 300 m
        const AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld());
        const bool bTower = Pois && Pois->IsOnTowerTop(Hero->GetActorLocation());
        const float Size = Data->ChunkSize(), Radius = bTower ? 30000.f : Data->HasWorld() ? 11000.f : 7000.f;
        const FVector Here = Hero->GetActorLocation();
        const int32 Span = FMath::CeilToInt(Radius / Size);
        const FIntPoint C(FMath::FloorToInt(Here.X / Size), FMath::FloorToInt(Here.Y / Size));
        for (int32 dy = -Span; dy <= Span; ++dy) for (int32 dx = -Span; dx <= Span; ++dx)
        {
            const FIntPoint P = C + FIntPoint(dx, dy) - SeenOrigin;
            if (P.X < 0 || P.Y < 0 || P.X >= SeenSize.X || P.Y >= SeenSize.Y || Seen[P.Y * SeenSize.X + P.X]) continue;
            const FVector2D Center((C.X + dx + .5f) * Size, (C.Y + dy + .5f) * Size);
            if (FVector2D::Distance(Center, FVector2D(Here)) > Radius) continue;
            Seen[P.Y * SeenSize.X + P.X] = 255;
            ++SeenCount;
            MarkFog(P);
            bSeenUnsaved = true;
        }
    }
    // refaz a textura (em outra thread, no máximo uma vez por segundo)
    FogClock += DeltaSeconds;
    if (FogJob.IsValid() && FogJob.IsReady())
    {
        const FFogPatch Patch = FogJob.Consume();
        UploadMapRect(Patch.Rect, Patch.Pixels);
    }
    if (bFogDirty && !FogJob.IsValid() && FogClock >= 1.f)
    {
        // só o retângulo dos pedaços que mudaram (antes refazia e enviava o mapa inteiro, 25 MB, a cada segundo)
        const int32 K = CellsPerChunk / MapStep;
        FIntRect Rect(FogDirty.Min * K, FogDirty.Max * K);
        Rect.Clip(FIntRect(FIntPoint::ZeroValue, MapSize));
        bFogDirty = false;
        FogDirty = FIntRect(0, 0, 0, 0);
        FogClock = 0.f;
        if (Rect.Area() > 0)
            FogJob = Async(EAsyncExecution::ThreadPool, [Base = MapBase, SeenCopy = Seen, S = SeenSize, K, M = MapSize, Rect]()
            {
                return FFogPatch{ Rect, FogMapPixels(*Base, SeenCopy, S, K, M, Rect) };
            });
    }
    SeenSaveClock += DeltaSeconds;
    // grava o save inteiro (~80 ms): uma vez por minuto basta (e ao sair)
    if (bSeenUnsaved && SeenSaveClock > 60.f) SaveDiscovery();
}

int32 AVortexAimHUD::RevealTravelerRoute(int32 Hundreds, int32 Seed, const FVector& Hero)
{
    // como o Main.Maps.cs do Godot: os mercadores andam por terra, fazendo curvas; a mão de quem
    // desenhou deixa a borda irregular e, de vez em quando, uma mancha do que se via de lado
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data || !bSeenReady || Hundreds <= 0) return 0;
    FRandomStream Rng(Seed);
    const float Size = Data->ChunkSize(), Step = 1200.f;
    const FBox2D Bounds = Data->MapBounds();
    auto Sea = [&](const FVector2D& P) { return !Bounds.IsInside(P) || (Data->HasWorld() && Data->RegionAt(P.X, P.Y) == 0); };
    const int32 Before = SeenCount;
    auto Reveal = [&](const FVector2D& At, float Radius)
    {
        const int32 Span = FMath::CeilToInt(Radius / Size);
        const FIntPoint C(FMath::FloorToInt(At.X / Size), FMath::FloorToInt(At.Y / Size));
        for (int32 dy = -Span; dy <= Span; ++dy) for (int32 dx = -Span; dx <= Span; ++dx)
        {
            const FIntPoint P = C + FIntPoint(dx, dy) - SeenOrigin;
            if (P.X < 0 || P.Y < 0 || P.X >= SeenSize.X || P.Y >= SeenSize.Y || Seen[P.Y * SeenSize.X + P.X]) continue;
            if (FVector2D::Distance(FVector2D((C.X + dx + .5f) * Size, (C.Y + dy + .5f) * Size), At) > Radius) continue;
            Seen[P.Y * SeenSize.X + P.X] = 255;
            ++SeenCount;
            MarkFog(P);
        }
    };
    const bool bFar = Hundreds >= 8;
    // perto: das terras em volta da cidade para fora; longe: começa a 500-1600 m (fora do mar)
    FVector2D P(0.f, 0.f), Heading(1.f, 0.f);
    for (int32 Try = 0; Try < 16; ++Try)
    {
        const float Angle = Rng.FRandRange(0.f, 2.f * PI), Distance = bFar ? Rng.FRandRange(50000.f, 160000.f) : Rng.FRandRange(15000.f, 40000.f);
        P = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
        Heading = P.GetSafeNormal().GetRotated(FMath::RadiansToDegrees(Rng.FRandRange(-1.2f, 1.2f)));
        if (!Sea(P)) break;
    }
    const float Length = Hundreds * 10000.f;
    FVector2D Last = P;
    for (float Walked = 0.f; Walked < Length; Walked += Step)
    {
        Heading = Heading.GetRotated(FMath::RadiansToDegrees(Rng.FRandRange(-.32f, .32f)));
        // não sai do mapa e contorna o mar
        if (!Bounds.ExpandBy(-15000.f).IsInside(P)) Heading = FMath::Lerp(Heading, (Bounds.GetCenter() - P).GetSafeNormal(), .35f).GetSafeNormal();
        FVector2D Next = P + Heading * Step;
        for (int32 Turn = 0; Turn < 6 && Sea(Next); ++Turn)
        {
            Heading = Heading.GetRotated((Rng.RandRange(0, 1) * 2 - 1) * 60.f);
            Next = P + Heading * Step;
        }
        P = Next;
        Last = P;
        Reveal(P, Rng.FRandRange(1400.f, 3000.f));
        if (Rng.FRand() < .07f)
        {
            const FVector2D Side = FVector2D(-Heading.Y, Heading.X) * (Rng.RandRange(0, 1) * 2 - 1) * Rng.FRandRange(2500.f, 6000.f);
            Reveal(P + Side, Rng.FRandRange(2000.f, 4200.f));
        }
    }
    // a carta distante termina num pedaço grande: as terras de onde a caravana veio
    if (bFar) Reveal(Last, 8000.f);
    bSeenUnsaved = true;
    SeenSaveClock = 1000.f;   // grava já (o mapa foi gasto)
    return SeenCount - Before;
}

void AVortexAimHUD::SaveDiscovery()
{
    SeenSaveClock = 0.f;
    bSeenUnsaved = false;
    UVortexGameInstance* Game = Cast<UVortexGameInstance>(GetGameInstance());
    UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr;
    UVortexSaveSubsystem* Saves = Game ? Game->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
    if (!Save || !Saves || !bSeenReady) return;
    Save->MapSeen = Seen;
    Save->MapSeenOrigin = SeenOrigin;
    Save->MapSeenSize = SeenSize;
    Saves->SaveLocalPlayer(Save);
}

void AVortexAimHUD::DrawControls(AVortexCharacter* Hero, float Alpha)
{
    // 07/11 (o usuário: "o tutorial do F1 uma porcaria; queria aqueles que ensinam um por um, ele vai fazendo, vai dando
    // check e avançando, com a cara do jogo"): Alpha < 0 = o tutorial passo a passo do herói novo (no Estádio). Um cartão
    // por vez: a tecla, o que fazer, a barrinha de progresso; quando o jogador faz, o check verde pula e passa para o
    // próximo. Backspace pula o tutorial; F1 continua abrindo a lista completa de comandos.
    if (Alpha < 0.f)
    {
        struct FStep { const TCHAR* Key; const TCHAR* Title; const TCHAR* Hint; float Need; };
        static const FStep Steps[] = {
            { TEXT("W A S D"), TEXT("ANDE"), TEXT("Ande um pouco pelo Estádio"), 1.2f },
            { TEXT("MOUSE"), TEXT("OLHE EM VOLTA"), TEXT("Mexa o mouse para girar a câmera"), 120.f },
            { TEXT("SHIFT"), TEXT("CORRA"), TEXT("Segure Shift enquanto anda"), 1.f },
            { TEXT("ESPAÇO"), TEXT("PULE"), TEXT("Aperte Espaço"), 1.f },
            { TEXT("CTRL"), TEXT("ROLE"), TEXT("Aperte Ctrl andando: a esquiva"), 1.f },
            { TEXT("E"), TEXT("PEGUE UMA ARMA"), TEXT("Chegue perto de um pedestal e aperte E"), 1.f },
            { TEXT("CLIQUE ESQ."), TEXT("ATAQUE"), TEXT("Clique com o botão esquerdo (no arco, segure)"), 1.f },
            { TEXT("CLIQUE DIR."), TEXT("GOLPE ESPECIAL"), TEXT("O golpe especial da sua arma"), 1.f },
            // 07/11 (o usuário: "o F de defender não defende; faltou primeira pessoa, chamar cavalo etc."): sem o F, com os
            // outros comandos do F1 (do 8 em diante é só apertar a tecla: a lista Keys abaixo)
            { TEXT("V"), TEXT("TROQUE O OMBRO"), TEXT("A câmera passa para o outro ombro"), 1.f },
            { TEXT("C"), TEXT("PRIMEIRA PESSOA"), TEXT("Aperte C (de novo para voltar)"), 1.f },
            { TEXT("Q"), TEXT("BEBA UMA POÇÃO"), TEXT("Q bebe a poção de vida (R: a de mana)"), 1.f },
            { TEXT("X"), TEXT("CHAME O CAVALO"), TEXT("Assobie: quando tiver um cavalo, ele vem correndo"), 1.f },
            { TEXT("Z"), TEXT("MARQUE UM PONTO"), TEXT("Marca no mapa onde você olha (para o grupo)"), 1.f },
            { TEXT("B"), TEXT("EMOTES"), TEXT("Segure B para a roda de emotes"), 1.f },
            { TEXT("M"), TEXT("MAPA"), TEXT("Troca o minimapa e abre o mapa grande"), 1.f },
            { TEXT("I"), TEXT("ABRA A MOCHILA"), TEXT("Itens, atributos e missões (I ou Tab)"), 1.f },
        };
        static const FKey StepKeys[] = { EKeys::V, EKeys::C, EKeys::Q, EKeys::X, EKeys::Z, EKeys::B, EKeys::M, EKeys::I };
        constexpr int32 Count = UE_ARRAY_COUNT(Steps);
        static int32 Step = 0;
        static float Progress = 0.f, DoneAt = -1.f;
        static FRotator LastView = FRotator::ZeroRotator;
        static bool bSkipped = false;
        APlayerController* PC = GetOwningPlayerController();
        const float Now = GetWorld()->GetTimeSeconds();
        const float Dt = GetWorld()->GetDeltaSeconds();
        if (!PC || bSkipped) return;
        if (PC->WasInputKeyJustPressed(EKeys::BackSpace)) { bSkipped = true; return; }
        if (Step >= Count)
        {
            // fim: o selo de pronto por 3 s e some
            if (DoneAt > 0.f && Now - DoneAt < 3.f)
                CartoonText(TEXT("PRONTO! BOA JORNADA"), Canvas->ClipX * .5f, Canvas->ClipY * .26f, 54.f * Scale, FLinearColor(.55f, 1.f, .45f), FLinearColor(.05f, .12f, .04f), 1.f - FMath::Max(0.f, Now - DoneAt - 2.2f) / .8f);
            return;
        }
        const FStep& S = Steps[Step];
        // o que o jogador fez neste quadro (só enquanto o passo não está concluído)
        const bool bDone = DoneAt > 0.f;
        if (!bDone)
        {
            const float Speed = Hero->GetVelocity().Size2D();
            const UVortexCharacterMovementComponent* Move = Cast<UVortexCharacterMovementComponent>(Hero->GetCharacterMovement());
            const FRotator View = PC->GetControlRotation();
            switch (Step)
            {
            case 0: if (Speed > 80.f) Progress += Dt; break;
            case 1: Progress += FMath::Abs(FRotator::NormalizeAxis(View.Yaw - LastView.Yaw)) + FMath::Abs(FRotator::NormalizeAxis(View.Pitch - LastView.Pitch)); break;
            case 2: if (Move && Move->IsSprinting() && Speed > 200.f) Progress += Dt; break;
            case 3: if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) Progress = 1.f; break;
            case 4: if (PC->WasInputKeyJustPressed(EKeys::LeftControl) || (Move && Move->IsDodging())) Progress = 1.f; break;
            case 5: if (Hero->GetSheet() && !Hero->GetSheet()->GetWeaponType().IsNone()) Progress = 1.f; break;
            case 6: if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)) Progress = 1.f; break;
            case 7: if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton)) Progress = 1.f; break;
            default:
                if (PC->WasInputKeyJustPressed(StepKeys[Step - 8]) || (Step == Count - 1 && PC->WasInputKeyJustPressed(EKeys::Tab))) Progress = 1.f;
                break;
            }
            LastView = View;
            if (Progress >= S.Need) { DoneAt = Now; VortexAudio::Play2D(Hero, TEXT("pickup"), .6f); }
        }
        else if (Now - DoneAt > .9f)
        {
            ++Step;
            Progress = 0.f;
            DoneAt = Step >= Count ? Now : -1.f;
            LastView = PC->GetControlRotation();
            return;
        }
        // o cartão: no alto à esquerda (abaixo do quadro do herói), azul-noite com moldura dourada e a faixa da etapa
        const float W = 520.f * Scale, H = 176.f * Scale, X0 = 24.f * Scale, Y0 = Canvas->ClipY * .26f;
        const float Pop = bDone ? 1.f + .06f * FMath::Sin(FMath::Clamp((Now - DoneAt) / .25f, 0.f, 1.f) * PI) : 1.f;
        auto Tile = [&](float X, float Y, float Tw, float Th, const FLinearColor& Color)
        {
            FCanvasTileItem Item(FVector2D(X, Y), FVector2D(Tw, Th), Color);
            Item.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Item);
        };
        const FLinearColor Ok(.42f, .9f, .38f);
        Tile(X0 + 6.f * Scale, Y0 + 8.f * Scale, W, H, FLinearColor(0.f, 0.f, 0.f, .35f));
        Tile(X0 - 3.f * Scale, Y0 - 3.f * Scale, W + 6.f * Scale, H + 6.f * Scale, bDone ? Ok : HudGold);
        Tile(X0, Y0, W, H, FLinearColor(.035f, .045f, .1f, .94f));
        Tile(X0, Y0, W, 30.f * Scale, FLinearColor(.12f, .1f, .2f, 1.f));
        Label(FString::Printf(TEXT("TUTORIAL  •  PASSO %d DE %d"), Step + 1, Count), X0 + 14.f * Scale, Y0 + 4.f * Scale, W, 22.f * Scale, 15.f * Scale, HudGold, 0, true);
        Label(TEXT("Backspace: pular"), X0, Y0 + 5.f * Scale, W - 14.f * Scale, 22.f * Scale, 12.f * Scale, FLinearColor(.7f, .72f, .82f, .8f), 2, false);
        // a tecla: o quadradinho dourado em relevo
        const float KW = FMath::Max(92.f, 24.f + FCString::Strlen(S.Key) * 15.f) * Scale * Pop, KH = 64.f * Scale * Pop;
        const float KX = X0 + 22.f * Scale, KY = Y0 + 52.f * Scale;
        Tile(KX + 3.f * Scale, KY + 5.f * Scale, KW, KH, FLinearColor(.25f, .17f, .04f, 1.f));
        Tile(KX, KY, KW, KH, bDone ? Ok : FLinearColor(.86f, .66f, .2f, 1.f));
        Tile(KX + 4.f * Scale, KY + 4.f * Scale, KW - 8.f * Scale, KH - 12.f * Scale, bDone ? FLinearColor(.55f, .97f, .5f) : FLinearColor(.98f, .82f, .38f, 1.f));
        Label(S.Key, KX, KY, KW, KH - 6.f * Scale, 20.f * Scale, FLinearColor(.18f, .1f, .02f), 1, true);
        // o que fazer
        const float TX = KX + KW + 22.f * Scale;
        Label(S.Title, TX, Y0 + 46.f * Scale, W - (TX - X0) - 14.f * Scale, 40.f * Scale, 30.f * Scale, bDone ? Ok : FLinearColor(1.f, .93f, .75f), 0, true);
        Label(S.Hint, TX, Y0 + 88.f * Scale, W - (TX - X0) - 14.f * Scale, 26.f * Scale, 16.f * Scale, FLinearColor(.86f, .88f, .96f), 0, false);
        // a barra deste passo e as bolinhas de todos
        const float Fill = bDone ? 1.f : FMath::Clamp(Progress / S.Need, 0.f, 1.f);
        Tile(TX, Y0 + 122.f * Scale, W - (TX - X0) - 20.f * Scale, 8.f * Scale, FLinearColor(.2f, .2f, .3f, 1.f));
        Tile(TX, Y0 + 122.f * Scale, (W - (TX - X0) - 20.f * Scale) * Fill, 8.f * Scale, bDone ? Ok : HudGold);
        for (int32 i = 0; i < Count; ++i)
        {
            const float DX = X0 + 24.f * Scale + i * 22.f * Scale, DY = Y0 + H - 22.f * Scale;
            Tile(DX, DY, 14.f * Scale, 14.f * Scale, i < Step || (i == Step && bDone) ? Ok : i == Step ? HudGold : FLinearColor(.25f, .26f, .36f, 1.f));
        }
        if (bDone)
            CartoonText(TEXT("OK!"), KX + KW * .5f, KY - 6.f * Scale, 58.f * Scale * Pop, Ok, FLinearColor(.04f, .12f, .03f), 1.f);
        return;
    }
    // um cartão à esquerda, abaixo do quadro do herói: a tecla num quadradinho dourado e o que ela faz
    static const TPair<const TCHAR*, const TCHAR*> Lines[] = {
        { TEXT("W A S D"), TEXT("andar") }, { TEXT("SHIFT"), TEXT("correr") }, { TEXT("CAPS"), TEXT("correr sempre (cajado: travar a mira)") },
        { TEXT("ESPAÇO"), TEXT("pular") }, { TEXT("CTRL"), TEXT("rolar (esquiva)") },
        { TEXT("CLIQUE ESQ."), TEXT("atacar (segure no arco)") }, { TEXT("CLIQUE DIR."), TEXT("golpe especial da arma") },
        { TEXT("1 2 3 4"), TEXT("habilidades da classe") },
        { TEXT("Q / R"), TEXT("poção de vida / de mana") }, { TEXT("E"), TEXT("interagir, falar, pegar, montar") },
        { TEXT("X"), TEXT("assobiar: chamar o cavalo") }, { TEXT("B (segure)"), TEXT("roda de emotes") },
        { TEXT("C"), TEXT("primeira pessoa") }, { TEXT("V"), TEXT("trocar o ombro da câmera") },
        { TEXT("M"), TEXT("mapa (minimapa e mapa grande)") }, { TEXT("I / TAB"), TEXT("mochila, atributos e missões") },
        { TEXT("Z"), TEXT("marcar um ponto (Shift+Z: sua cor)") }, { TEXT("ESC"), TEXT("pausa e opções") },
    };
    const int32 Count = UE_ARRAY_COUNT(Lines);
    const float Row = 30.f * Scale, W = 430.f * Scale, H = (58.f + Count * 30.f + 16.f) * Scale;
    const float X0 = 18.f * Scale, Y0 = Canvas->ClipY * .5f - H * .5f;
    auto Tile = [&](float X, float Y, float Tw, float Th, const FLinearColor& Color)
    {
        FCanvasTileItem Item(FVector2D(X, Y), FVector2D(Tw, Th), Color);
        Item.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Item);
    };
    Tile(X0, Y0, W, H, FLinearColor(.03f, .04f, .09f, .78f * Alpha));
    Tile(X0, Y0, W, 4.f * Scale, FLinearColor(HudGold.R, HudGold.G, HudGold.B, .9f * Alpha));
    Label(TEXT("COMANDOS"), X0, Y0 + 10.f * Scale, W, 30.f * Scale, 24.f * Scale, FLinearColor(1.f, .85f, .35f, Alpha), 1, true);
    Label(TEXT("F1 mostra e esconde"), X0, Y0 + 36.f * Scale, W, 18.f * Scale, 14.f * Scale, FLinearColor(.8f, .82f, .9f, .8f * Alpha), 1);
    for (int32 i = 0; i < Count; ++i)
    {
        const float Y = Y0 + 62.f * Scale + i * Row;
        Tile(X0 + 12.f * Scale, Y + 2.f * Scale, 130.f * Scale, Row - 6.f * Scale, FLinearColor(.55f, .42f, .12f, .55f * Alpha));
        Label(Lines[i].Key, X0 + 12.f * Scale, Y, 130.f * Scale, Row - 2.f * Scale, 16.f * Scale, FLinearColor(1.f, .95f, .8f, Alpha), 1, true);
        Label(Lines[i].Value, X0 + 154.f * Scale, Y, W - 160.f * Scale, Row - 2.f * Scale, 17.f * Scale, FLinearColor(.92f, .94f, 1.f, Alpha), 0);
    }
}

// Estádio do Despertar: o cartão da arma do pedestal perto (como o GameHud.WeaponTitle do Godot) e
// o convite do portão dourado.
void AVortexAimHUD::DrawStadium(AVortexCharacter* Hero)
{
    const UVortexHeroSheet* Sheet = Hero->GetSheet();
    const AVortexAwakeningStadium* Stadium = AVortexAwakeningStadium::Find(GetWorld());
    if (!Sheet || !Sheet->NeedsAwakening() || !Stadium) return;
    const FVector Where = Hero->GetActorLocation();
    const float CX = Canvas->ClipX * .5f;
    const float Now = GetWorld()->GetTimeSeconds();
    // herói novo acordando no Estádio: a HQ da história do mundo, uma vez por herói neste computador
    // (os testes automáticos, que passam -Vortex..., não a veem; -VortexIntroShots a testa)
    const FLinearColor Ink(.12f, .05f, .02f);
    const float Pulse = 1.f + .05f * FMath::Sin(Now * 6.f);
    if (Stadium->IsAtExit(Where))
    {
        const bool bArmed = !Sheet->GetWeaponType().IsNone();
        CartoonText(bArmed ? TEXT("[E]  COMEÇAR A JORNADA") : TEXT("ESCOLHA UMA ARMA ANTES DE SAIR"), CX, Canvas->ClipY * .7f,
            (bArmed ? 46.f * Pulse : 36.f) * Scale, bArmed ? FLinearColor(1.f, .85f, .3f) : FLinearColor(1.f, .96f, .88f), Ink, 1.f);
        return;
    }
    const AVortexAwakeningStadium::FPedestal* Pedestal = Stadium->NearestPedestal(Where);
    const UVortexWeaponDefinition* D = Pedestal ? Pedestal->Definition.Get() : nullptr;
    if (!D)
    {
        StadiumCardType = NAME_None;
        CartoonText(TEXT("CHEGUE PERTO DE UMA ARMA E APERTE E  •  SAÍDA DOURADA A SUDESTE"), CX, Canvas->ClipY * .15f, 26.f * Scale, FLinearColor(1.f, .96f, .88f), Ink, .95f);
        return;
    }
    // cada ala tem a sua cor (a mesma dos estandartes e tapetes do estádio)
    struct FWingLook { const TCHAR* Name; FColor Color; };
    static const TMap<FName, FWingLook> WingLooks = {
        { TEXT("marcial"), { TEXT("ALA MARCIAL"), FColor(0xa8, 0x28, 0x1f) } }, { TEXT("distancia"), { TEXT("ALA DE DISTÂNCIA"), FColor(0x2f, 0x7a, 0x34) } },
        { TEXT("arcana"), { TEXT("ALA ARCANA"), FColor(0x27, 0x50, 0x9c) } }, { TEXT("exotica"), { TEXT("ALA EXÓTICA"), FColor(0x5e, 0x2f, 0x80) } } };
    const FWingLook* Wing = WingLooks.Find(D->Wing);
    const FLinearColor WingColor = Wing ? FLinearColor(Wing->Color) : FLinearColor(.6f, .5f, .3f);
    const FLinearColor Bright = FMath::Lerp(WingColor, FLinearColor::White, .45f);
    const bool bHolding = Sheet->GetWeaponType() == Pedestal->Type;
    // chegou perto de outro pedestal: o cartão pula de novo
    if (Pedestal->Type != StadiumCardType) { StadiumCardType = Pedestal->Type; StadiumCardAt = Now; }
    const float T = FMath::Clamp((Now - StadiumCardAt) / .45f, 0.f, 1.f);
    const float Spring = 1.f - FMath::Exp(-7.f * T) * FMath::Cos(T * 12.f);
    const float A = FMath::Clamp(T * 4.f, 0.f, 1.f);
    // o cartão: sombra, fundo azul-noite, faixa na cor da ala e fios dourados
    const float W = 1000.f * Scale, H = 300.f * Scale, Y0 = Canvas->ClipY * .06f + (1.f - FMath::Min(Spring, 1.f)) * -30.f * Scale;
    auto Tile = [&](float X, float Y, float Tw, float Th, const FLinearColor& Color)
    {
        FCanvasTileItem Item(FVector2D(X, Y), FVector2D(Tw, Th), Color);
        Item.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Item);
    };
    Tile(CX - W * .5f + 6.f * Scale, Y0 + 8.f * Scale, W, H, FLinearColor(0.f, 0.f, 0.f, .35f * A));
    Tile(CX - W * .5f, Y0, W, H, FLinearColor(.03f, .04f, .09f, .82f * A));
    Tile(CX - W * .5f, Y0, W, 10.f * Scale, FLinearColor(WingColor.R, WingColor.G, WingColor.B, A));
    Tile(CX - W * .5f, Y0 + 10.f * Scale, W, 2.f * Scale, FLinearColor(HudGold.R, HudGold.G, HudGold.B, .9f * A));
    Tile(CX - W * .5f, Y0 + H - 2.f * Scale, W, 2.f * Scale, FLinearColor(HudGold.R, HudGold.G, HudGold.B, .9f * A));
    // o nome em letra de desenho animado (pula ao aparecer) e a ala
    CartoonText(FText::FromString(D->DisplayName).ToUpper().ToString(), CX, Y0 + 58.f * Scale, 72.f * Scale * FMath::Max(.3f, Spring), Bright, Ink, A);
    CartoonText(Wing ? Wing->Name : *D->Wing.ToString().ToUpper(), CX, Y0 + 106.f * Scale, 22.f * Scale, FLinearColor(1.f, .96f, .88f), Ink, A);
    // os dois golpes: o título em desenho animado, a explicação em letra comum (lê melhor)
    float Y = Y0 + 130.f * Scale;
    CartoonText(TEXT("CLIQUE ESQUERDO"), CX, Y + 12.f * Scale, 22.f * Scale, FLinearColor(1.f, .85f, .3f), Ink, A);
    Label(D->PrimaryText, CX - W * .47f, Y + 24.f * Scale, W * .94f, 28.f * Scale, 21.f * Scale, FLinearColor(HudCream.R, HudCream.G, HudCream.B, A), 1, false);
    Y += 64.f * Scale;
    CartoonText(FString::Printf(TEXT("BOTÃO DIREITO: %s"), *D->SecondaryName.ToUpper()), CX, Y + 12.f * Scale, 22.f * Scale, FLinearColor(1.f, .85f, .3f), Ink, A);
    Label(D->SecondaryText, CX - W * .47f, Y + 24.f * Scale, W * .94f, 28.f * Scale, 21.f * Scale, FLinearColor(HudCream.R, HudCream.G, HudCream.B, A), 1, false);
    // o que o E faz, pulsando, logo abaixo do cartão
    const FString Action = !D->bPlayable ? TEXT("ESTA ARMA AINDA NÃO CHEGOU AO JOGO") : bHolding ? TEXT("[E]  DEVOLVER AO PEDESTAL") : TEXT("[E]  PEGAR E TESTAR NOS BONECOS");
    CartoonText(Action, CX, Y0 + H + 34.f * Scale, (D->bPlayable ? 36.f * Pulse : 28.f) * Scale, D->bPlayable ? FLinearColor(1.f, .85f, .3f) : FLinearColor(.75f, .72f, .66f), Ink, A);
}

void AVortexAimHUD::DrawWeaponAim(AVortexCharacter* Hero)
{
    UVortexWeaponComponent* Weapon = Hero ? Hero->GetWeapon() : nullptr;
    if (!Weapon || !Canvas) return;
    const float X = Canvas->ClipX * .5f, Y = Canvas->ClipY * .5f;
    auto Arc = [&](float CX, float CY, float Radius, float From, float To, const FLinearColor& Color, float Thick)
    {
        const int32 Steps = FMath::Max(2, int32((To - From) / (2.f * PI) * 48.f));
        for (int32 i = 0; i < Steps; ++i)
        {
            const float A = From + (To - From) * i / Steps, B = From + (To - From) * (i + 1) / Steps;
            DrawLine(CX + FMath::Sin(A) * Radius, CY - FMath::Cos(A) * Radius, CX + FMath::Sin(B) * Radius, CY - FMath::Cos(B) * Radius, Color, Thick);
        }
    };
    // arco: o anel enche enquanto segura a corda; cheio, fica dourado (o tiro mais forte, mais longe e sem cair)
    if (const float Charge = Weapon->DrawCharge(); Charge > 0.f)
    {
        const float R = 20.f * Scale;
        Arc(X, Y, R, 0.f, 2.f * PI, FLinearColor(0.f, 0.f, 0.f, .45f), 4.f);
        const FLinearColor Fill = Charge >= 1.f ? FLinearColor(1.f, .8f, .25f, 1.f) : FLinearColor(.9f, .95f, 1.f, .9f);
        Arc(X, Y, R, 0.f, 2.f * PI * Charge, Fill, Charge >= 1.f ? 3.f : 2.f);
    }
    const double Now = GetWorld()->GetTimeSeconds();
    // cajado, mira travada: cantoneiras vermelhas girando em volta do inimigo
    if (const AActor* Locked = Weapon->GetLockTarget())
    {
        const FVector S = Project(Locked->GetActorLocation() + FVector(0.f, 0.f, 20.f), false);
        if (S.Z > 0.f)
        {
            const float R = 34.f * Scale, Spin = float(Now) * 1.6f;
            for (int32 i = 0; i < 4; ++i)
            {
                const float A = Spin + i * PI * .5f;
                Arc(S.X, S.Y, R, A - .35f, A + .35f, FLinearColor(0.f, 0.f, 0.f, .6f), 5.f);
                Arc(S.X, S.Y, R, A - .35f, A + .35f, FLinearColor(1.f, .25f, .2f, 1.f), 2.5f);
            }
            Label(TEXT("TRAVADO"), S.X - 80.f * Scale, S.Y + R + 4.f * Scale, 160.f * Scale, 18.f * Scale, 13.f * Scale, FLinearColor(1.f, .45f, .4f, 1.f), 1, true);
        }
    }
    // cajado com a cura escolhida: quem vai receber (o aliado da mira, com uma cruz verde; senão "você")
    if (Weapon->UsesLock() && Hero->GetSheet() && Hero->GetSheet()->GetStaffSpell() == TEXT("heal"))
    {
        if (AVortexCharacter* Mate = Weapon->HealTarget(2200.f))
        {
            const FVector S = Project(Mate->GetActorLocation() + FVector(0.f, 0.f, 120.f), false);
            if (S.Z > 0.f)
            {
                const float Pulse = 1.f + .12f * FMath::Sin(float(Now) * 6.f), C = 9.f * Scale * Pulse, T = 4.f * Scale;
                const FLinearColor Green(.35f, 1.f, .45f, 1.f);
                for (const FLinearColor& Color : { FLinearColor(0.f, 0.f, 0.f, .6f), Green })
                {
                    const float Extra = Color == Green ? 0.f : 2.f;
                    DrawLine(S.X - C - Extra, S.Y, S.X + C + Extra, S.Y, Color, T + Extra * 2.f);
                    DrawLine(S.X, S.Y - C - Extra, S.X, S.Y + C + Extra, Color, T + Extra * 2.f);
                }
                const FString Name = Mate->GetPlayerState() ? Mate->GetPlayerState()->GetPlayerName() : FString(TEXT("aliado"));
                Label(TEXT("CURAR ") + Name.ToUpper(), S.X - 120.f * Scale, S.Y + C + 4.f * Scale, 240.f * Scale, 18.f * Scale, 13.f * Scale, Green, 1, true);
            }
        }
        else Label(TEXT("cura: você e o grupo"), X - 120.f * Scale, Y + 26.f * Scale, 240.f * Scale, 16.f * Scale, 12.f * Scale, FLinearColor(.6f, 1.f, .65f, .8f), 1, false);
    }
}

void AVortexAimHUD::DrawCrosshair()
{
    const float X = Canvas->ClipX * .5f, Y = Canvas->ClipY * .5f;
    const FLinearColor Shadow(0.f, 0.f, 0.f, .75f);
    const FLinearColor White(.95f, .98f, 1.f, .9f);
    for (const FLinearColor& Color : { Shadow, White })
    {
        const float Thick = Color == Shadow ? 3.f : 1.f;
        DrawLine(X - 7.f, Y, X - 2.f, Y, Color, Thick);
        DrawLine(X + 2.f, Y, X + 7.f, Y, Color, Thick);
        DrawLine(X, Y - 7.f, X, Y - 2.f, Color, Thick);
        DrawLine(X, Y + 2.f, X, Y + 7.f, Color, Thick);
    }
}

// ---- herói ------------------------------------------------------------------------------------

// ---- minimapa ---------------------------------------------------------------------------------

// O mapa do vale inteiro numa textura (uma célula = um pixel), montado uma vez em outra thread:
// cor do chão com sombra de relevo vinda do noroeste, contorno nos degraus, água pela profundidade
// e copas escuras onde há árvore.
void AVortexAimHUD::RequestMinimap()
{
    if (MapTexture || !Minimap) return;
    if (!bMapRequested)
    {
        bool bTerrain = false;
        if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { bTerrain = true; }
        if (!bTerrain) return;
        const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
        if (!Data) return;
        bMapRequested = true;
        // o mapa do continente inteiro (3 m por pixel) ou, sem ele, o vale célula a célula
        MapBounds = Data->MapBounds();
        MapStep = Data->HasWorld() ? 4 : 1;
        CellsPerChunk = Data->CellsPerChunk;
        const FIntPoint ChunkMin = Data->MapChunkMin(), ChunkCount = Data->MapChunkCount();
        MapSize = FIntPoint(ChunkCount.X * Data->CellsPerChunk / MapStep, ChunkCount.Y * Data->CellsPerChunk / MapStep);
        TSharedRef<int32, ESPMode::ThreadSafe> Land = MakeShared<int32, ESPMode::ThreadSafe>(0);
        MapLand = Land;
        AVortexTown::LoadGrassMask();   // a planta da vila (pintada no mapa), na thread do jogo
        MapJob = Async(EAsyncExecution::ThreadPool, [Data, Size = MapSize, K = MapStep, ChunkMin, ChunkCount, Land]()
        {
            const int32 W = Size.X, H = Size.Y;
            const int32 BaseX = ChunkMin.X * Data->CellsPerChunk, BaseY = ChunkMin.Y * Data->CellsPerChunk;
            TArray<float> Heights;
            TArray<FColor> Ground;
            Heights.SetNumUninitialized(W * H);
            Ground.SetNumUninitialized(W * H);
            // linha por linha em várias threads (o continente tem ~6 milhões de pontos)
            ParallelFor(H, [&](int32 Y)
            {
                for (int32 X = 0; X < W; ++X)
                    Data->SampleCell(BaseX + X * K + K / 2, BaseY + Y * K + K / 2, Heights[Y * W + X], Ground[Y * W + X]);
            });
            // pedaços de terra (o "explorado" conta só a terra)
            const int32 PerChunk = Data->CellsPerChunk / K;
            int32 LandChunks = 0;
            for (int32 Cy = 0; Cy < ChunkCount.Y; ++Cy) for (int32 Cx = 0; Cx < ChunkCount.X; ++Cx)
            {
                const int32 Px = FMath::Min(W - 1, Cx * PerChunk + PerChunk / 2), Py = FMath::Min(H - 1, Cy * PerChunk + PerChunk / 2);
                LandChunks += Heights[Py * W + Px] >= Data->WaterLevel ? 1 : 0;
            }
            *Land = LandChunks;
            auto At = [&](int32 X, int32 Y) { return Heights[FMath::Clamp(Y, 0, H - 1) * W + FMath::Clamp(X, 0, W - 1)]; };
            auto Wet = [&](int32 X, int32 Y) { return At(X, Y) < Data->WaterLevel; };
            // estilo de desenho: a cor do chão suavizada (sem o chuvisco das células) e chapada em poucas
            // faixas de luz, patamares de altitude em degraus com contorno fino, água em dois tons com
            // espuma e contorno na costa
            TArray<FLinearColor> Smooth;
            Smooth.SetNumUninitialized(W * H);
            const int32 Blur = K > 1 ? 2 : 1;
            ParallelFor(H, [&](int32 Y)
            {
                for (int32 X = 0; X < W; ++X)
                {
                    FLinearColor Sum = FLinearColor::Transparent;
                    int32 Count = 0;
                    for (int32 Oy = -Blur; Oy <= Blur; ++Oy) for (int32 Ox = -Blur; Ox <= Blur; ++Ox)
                    {
                        const int32 Sx = FMath::Clamp(X + Ox, 0, W - 1), Sy = FMath::Clamp(Y + Oy, 0, H - 1);
                        if (Heights[Sy * W + Sx] < Data->WaterLevel) continue;
                        Sum += FLinearColor::FromSRGBColor(Ground[Sy * W + Sx]);
                        ++Count;
                    }
                    Smooth[Y * W + X] = Count ? Sum / float(Count) : FLinearColor::FromSRGBColor(Ground[Y * W + X]);
                }
            });
            TArray<FColor> Pixels;
            Pixels.SetNumUninitialized(W * H);
            const FLinearColor Shallow = FLinearColor::FromSRGBColor(FColor(96, 196, 206)), Deep = FLinearColor::FromSRGBColor(FColor(44, 112, 170));
            const FLinearColor Ocean = FLinearColor::FromSRGBColor(FColor(36, 88, 150)), Ink = FLinearColor::FromSRGBColor(FColor(38, 30, 24));
            const float Shade = .004f * K;   // o relevo pesa igual em qualquer escala
            const float Terrace = 2500.f;     // um patamar a cada 25 m de altitude
            ParallelFor(H, [&](int32 Y)
            {
                for (int32 X = 0; X < W; ++X)
                {
                    const float Height = At(X, Y);
                    FLinearColor Color;
                    if (Height <= -299.f) Color = Ocean;
                    else if (Height < Data->WaterLevel)
                    {
                        // dois tons: raso (perto da margem) e fundo; espuma clara colada na terra
                        const bool bShore = !Wet(X - 1, Y) || !Wet(X + 1, Y) || !Wet(X, Y - 1) || !Wet(X, Y + 1);
                        bool bNear = false;
                        for (int32 R = 2; R <= 4 && !bNear; ++R) bNear = !Wet(X - R, Y) || !Wet(X + R, Y) || !Wet(X, Y - R) || !Wet(X, Y + R);
                        Color = bShore ? FLinearColor(.92f, .97f, .96f) : (bNear || Data->WaterSurface - Height < 120.f) ? Shallow : Deep;
                    }
                    else
                    {
                        Color = Smooth[Y * W + X];
                        // luz em três faixas (sem degradê): sombra, meia-luz e luz
                        const float Slope = (At(X + 1, Y + 1) - At(X - 1, Y - 1)) * Shade / K;
                        Color *= Slope < -.12f ? .78f : Slope > .12f ? 1.14f : 1.f;
                        // patamares: cada 25 m um pouco mais claro, com um fio escuro na borda
                        const int32 Band = FMath::FloorToInt(Height / Terrace);
                        Color = FMath::Lerp(Color, FLinearColor(1.f, .97f, .88f), FMath::Clamp(Band * .035f, 0.f, .25f));
                        if (Band != FMath::FloorToInt(At(X + 1, Y) / Terrace) || Band != FMath::FloorToInt(At(X, Y + 1) / Terrace)) Color *= .82f;
                        // contorno da costa (tinta escura do lado da terra)
                        if (Wet(X - 1, Y) || Wet(X + 1, Y) || Wet(X, Y - 1) || Wet(X, Y + 1)) Color = FMath::Lerp(Color, Ink, .65f);
                    }
                    Pixels[Y * W + X] = Color.ToFColor(true);
                }
            });
            // árvores do vale exportado: copa redonda escura (no continente, um pixel mais escuro)
            for (int32 Cy = 0; Cy < Data->ChunkCountY; ++Cy)
            for (int32 Cx = 0; Cx < Data->ChunkCountX; ++Cx)
            {
                const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(FIntPoint(Data->ChunkMinX + Cx, Data->ChunkMinY + Cy));
                if (!Props) continue;
                for (const FVortexTerrainProp& Prop : *Props)
                {
                    if (!Data->IsTree(Prop.Kind)) continue;
                    const int32 PX = (FMath::FloorToInt(Prop.Location.X / Data->CellSize) - BaseX) / K;
                    const int32 PY = (FMath::FloorToInt(Prop.Location.Y / Data->CellSize) - BaseY) / K;
                    const int32 Reach = K > 1 ? 0 : 2;
                    for (int32 Oy = -Reach; Oy <= Reach; ++Oy)
                    for (int32 Ox = -Reach; Ox <= Reach; ++Ox)
                    {
                        if (Ox * Ox + Oy * Oy > 5) continue;
                        const int32 X = PX + Ox, Y = PY + Oy;
                        if (X < 0 || Y < 0 || X >= W || Y >= H) continue;
                        FLinearColor C = FLinearColor::FromSRGBColor(Pixels[Y * W + X]);
                        const FLinearColor Leaf = (Ox * Ox + Oy * Oy) <= 1 ? FLinearColor(.035f, .11f, .03f) : FLinearColor(.015f, .05f, .015f);
                        Pixels[Y * W + X] = FMath::Lerp(C, Leaf, K > 1 ? .35f : .7f).ToFColor(true);
                    }
                }
            }
            // 02/11: a Cidade do Santuário (o chão de pedra e os telhados), como no minimapa
            for (int32 Y = 0; Y < H; ++Y)
                for (int32 X = 0; X < W; ++X)
                {
                    const int32 Kind = AVortexTown::MapKindAt((BaseX + X * K + K * .5f) * Data->CellSize, (BaseY + Y * K + K * .5f) * Data->CellSize);
                    if (Kind == 1) Pixels[Y * W + X] = FColor(206, 192, 162);
                    else if (Kind == 2) Pixels[Y * W + X] = FColor(150, 74, 52);
                }
            return Pixels;
        });
        return;
    }
    if (!MapJob.IsReady()) return;
    TArray<FColor> Pixels = MapJob.Consume();
    MapBase = MakeShared<const TArray<FColor>>(Pixels);
    UTexture2D* Texture = UTexture2D::CreateTransient(MapSize.X, MapSize.Y, PF_B8G8R8A8);
    if (!Texture) return;
    Texture->SRGB = true;
    Texture->Filter = TF_Nearest;   // blocos nítidos, como o mundo
    Texture->AddressX = Texture->AddressY = TA_Clamp;
    Texture->LODGroup = TEXTUREGROUP_UI;
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    MapTexture = Texture;
    Minimap->SetTextureParameterValue(TEXT("Map"), MapTexture);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_HUD_MINIMAP %dx%d"), MapSize.X, MapSize.Y);
}

// ---- alvo -------------------------------------------------------------------------------------

AActor* AVortexAimHUD::FindCombatCreature(const AVortexCharacter* Hero) const
{
    const FVector Here = Hero->GetActorLocation();
    AActor* Best = nullptr;
    float BestDistance = FMath::Square(3000.f);
    for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
        if (!Creature || Creature->IsCreatureDead() || !Creature->IsEngaged() || Creature->IsPassive()) continue;
        // chefe acordado vale de mais longe (a arena é grande)
        const float Distance = FVector::DistSquared(It->GetActorLocation(), Here) * (Creature->IsBoss() ? .2f : 1.f);
        if (Distance < BestDistance) { Best = *It; BestDistance = Distance; }
    }
    return Best;
}

// ---- barra de ações ---------------------------------------------------------------------------

// ---- números de dano --------------------------------------------------------------------------

void AVortexAimHUD::OnHeroHit(AActor* InstigatorActor, AActor* TargetActor, float Damage)
{
    if (!TargetActor || Damage <= 0.f) return;
    const UVortexCombatComponent* Combat = InstigatorActor ? InstigatorActor->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    const bool bCrit = Combat && Combat->LastCritTarget.Get() == TargetActor && GetWorld()->GetTimeSeconds() - Combat->LastCritTime < .1f;
    DamageNumbers.Add({ FaceOf(TargetActor, 40.f, 40.f) + FVector(0, 0, 50.f), Damage, float(GetWorld()->GetTimeSeconds()), false, bCrit, FMath::FRandRange(-1.f, 1.f) });
    if (const IVortexCreature* Creature = Cast<IVortexCreature>(TargetActor); Creature && !Creature->IsPassive())
    {
        if (TargetCreature.Get() != TargetActor) TargetHealthShown = 1.f, WolfCaptureTime = 0.f, bTargetPortraitReady = false;
        TargetCreature = TargetActor;
        TargetSeenTime = GetWorld()->GetTimeSeconds();
    }
}

namespace
{
    // a letra de desenho animado (Content/Vortex/UI/Fonts, licença Apache 2.0), lida direto do .ttf como a Cinzel
    // do menu; sem o arquivo, cai na letra comum em negrito
    FSlateFontInfo CartoonFont(float Size)
    {
        static TSharedPtr<const FCompositeFont> Cartoon = []()
        {
            const FString Path = FPaths::ProjectContentDir() / TEXT("Vortex/UI/Fonts/Cinzel-900.ttf");   // 07/11: a letra medieval (combina com as habilidades)
            return FPaths::FileExists(Path) ? TSharedPtr<const FCompositeFont>(MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad))
                : TSharedPtr<const FCompositeFont>();
        }();
        const int32 Points = FMath::Max(8, FMath::RoundToInt(Size * .63f));
        return Cartoon.IsValid() ? FSlateFontInfo(Cartoon, Points) : FCoreStyle::GetDefaultFontStyle("Bold", Points);
    }
}

void AVortexAimHUD::CartoonText(const FString& Text, float CX, float CY, float Size, const FLinearColor& Fill, const FLinearColor& Outline, float Alpha)
{
    if (Text.IsEmpty() || Alpha <= .01f) return;
    FSlateFontInfo Font = CartoonFont(Size);
    // contorno grosso e escuro, como nos números de dano dos jogos de desenho animado
    Font.OutlineSettings.OutlineSize = FMath::Max(2, FMath::RoundToInt(Size / 8.f));
    Font.OutlineSettings.OutlineColor = FLinearColor(Outline.R, Outline.G, Outline.B, Alpha);
    const FVector2D Extent = Measure(Text, Font);
    FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(CX - Extent.X * .5f), FMath::RoundToFloat(CY - Extent.Y * .5f)), FText::FromString(Text), Font,
        FLinearColor(Fill.R, Fill.G, Fill.B, Alpha));
    if (!TextFont) TextFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    Item.Font = TextFont;
    Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, .55f * Alpha), FVector2D(Size / 16.f, Size / 10.f));
    Canvas->DrawItem(Item);
}

void AVortexAimHUD::DrawPlaceBanner(AVortexCharacter* Hero, float DeltaSeconds)
{
    const float Now = GetWorld()->GetTimeSeconds();
    // de meio em meio segundo: onde o herói está (cidade, lugar marcado ou região)
    if (Now >= PlaceCheckAt)
    {
        PlaceCheckAt = Now + .5f;
        const FVector At = Hero->GetActorLocation();
        const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
        FString Title, Sub;
        FLinearColor Tint(1.f, .84f, .36f);
        if (AVortexTown::Contains(At)) { Title = TEXT("Cidade do Santuário"); Sub = TEXT("cidade"); }
        for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
        {
            if (!Title.IsEmpty()) break;
            if (FVector2D::Distance(FVector2D(At), Site.Center) < Site.Radius)
            {
                Title = Site.Name;
                Sub = Site.Name.StartsWith(TEXT("Vila")) || Site.Name.StartsWith(TEXT("Acampamento")) ? TEXT("vila") : TEXT("lugar lendário");
                Tint = FLinearColor(.75f, 1.f, .55f);
            }
        }
        if (Title.IsEmpty() && Data)
        {
            const int32 Region = Data->HasWorld() ? Data->RegionAt(At.X, At.Y) : 1;
            Title = Data->HasWorld() ? Data->RegionName(Region) : FString(TEXT("Vale do Santuário"));
            Sub = FString::Printf(TEXT("região · criaturas nível %d"), Data->AreaLevel(At.X, At.Y));
            // a cor do lugar: 1 Valdora, 2 Brasafenda, 3 Raikor, 4 Nivaluz, 5 Solcânion, 6 Aurivale, 7 Lodonegro, 8 Morthúria, 9 Verdalume
            static const FLinearColor Tints[10] = { { .5f, .8f, 1.f }, { .62f, 1.f, .45f }, { 1.f, .45f, .25f }, { .8f, .85f, .95f }, { .65f, .9f, 1.f },
                { 1.f, .66f, .3f }, { 1.f, .84f, .3f }, { .6f, .8f, .45f }, { .8f, .55f, 1.f }, { .45f, 1.f, .7f } };
            Tint = Tints[FMath::Clamp(Region, 0, 9)];
            if (Region == 0) Sub = TEXT("mar aberto");
        }
        // na divisa o nome não pisca: o lugar novo precisa durar 1 s, e o mesmo nome não volta antes de 40 s
        if (Title != PlacePending) { PlacePending = Title; PlacePendingSince = Now; }
        if (!Title.IsEmpty() && Title != PlaceShown && Now - PlacePendingSince >= 1.f)
        {
            PlaceShown = Title;
            const float* Last = PlaceBannerTimes.Find(Title);
            if (!Last || Now - *Last > 40.f)
            {
                PlaceBannerTimes.Add(Title, Now);
                BannerTitle = Title;
                BannerSub = Sub;
                BannerTint = Tint;
                BannerAt = Now;
                VortexAudio::Play2D(Hero, TEXT("ui_open"), .45f);
            }
        }
    }
    const float Age = Now - BannerAt;
    constexpr float Life = 4.6f;
    if (Age < 0.f || Age > Life || BannerTitle.IsEmpty()) return;
    const float Out = FMath::Clamp((Life - Age) / .7f, 0.f, 1.f);   // some no fim, subindo um pouco
    const float Lift = (1.f - Out) * 30.f * Scale;
    // no alto, logo abaixo da bússola: mais embaixo ele cobria o nome e a vida dos monstros à frente
    const float CX = Canvas->ClipX * .5f, CY = Canvas->ClipY * .13f - Lift;
    const float Size = 66.f * Scale;
    const FLinearColor Outline(.16f, .07f, .02f);
    // faixa escura macia atrás (abre do meio para os lados)
    const float Open = FMath::Clamp(Age / .35f, 0.f, 1.f);
    const float BandW = Canvas->ClipX * .55f * (1.f - FMath::Pow(1.f - Open, 3.f));
    for (int32 Step = 0; Step < 8; ++Step)
    {
        const float F = 1.f - Step / 8.f;
        FCanvasTileItem Band(FVector2D(CX - BandW * F * .5f, CY - 62.f * Scale), FVector2D(BandW * F, 134.f * Scale), FLinearColor(0.f, 0.f, .02f, .07f * Out));
        Band.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Band);
    }
    // as letras caem uma a uma e quicam (passam do lugar e voltam), crescendo de pequenas
    const FString Title = BannerTitle.ToUpper();
    const FSlateFontInfo Font = CartoonFont(Size);
    TArray<float> Widths;
    float Total = 0.f;
    for (int32 i = 0; i < Title.Len(); ++i) { Widths.Add(Measure(FString::Chr(Title[i]), Font).X * .97f); Total += Widths.Last(); }
    float X = CX - Total * .5f;
    for (int32 i = 0; i < Title.Len(); ++i)
    {
        const float T = FMath::Clamp((Age - .12f - i * .045f) / .55f, 0.f, 1.f);
        if (T > 0.f && Title[i] != TEXT(' '))
        {
            // mola amortecida: cai de cima, passa do ponto, volta
            const float Spring = 1.f - FMath::Exp(-6.f * T) * FMath::Cos(T * 13.f);
            const float Drop = (1.f - Spring) * -70.f * Scale;
            const float Pop = FMath::Lerp(.3f, 1.f, FMath::Clamp(Spring, 0.f, 1.25f));
            // depois de pousar, a palavra balança devagar como bandeirinha
            const float Wave = FMath::Sin(Now * 3.2f - i * .55f) * 4.f * Scale * FMath::Clamp((Age - 1.f) / .5f, 0.f, 1.f);
            const FLinearColor Fill = FMath::Lerp(FLinearColor::White, BannerTint, FMath::Clamp(T * 2.f, 0.f, 1.f));
            CartoonText(FString::Chr(Title[i]), X + Widths[i] * .5f, CY + Drop + Wave, Size * Pop, Fill, Outline, Out * FMath::Clamp(T * 4.f, 0.f, 1.f));
        }
        X += Widths[i];
    }
    // o tipo do lugar embaixo, subindo devagar
    const float SubT = FMath::Clamp((Age - .5f - Title.Len() * .045f) / .4f, 0.f, 1.f);
    CartoonText(BannerSub.ToUpper(), CX, CY + 58.f * Scale + (1.f - SubT) * 16.f * Scale, 26.f * Scale, FLinearColor(1.f, .97f, .88f), Outline, Out * SubT);
    // estrelinhas que estouram dos lados quando a palavra pousa
    const float Burst = FMath::Clamp((Age - .55f) / .9f, 0.f, 1.f);
    if (Burst > 0.f && Burst < 1.f)
    {
        for (int32 s = 0; s < 12; ++s)
        {
            const float Angle = s * 30.f + 15.f;
            const FVector2D Dir = FVector2D(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)) * .55f);
            const float Side = Dir.X >= 0.f ? 1.f : -1.f;
            const FVector2D From(CX + Side * (Total * .5f + 10.f * Scale), CY);
            const FVector2D P = From + Dir * (40.f + 90.f * Burst) * Scale;
            const float R = (6.f + (s % 3) * 2.f) * Scale * (1.f - Burst);
            Disc(P.X, P.Y, R, FLinearColor(1.f, 1.f, .9f, (1.f - Burst) * Out), FLinearColor(BannerTint.R, BannerTint.G, BannerTint.B, (1.f - Burst) * Out));
        }
    }
}

void AVortexAimHUD::DrawDamage()
{
    const float Now = GetWorld()->GetTimeSeconds();
    DamageNumbers.RemoveAll([Now](const FDamageNumber& Number) { return Now - Number.Born > 1.1f; });
    for (const FDamageNumber& Number : DamageNumbers)
    {
        const float T = Now - Number.Born;
        const float Age = T / 1.1f;
        const FVector Screen = Project(Number.Location, false);
        if (Screen.Z <= 0.f) continue;
        // estilo desenho animado: o número "estoura" (cresce passando do tamanho e volta, 0,22 s), pula para
        // o lado num arco e some no fim; o crítico é bem maior, vermelho, treme ao nascer e vem numa explosão
        // amarela (estrela de pontas) com "CRÍTICO!" em cima
        const float Grow = FMath::Clamp(T / .22f, 0.f, 1.f);
        const float Pop = Number.bOnHero ? FMath::Lerp(.7f, 1.f, Grow) : FMath::Sin(Grow * UE_PI * .5f) * (1.f + .45f * FMath::Sin(Grow * UE_PI));
        const float Base = Number.bOnHero ? 52.f : Number.bCrit ? 136.f : 80.f;
        const float Size = Base * Scale * FMath::Max(.05f, Pop);
        const float Side = Number.Drift * 55.f * Scale * FMath::Min(1.f, Age * 1.6f);
        const float Rise = (90.f * Age - 40.f * Age * Age) * Scale;
        const float Shake = Number.bCrit && T < .3f ? FMath::Sin(T * 90.f) * 7.f * Scale * (1.f - T / .3f) : 0.f;
        const float Alpha = Age < .7f ? 1.f : 1.f - (Age - .7f) / .3f;
        // crítico: vermelho vivo que pisca em branco no nascer
        const FLinearColor CritRed = FMath::Lerp(FLinearColor(1.f, 1.f, .9f), FLinearColor(1.f, .1f, .06f), FMath::Clamp(T / .12f, 0.f, 1.f));
        const FLinearColor Fill = Number.bOnHero ? FLinearColor(1.f, .22f, .18f) : Number.bCrit ? CritRed : FLinearColor(1.f, .86f, .16f);
        const FLinearColor Outline = Number.bOnHero ? FLinearColor(.18f, .02f, .02f) : Number.bCrit ? FLinearColor(.22f, .0f, .02f) : FLinearColor(.24f, .1f, .02f);
        const FString Text = (Number.bOnHero ? TEXT("-") : TEXT("")) + FString::FromInt(FMath::RoundToInt(Number.Damage)) + (Number.bCrit ? TEXT("!") : TEXT(""));
        const float X = Screen.X + Side + Shake, Y = Screen.Y - Rise - Size * .6f;
        if (Number.bCrit)
        {
            // a explosão atrás do número: estrela de 12 pontas que estoura, gira devagar e some antes do número
            const float BurstA = Alpha * FMath::Clamp(1.f - (Age - .45f) / .35f, 0.f, 1.f);
            const float Spin = T * 40.f + Number.Drift * 30.f;
            auto Burst = [&](float Outer, float Inner, const FLinearColor& Color)
            {
                constexpr int32 Points = 12;
                const FVector2D C(X, Y + Size * .05f);
                for (int32 k = 0; k < Points * 2; ++k)
                {
                    const float A0 = FMath::DegreesToRadians(Spin + k * 180.f / Points), A1 = FMath::DegreesToRadians(Spin + (k + 1) * 180.f / Points);
                    const float R0 = k % 2 ? Inner : Outer, R1 = k % 2 ? Outer : Inner;
                    FCanvasTriangleItem Tri(C, C + FVector2D(FMath::Cos(A0), FMath::Sin(A0) * .8f) * R0, C + FVector2D(FMath::Cos(A1), FMath::Sin(A1) * .8f) * R1, GWhiteTexture);
                    Tri.SetColor(Color);
                    Tri.BlendMode = SE_BLEND_Translucent;
                    Canvas->DrawItem(Tri);
                }
            };
            if (BurstA > .01f)
            {
                Burst(Size * 1.02f, Size * .62f, FLinearColor(.25f, .02f, 0.f, .85f * BurstA));
                Burst(Size * .92f, Size * .55f, FLinearColor(1.f, .62f, .08f, BurstA));
                Burst(Size * .72f, Size * .45f, FLinearColor(1.f, .9f, .3f, BurstA));
            }
        }
        CartoonText(Text, X, Y, Size, Fill, Outline, Alpha);
        if (Number.bCrit) CartoonText(TEXT("CRÍTICO!"), X, Y - Size * .8f, Size * .4f, FLinearColor(1.f, .9f, .2f), Outline, Alpha);
    }
}

void AVortexAimHUD::DrawFocus(AVortexCharacter* Hero, float DeltaSeconds)
{
    // sem marca na tela (pedido em 30/10: a marca de alvo saiu); fica só a troca de foco: com arma de perto,
    // uma virada rápida e forte do mouse (~28° em ~0,1 s) passa o foco para o inimigo que ficou na frente
    const bool bWant = Hero->IsMeleeWeapon() && !Hero->IsFirstPerson() && !Hero->IsAimHeld();
    const float Now = GetWorld()->GetTimeSeconds();
    const float Yaw = PlayerOwner ? PlayerOwner->GetControlRotation().Yaw : 0.f;
    const float Turn = FMath::FindDeltaAngleDegrees(FlickLastYaw, Yaw);
    FlickLastYaw = Yaw;
    FlickYaw = FlickYaw * FMath::Exp(-DeltaSeconds * 10.f) + Turn;
    if (bWant && FMath::Abs(FlickYaw) > 28.f && Now >= FlickReadyAt)
    {
        FlickYaw = 0.f;
        FlickReadyAt = Now + .35f;
        Hero->FlickFocus();
    }
}

// ---- retratos ---------------------------------------------------------------------------------

void AVortexAimHUD::CaptureFace(AActor* Actor, UTextureRenderTarget2D* Target, float Distance, float FieldOfView)
{
    if (!Capture || !Actor || !Target) return;
    const bool bWolf = Actor->IsA<AVortexWolf>();
    FVector Face = FaceOf(Actor, bWolf ? 55.f : 0.f, bWolf ? 30.f : 52.f);
    // criaturas do Godot: o alto do corpo, um pouco à frente (cada espécie tem um tamanho)
    if (const AVortexMonster* Monster = Cast<AVortexMonster>(Actor))
    {
        const float Half = Monster->GetSimpleCollisionHalfHeight(), Radius = Monster->GetSimpleCollisionRadius();
        Face = Monster->GetActorLocation() + FVector(0.f, 0.f, Half * .45f) + Actor->GetActorForwardVector() * Radius * .6f;
        Distance = FMath::Max(Distance, Half * 2.6f + Radius * 1.5f);
    }
    // o osso da cabeça do herói fica no pescoço; o do lobo, atrás dos olhos
    if (const ACharacter* Character = Cast<ACharacter>(Actor))
    {
        const USkeletalMeshComponent* Mesh = Character->GetMesh();
        if (bWolf && Mesh && Mesh->GetBoneIndex(TEXT("Eyes")) != INDEX_NONE) Face = Mesh->GetBoneLocation(TEXT("Eyes"));
        else if (!bWolf) Face.Z += 16.f;
    }
    // de frente e um pouco de lado, como o retrato do Godot
    const FVector Forward = Actor->GetActorForwardVector().RotateAngleAxis(-18.f, FVector::UpVector);
    const FVector Eye = Face + Forward * Distance + FVector(0, 0, Distance * .05f);
    Capture->SetWorldLocationAndRotation(Eye, (Face - Eye).Rotation());
    Capture->FOVAngle = FieldOfView;
    Capture->TextureTarget = Target;
    Capture->ShowOnlyActors.Reset();
    Capture->ShowOnlyActors.Add(Actor);
    TArray<AActor*> Attached;
    Actor->GetAttachedActors(Attached, true, true);
    for (AActor* Child : Attached) Capture->ShowOnlyActors.Add(Child);
    Capture->CaptureScene();
}

void AVortexAimHUD::UpdatePortraits(AVortexCharacter* Hero)
{
    const float Now = GetWorld()->GetTimeSeconds();
    // o retrato do herói é uma foto parada do rosto (tirada na criação); herói antigo sem foto: tira
    // uma vez aqui, com ele parado no chão e fora da primeira pessoa
    if (HeroPortrait && !bHeroPortraitFixed && Now >= HeroCaptureTime)
    {
        HeroCaptureTime = Now + .5f;
        const UVortexGameInstance* Game = Cast<UVortexGameInstance>(GetGameInstance());
        const UVortexPlayerSave* Saved = Game ? Game->GetSelectedHero() : nullptr;
        const FGuid Id = Saved ? Saved->Identity.PlayerId : FGuid();
        UTexture2D* Photo = VortexPortrait::Load(Id);
        if (!Photo && Id.IsValid() && Now > 3.f && !Hero->IsFirstPerson() && Hero->GetVelocity().Size2D() < 5.f && Hero->GetCharacterMovement()->IsMovingOnGround())
        {
            if (VortexPortrait::CaptureAndSave(Hero, Id)) Photo = VortexPortrait::Load(Id);
        }
        if (Photo)
        {
            HeroPortrait->SetTextureParameterValue(TEXT("Portrait"), Photo);
            HeroPortrait->SetScalarParameterValue(TEXT("Ready"), 1.f);
            bHeroPortraitFixed = true;
        }
        else if (!Id.IsValid() && !Hero->IsFirstPerson())
        {
            // herói sem save (testes): o retrato ao vivo de antes
            CaptureFace(Hero, HeroTarget, 150.f, 28.f);
            HeroPortrait->SetTextureParameterValue(TEXT("Portrait"), HeroTarget);
            HeroPortrait->SetScalarParameterValue(TEXT("Ready"), 1.f);
        }
    }
    AActor* Wolf = TargetCreature.Get();
    if (TargetPortrait && Wolf && TargetAlpha > 0.f && !bTargetPortraitReady && Now >= WolfCaptureTime)
    {
        WolfCaptureTime = Now + .4f;
        CaptureFace(Wolf, WolfTarget, 170.f, 30.f);
        TargetPortrait->SetTextureParameterValue(TEXT("Portrait"), WolfTarget);
        TargetPortrait->SetScalarParameterValue(TEXT("Ready"), 1.f);
        bTargetPortraitReady = true;
    }
}

// ---- avisos -----------------------------------------------------------------------------------

