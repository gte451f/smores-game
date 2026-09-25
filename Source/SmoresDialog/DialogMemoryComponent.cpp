// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogMemoryComponent.h"
#include "GameFramework/Actor.h"

bool FDialogMemoryRecord::SetFlag(FName Flag, bool bValue)
{
	if (Flag.IsNone())
	{
		return false;
	}

	if (bValue)
	{
		if (Flags.Contains(Flag))
		{
			return false;
		}

		Flags.Add(Flag);
		return true;
	}

	return Flags.Remove(Flag) > 0;
}

const FDialogSeenEntry* FDialogMemoryRecord::FindSeen(FName ConversationId) const
{
	return Seen.FindByPredicate([ConversationId](const FDialogSeenEntry& Entry)
	{
		return Entry.ConversationId == ConversationId;
	});
}

bool FDialogMemoryRecord::HasSeen(FName Id) const
{
	const FString Wanted = Id.ToString();

	// a qualified id names exactly one conversation
	if (Wanted.Contains(TEXT(".")))
	{
		return FindSeen(Id) != nullptr;
	}

	// a bare title matches that title in any package - FName compares case-insensitively, and so
	// does this
	return Seen.ContainsByPredicate([&Wanted](const FDialogSeenEntry& Entry)
	{
		FString Package;
		FString Title;

		return Entry.ConversationId.ToString().Split(TEXT("."), &Package, &Title) && Title.Equals(Wanted, ESearchCase::IgnoreCase);
	});
}

void FDialogMemoryRecord::MarkSeen(FName ConversationId)
{
	if (ConversationId.IsNone())
	{
		return;
	}

	++SeenCounter;

	FDialogSeenEntry* Entry = Seen.FindByPredicate([ConversationId](const FDialogSeenEntry& Candidate)
	{
		return Candidate.ConversationId == ConversationId;
	});

	if (!Entry)
	{
		Entry = &Seen.AddDefaulted_GetRef();
		Entry->ConversationId = ConversationId;
	}

	++Entry->TimesSeen;
	Entry->LastSeenOrder = SeenCounter;
}

UDialogMemoryComponent::UDialogMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// the server is the only machine that reads it - see the class comment
	SetIsReplicatedByDefault(false);
}

UDialogMemoryComponent* UDialogMemoryComponent::Get(const AActor* PlayerState)
{
	return PlayerState ? PlayerState->FindComponentByClass<UDialogMemoryComponent>() : nullptr;
}

bool UDialogMemoryComponent::SetFlag(FName Flag, bool bValue)
{
	if (!HasOwnerAuthority())
	{
		return false;
	}

	return Record.SetFlag(Flag, bValue);
}

bool UDialogMemoryComponent::MarkSeen(FName ConversationId)
{
	if (!HasOwnerAuthority() || ConversationId.IsNone())
	{
		return false;
	}

	Record.MarkSeen(ConversationId);
	return true;
}

bool UDialogMemoryComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}
