// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class AActor;

/**
 *  How long a bark bubble stays up, and how long it takes to go. All in **real seconds** - a bubble
 *  is read by a person, and a person doesn't read eight times faster at 8x.
 */
struct SMORESUI_API FBarkBubbleTiming
{
	/** The shortest time any line stays up, however short it is */
	float FloorSeconds = 2.0f;

	/** Added per character of the line, so a long line stays up long enough to read */
	float PerCharacterSeconds = 0.06f;

	/** The longest time any line stays up, however long it is */
	float CapSeconds = 6.0f;

	/** After its time is up, how long the bubble takes to fade out */
	float FadeSeconds = 0.6f;

	/** Seconds at full opacity for a line CharacterCount characters long: the floor plus the per-character time, capped */
	float GetLifetime(int32 CharacterCount) const;
};

/** One bubble: who said what, and when */
struct SMORESUI_API FBarkBubbleEntry
{
	TWeakObjectPtr<const AActor> Speaker;

	FText Text;

	/** When the line was said, in real seconds */
	double StartTime = 0.0;

	/** Seconds at full opacity, from FBarkBubbleTiming::GetLifetime */
	float Lifetime = 0.0f;

	/**
	 *  Different every time a line is shown, including a replacement on the same speaker - so a
	 *  widget can tell "still the same line" from "a new line" without comparing text.
	 */
	uint32 Serial = 0;
};

/**
 *  Which bark bubbles are up: at most one per speaker, each with its own clock.
 *
 *  A plain value type, so the rules are testable without a widget or a world:
 *
 *  - **One bubble per speaker.** A new line from someone already showing one replaces it - text and
 *    clock both - and keeps its place in the order, so a chatty speaker's bubble doesn't jump to the
 *    top of a stack every time they speak.
 *  - **It lasts long enough to read, then fades.** GetOpacity is 1 for the line's lifetime, then
 *    falls to 0 across FadeSeconds; Prune drops it once the fade is over.
 *  - **A bubble is an event, not a record.** Nothing here is kept after it fades. The activity feed
 *    is where the line stays (hud-and-panels.md).
 */
class SMORESUI_API FBarkBubbleSchedule
{
public:

	FBarkBubbleTiming Timing;

	/** Speaker said Text at Now (real seconds): replaces Speaker's bubble if they have one, else starts a new one after the rest */
	void Show(const AActor* Speaker, const FText& Text, double Now);

	/** Drops every bubble that has finished fading, and every bubble whose speaker has gone */
	void Prune(double Now);

	/** 1 while the line is up, falling to 0 across the fade, 0 once it is over */
	float GetOpacity(const FBarkBubbleEntry& Entry, double Now) const;

	/** True once the line's lifetime and its fade have both passed */
	bool IsExpired(const FBarkBubbleEntry& Entry, double Now) const;

	/** Speaker's bubble, or null */
	const FBarkBubbleEntry* Find(const AActor* Speaker) const;

	/** Every bubble, in the order its speaker first got one */
	const TArray<FBarkBubbleEntry>& GetEntries() const { return Entries; }

	void Reset() { Entries.Reset(); }

private:

	TArray<FBarkBubbleEntry> Entries;

	uint32 NextSerial = 1;
};

namespace SmoresBarkBubbles
{
	/**
	 *  Moves bubbles that would overlap so they stack instead. Screen space: Y grows downward.
	 *
	 *  Boxes are settled in array order. The first keeps its place; each later one that would overlap
	 *  a box already settled moves straight up until it clears it by Gap, and so on until it overlaps
	 *  nothing. **Only ever up** - never sideways, so a bubble stays over its speaker's column, and
	 *  never down, so nothing is pushed over a speaker's head. Boxes side by side that don't overlap
	 *  horizontally are left alone.
	 */
	SMORESUI_API void StackBoxes(TArray<FBox2D>& Boxes, float Gap);
}
