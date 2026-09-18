# SCP Foundation

A Cataclysm: Dark Days Ahead mod adding anomalies of the SCP Foundation.

The premise: containment failed a long time before the Cataclysm did. The things in the boxes are still in the boxes, and the boxes are still locked.

> **Developing this mod?** See [`DEVELOPING.md`](DEVELOPING.md) — a jumpstart guide covering the repo layout, the engine constraints that shaped the design, the SCP-087 architecture in depth, the gotchas, and how to validate changes. This README is the player/design-facing description; `DEVELOPING.md` is the developer guide.

---

## SCP-173 — "The Sculpture"

*Object Class: Euclid*

Animate concrete and rebar, hostile, and lethal by neck-snapping or strangulation. It **cannot move while within a direct line of sight**, and — the part that matters — **it cannot be harmed while you are looking at it either.**

You do not kill it. You leave.

### How it works in-game

CDDA has no facing direction and no vision cone: player sight is omnidirectional, limited only by radius, light level, and opaque obstacles. That means you *cannot* choose to look away, and a naive "frozen while observed" monster would simply stand there being harmless forever. The mechanic is therefore inverted, using the article's own containment procedure as the driver:

> *Personnel assigned to enter container are instructed to alert one another before blinking.*

**Blinking is involuntary.** So SCP-173 forces the window open itself:

1. **While you can see it** — it pins itself in place (`immobilization`, refreshed every turn) and rolls a **20% chance per turn to make you blink**, applying a 2-turn `blind` to you. You also hear grinding stone: the article's scraping-stone ambience, present even while it stands perfectly still.
2. **The moment you are blinded or asleep** — observation is broken. It **teleports to a tile beside you** and lands a bash to the head.
3. **Sight returns** — the freeze resumes.

**Light and darkness matter.** Your sight radius collapses at night, and an unlit SCP-173 is an unobserved one, free to stalk at its own speed. Bringing a light is not optional.

**Sleep is a total blind window.** It will wake you first — you get the reveal, never a silent death — and then strike.

**You cannot win a fight with it.** Any damage dealt while it is observed is instantly undone (it restores to full HP). The only way to hurt it is during a blink, which is precisely when it is trying to kill you. Hit points, armor, and regeneration exist only so that a determined, well-equipped player eventually realises it is pointless.

**The win condition is escape.** Its speed (45) is well below average, so a committed retreat works. Get out, and stay out.

### Where to find it

Rare spawn in lab and science sites (`GROUP_LAB`, `GROUP_LAB_SECURITY`, `GROUP_LAB_SURFACE`) at weight 1. Also debug-spawnable for testing.

### Only one at a time

CDDA has no `unique_id` for monsters — that machinery exists only for NPCs (`src/game.h:1187`). So the invariant is *maintained* rather than declared: a recurring world-scoped EOC sweeps every 10 minutes, grants a single instance the `scp_173_primary` marker, and retires any duplicate **only when you cannot see it**, so nothing ever pops out of existence in front of you. When the primary dies, the latch clears and a later instance can take its place.

---

## SCP-087 — "The Stairwell"

*Object Class: Euclid*

An unlit concrete staircase that descends twenty, or two hundred, or two thousand steps before turning back on itself, with a child's crying somewhere far below that no one can reach.  You are not meant to reach the bottom.  There is no bottom to reach.

### How it works in-game

CDDA caps the world at **10 z-levels below ground** (`src/map_scale_constants.h:47`, `OVERMAP_DEPTH = 10`) and caps vision at ~60 tiles.  So an "endless" staircase cannot be literally endless — it is **authored as a loop that feels endless**.

