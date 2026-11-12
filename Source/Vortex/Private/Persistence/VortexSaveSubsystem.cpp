#include "Persistence/VortexSaveSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Persistence/VortexPlayerSave.h"
#include "Persistence/VortexWorldSave.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "World/VortexArsenal.h"

namespace
{
constexpr int32 SaveUserIndex = 0;
constexpr uint8 SaveMagic[8] = { 'V', 'O', 'R', 'T', 'E', 'X', '0', '1' };
constexpr int32 VortexSaveHeaderSize = 16;
constexpr int32 MaxSaveBytes = 64 * 1024 * 1024;

void WriteUInt32(TArray<uint8>& Bytes, uint32 Value)
{
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Bytes.Add(static_cast<uint8>((Value >> (Index * 8)) & 0xff));
    }
}

uint32 ReadUInt32(const TArray<uint8>& Bytes, int32 Offset)
{
    uint32 Value = 0;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Value |= static_cast<uint32>(Bytes[Offset + Index]) << (Index * 8);
    }
    return Value;
}

bool ExtractPayload(const TArray<uint8>& Bytes, TArray<uint8>& Payload)
{
    if (Bytes.Num() < VortexSaveHeaderSize || Bytes.Num() > MaxSaveBytes + VortexSaveHeaderSize ||
        FMemory::Memcmp(Bytes.GetData(), SaveMagic, sizeof(SaveMagic)) != 0)
    {
        return false;
    }
    const uint32 Length = ReadUInt32(Bytes, 8);
    const uint32 ExpectedCrc = ReadUInt32(Bytes, 12);
    if (Length < 4 || Length > MaxSaveBytes || Length != static_cast<uint32>(Bytes.Num() - VortexSaveHeaderSize))
    {
        return false;
    }
    Payload.Append(Bytes.GetData() + VortexSaveHeaderSize, Length);
    // Unreal SaveGame memory data begins with GVAS. Check before asking the engine to deserialize.
    if (Payload[0] != 'G' || Payload[1] != 'V' || Payload[2] != 'A' || Payload[3] != 'S')
    {
        return false;
    }
    return FCrc::MemCrc32(Payload.GetData(), Payload.Num()) == ExpectedCrc;
}

TArray<uint8> WrapPayload(const TArray<uint8>& Payload)
{
    TArray<uint8> Bytes;
    Bytes.Append(SaveMagic, sizeof(SaveMagic));
    WriteUInt32(Bytes, static_cast<uint32>(Payload.Num()));
    WriteUInt32(Bytes, FCrc::MemCrc32(Payload.GetData(), Payload.Num()));
    Bytes.Append(Payload);
    return Bytes;
}

bool CheckSave(const USaveGame* Save, const UClass* ExpectedClass, const FGuid& ExpectedId)
{
    if (!Save || !Save->IsA(ExpectedClass))
    {
        return false;
    }
    if (const UVortexWorldSave* World = Cast<UVortexWorldSave>(Save))
    {
        return World->IsSupported() && World->WorldId == ExpectedId;
    }
    if (const UVortexPlayerSave* Player = Cast<UVortexPlayerSave>(Save))
    {
        return Player->IsSupported() && Player->Identity.PlayerId == ExpectedId;
    }
    return false;
}
}

bool UVortexSaveSubsystem::CanWriteWorld() const
{
    const UGameInstance* Instance = GetGameInstance();
    const UWorld* World = Instance ? Instance->GetWorld() : nullptr;
    return World && World->GetNetMode() != NM_Client;
}

FString UVortexSaveSubsystem::SlotName(const TCHAR* Prefix, const FGuid& Id)
{
    return FString(Prefix) + Id.ToString(EGuidFormats::Digits);
}

static USaveGame* DecodeSave(const TArray<uint8>& Bytes, const UClass* ExpectedClass, const FGuid& ExpectedId)
{
    TArray<uint8> Payload;
    if (!ExtractPayload(Bytes, Payload)) return nullptr;
    USaveGame* Save = UGameplayStatics::LoadGameFromMemory(Payload);
    return CheckSave(Save, ExpectedClass, ExpectedId) ? Save : nullptr;
}

bool UVortexSaveSubsystem::IsValidBytes(const TArray<uint8>& Bytes, UClass* ExpectedClass, const FGuid& ExpectedId)
{
    return DecodeSave(Bytes, ExpectedClass, ExpectedId) != nullptr;
}

USaveGame* UVortexSaveSubsystem::LoadValidated(const FString& Slot, UClass* ExpectedClass, const FGuid& ExpectedId)
{
    TArray<uint8> Bytes;
    if (UGameplayStatics::LoadDataFromSlot(Bytes, Slot, SaveUserIndex))
        if (USaveGame* Main = DecodeSave(Bytes, ExpectedClass, ExpectedId)) return Main;
    Bytes.Reset();
    if (UGameplayStatics::LoadDataFromSlot(Bytes, Slot + TEXT("_backup"), SaveUserIndex))
        if (USaveGame* Backup = DecodeSave(Bytes, ExpectedClass, ExpectedId))
        {
            UE_LOG(LogTemp, Warning, TEXT("Vortex save: using validated backup for %s"), *Slot);
            return Backup;
        }
    // uma vez por sessão: o menu relê a lista de mundos a cada tela e um save velho ou estragado enchia o log
    static TSet<FString> Warned;
    if (!Warned.Contains(Slot))
    {
        Warned.Add(Slot);
        UE_LOG(LogTemp, Warning, TEXT("Vortex save: no supported save or backup for %s"), *Slot);
    }
    return nullptr;
}

