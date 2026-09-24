# Notifications and Alerts

## Purpose

How the game tells the player something happened, and — just as importantly — what it
refuses to do about it on their behalf. `player-interface.md` owns *where* these surface;
this topic owns *what* generates one and *how loud* it is allowed to be.

The governing principle is `vision-and-pillars.md`'s **failure should be informative**: the
game's job is to make sure the player can always find out what happened and why, not to
make sure nothing bad happens while they were looking elsewhere.

## Two Channels, Deliberately Separate

Every notable event splits into two things that must not be collapsed into one:

- **The flash** — a brief, in-the-moment signal that a transition just occurred. It is
  transient by design. It can be missed.
- **The record** — the durable line in the activity feed (`player-interface.md`'s Activity,
  Chat and Alerts Panel). It never expires, so the player who looked away can always
  reconstruct what happened.

The flash is how the player finds out *now*. The record is how they find out *at all*. The
accessibility commitment in `player-experience.md` — notifications that persist rather than
auto-expiring, so high-speed play is safe — is carried entirely by the record. That is what
lets the flash be genuinely missable without the game losing information.

## The Danger Flash

The one alert the game raises with real urgency: **a member of the player's squad has
entered danger.**

**It flashes the thing that represents that character**, at whatever zoom the player is
currently at — their portrait when the portrait bar shows them, their group's entry in the
division switcher when it doesn't, and the map. One concept, several surfaces; the player
should never have to learn a different signal per screen.

**The engagement is the unit, not the event.** A fight produces a continuous stream of
attacks, and one signal per attack is noise that players learn to ignore. A character
entering a hostile engagement it was not already in flashes once. Everything inside that
engagement is silent — not because the player dismissed it, but because they have already
been told about this fight.

**Escalation flashes again.** Crossing into genuinely worse territory — a serious wound, a
character going down — is a new signal, because the player's earlier read on the situation
is now wrong. Escalation is what makes the first flash safe to ignore.

**The ongoing state lives underneath, not in the flash.** The health bar on the portrait is
already a continuous readout. The flash marks the transition into danger; the bar answers
how it is going. This is why the flash needs no duration tuning beyond "brief."

**There is no dismissal, because there is nothing to dismiss.** A flash does not interrupt,
does not block input, and does not wait for acknowledgement, so it needs no acknowledgement
mechanism, no suppression state, and no "don't tell me again."

## The Game Does Not Slow Itself Down — Decided

**An alert never changes the simulation.** It does not pause, does not drop the time pace,
and does not seize the camera. The player is told; what they do about it is theirs.

Three reasons, in order of weight:

1. **It works identically in single-player and multiplayer.** Time pace is single-player
   only (see `player-experience.md`), so any alert that reached for a pace drop would have
   to behave differently in co-op — a second design, a second set of bugs, and a different
   thing for players to learn depending on who they are playing with.
2. **It is a lighter touch.** A flash costs the player a glance. An interruption costs them
   the thing they were doing, every time the game decides something was important.
3. **It preserves consequence.** A player running at high speed with their attention
   elsewhere *can* miss a character getting into trouble, and can lose them. That is the
   honest cost of the pace control, and protecting the player from it would quietly make
   high-speed play the strictly correct choice. The record in the feed means the loss is
   always explainable afterwards, which is the standard the pillar actually sets — not that
   it was preventable.

## Accessibility

The flash is motion and colour, which makes it exactly the kind of signal
`player-experience.md`'s accessibility baseline exists to catch. It must carry a
non-colour channel — a shape or icon change alongside the colour — and it must be
distinguishable at a glance from the selection ring, which is the other thing that
decorates a portrait. Colour alone is not an acceptable implementation.

## Everything Else Is Just the Record

Only squad danger earns a flash. Trade opportunities, faction standing shifts, arrivals,
job completions, world events the player has learned about — these go to the feed and
nowhere else. The moment a second event type competes for the same urgency channel, the
channel stops meaning anything.

## Open Design Questions Worth Tracking

- The full trigger list beyond squad danger is not written. The current position is that
  nothing else reaches the flash channel, which is a deliberate floor, not an oversight.
- What exactly counts as "entering danger" — being targeted, being hit, or being detected
  by something hostile — is a tuning question that needs eyes on it in play.
- What counts as escalation worth re-flashing, beyond a character going down.
- Whether a notification *history* separate from the activity feed is warranted, or whether
  the feed's own persistence is the whole answer. Currently leaning on the feed.
- How the flash behaves for a group the player has no on-screen representation of at all —
  the division switcher and map surfaces above assume divisions exist
  (`characters-and-squads.md`), which they do not yet.
