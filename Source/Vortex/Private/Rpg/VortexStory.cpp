#include "Rpg/VortexStory.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FVortexStoryCatalog& FVortexStoryCatalog::Get()
{
    static FVortexStoryCatalog Instance;
    return Instance;
}

const FVortexQuestDef* FVortexStoryCatalog::Quest(FName Id) const
{
    return Quests.FindByPredicate([Id](const FVortexQuestDef& Q) { return Q.Id == Id; });
}

const FVortexDialogue* FVortexStoryCatalog::DialogueOf(FName NpcId) const
{
    const FVortexNpcInfo* Info = Npcs.Find(NpcId);
    return Dialogues.Find(Info && !Info->Dialogue.IsNone() ? Info->Dialogue : NpcId);
}

FString FVortexStoryCatalog::NpcCallName(FName NpcId) const
{
    static const TMap<FName, FString> Names = {
        { TEXT("rhea"), TEXT("a Capitã Rhea") }, { TEXT("ilsa"), TEXT("a Ilsa") }, { TEXT("bram"), TEXT("o Bram") },
        { TEXT("nicolly"), TEXT("a Nicolly") }, { TEXT("lary"), TEXT("a Lary") }, { TEXT("lexy"), TEXT("a Lexy") },
        { TEXT("ducci"), TEXT("o Ducci") }, { TEXT("amal"), TEXT("a Amal") }, { TEXT("lyra"), TEXT("a Lyra") },
        { TEXT("garrick"), TEXT("o Garrick") }, { TEXT("rosa"), TEXT("a Rosa") }, { TEXT("marta"), TEXT("a Marta") },
        { TEXT("julia"), TEXT("a Julia") }, { TEXT("sahil"), TEXT("o Sahil") }, { TEXT("tavian"), TEXT("o Tavian") },
    };
    if (const FString* Found = Names.Find(NpcId)) return *Found;
    const FVortexNpcInfo* Info = Npcs.Find(NpcId);
    return Info ? Info->Name : NpcId.ToString();
}

