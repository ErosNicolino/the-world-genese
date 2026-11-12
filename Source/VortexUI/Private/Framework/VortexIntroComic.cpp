#include "Framework/VortexIntroComic.h"
#include "VortexUiImpl.h"

#include "Components/AudioComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexUiKit.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Styling/CoreStyle.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

namespace
{
    float Smooth(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
    float EaseOut(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 3.f); }

    // a cor do papel das páginas (as sarjetas): cobre o quadro antes de ele "pintar"
    const FLinearColor Ink(.1f, .06f, .035f);

    // Brush: a cena pintada do verso (Data/UI/Abertura/Cinema/Cxx) ou, sem ela, só este quadro da folha; BrushB: a 2ª
    // cena dos versos de 4 linhas (C06b...), que entra quando a 3ª linha começa a ser cantada
    struct FPanel { FBox2f Rect; FString Text; FName Fx; FName Voice; float At = -1.f; TArray<FVector2f> Sung; TSharedPtr<FSlateBrush> Brush, BrushB; };
    struct FPage { TStrongObjectPtr<UTexture2D> Texture; FVector2f Size = FVector2f(1672.f, 941.f); TArray<FPanel> Panels; };

    // um passo da HQ: a página chega, a câmera vai a um quadro, a página vira, ou o fim (o "Acorde")
    enum class EBeat : uint8 { PageIn, Focus, PageOut, End, Dark };
    struct FBeat { EBeat Kind; int32 Page = 0, Panel = 0; float Duration = 1.f; float Hold = 0.f; };

    constexpr float MoveTime = .9f;     // a câmera de um quadro ao outro
    constexpr float PageInTime = 1.9f;  // a página chega e os quadros pintam
    constexpr float PageOutTime = 1.15f;
    constexpr float EndTime = 3.2f;
    constexpr float NextPageInTime = 2.4f;   // com música: a folha seguinte pinta mais rápido (o tempo sai do quadro anterior)