1. **The entrance** is a disguised janitorial closet on the surface, and it announces itself before you are even inside.  The reinforced steel door is an ordinary door — **`o` opens it**, no lockpicking and no smashing — but two beats sit either side of it: as you walk up, something **knocks twice from the other side**, and the moment you step through, the closet turns out to be **empty** and somewhere below the floor a **child is crying**.  Both are pure atmosphere — no effect, no damage, no sound in the world — and both re-arm once you walk away and come back.  Both are also **strictly for arrivals**: walking back *out* through the door and being ejected into the closet are silent.
2. **The shaft** is a hand-built spiral switchback spanning all ten z-levels.  Solid walls occlude line-of-sight past ~1.5 flights geometrically (there is no vision field on effects, so occlusion is done with real walls).  Every level is unlit — no fixtures, no windows — so **a light source is mandatory**.
3. **The seam.**  The deepest authored step `t_scp_087_seam` carries an invisible trap that teleports you back to a layout-identical landing at the top of the shaft and increments the loop counter.  The map has a bottom; you can never reach it, because the bottom quietly returns you to the top.  This is how "unknown if it has an endpoint" stays true without pretending to be infinite.
4. **The escape model** is driven by a per-world loop counter (`global_val`, survives save/load):
   - **Loops 1–4 (reversible):** ascending returns you to the surface closet.
   - **Loop 5 (point of no return):** `EOC_SCP_087_PHASE5` swaps the up-stairs for `t_scp_087_stairs_none` (no `GOES_UP`), so the way up is genuinely gone — ordinary stairs cannot lie, so the disappearance is real terrain.
   - **Loop 13 (ejection):** the seam spits you out at the surface.  It is **not** a victory — the stairwell rejected you.  It costs heavy morale, lingering paranoia, and it flags the world so the anomaly can later recur with different first-time cues.
5. **The crying** is delivered as `u_message` narration, never positional sound.  A "sound 200 m below" is impossible in CDDA's engine: `src/sounds.cpp:256-273` imposes ~970 tiles of effective vertical attenuation across ten z-levels, and sub-threshold sounds are silently dropped (`sounds.cpp:627`).  So the cry is prose, fired regardless of position, its source never placed on the map.  **This is the reason SCP-087's inter-anomaly fiction must never correlate with real geometry.**
6. **SCP-087-1** — the face with no pupils, nostrils, or mouth.  It is not the source of the crying.  It trails you by the native `KEEP_DISTANCE` mechanics once you are past loop 5, **ignores every creature but you** (its own player-only faction), **cannot be killed** — nothing you carry will bring its health down, and a solid hit only staggers it back — and is **confined to the shaft**: the mouth tile carries `MON_AVOID_STRICT` (`src/monmove.cpp:172-174`), a hard stop in the monster path that the player walks straight through.
   - On reaching you (non-lethal by design): paranoia pins to the **−99 morale floor**, your hands shake, you drop what you hold.  This block is isolated in `EOC_SCP_087_1_CLOSE` so making it lethal is a one-line change.
7. **Paranoia** escalates with the loop counter via a `RECURRING` EOC, **only** while you stand on a shaft tile.  The morale step is applied **once per loop advance**, not every tick, so the ladder is a function of how far you have descended rather than of how long you linger: −5, −15, −30, −50, −75, then the floor from loop 6.  The morale is applied with **`"capped": true`** — the flag that makes it accumulate linearly (`src/morale.cpp:195-204`; without it, repeated −5 bonuses stack by root-sum-of-squares and would never reach −30, let alone −99).  Hard-capped at −99 so the shaft is never unwinnable.
8. **What the dead left.** Nobody who walks this stairwell walks back out, and their gear is still lying where it caught up with them.  Up to four bodies can be down there — a Foundation researcher, a security escort, a test subject, and a scavenger who found the door herself — each with the kit they were carrying.  **The bodies are the gate and the rolls are inside them:** every body is a 45% spawn and every item on a body rolls independently, so a descent can yield a full researcher's kit or a corpse wearing nothing but a jumpsuit.  Nothing ever restocks, so the shaft is a cache you can strip clean over a single run.  The one real prize is the security escort's long arm — an AR-15 or a shotgun — and it sits behind *two* rolls, 5% inside a 45% body, so the true per-descent odds are about **2.25%**; it also never appears as loose ammunition for a weapon that isn't there.  He carries a half-spent magazine, loose rounds, and spent casings where he fell, so a body with casings and no rifle reads as a fight he lost.  The subject carries almost nothing, which is the point.  And on the deepest landing is the one thing down there that is **not** random, and the reason to reach the bottom at all: a **field log**, guaranteed, holding the last entry of someone working out in their own words that the stairs do not end.

### Where to find it

A **very rare** overmap special (`SCP-087`), placed on buildable land near settlements: **about one overmap in twelve**, and never more than one in the same overmap. It is meant to be something you hear about and eventually find, not a landmark you pass on the way to the shops.

### Sounds, no light, no bottom, no companions

