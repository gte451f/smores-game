// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "SpikeConversation.h"
#include "DialogLoader.h"
#include "Misc/Paths.h"

FString SmoresDialogSpike::GetSceneDirectory()
{
	return FPaths::Combine(SmoresDialog::GetModsDirectory(), TEXT("example"), TEXT("conversations"));
}

void SmoresDialogSpike::SplitSpeaker(const FString& Raw, FString& OutSpeaker, FString& OutText)
{
	const FString Line = Raw.TrimStartAndEnd();

	// "Name: words", where the name is short and has no sentence punctuation - so a line that merely
	// contains a colon ("Listen: it's a trap") isn't taken for a speaker called "Listen".
	const int32 Colon = Line.Find(TEXT(": "));
	if (Colon > 0 && Colon <= 32)
	{
		const FString Name = Line.Left(Colon);
		if (!Name.Contains(TEXT(".")) && !Name.Contains(TEXT(",")) && !Name.Contains(TEXT("!")) && !Name.Contains(TEXT("?")))
		{
			OutSpeaker = Name;
			OutText = Line.Mid(Colon + 2).TrimStartAndEnd();
			return;
		}
	}

	OutSpeaker.Reset();
	OutText = Line;
}