    class SVortexIntroComic : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexIntroComic) {}
            SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Controller = Args._Controller;
            SetCanTick(true);
            White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
            Load();
            Build();
            if (APlayerController* PC = Controller.Get())
            {
                // a canção da abertura (os quadros seguem o tempo dela); sem ela, o tema do jogo baixinho
                if (Song.IsValid()) Music = UGameplayStatics::CreateSound2D(PC, Song.Get(), .95f, 1.f, 0.f, nullptr, false, false);
                else if (USoundBase* Theme = LoadObject<USoundBase>(nullptr, TEXT("/Game/Vortex/Audio/Music/MUS_Theme.MUS_Theme")))
                    Music = UGameplayStatics::CreateSound2D(PC, Theme, .3f, 1.f, 0.f, nullptr, false, false);
                SongStartClock = FPlatformTime::Seconds();
                if (Music.IsValid()) { if (Song.IsValid()) Music->Play(0.f); else Music->FadeIn(2.5f, .3f); }
            }
            RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SVortexIntroComic::Animate));
        }

        virtual ~SVortexIntroComic() override { StopAudio(); }

        bool IsValidComic() const { return Beats.Num() > 0; }
        bool IsFinished() const { return bFinished; }

        void StopAudio()
        {
            if (Voice.IsValid()) Voice->Stop();
            Voice.Reset();
            if (Music.IsValid()) Music->FadeOut(1.2f, 0.f);
            Music.Reset();
        }

        void Jump(int32 Page, int32 Panel)
        {
            // quadro 0..N-1: a câmera chegando nele; -1: a página virando; -2: o fim; -3: a página chegando
            for (int32 i = 0; i < Beats.Num(); ++i)
            {
                const FBeat& B = Beats[i];
                const bool bHit = Panel >= 0 ? B.Kind == EBeat::Focus && B.Page == Page && B.Panel == Panel
                    : Panel == -1 ? B.Kind == EBeat::PageOut && B.Page == Page : Panel == -3 ? B.Kind == EBeat::PageIn && B.Page == Page : B.Kind == EBeat::End;
                if (bHit) { Seek(i, Panel >= 0 ? MoveTime - .01f : 0.f); if (Panel >= 0) Time = MoveTime - .01f; return; }
            }
        }

        FString State() const
        {
            const FBeat& B = Beats[FMath::Clamp(Beat, 0, Beats.Num() - 1)];
            static const TCHAR* Kinds[] = { TEXT("chega"), TEXT("quadro"), TEXT("vira"), TEXT("fim"), TEXT("escuro") };
            return FString::Printf(TEXT("%s pagina=%d quadro=%d t=%.1f/%.1f"), Kinds[int32(B.Kind)], B.Page + 1, B.Panel + 1, Time, B.Duration);
        }

        bool Sync(float& Image, float& SongNow) const
        {
            Image = SongTime();
            SongNow = float(SongClock(FPlatformTime::Seconds()));
            return Song.IsValid() && Music.IsValid() && Music->IsPlaying();
        }

        virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }
        virtual bool SupportsKeyboardFocus() const override { return true; }

        // nenhuma tecla mexe na HQ: o herói novo chega ao mundo apertando espaço e clicando, e cada toque passava um
        // quadro (a HQ inteira corria em segundos e ficava marcada como vista). Só o botão PULAR sai dela.
        virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }

        virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            Mouse = FVector2f(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
            return FReply::Handled();
        }

        virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            Mouse = FVector2f(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
            if (SkipBox.ContainsPoint(Mouse)) Skip();
            return FReply::Handled();
        }

        virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer,
            const FWidgetStyle&, bool) const override
        {
            const FVector2f Screen = FVector2f(G.GetLocalSize());
            const float S = Screen.Y / 1080.f;
            const FBeat& B = Beats[Beat];
            const float Fade = FadeOutAlpha();
            // modo cinema: cada quadro ocupa a tela, com a câmera andando devagar por ele; o verso inteiro aparece
            // em baixo, entre faixas pretas (a legenda palavra por palavra saía fora do tempo do canto)
            Box(Out, Layer, G, FVector2f(0, 0), Screen, White, FLinearColor(0.f, 0.f, 0.f, 1.f));
            const float Letterbox = Screen.Y * .085f;

            if (B.Kind == EBeat::Dark)
            {
                // a introdução da canção: o título nasce do escuro, com um fio de luz respirando
                // o Vórtice sobre o mar (C00) nasce do preto atrás do título e se apaga antes do 1º verso
                if (TitleBrush.IsValid())
                    DrawScene(Out, Layer + 1, G, Screen, TitleBrush.Get(), Time / FMath::Max(1.f, B.Duration),
                        .55f * Smooth(Time / 3.f) * Smooth((B.Duration - Time) / 1.5f), FVector2f(0, 0), .7f);
                const float Breath = .05f + .04f * FMath::Sin(Time * 1.3f);
                Box(Out, Layer + 1, G, FVector2f(Screen.X * .3f, Screen.Y * .56f), FVector2f(Screen.X * .4f, 2.f * S), White, FLinearColor(.4f, 1.f, .75f, Breath));
                const float Title = Smooth((Time - 1.5f) / 2.f) * Smooth((B.Duration - Time - .4f) / 1.2f);
                if (Title > 0.f)
                {
                    CinemaText(Out, Layer + 2, G, TEXT("THE WORLD GÊNESES"), FVector2f(Screen.X * .5f, Screen.Y * .47f), 64.f * S, FLinearColor(1.f, .9f, .66f, Title), true);
                    CinemaText(Out, Layer + 2, G, TEXT("A LENDA DE GÊNESE"), FVector2f(Screen.X * .5f, Screen.Y * .53f), 22.f * S, FLinearColor(.6f, 1.f, .8f, .8f * Title), false);
                }
                DrawMotes(Out, Layer + 3, G, Screen, S, .6f);
                DrawSkip(Out, Layer + 4, G, Screen, S, 1.f);
                return Layer + 5;
            }

            const int32 Global = GlobalIndex(B.Page, B.Panel);
            FVector2f Shake(0.f, 0.f);
            if (B.Kind == EBeat::Focus)
            {
                const FPanel& P = Pages[B.Page].Panels[B.Panel];
                const float Hold = Time - MoveTime;
                const float Strength = P.Fx == TEXT("impact") ? 22.f : P.Fx == TEXT("shake") ? 11.f : 0.f;
                const float Decay = FMath::Max(0.f, 1.f - Hold / .9f);
                if (Strength > 0.f && Hold > 0.f)
                    Shake = FVector2f(FMath::Sin(Time * 61.f), FMath::Cos(Time * 47.f)) * Strength * Decay * Decay * S;
                // o quadro anterior fica por baixo enquanto este aparece (dissolve)
                if (Time < MoveTime && B.Panel > 0) DrawCinemaPanel(Out, Layer + 1, G, Screen, B.Page, B.Panel - 1, 1.f, 1.f, FVector2f(0, 0), 1.f);
                // a 2ª cena do verso entra junto com a 3ª linha cantada (dissolve de 0,8 s)
                const float Second = P.BrushB.IsValid() ? Smooth((SongTime() - VerseSwitch(P, BeatStart[Beat], BeatStart[Beat] + B.Duration)) / .8f) : 0.f;
                DrawCinemaPanel(Out, Layer + 2, G, Screen, B.Page, B.Panel, Time / FMath::Max(1.f, B.Duration), Smooth(Time / MoveTime), Shake, Second);
                if (P.Fx == TEXT("impact") && Hold > 0.f)
                    Box(Out, Layer + 3, G, FVector2f(0, 0), Screen, White, FLinearColor(1.f, .93f, .75f, FMath::Max(0.f, 1.f - Hold / .6f) * .9f));
                if (P.Fx == TEXT("glow"))
                    Box(Out, Layer + 3, G, FVector2f(0, 0), Screen, White, FLinearColor(.35f, 1.f, .7f, .05f + .04f * FMath::Sin(Time * 2.2f)));
                if (P.Fx == TEXT("spin"))
                    Box(Out, Layer + 3, G, FVector2f(0, 0), Screen, White, FLinearColor(.45f, .2f, .9f, .05f + .04f * FMath::Sin(Time * 1.7f)));
            }
            else if (B.Kind == EBeat::PageOut)
                // o último quadro da folha apaga para o preto
                DrawCinemaPanel(Out, Layer + 2, G, Screen, B.Page, B.Panel, 1.f, 1.f - Smooth(Time / FMath::Max(.3f, B.Duration * .8f)), Shake, 1.f);
            else if (B.Kind == EBeat::End)
                DrawCinemaPanel(Out, Layer + 2, G, Screen, B.Page, B.Panel, 1.f, 1.f, Shake, 1.f);
            // PageIn: o escuro entre as folhas (o primeiro quadro da próxima nasce dele)

            // vinheta e as faixas de cinema
            // muitas camadas finas e fracas: com poucas, os degraus da vinheta apareciam
            for (int32 k = 1; k <= 16; ++k)
            {
                const float In = Screen.Y * .012f * k;
                const FLinearColor Shade(0.f, 0.f, 0.f, .022f);
                Box(Out, Layer + 4, G, FVector2f(0, 0), FVector2f(In, Screen.Y), White, Shade);
                Box(Out, Layer + 4, G, FVector2f(Screen.X - In, 0), FVector2f(In, Screen.Y), White, Shade);
            }
            Box(Out, Layer + 5, G, FVector2f(0, 0), FVector2f(Screen.X, Letterbox), White, FLinearColor(0.f, 0.f, 0.f, 1.f));
            Box(Out, Layer + 5, G, FVector2f(0, Screen.Y - Letterbox), FVector2f(Screen.X, Letterbox), White, FLinearColor(0.f, 0.f, 0.f, 1.f));
            for (int32 k = 1; k <= 24; ++k)   // a imagem escurece um pouco em cima da faixa de baixo, para o verso ler bem
                Box(Out, Layer + 5, G, FVector2f(0, Screen.Y - Letterbox - 10.f * k * S), FVector2f(Screen.X, 10.f * k * S), White, FLinearColor(0.f, 0.f, 0.f, .022f));
            DrawMotes(Out, Layer + 6, G, Screen, S, 1.f);

            if (B.Kind == EBeat::Focus) DrawVerse(Out, Layer + 7, G, Screen, S, Pages[B.Page].Panels[B.Panel], Letterbox);
            if (B.Kind == EBeat::End)
            {
                const float White1 = Smooth((Time - .5f) / 1.1f);
                Box(Out, Layer + 8, G, FVector2f(0, 0), Screen, White, FLinearColor(1.f, .98f, .93f, White1 * Fade));
                const float Word = Smooth((Time - 1.2f) / .5f) * (1.f - Smooth((Time - 2.2f) / .5f));
                if (Word > 0.f)
                    CinemaText(Out, Layer + 9, G, TEXT("ACORDE"), Screen * .5f, (84.f + 12.f * Time) * S, FLinearColor(Ink.R, Ink.G, Ink.B, Word * Fade), true);
            }
            // capítulos (as duas folhas) e o pular
            if (B.Kind != EBeat::End)
            {
                for (int32 i = 0; i < TotalPanels(); ++i)
                {
                    const float D = (i == Global ? 8.f : 5.f) * S;
                    const FVector2f At(Screen.X * .5f + (i - (TotalPanels() - 1) * .5f) * 14.f * S - D * .5f, Letterbox * .5f - D * .5f);
                    Box(Out, Layer + 10, G, At, FVector2f(D, D), White, i == Global ? FLinearColor(1.f, .85f, .45f, .95f)
                        : FLinearColor(1.f, 1.f, 1.f, i < Global ? .5f : .18f));
                }
                DrawSkip(Out, Layer + 10, G, Screen, S, 1.f);
            }
            return Layer + 12;
        }

    private:

        TWeakObjectPtr<APlayerController> Controller;
        TArray<FPage> Pages;
        TArray<TStrongObjectPtr<UTexture2D>> Scenes;   // as cenas de cinema ficam carregadas enquanto a HQ está aberta
        TSharedPtr<FSlateBrush> TitleBrush;
        TArray<FBeat> Beats;
        int32 Beat = 0;
        float Time = 0.f;
        bool bFinished = false;
        bool bImpactSounded = false;
        const FSlateBrush* White = nullptr;
        FVector2f Mouse = FVector2f(-1.f, -1.f);
        mutable FSlateRect SkipBox;
        TWeakObjectPtr<UAudioComponent> Voice, Music;
        TMap<FName, TStrongObjectPtr<USoundBase>> Voices;
        TStrongObjectPtr<USoundBase> Song;
        float SongLength = 0.f;
        TArray<float> BeatStart;   // em que segundo da canção cada passo começa (para pular junto com a música)
        double LastClock = 0.0;
        // quando a canção começou (no relógio do sistema): a imagem segue esse relógio, então uma travada do jogo (o
        // mundo carregando por trás) não deixa a HQ atrasada em relação ao canto
        double SongStartClock = 0.0;
        bool bSkipped = false;

        void Load()
        {
            FString Json;
            TSharedPtr<FJsonObject> Root;
            if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectDir() / TEXT("Data/UI/Abertura/abertura.json"))) ||
                !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) return;
            FString SongName;
            if (Root->TryGetStringField(TEXT("music"), SongName))
            {
                Song.Reset(LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Audio/Music/%s.%s"), *SongName, *SongName)));
                double Length = 0.0;
                SongLength = Song.IsValid() ? (Root->TryGetNumberField(TEXT("musicLength"), Length) ? float(Length) : Song->GetDuration()) : 0.f;
            }
            for (const TSharedPtr<FJsonValue>& PageValue : Root->GetArrayField(TEXT("pages")))
            {
                const TSharedPtr<FJsonObject> P = PageValue->AsObject();
                FPage Page;
                const FString Name = P->GetStringField(TEXT("texture"));
                Page.Texture.Reset(LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Abertura/%s.%s"), *Name, *Name)));
                if (!Page.Texture.IsValid()) continue;
                // a proporção vem do tamanho da imagem original (a textura importada pode ter sido arredondada)
                const TArray<TSharedPtr<FJsonValue>>* SizeJson = nullptr;
                Page.Size = P->TryGetArrayField(TEXT("size"), SizeJson) && SizeJson->Num() == 2
                    ? FVector2f((*SizeJson)[0]->AsNumber(), (*SizeJson)[1]->AsNumber()) : FVector2f(Page.Texture->GetSizeX(), Page.Texture->GetSizeY());
                for (const TSharedPtr<FJsonValue>& PanelValue : P->GetArrayField(TEXT("panels")))
                {
                    const TSharedPtr<FJsonObject> Q = PanelValue->AsObject();
                    const TArray<TSharedPtr<FJsonValue>>& R = Q->GetArrayField(TEXT("rect"));
                    FPanel Panel;
                    Panel.Rect = FBox2f(FVector2f(R[0]->AsNumber(), R[1]->AsNumber()), FVector2f(R[2]->AsNumber(), R[3]->AsNumber()));
                    Panel.Text = Q->GetStringField(TEXT("text"));
                    Panel.Fx = FName(*Q->GetStringField(TEXT("fx")));
                    double At = -1.0;
                    if (Q->TryGetNumberField(TEXT("at"), At)) Panel.At = float(At);
                    const TArray<TSharedPtr<FJsonValue>>* SungJson = nullptr;
                    if (Q->TryGetArrayField(TEXT("sung"), SungJson))
                        for (const TSharedPtr<FJsonValue>& Word : *SungJson)
                        {
                            const TArray<TSharedPtr<FJsonValue>>& T = Word->AsArray();
                            if (T.Num() == 2) Panel.Sung.Add(FVector2f(T[0]->AsNumber(), T[1]->AsNumber()));
                        }
                    FString VoiceName;
                    if (Q->TryGetStringField(TEXT("voice"), VoiceName))
                    {
                        Panel.Voice = FName(*VoiceName);
                        if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Audio/Narracao/%s.%s"), *VoiceName, *VoiceName)))
                            Voices.Add(Panel.Voice, TStrongObjectPtr<USoundBase>(Sound));
                    }
                    Panel.Brush = MakeShared<FSlateBrush>();
                    Panel.Brush->SetResourceObject(Page.Texture.Get());
                    Panel.Brush->SetUVRegion(FBox2D(FVector2D(Panel.Rect.Min), FVector2D(Panel.Rect.Max)));
                    Panel.Brush->ImageSize = FVector2D(Panel.Rect.GetSize() * Page.Size);
                    Panel.Brush->DrawAs = ESlateBrushDrawType::Image;
                    Page.Panels.Add(Panel);
                }
                if (Page.Panels.Num() > 0) Pages.Add(MoveTemp(Page));
            }
            // as cenas de cinema: C01..C24 pela ordem dos quadros (C06a + C06b quando o verso tem duas), C00 no título
            int32 Global = 0, Painted = 0;
            for (FPage& Page : Pages)
                for (FPanel& Panel : Page.Panels)
                {
                    const FString Code = FString::Printf(TEXT("C%02d"), ++Global);
                    if (TSharedPtr<FSlateBrush> Scene = SceneBrush(Code)) { Panel.Brush = Scene; ++Painted; }
                    else if (TSharedPtr<FSlateBrush> First = SceneBrush(Code + TEXT("a")))
                    {
                        Panel.Brush = First;
                        Panel.BrushB = SceneBrush(Code + TEXT("b"));
                        ++Painted;
                    }
                }
            TitleBrush = SceneBrush(TEXT("C00"));
            UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO cenas pintadas: %d de %d quadros%s"), Painted, Global, TitleBrush.IsValid() ? TEXT(" + título") : TEXT(""));
            RepairCaptionTimings();
        }

        TSharedPtr<FSlateBrush> SceneBrush(const FString& Code)
        {
            const FString Name = TEXT("T_") + Code;
            UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Abertura/Cinema/%s.%s"), *Name, *Name));
            if (!Texture) return nullptr;
            Scenes.Emplace(Texture);
            TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
            Brush->SetResourceObject(Texture);
            Brush->ImageSize = FVector2D(FMath::Max(1, Texture->GetSizeX()), FMath::Max(1, Texture->GetSizeY()));
            Brush->DrawAs = ESlateBrushDrawType::Image;
            return Brush;
        }

        // Dados antigos podem ter atribuído o mesmo instante a várias palavras não
        // reconhecidas. Espalha esses trechos para que a legenda não fique parada e
        // despeje uma frase inteira de uma vez.
        void RepairCaptionTimings()
        {
            int32 Repaired = 0;
            for (FPage& Page : Pages)
            {
                for (FPanel& Panel : Page.Panels)
                {
                    TArray<FString> Words;
                    const FString NormalizedText = Panel.Text.Replace(TEXT("\n"), TEXT(" "));
                    NormalizedText.ParseIntoArray(Words, TEXT(" "));
                    if (Words.Num() != Panel.Sung.Num()) continue;
                    int32 i = 0;
                    while (i < Panel.Sung.Num())
                    {
                        const bool bShort = Panel.Sung[i].Y - Panel.Sung[i].X < .06f;
                        const bool bOutOfOrder = i > 0 && Panel.Sung[i].X < Panel.Sung[i - 1].X - .02f;
                        if (!bShort && !bOutOfOrder) { ++i; continue; }
                        const int32 BadIndex = i;
                        int32 First = i;
                        if (bOutOfOrder)
                            while (First > 0 && Panel.Sung[First - 1].X > Panel.Sung[BadIndex].X + .02f) --First;
                        const float Previous = First > 0 ? Panel.Sung[First - 1].Y : FMath::Max(0.f, Panel.At);
                        i = BadIndex + 1;
                        while (i < Panel.Sung.Num() && (Panel.Sung[i].Y - Panel.Sung[i].X < .06f || Panel.Sung[i].X <= Previous + .02f)) ++i;
                        const int32 Count = i - First;
                        float Next = i < Panel.Sung.Num() ? Panel.Sung[i].X : Previous + .35f * (Count + 1);
                        if (Next <= Previous) Next = Previous + .35f * (Count + 1);
                        const float Step = (Next - Previous) / float(Count + 1);
                        for (int32 Offset = 0; Offset < Count; ++Offset)
                        {
                            const float Start = Previous + Step * (Offset + .15f);
                            Panel.Sung[First + Offset] = FVector2f(Start, FMath::Max(Start + .06f, Previous + Step * (Offset + .85f)));
                            ++Repaired;
                        }
                    }
                }
            }
            if (Repaired) UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO legenda: tempos corrigidos em %d palavras"), Repaired);
        }

        void Build()
        {
            if (!BuildFromSong())
            {
                for (int32 p = 0; p < Pages.Num(); ++p)
                {
                    Beats.Add({ EBeat::PageIn, p, 0, PageInTime });
                    const int32 Count = Pages[p].Panels.Num();
                    for (int32 k = 0; k < Count; ++k)
                    {
                        const FPanel& Panel = Pages[p].Panels[k];
                        const TStrongObjectPtr<USoundBase>* Sound = Voices.Find(Panel.Voice);
                        // com voz: o tempo da fala; sem voz: o tempo de ler a legenda
                        const float Hold = Sound ? (*Sound)->GetDuration() + .8f : FMath::Clamp(Panel.Text.Len() / 14.f + 1.2f, 3.6f, 9.f);
                        Beats.Add({ EBeat::Focus, p, k, MoveTime + Hold, Hold });
                    }
                    Beats.Add(p + 1 < Pages.Num() ? FBeat{ EBeat::PageOut, p, Count - 1, PageOutTime } : FBeat{ EBeat::End, p, Count - 1, EndTime });
                }
                float Start = 0.f;
                for (const FBeat& B : Beats) { BeatStart.Add(Start); Start += B.Duration; }
            }
            if (Beats.Num()) Enter(0);
        }

        /** Com a canção: cada quadro chega quando o trecho dele começa a ser cantado (o "at" do abertura.json). */
        bool BuildFromSong()
        {
            if (!Song.IsValid() || SongLength <= 0.f) return false;
            // os tempos dos quadros, em ordem; os que faltarem (ou fora de ordem) ficam entre os vizinhos
            TArray<float> At;
            for (const FPage& P : Pages) for (const FPanel& Q : P.Panels) At.Add(Q.At);
            if (At.IsEmpty() || At[0] < 0.f) return false;
            for (int32 i = 1; i < At.Num(); ++i)
                if (At[i] <= At[i - 1])
                {
                    int32 j = i;
                    while (j < At.Num() && At[j] <= At[i - 1]) ++j;
                    const float Next = j < At.Num() ? At[j] : SongLength - EndTime;
                    for (int32 k = i; k < j; ++k) At[k] = At[i - 1] + (Next - At[i - 1]) * float(k - i + 1) / float(j - i + 1);
                }
            struct FStart { FBeat Beat; float Start; };
            TArray<FStart> Plan;
            int32 Global = 0;
            for (int32 p = 0; p < Pages.Num(); ++p)
            {
                const float First = At[Global] - MoveTime;
                if (p == 0)
                {
                    // a introdução da canção fica no escuro; a folha chega e pinta logo antes do primeiro verso
                    const float In = FMath::Max(0.f, First - PageInTime);
                    if (In > .5f) Plan.Add({ FBeat{ EBeat::Dark, p, 0 }, 0.f });
                    Plan.Add({ FBeat{ EBeat::PageIn, p, 0 }, In > .5f ? In : 0.f });
                }
                else
                {
                    Plan.Add({ FBeat{ EBeat::PageOut, p - 1, Pages[p - 1].Panels.Num() - 1 }, First - NextPageInTime - PageOutTime });
                    Plan.Add({ FBeat{ EBeat::PageIn, p, 0 }, First - NextPageInTime });
                }
                for (int32 k = 0; k < Pages[p].Panels.Num(); ++k, ++Global) Plan.Add({ FBeat{ EBeat::Focus, p, k }, At[Global] - MoveTime });
            }
            Plan.Add({ FBeat{ EBeat::End, Pages.Num() - 1, Pages.Last().Panels.Num() - 1 }, FMath::Max(SongLength - EndTime, At.Last() + 2.5f) });
            for (int32 i = 0; i < Plan.Num(); ++i)
            {
                FBeat B = Plan[i].Beat;
                const float Start = FMath::Max(Plan[i].Start, i ? BeatStart.Last() + .3f : 0.f);
                const float End = i + 1 < Plan.Num() ? Plan[i + 1].Start : Start + EndTime;
                B.Duration = FMath::Max(.3f, End - Start);
                B.Hold = FMath::Max(.1f, B.Duration - MoveTime);
                Beats.Add(B);
                BeatStart.Add(Start);
            }
            return true;
        }

        /** Pula para um passo (Offset segundos dentro dele) e leva a canção junto. */
        void Seek(int32 Index, float Offset = 0.f)
        {
            Enter(Index);
            if (Song.IsValid() && Music.IsValid() && BeatStart.IsValidIndex(Index))
            {
                const float At = BeatStart[Index] + Offset;
                Music->Play(At);
                SongStartClock = FPlatformTime::Seconds() - At;
            }
        }

        /** Em que segundo a canção está: o relógio real desde que ela começou. A música não para quando o jogo trava,
         *  e o relógio também não; o progresso que a própria música informa pulava nos jogos instalados. */
        double SongClock(double Now) const { return Now - SongStartClock; }

        void Enter(int32 Index)
        {
            Beat = Index;
            Time = 0.f;
            bImpactSounded = false;
            if (Voice.IsValid()) Voice->FadeOut(.25f, 0.f);
            Voice.Reset();
            const FBeat& B = Beats[Beat];
            APlayerController* PC = Controller.Get();
            if (B.Kind == EBeat::PageIn) VortexAudio::Play2D(PC, TEXT("wfx_book_page"), .9f);
            if (B.Kind == EBeat::PageOut) VortexAudio::Play2D(PC, TEXT("wfx_book_page"), 1.f);
            if (B.Kind == EBeat::End) VortexAudio::Play2D(PC, TEXT("holy_cast"), .7f);
        }

        // o tempo anda no Tick do widget (roda junto com o desenho) e no temporizador (por garantia): os dois medem
        // pelo relógio do sistema, então chamar os dois não adianta nada a mais
        virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override
        {
            SLeafWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);
            Step(DeltaTime);
        }

        EActiveTimerReturnType Animate(double, float Delta)
        {
            Step(Delta);
            return bFinished ? EActiveTimerReturnType::Stop : EActiveTimerReturnType::Continue;
        }

        void Step(float Delta)
        {
            if (bFinished || Beats.IsEmpty()) return;
            const float Before = Time;
            const int32 BeatBefore = Beat;
            const double Now = FPlatformTime::Seconds();
            if (Song.IsValid() && Music.IsValid() && !bSkipped && BeatStart.Num() == Beats.Num())
            {
                // com a canção: o passo e o tempo saem do ponto em que a música está (depois de uma travada, a imagem
                // cai no lugar certo em vez de ficar para trás e cortar a legenda do quadro)
                const float At = float(SongClock(Now));
                int32 Index = Beat;
                while (Index + 1 < Beats.Num() && At >= BeatStart[Index + 1]) ++Index;
                while (Index > 0 && At < BeatStart[Index]) --Index;
                if (Index != Beat) Enter(Index);
                Time = FMath::Max(0.f, At - BeatStart[Beat]);
            }
            else
                // sem a canção: tempo real pelo relógio do sistema (o temporizador da interface pode ser chamado menos vezes)
                Time += float(FMath::Clamp(LastClock > 0.0 ? Now - LastClock : double(Delta), 0.0, 5.0));
            LastClock = Now;
            const FBeat& B = Beats[Beat];
            if (B.Kind == EBeat::Focus && (Beat == BeatBefore ? Before : 0.f) < MoveTime && Time >= MoveTime)
            {
                // a câmera chegou: a fala e o som do quadro
                const FPanel& P = Pages[B.Page].Panels[B.Panel];
                if (const TStrongObjectPtr<USoundBase>* Sound = Voices.Find(P.Voice))
                    if (APlayerController* PC = Controller.Get()) Voice = UGameplayStatics::SpawnSound2D(PC, Sound->Get(), 1.f);
                // com a canção, o estrondo já está na música: os sons só entram sem ela
                if (!Song.IsValid() && P.Fx == TEXT("impact")) { VortexAudio::Play2D(Controller.Get(), TEXT("meteor"), 1.f); VortexAudio::Play2D(Controller.Get(), TEXT("boss_impact"), .8f); }
                if (!Song.IsValid() && P.Fx == TEXT("shake")) VortexAudio::Play2D(Controller.Get(), TEXT("boss_awaken"), .6f);
            }
            while (Time >= Beats[Beat].Duration)
            {
                if (Beat + 1 < Beats.Num())
                {
                    // o que passou do fim deste passo vai para o próximo (antes era jogado fora e a imagem atrasava)
                    const float Carry = Time - Beats[Beat].Duration;
                    Enter(Beat + 1);
                    Time = Carry;
                }
                else
                {
                    // fecha no próximo quadro (fechar remove este widget, que está no meio do próprio Tick)
                    bFinished = true;
                    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { VortexIntroComic::Close(); return false; }));
                    break;
                }
            }
        }

        void Skip()
        {
            VortexAudio::Play2D(Controller.Get(), TEXT("ui_click"), .6f);
            // pula para o fim (o branco do "Acorde"), sem cortar seco
            if (Beats.Last().Kind == EBeat::End && Beats[Beat].Kind != EBeat::End)
            {
                bSkipped = true;   // daqui em diante o fim corre pelo relógio (a música está sumindo)
                Enter(Beats.Num() - 1);
                Time = .4f;
                if (Music.IsValid()) Music->FadeOut(2.5f, 0.f);
            }
        }

        float FadeOutAlpha() const
        {
            const FBeat& B = Beats[Beat];
            return B.Kind == EBeat::End ? 1.f - Smooth((Time - 2.5f) / .7f) : 1.f;
        }

        int32 TotalPanels() const { int32 N = 0; for (const FPage& P : Pages) N += P.Panels.Num(); return N; }

        int32 GlobalIndex(int32 Page, int32 Panel) const
        {
            int32 N = 0;
            for (int32 p = 0; p < Page && p < Pages.Num(); ++p) N += Pages[p].Panels.Num();
            return N + Panel;
        }

        /** Um quadro cobrindo a tela; Progress (0 a 1) leva a câmera devagar, cada quadro para um lado. Second (0 a 1):
         *  quanto a 2ª cena do verso (BrushB) já cobriu a 1ª. */
        void DrawCinemaPanel(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Screen, int32 Page, int32 Panel,
            float Progress, float Alpha, const FVector2f& Shake, float Second = 0.f) const
        {
            if (Alpha <= 0.f || !Pages.IsValidIndex(Page) || !Pages[Page].Panels.IsValidIndex(Panel)) return;
            const FPanel& P = Pages[Page].Panels[Panel];
            const float Seed = GlobalIndex(Page, Panel) * 2.39f;
            if (Second < 1.f || !P.BrushB.IsValid()) DrawScene(Out, Layer, G, Screen, P.Brush.Get(), Progress, Alpha, Shake, Seed);
            if (Second > 0.f && P.BrushB.IsValid()) DrawScene(Out, Layer, G, Screen, P.BrushB.Get(), Progress, Alpha * Second, Shake, Seed + 1.3f);
        }

        void DrawScene(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Screen, const FSlateBrush* Brush,
            float Progress, float Alpha, const FVector2f& Shake, float Seed) const
        {
            if (!Brush || Alpha <= 0.f) return;
            const FVector2f Px = FVector2f(Brush->ImageSize);
            if (Px.X < 1.f || Px.Y < 1.f) return;
            const float T = FMath::Clamp(Progress, 0.f, 1.f);
            const float Fit = FMath::Max(Screen.X / Px.X, Screen.Y / Px.Y) * (1.06f + .08f * T);
            const FVector2f Size = Px * Fit;
            const FVector2f Pan = FVector2f(FMath::Cos(Seed), FMath::Sin(Seed)) * (T - .5f) * .035f * Screen;
            FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform((Screen - Size) * .5f + Pan + Shake)), Brush,
                ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Alpha));
        }

        /** Em que segundo da canção o verso passa para a 2ª metade (a 1ª palavra da 3ª linha); sem 3ª linha, o fim do passo. */
        float VerseSwitch(const FPanel& P, float Start, float End) const
        {
            TArray<FString> Lines;
            P.Text.ParseIntoArray(Lines, TEXT("\n"));
            if (Lines.Num() <= 2) return End;
            int32 Words = 0;
            for (int32 i = 0; i < 2; ++i) { TArray<FString> W; Lines[i].ParseIntoArray(W, TEXT(" ")); Words += W.Num(); }
            const float Switch = P.Sung.IsValidIndex(Words) ? P.Sung[Words].X - .25f : (Start + MoveTime + End) * .5f;
            return FMath::Clamp(Switch, Start + MoveTime + 2.f, End - 1.5f);
        }

        /** O verso do quadro, inteiro. Quadros de 4 linhas mostram 2 de cada vez: a troca é quando a 3ª começa a ser cantada. */
        void DrawVerse(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Screen, float S, const FPanel& P, float Letterbox) const
        {
            TArray<FString> Lines;
            P.Text.ParseIntoArray(Lines, TEXT("\n"));
            if (Lines.IsEmpty()) return;
            const FBeat& B = Beats[Beat];
            const float Start = BeatStart[Beat], End = Start + B.Duration, Now = SongTime();
            const float Switch = VerseSwitch(P, Start, End);
            const bool bSecond = Lines.Num() > 2 && Now >= Switch;
            const float From = bSecond ? Switch : Start + MoveTime * .6f, To = bSecond ? End : Switch;
            const float Alpha = Smooth((Now - From) / .6f) * Smooth((To - Now) / .45f);
            if (Alpha <= 0.f) return;
            const int32 First = bSecond ? 2 : 0, Count = FMath::Min(2, Lines.Num() - First);
            const FSlateFontInfo Font = CinemaFont(FMath::RoundToInt(34.f * S));
            const float LineH = Measure(TEXT("Ag"), Font).Y * 1.12f;
            const float Rise = (1.f - EaseOut((Now - From) / .8f)) * 10.f * S;
            for (int32 i = 0; i < Count; ++i)
            {
                const FString& Line = Lines[First + i];
                const FVector2f Sz = Measure(Line, Font);
                const FVector2f At(Screen.X * .5f - Sz.X * .5f, Screen.Y - Letterbox - 34.f * S - (Count - i) * LineH + Rise);
                FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(Sz, FSlateLayoutTransform(At)), Line, Font, ESlateDrawEffect::None,
                    FLinearColor(1.f, .95f, .84f, Alpha));
            }
        }

        static FSlateFontInfo CinemaFont(int32 Size)
        {
            static TSharedPtr<const FCompositeFont> Composite = []()
            {
                const FString Path = FPaths::ProjectContentDir() / TEXT("Vortex/UI/Fonts/Cinzel-700.ttf");
                return FPaths::FileExists(Path) ? TSharedPtr<const FCompositeFont>(MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad))
                    : TSharedPtr<const FCompositeFont>();
            }();
            FSlateFontInfo Info = Composite.IsValid() ? FSlateFontInfo(Composite, FMath::Max(6, Size)) : FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(6, Size));
            Info.OutlineSettings.OutlineSize = FMath::Max(1, Size / 14);
            Info.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, .85f);
            return Info;
        }

        void CinemaText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FString& Value, const FVector2f& Center, float Size,
            const FLinearColor& Color, bool bBold) const
        {
            const FSlateFontInfo Font = CinemaFont(FMath::RoundToInt(Size * (bBold ? 1.f : .9f)));
            const FVector2f Sz = Measure(Value, Font);
            FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(Sz, FSlateLayoutTransform(Center - Sz * .5f)), Value, Font, ESlateDrawEffect::None, Color);
        }

        /** Partículas de seiva subindo devagar (jade e ouro), sempre as mesmas, só o tempo as move. */
        void DrawMotes(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Screen, float S, float Amount) const
        {
            const float Now = SongTime();
            for (int32 i = 0; i < 46; ++i)
            {
                const float Hash = FMath::Frac(FMath::Sin(i * 12.9898f) * 43758.547f);
                const float Speed = .02f + .03f * FMath::Frac(Hash * 7.f);
                const float Y = 1.f - FMath::Frac(Now * Speed + Hash);
                const float X = FMath::Frac(Hash * 3.7f) + .015f * FMath::Sin(Now * .7f + i);
                const float D = (2.f + 3.f * FMath::Frac(Hash * 11.f)) * S;
                const float A = Amount * (.25f + .35f * FMath::Sin(Now * 2.f + i)) * Smooth(Y / .15f) * Smooth((1.f - Y) / .15f);
                if (A <= 0.f) continue;
                const FLinearColor C = i % 3 == 0 ? FLinearColor(1.f, .82f, .45f, A) : FLinearColor(.45f, 1.f, .75f, A);
                Box(Out, Layer, G, FVector2f(X * Screen.X, Y * Screen.Y), FVector2f(D, D), White, C);
            }
        }

        void DrawSkip(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Screen, float S, float Alpha) const
        {
            const FVector2f BtnSize(150.f * S, 42.f * S), BtnPos(Screen.X - BtnSize.X - 30.f * S, Screen.Y - BtnSize.Y - 18.f * S);
            SkipBox = FSlateRect(BtnPos.X, BtnPos.Y, BtnPos.X + BtnSize.X, BtnPos.Y + BtnSize.Y);
            const bool bHover = SkipBox.ContainsPoint(Mouse);
            Box(Out, Layer, G, BtnPos, BtnSize, White, FLinearColor(1.f, .85f, .45f, (bHover ? .3f : .1f) * Alpha));
            Box(Out, Layer, G, BtnPos, FVector2f(BtnSize.X, 1.5f * S), White, FLinearColor(1.f, .85f, .45f, .7f * Alpha));
            Box(Out, Layer, G, BtnPos + FVector2f(0.f, BtnSize.Y - 1.5f * S), FVector2f(BtnSize.X, 1.5f * S), White, FLinearColor(1.f, .85f, .45f, .7f * Alpha));
            CinemaText(Out, Layer + 1, G, TEXT("PULAR"), BtnPos + BtnSize * .5f, 20.f * S, FLinearColor(1.f, .92f, .75f, Alpha), false);
        }

        /** Em que segundo da canção a HQ está. */
        float SongTime() const { return BeatStart.IsValidIndex(Beat) ? BeatStart[Beat] + Time : Time; }

        static FVector2f Measure(const FString& Value, const FSlateFontInfo& Font)
        {
            return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value, Font));
        }

        void Box(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, const FSlateBrush* B, const FLinearColor& Tint) const
        {
            if (B) FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), B, ESlateDrawEffect::None, Tint);
        }

        void Text(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FString& Value, const FVector2f& At, float Size, const FLinearColor& Color, bool bCenter) const
        {
            const FSlateFontInfo Font = VortexUiKit::Cartoon::Font(FMath::Max(6, FMath::RoundToInt(Size)), 0);
            const FVector2f Sz = Measure(Value, Font);
            const FVector2f Pos = bCenter ? At - Sz * .5f : At;
            FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)), Value, Font, ESlateDrawEffect::None, Color);
        }
    };

    TSharedPtr<SVortexIntroComic> Window;
    TWeakObjectPtr<APlayerController> WindowOwner;
    TFunction<void()> WhenClosed;
}