FVortexStoryCatalog::FVortexStoryCatalog()
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Story/story.json"))))
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_STORY sem Data/Story/story.json (Tools/BuildStoryData.py)"));
        return;
    }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return;

    for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("chapters")))
    {
        const TSharedPtr<FJsonObject> O = Value->AsObject();
        FVortexChapter C;
        C.Id = int32(O->GetNumberField(TEXT("id")));
        C.Title = O->GetStringField(TEXT("title"));
        C.Region = O->GetStringField(TEXT("region"));
        C.Text = O->GetStringField(TEXT("text"));
        Chapters.Add(C);
    }
    for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("quests")))
    {
        const TSharedPtr<FJsonObject> O = Value->AsObject();
        FVortexQuestDef Q;
        Q.Id = FName(*O->GetStringField(TEXT("id")));
        Q.Giver = FName(*O->GetStringField(TEXT("giver")));
        Q.Title = O->GetStringField(TEXT("title"));
        O->TryGetStringField(TEXT("offer"), Q.Offer);
        O->TryGetStringField(TEXT("progress"), Q.Progress);
        O->TryGetStringField(TEXT("done"), Q.Done);
        O->TryGetStringField(TEXT("region"), Q.Region);
        O->TryGetNumberField(TEXT("chapter"), Q.Chapter);
        O->TryGetBoolField(TEXT("main"), Q.bMain);
        O->TryGetBoolField(TEXT("auto"), Q.bAuto);
        const TArray<TSharedPtr<FJsonValue>>* Requires = nullptr;
        if (O->TryGetArrayField(TEXT("requires"), Requires))
        {
            for (const TSharedPtr<FJsonValue>& R : *Requires) Q.Requires.Add(FName(*R->AsString()));
        }
        for (const TSharedPtr<FJsonValue>& ObjectiveValue : O->GetArrayField(TEXT("objectives")))
        {
            const TSharedPtr<FJsonObject> J = ObjectiveValue->AsObject();
            FVortexQuestObjective Objective;
            Objective.Type = FName(*J->GetStringField(TEXT("type")));
            J->TryGetStringField(TEXT("text"), Objective.Text);
            J->TryGetNumberField(TEXT("count"), Objective.Count);
            J->TryGetBoolField(TEXT("keep"), Objective.bKeep);
            double Radius = 12.0;
            if (J->TryGetNumberField(TEXT("radius"), Radius)) Objective.Radius = float(Radius) * 100.f;
            const TSharedPtr<FJsonValue> Target = J->TryGetField(TEXT("target"));
            if (Target.IsValid() && Target->Type == EJson::Array)
            {
                // Godot (x, z) em metros -> Unreal (X, Y) em cm
                const TArray<TSharedPtr<FJsonValue>>& P = Target->AsArray();
                if (P.Num() >= 2) Objective.Point = FVector2D(P[0]->AsNumber() * 100.0, P[1]->AsNumber() * 100.0);
            }
            else if (Target.IsValid()) Objective.Target = FName(*Target->AsString());
            Q.Objectives.Add(Objective);
        }
        const TSharedPtr<FJsonObject>* Rewards = nullptr;
        if (O->TryGetObjectField(TEXT("rewards"), Rewards))
        {
            (*Rewards)->TryGetNumberField(TEXT("xp"), Q.Xp);
            (*Rewards)->TryGetNumberField(TEXT("coins"), Q.Coins);
            const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
            if ((*Rewards)->TryGetArrayField(TEXT("items"), Items))
            {
                for (const TSharedPtr<FJsonValue>& Item : *Items)
                {
                    const TArray<TSharedPtr<FJsonValue>>& Pair = Item->AsArray();
                    if (Pair.Num() >= 2) Q.Items.Add({ FName(*Pair[0]->AsString()), int32(Pair[1]->AsNumber()) });
                }
            }
        }
        Quests.Add(MoveTemp(Q));
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("dialogues"))->Values)
    {
        const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
        FVortexDialogue D;
        D.Start = FName(*O->GetStringField(TEXT("start")));
        for (const TPair<FString, TSharedPtr<FJsonValue>> NodePair : O->GetObjectField(TEXT("nodes"))->Values)
        {
            const TSharedPtr<FJsonObject> N = NodePair.Value->AsObject();
            FVortexDialogueNode Node;
            Node.Id = FName(*NodePair.Key);
            N->TryGetStringField(TEXT("speaker"), Node.Speaker);
            N->TryGetStringField(TEXT("text"), Node.Text);
            if (const TSharedPtr<FJsonValue> Next = N->TryGetField(TEXT("next")))
            {
                Node.bHasNext = true;
                if (Next->Type == EJson::String) Node.Next = FName(*Next->AsString());
            }
            const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
            if (N->TryGetArrayField(TEXT("choices"), Choices))
            {
                for (const TSharedPtr<FJsonValue>& C : *Choices)
                {
                    const TSharedPtr<FJsonObject> J = C->AsObject();
                    FVortexDialogueChoice Choice;
                    J->TryGetStringField(TEXT("text"), Choice.Text);
                    FString Value;
                    if (J->TryGetStringField(TEXT("next"), Value)) Choice.Next = FName(*Value);
                    if (J->TryGetStringField(TEXT("if_flag"), Value)) Choice.IfFlag = FName(*Value);
                    if (J->TryGetStringField(TEXT("if_not_flag"), Value)) Choice.IfNotFlag = FName(*Value);
                    Node.Choices.Add(Choice);
                }
            }
            const TArray<TSharedPtr<FJsonValue>>* Actions = nullptr;
            if (N->TryGetArrayField(TEXT("actions"), Actions))
            {
                for (const TSharedPtr<FJsonValue>& A : *Actions)
                {
                    TArray<FString> Parts;
                    for (const TSharedPtr<FJsonValue>& Part : A->AsArray()) Parts.Add(Part->Type == EJson::Number ? FString::FromInt(int32(Part->AsNumber())) : Part->AsString());
                    Node.Actions.Add(Parts);
                }
            }
            D.Nodes.Add(Node.Id, MoveTemp(Node));
        }
        Dialogues.Add(FName(*Pair.Key), MoveTemp(D));
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("npcs"))->Values)
    {
        const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
        FVortexNpcInfo Info;
        Info.Id = FName(*Pair.Key);
        Info.Name = O->GetStringField(TEXT("name"));
        O->TryGetStringField(TEXT("title"), Info.Title);
        FString Dialogue;
        if (O->TryGetStringField(TEXT("dialogue"), Dialogue)) Info.Dialogue = FName(*Dialogue);
        const TArray<TSharedPtr<FJsonValue>>* Barks = nullptr;
        if (O->TryGetArrayField(TEXT("barks"), Barks)) for (const TSharedPtr<FJsonValue>& B : *Barks) Info.Barks.Add(B->AsString());
        Npcs.Add(Info.Id, MoveTemp(Info));
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_STORY %d missões, %d diálogos, %d NPCs"), Quests.Num(), Dialogues.Num(), Npcs.Num());
}