bool UVortexSaveSubsystem::SaveValidated(USaveGame* Save, const FString& Slot, UClass* ExpectedClass, const FGuid& ExpectedId)
{
    if (!CheckSave(Save, ExpectedClass, ExpectedId))
    {
        return false;
    }

    TArray<uint8> Payload;
    if (!UGameplayStatics::SaveGameToMemory(Save, Payload) || Payload.Num() > MaxSaveBytes)
    {
        return false;
    }
    TArray<uint8> NewBytes = WrapPayload(Payload);
    if (!IsValidBytes(NewBytes, ExpectedClass, ExpectedId))
    {
        return false;
    }

    const FString StageSlot = Slot + TEXT("_staging");
    if (!UGameplayStatics::SaveDataToSlot(NewBytes, StageSlot, SaveUserIndex))
    {
        return false;
    }
    TArray<uint8> StagedBytes;
    if (!UGameplayStatics::LoadDataFromSlot(StagedBytes, StageSlot, SaveUserIndex) ||
        StagedBytes != NewBytes)
    {
        return false;
    }

    TArray<uint8> OldBytes;
    if (UGameplayStatics::LoadDataFromSlot(OldBytes, Slot, SaveUserIndex) &&
        IsValidBytes(OldBytes, ExpectedClass, ExpectedId) &&
        !UGameplayStatics::SaveDataToSlot(OldBytes, Slot + TEXT("_backup"), SaveUserIndex))
    {
        return false;
    }

    if (!UGameplayStatics::SaveDataToSlot(NewBytes, Slot, SaveUserIndex))
    {
        return false;
    }
    TArray<uint8> WrittenBytes;
    const bool bVerified = UGameplayStatics::LoadDataFromSlot(WrittenBytes, Slot, SaveUserIndex) &&
        WrittenBytes == NewBytes;
    if (bVerified)
    {
        UGameplayStatics::DeleteGameInSlot(StageSlot, SaveUserIndex);
    }
    return bVerified;
}

UVortexWorldSave* UVortexSaveSubsystem::CreateNewWorld(int32 Seed, int32 GeneratorVersion)
{
    if (!CanWriteWorld() || GeneratorVersion < 1)
    {
        return nullptr;
    }
    UVortexWorldSave* Save = Cast<UVortexWorldSave>(UGameplayStatics::CreateSaveGameObject(UVortexWorldSave::StaticClass()));
    if (Save)
    {
        Save->WorldId = FGuid::NewGuid();
        Save->Seed = Seed;
        Save->GeneratorVersion = GeneratorVersion;
        Save->bGuestCopies = true;
    }
    return Save;
}

bool UVortexSaveSubsystem::SaveHostedWorld(UVortexWorldSave* WorldSave)
{
    return CanWriteWorld() && WorldSave && WorldSave->IsSupported() &&
        SaveValidated(WorldSave, SlotName(TEXT("Vortex_World_"), WorldSave->WorldId),
            UVortexWorldSave::StaticClass(), WorldSave->WorldId);
}

UVortexWorldSave* UVortexSaveSubsystem::LoadHostedWorld(FGuid WorldId)
{
    return CanWriteWorld() && WorldId.IsValid()
        ? Cast<UVortexWorldSave>(LoadValidated(SlotName(TEXT("Vortex_World_"), WorldId), UVortexWorldSave::StaticClass(), WorldId))
        : nullptr;
}

TArray<FVortexWorldSummary> UVortexSaveSubsystem::ListHostedWorlds()
{
    TArray<FVortexWorldSummary> Result;
    if (!CanWriteWorld()) return Result;

    const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    TArray<FString> Names;
    IFileManager::Get().FindFiles(Names, *(SaveDirectory / TEXT("Vortex_World_*.sav")), true, false);
    for (const FString& Name : Names)
    {
        // Only primary world slots are listed; backups and staging files are recovery data.
        const FString Stem = FPaths::GetBaseFilename(Name);
        const FString IdText = Stem.RightChop(FString(TEXT("Vortex_World_")).Len());
        FGuid Id;
        if (Stem.Len() != 45 || !FGuid::ParseExact(IdText, EGuidFormats::Digits, Id)) continue;
        const UVortexWorldSave* World = LoadHostedWorld(Id);
        if (!World) continue;
        FVortexWorldSummary& Summary = Result.AddDefaulted_GetRef();
        Summary.WorldId = Id;
        Summary.DisplayName = World->DisplayName.IsEmpty() ? TEXT("Mundo sem nome") : World->DisplayName;
        Summary.Seed = World->Seed;
        Summary.bMultiplayer = World->bMultiplayer;
        Summary.LastModified = IFileManager::Get().GetTimeStamp(*(SaveDirectory / Name));
    }
    Result.Sort([](const FVortexWorldSummary& A, const FVortexWorldSummary& B)
    {
        return A.LastModified > B.LastModified;
    });
    return Result;
}

