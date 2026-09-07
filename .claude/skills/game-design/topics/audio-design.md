# Audio Design

## Purpose

Audio reinforces the camera as the player's diegetic vantage point (see `player-interface.md`)
and keeps the world feeling alive without demanding attention the way a notification does.
This topic covers music, spatial/mix behavior, and player-facing audio controls; the
accessibility fallback (subtitles for information-carrying audio) is tracked separately in
`player-experience.md`.

## Music

Background music is not a single looping track: the game cycles through a rotation of
tracks over the course of play, so a long session doesn't feel like it's on repeat. Whether
rotation is sequential, random, or driven by context (combat vs. exploration vs. downtime)
isn't decided yet — see open questions.

## Player Audio Controls

Audio is expected to surface player-facing controls consistent with the "full audio
configurability" baseline in `player-experience.md` — at minimum independent volume control
per channel (music/SFX/ambient, at least). Whether the player gets more direct control over
music specifically (skip track, choose a mood/playlist) is an open question, not a
commitment yet.

## Spatial Perspective: the Camera Is the Listener

The player never possesses a single character — they command a squad from a floating
camera (see `vision-and-pillars.md`). Audio perspective follows that fact: the **camera**,
not any individual unit, is the player's diegetic ear. Two concrete implications:

- **Distance:** a sound effect gets louder as the camera moves closer to its source, and
  quieter as it pulls away — same as it would for a possessed character, just re-centered on
  the camera.
- **Direction:** a sound's position on screen should match its position in the mix — combat
  happening on the left edge of frame should audibly come from the left speaker, panning as
  the camera moves or rotates relative to the source.

This is a design commitment (the camera is the listener), not an implementation plan — the
technical approach (which engine/plugin features realize it) is a `game-systems` concern
once work starts, not something this topic should prescribe.

## Open Design Questions Worth Tracking

- Adaptive/dynamic music vs. fixed tracks isn't decided — rotation among multiple tracks is
  the only committed piece so far.
- Track rotation logic (sequential, random, or context-triggered by combat/exploration/
  downtime) isn't decided.
- How far player-facing music control goes (per-channel volume only, vs. track skip/mood
  selection) isn't decided.
- Diegetic vs. non-diegetic sound design intent more broadly, relevant to
  `player-interface.md`.
- Whether the mix should differentiate squad/ally sounds from enemy/hostile sounds (e.g. to
  keep the player's own squad legible in a crowded fight) hasn't been discussed.