namespace VortexIntroComicImpl
{
    bool IsOpen() { return Window.IsValid(); }

    void Open(APlayerController* Controller, TFunction<void()> OnClosed)
    {
        if (IsOpen() || !Controller || !GEngine || !GEngine->GameViewport) return;
        TSharedRef<SVortexIntroComic> Comic = SNew(SVortexIntroComic).Controller(Controller);
        if (!Comic->IsValidComic())
        {
            UE_LOG(LogTemp, Warning, TEXT("VORTEX_INTRO sem páginas (Tools/MakeIntroComic.py e ImportIntroComic.py)"));
            if (OnClosed) OnClosed();
            return;
        }
        Window = Comic;
        WindowOwner = Controller;
        WhenClosed = MoveTemp(OnClosed);
        GEngine->GameViewport->AddViewportWidgetContent(Comic, 80);
        Controller->SetShowMouseCursor(true);
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(Comic);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Controller->SetInputMode(Mode);
        if (UWorld* World = Controller->GetWorld())
            if (UVortexMusicSubsystem* Music = World->GetSubsystem<UVortexMusicSubsystem>()) Music->SetDucked(true);
        VortexAudio::SetCutscene(Controller, true);   // sem pássaros, vento e passos por cima da canção
        UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO aberta"));
    }

    void Close()
    {
        if (!IsOpen()) return;
        Window->StopAudio();
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Window.ToSharedRef());
        Window.Reset();
        if (APlayerController* Controller = WindowOwner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
            if (UWorld* World = Controller->GetWorld())
                if (UVortexMusicSubsystem* Music = World->GetSubsystem<UVortexMusicSubsystem>()) Music->SetDucked(false);
            VortexAudio::SetCutscene(Controller, false);
        }
        WindowOwner.Reset();
        UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO fechada"));
        if (TFunction<void()> Done = MoveTemp(WhenClosed)) Done();
    }

    void DebugJump(int32 Page, int32 Panel) { if (Window.IsValid()) Window->Jump(Page, Panel); }
    FString DebugState() { return Window.IsValid() ? Window->State() : TEXT("fechada"); }
    bool DebugSync(float& Image, float& Song) { Image = Song = -1.f; return Window.IsValid() && Window->Sync(Image, Song); }
}