UVortexPlayerSave* UVortexSaveSubsystem::CreateLocalPlayer()
{
    UVortexPlayerSave* Save = Cast<UVortexPlayerSave>(UGameplayStatics::CreateSaveGameObject(UVortexPlayerSave::StaticClass()));
    if (Save)
    {
        Save->Identity.PlayerId = FGuid::NewGuid();
        Save->Identity.CharacterId = FGuid::NewGuid();
    }
    return Save;
}

bool UVortexSaveSubsystem::SaveLocalPlayer(UVortexPlayerSave* PlayerSave)
{
    // 07/11: o Arsenal (o modo de teste) nunca grava o herói
    if (AVortexArsenal::IsArsenal(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)) return true;
    const bool bSaved = PlayerSave && PlayerSave->IsSupported() &&
        SaveValidated(PlayerSave, SlotName(TEXT("Vortex_Player_"), PlayerSave->Identity.PlayerId),
            UVortexPlayerSave::StaticClass(), PlayerSave->Identity.PlayerId);
    // a cópia em memória passa a ser a que acabou de ir para o disco
    if (bSaved) PlayerCache.Add(PlayerSave->Identity.PlayerId, PlayerSave);
    return bSaved;
}

UVortexPlayerSave* UVortexSaveSubsystem::LoadLocalPlayer(FGuid PlayerId)
{
    if (!PlayerId.IsValid()) return nullptr;
    if (const TObjectPtr<UVortexPlayerSave>* Cached = PlayerCache.Find(PlayerId); Cached && *Cached) return *Cached;
    UVortexPlayerSave* Loaded = LoadLocalPlayerFromDisk(PlayerId);
    if (Loaded) PlayerCache.Add(PlayerId, Loaded);
    return Loaded;
}

UVortexPlayerSave* UVortexSaveSubsystem::LoadLocalPlayerFromDisk(FGuid PlayerId)
{
    return PlayerId.IsValid()
        ? Cast<UVortexPlayerSave>(LoadValidated(SlotName(TEXT("Vortex_Player_"), PlayerId), UVortexPlayerSave::StaticClass(), PlayerId))
        : nullptr;
}

TArray<FVortexHeroSummary> UVortexSaveSubsystem::ListWorldHeroes(FGuid WorldId)
{
    TArray<FVortexHeroSummary> Result;
    if (!WorldId.IsValid()) return Result;
    const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    TArray<FString> Names;
    IFileManager::Get().FindFiles(Names, *(SaveDirectory / TEXT("Vortex_Player_*.sav")), true, false);
    for (const FString& Name : Names)
    {
        const FString Stem = FPaths::GetBaseFilename(Name);
        FGuid Id;
        if (Stem.Len() != 46 || !FGuid::ParseExact(Stem.RightChop(14), EGuidFormats::Digits, Id)) continue;
        const UVortexPlayerSave* Player = LoadLocalPlayer(Id);
        if (!Player || Player->WorldId != WorldId || Player->HeroName.IsEmpty()) continue;
        FVortexHeroSummary& Summary = Result.AddDefaulted_GetRef();
        Summary.PlayerId = Id;
        Summary.HeroName = Player->HeroName;
        Summary.RaceId = Player->RaceId;
    }
    Result.Sort([](const FVortexHeroSummary& A, const FVortexHeroSummary& B)
    {
        return A.HeroName < B.HeroName;
    });
    return Result;
}

bool UVortexSaveSubsystem::DeleteHostedWorld(FGuid WorldId)
{
    if (!WorldId.IsValid() || !LoadHostedWorld(WorldId)) return false;
    for (const FVortexHeroSummary& Hero : ListWorldHeroes(WorldId)) DeleteWorldHero(WorldId, Hero.PlayerId);
    const FString Slot = SlotName(TEXT("Vortex_World_"), WorldId);
    if (!UGameplayStatics::DeleteGameInSlot(Slot, SaveUserIndex)) return false;
    UGameplayStatics::DeleteGameInSlot(Slot + TEXT("_backup"), SaveUserIndex);
    UGameplayStatics::DeleteGameInSlot(Slot + TEXT("_staging"), SaveUserIndex);
    return true;
}

bool UVortexSaveSubsystem::DeleteWorldHero(FGuid WorldId, FGuid PlayerId)
{
    const UVortexPlayerSave* Hero = LoadLocalPlayer(PlayerId);
    if (!Hero || Hero->WorldId != WorldId) return false;
    const FString Slot = SlotName(TEXT("Vortex_Player_"), PlayerId);
    PlayerCache.Remove(PlayerId);
    if (!UGameplayStatics::DeleteGameInSlot(Slot, SaveUserIndex)) return false;
    UGameplayStatics::DeleteGameInSlot(Slot + TEXT("_backup"), SaveUserIndex);
    UGameplayStatics::DeleteGameInSlot(Slot + TEXT("_staging"), SaveUserIndex);
    return true;
}
