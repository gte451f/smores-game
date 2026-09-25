// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "BarkBubbleSchedule.h"
#include "GameFramework/Actor.h"

float FBarkBubbleTiming::GetLifetime(int32 CharacterCount) const
{
	const float Uncapped = FloorSeconds + PerCharacterSeconds * FMath::Max(CharacterCount, 0);

	return FMath::Max(FMath::Min(Uncapped, CapSeconds), 0.0f);
}

void FBarkBubbleSchedule::Show(const AActor* Speaker, const FText& Text, double Now)
{
	if (!Speaker)
	{
		return;
	}

	FBarkBubbleEntry* Entry = Entries.FindByPredicate([Speaker](const FBarkBubbleEntry& Candidate)
	{
		return Candidate.Speaker.Get() == Speaker;
	});

	// a new line replaces the speaker's old one where it stands, so it keeps its place in a stack
	if (!Entry)
	{
		Entry = &Entries.AddDefaulted_GetRef();
		Entry->Speaker = Speaker;
	}

	Entry->Text = Text;
	Entry->StartTime = Now;
	Entry->Lifetime = Timing.GetLifetime(Text.ToString().Len());
	Entry->Serial = NextSerial++;
}

void FBarkBubbleSchedule::Prune(double Now)
{
	Entries.RemoveAll([this, Now](const FBarkBubbleEntry& Entry)
	{
		return !Entry.Speaker.IsValid() || IsExpired(Entry, Now);
	});
}

float FBarkBubbleSchedule::GetOpacity(const FBarkBubbleEntry& Entry, double Now) const
{
	const double Age = Now - Entry.StartTime;

	if (Age < Entry.Lifetime)
	{
		return 1.0f;
	}

	if (Timing.FadeSeconds <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(1.0f - static_cast<float>((Age - Entry.Lifetime) / Timing.FadeSeconds), 0.0f, 1.0f);
}

bool FBarkBubbleSchedule::IsExpired(const FBarkBubbleEntry& Entry, double Now) const
{
	return Now - Entry.StartTime >= Entry.Lifetime + FMath::Max(Timing.FadeSeconds, 0.0f);
}

const FBarkBubbleEntry* FBarkBubbleSchedule::Find(const AActor* Speaker) const
{
	return Entries.FindByPredicate([Speaker](const FBarkBubbleEntry& Candidate)
	{
		return Candidate.Speaker.Get() == Speaker;
	});
}

namespace SmoresBarkBubbles
{
	void StackBoxes(TArray<FBox2D>& Boxes, float Gap)
	{
		for (int32 Index = 1; Index < Boxes.Num(); ++Index)
		{
			FBox2D& Box = Boxes[Index];

			// each settled box can lift this one at most once - once above a box, moving further up
			// never brings it back into it - so Index + 1 passes always reach a pass with no lift
			for (int32 Pass = 0; Pass <= Index; ++Pass)
			{
				bool bLifted = false;

				for (int32 Settled = 0; Settled < Index; ++Settled)
				{
					const FBox2D& Other = Boxes[Settled];

					// strictly overlapping across, so bubbles merely side by side stay where they are;
					// within Gap up and down, so a stack has breathing room between its bubbles
					const bool bOverlapsAcross = Box.Min.X < Other.Max.X && Other.Min.X < Box.Max.X;
					const bool bOverlapsUpDown = Box.Min.Y < Other.Max.Y + Gap && Other.Min.Y < Box.Max.Y + Gap;

					if (bOverlapsAcross && bOverlapsUpDown)
					{
						const double Lift = Box.Max.Y - (Other.Min.Y - Gap);

						Box.Min.Y -= Lift;
						Box.Max.Y -= Lift;

						bLifted = true;
					}
				}

				if (!bLifted)
				{
					break;
				}
			}
		}
	}
}