The threshold refuses companions ("They will not go down there"), and it is **enforced**: on entering the closet, any NPC with you is moved *outside* the building, and for as long as you are below ground an escort net keeps lifting any NPC off the shaft's terrain back to the surface and out again.  A pet cannot enter at all — `MON_AVOID_STRICT` on the shaft mouth is a hard stop in the monster path (`src/monmove.cpp:172-174`), the same boundary that holds SCP-087-1 in.  There is **no UI depth counter** — the depth is felt through the cues, never shown as a number.

---

## Adding a new SCP
1. **Define the monster** in `monsters/` (see `scp_173.json` for the template).
2. **Add it to the registry** — `monstergroups/scp_registry.json` (`GROUP_SCP_ALL`) is the canonical index.
3. **Set its spawn rate** by adding a low-weight entry to the relevant monstergroups in `monstergroups/scp_spawns.json`. Note that same-id monstergroups **merge** with the base game rather than replacing them (`src/mongroup.cpp:470`), so you are adding to the lab groups, not overwriting them.
4. **Set its instance cap** by copying the two sweep EOCs in `effects_on_condition/scp_spawn_policy.json` and changing the `mtype_ids` entry.

Spawn *rate* (monstergroup weight) and instance *cap* (the sweep) are deliberately separate controls.

### Quota patterns

| Intent | How |
| --- | --- |
| Unique (SCP-173) | One primary marker, as implemented. |
| A pair | Two markers, or a counter with a threshold of 2. |
| A hive | Skip the sweep entirely — let monstergroup weight and `pack_size` do the work. |

---

## Gotchas discovered while building this

These cost real debugging time and are worth knowing before extending the mod:

- **`special_attacks` needs objects, not strings**: `{ "id": "foo" }`, not `"foo"`.
- **`u_run_monster_eocs` takes bare string ids.** Passing `{ "id": "X" }` *defines an inline EOC*, producing a duplicate-definition error.
- **`allow_unlock_doors` is documented in `MONSTERS.md` but rejected by the loader** (`src/monstergenerator.cpp:1196`). Don't use it.
- **Monster factions inherit the engine's reciprocity invariant**: if you declare another faction `neutral` or `friendly`, it must reciprocate, or `tests/monfactions_test.cpp` fails. Base-game factions only declare `by_mood` toward unknown factions, so leave them unlisted.
- **`Creature::sees` returns true unconditionally at range ≤ 1** (`src/creature.cpp:598`), bypassing blindness. Any "can the player see this" check must *also* exclude `blind`/`sleep`, or it will be wrong at melee range — which is exactly where it matters.
- **Effect durations decay in `process_turn`, before `move()`** (`src/do_turn.cpp:316` vs `:337`). A 1-turn self-applied freeze expires before the monster's next move; use 2 turns.
- **Use `immobilization`, not `stunned`,** to freeze a monster: `stunned` carries `NO_SPELLCASTING`, which would stop the monster refreshing its own freeze.
- **Every monster is transformation-connected by default.** Without explicitly omitting `zombify_into` / `fungalize_into` / `burn_into` / `revive_forms` and setting `"upgrades": false`, an animate concrete statue will eventually turn into a fungal zombie.

---

## Validating changes

The repository ships a test binary that loads mods properly. From the repo root (MSYS2 DLLs must be on `PATH`):

```sh
PATH="/c/msys64/mingw64/bin:$PATH" ./tests/cata_test.exe --mods dda,scp_foundation "[monster]"
PATH="/c/msys64/mingw64/bin:$PATH" ./cataclysm-tiles.exe --check-mods dda scp_foundation
```

Both must report no `Json error` and no `ERROR` lines. Note that `cataclysm-tiles.exe` segfaults on exit *after* printing its results — this reproduces with no mods loaded and is unrelated to this mod.

---

## Attribution

Content is derived from [SCP-173](https://scp-wiki.wikidot.com/scp-173) by Zaeyde and [SCP-087](https://scp-wiki.wikidot.com/scp-087) by Zaeyde on the SCP Wiki, both licensed **CC BY-SA 3.0**. Image credits for SCP-087: `087face.jpg` by CityToast (CC BY-SA 3.0); `087stair.png` by Josh Plueger, Aeou and CityToast (CC BY-SA 3.0). This mod is for personal use and is not redistributed.
