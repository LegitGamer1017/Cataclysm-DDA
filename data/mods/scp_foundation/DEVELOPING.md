# Developing the SCP Foundation Mod

A jumpstart for developers extending this mod. It assumes you know CDDA's basic
JSON mod format; it focuses on **how this mod is put together, why, and where the
sharp edges are**.

Read this before touching SCP-087. Read [`README.md`](README.md) for the
player-facing description of each anomaly.

---

## 1. What this is

A Cataclysm: Dark Days Ahead mod adding SCP Foundation anomalies. Two are
implemented:

| SCP | What it is | Impl. status |
| --- | --- | --- |
| **SCP-173** | Animate concrete, frozen while observed, invulnerable while observed. | Feature-complete |
| **SCP-087** | An endless unlit stairwell that loops you forever; a faceless pursuer inside. | Feature-complete |

Everything is pure JSON. There is **no C++ code** — no `src/` patching. Every
mechanic is built from the engine's existing hooks (EOCs, `ter_furn_transform`,
monster special attacks, `MON_AVOID_STRICT`, factions). If you can't express an
idea with those, the answer is usually a different engine hook, not a code change.

Current version: `0.2` (see `modinfo.json`).

---

## 2. Repo layout

All paths are relative to `data/mods/scp_foundation/`.

### Shared / general-mod files

| File | Purpose |
| --- | --- |
| `modinfo.json` | Mod metadata + dependencies (`dda`). |
| `README.md` | Player/design-facing documentation. |
| `species.json` | `SPECIES` definitions: `SCP` (broad) and `SCP_087` (narrow, child of `SCP`). |
| `monster_factions.json` | `foundation` (173) and `scp_087` (player-only) factions. |
| `monstergroups/scp_registry.json` | `GROUP_SCP_ALL` — the canonical index of every SCP. |
| `monstergroups/scp_spawns.json` | Natural world-spawn weights (173 only, into lab groups). |
| `effects_on_condition/scp_spawn_policy.json` | The "only one 173 at a time" quota sweep. |
| `disabled_spawning.json` | On/off switch: `MONSTER_BLACKLIST` for `mon_scp_173`. |

### SCP-173 files

| File | Purpose |
| --- | --- |
| `monsters/scp_173.json` | The monster. |
| `monster_attacks/scp_173_attacks.json` | Freeze / lunge special attacks + death hook. |
| `effects_on_condition/scp_173_effects.json` | The `scp_173_observed` marker effect. |
| `effects_on_condition/scp_173_invulnerability.json` | "Can't be hurt while watched." |

### SCP-087 files

| File | Purpose |
| --- | --- |
| `overmap_special/scp_087_special.json` | Places the 10-level shaft (z0 + z-1..z-9). |
| `overmap_terrain/scp_087_overmap_terrain.json` | Overmap tiles `scp_087_z0` … `scp_087_z-9`. |
| `mapgen/scp_087_mapgen.json` | The three level layouts, plus one extra object per loot level — see §8 on why a loot placement needs an object of its own. |
| `furniture_and_terrain/scp_087_shaft_terrain.json` | Every terrain tile (walls, floors, stairs, seam, door). |
| `items/scp_087_loot.json` | The four corpse kits and the field log — the mod's only loot. |
| `furniture_and_terrain/scp_087_transform.json` | The two `ter_furn_transform`s (remove/restore the exit). |
| `traps/scp_087_traps.json` | The seam trap `tr_scp_087_seam` and the doorway trigger `tr_scp_087_doorway`. |
| `snippets/scp_087_snippets.json` | All prose (cry / turn / phase5 / phase13 / see + `*_again` variants). |
| `effects_on_condition/scp_087_seam.json` | The loop counter, seam descent, loop-5, loop-13 ejection. |
| `effects_on_condition/scp_087_1_pursuer.json` | The pursuer: summon / keeper / follow / confine / retire. |
| `monsters/scp_087_1.json` | `mon_scp_087_1`. |
| `monster_attacks/scp_087_1_attacks.json` | Contact attack, sleep interrupt, invuln-and-stagger. |
| `effects_on_condition/scp_087_paranoia_driver.json` | Paranoia + crying recurring EOCs. |
| `effects_on_condition/scp_087_paranoia_effect.json` | `scp_087_paranoia` / `scp_087_contact` effects. |
| `effects_on_condition/scp_087_paranoia_morale.json` | The morale type. |
| `effects_on_condition/scp_087_companion.json` | "Companions won't go down there." |

---

## 3. Engine constraints that shaped the design

Know these or the design will look arbitrary. Each is a real engine limit, and
the mod's structure is a response to it.

1. **10 z-levels max.** `OVERMAP_DEPTH = 10` (`src/map_scale_constants.h:47`).
   An "endless" stairwell is impossible, so it is **authored as a finite loop
   that returns you to the top** (the seam). See §5.
2. **Vision caps at ~60 tiles** and CDDA has no vision cone / facing. Occlusion
   is done with **real walls** — the flight is straight so the turn hides ~1.5
   flights. This is also why SCP-173 can't use a "look away" mechanic (§6).
3. **Sound attenuates hard vertically.** ~970 tiles of effective attenuation
   across 10 z (`src/sounds.cpp:256-273`), and sub-threshold sounds are dropped
   (`src/sounds.cpp:627`). A "cry 200 m below" is therefore impossible as sound —
   it is delivered as **prose** (`u_message` + snippets). This is the reason SCP-087's
   fiction must never correlate with real geometry.
4. **No `unique_id` for monsters** (`src/game.h:1187` is NPC-only). "Only one
   173" has to be **maintained by a sweep**, not declared (§4).
5. **`u_transform_radius` only affects the caster's current z-level**
   (`map::transform_radius` → `points_in_radius(loc, radi, 0)`, z-radius
   hard-coded to 0). To change another level you must aim it with `target_var`.
6. **EOC-level `condition` runs before the effect body**
   (`src/effect_on_condition.cpp:370`). This is the primary performance lever:
   gate recurring EOCs there so idle ticks do nothing.

---

## 4. General-mod patterns (apply to every SCP)

### Species

- `SCP` — the broad family. Every anomaly is in it.
- Add a **narrowing child species** (like `SCP_087`) when an effect must target
  one SCP and not the others. `mtype` `species` is a **list**, so a monster can
  be `[ "SCP", "SCP_087" ]`.

> **Why this exists:** 173 and 087-1 originally both had just `[ "SCP" ]`, so an
> EOC gated on `"SCP"` hit both. A narrow species is how you say "087-1 only".
> Any new species **must be defined** — `MonsterGenerator::validate_species_ids`
> debugmsg()s on an unknown one (`src/monstergenerator.cpp:726`, called from
> `:372`). Species support `copy-from` like any other factory type.

### Factions

Each anomaly gets a faction tuned to who it hunts. `scp_087` hates **only**
`player`; `foundation` hates `player` and `human`.

> **The reciprocity invariant:** if you declare another faction `neutral` or
> `friendly`, it must reciprocate, or `tests/monfactions_test.cpp` fails. Base-game
> factions only declare `by_mood` toward unknown factions — so **leave them
> unlisted**. `hate` needs no reciprocation.

> **Never list a faction in its own `by_mood`.** `monfaction::load` builds the
> attitude map from the JSON lists and only *then* runs
> `attitude_map.emplace( id, MFA_FRIENDLY )` (`src/monfaction.cpp:194-208`) — and
> `emplace` will not overwrite an existing key. So `"by_mood": [ "my_faction" ]`
> silently replaces the engine's friendly-to-self default with a mood-based
> attitude, and two anomalies of the same faction may then treat each other as
> targets instead of allies. The self-entry buys nothing: the reciprocity test
> only inspects `friendly`/`neutral`, and `hate` is one-directional.

### Registry

`monstergroups/scp_registry.json` (`GROUP_SCP_ALL`) is the canonical list of
every SCP. Nothing spawns from it — it's an index. Add one line per new SCP and
keep it in step with the quota sweeps.

### Spawn rate vs. instance cap — two separate controls

- **Rate** = monstergroup weight in `monstergroups/scp_spawns.json`. Note that
  same-id monstergroups **merge** with the base game (`src/mongroup.cpp:470`) —
  you *add to* `GROUP_LAB`, you don't replace it.
- **Cap** = the quota sweep in `effects_on_condition/scp_spawn_policy.json`
  (copy `EOC_SCP_173_QUOTA_SWEEP` + `_ONE`, change `mtype_ids`).

### The blacklist switch

`disabled_spawning.json` uses `MONSTER_BLACKLIST` to switch a monster **fully
off**.

> **Critical:** the blacklist is enforced in `creature_tracker::add`
> (`src/creature_tracker.cpp:93`), which **every** placement path goes through —
> including `u_spawn_monster` (`src/game.cpp:4536`) and hallucinations
> (`src/game.cpp:4727`). A blacklisted monster cannot be placed by *any* means.
>
> **Do not blacklist an SCP that is summoned by an EOC** (like 087-1). It won't
> just stop natural spawning — it silently disables the whole encounter. Only
> blacklist monsters that have a natural spawn path. 087-1 needs no entry at all
> (`GROUP_SCP_ALL` is referenced nowhere).

---

## 5. SCP-087 architecture (the deep dive)

This is the part most likely to need work, so it gets the detailed treatment.

### The level layout

The special spans **z0 + z-1 … z-9** (a surface closet plus nine shaft levels):

- **z0** — the disguised janitorial closet, one row inside the tile's south edge, with a passable concrete apron at the boundary. The door is an ordinary openable door (`o`); the two-row entrance is what makes the knock beat possible (§5).
- **Layout A** (odd depths: z-1, z-3, z-5, z-7): arrive at `(11,2)`, leave at `(11,20)`.
- **Layout B** (even depths: z-2, z-4, z-6, z-8): mirrored — arrive at `(11,20)`, leave at `(11,2)`.
- **z-9** — Layout A, but the down-stairs is replaced by the **seam**.

Each level is a **13-tile flight** (cols 10-12, rows 2-14) plus a wide landing
(cols 6-16, rows 15-21). A and B are mirror images, so the descent **reverses
180° every platform** — the article's "descent direction rotates 180 degrees at
each platform". The mirroring is also why the levels are hard to tell apart.

**Stair chaining rule:** descending looks for a `GOES_UP` tile on the level
below at the *same x,y* (`src/game.cpp:9955, 9976-9981`). So z0's `>` at
`(11,2)` sits directly above z-1's `<` at `(11,2)`. If you move stairs in mapgen,
they must stay column-aligned across levels.

### The seam — how "endless" is faked

`t_scp_087_seam` (at `(11,20)` on z-9) carries `tr_scp_087_seam`, an invisible,
`UNDODGEABLE` trap. Stepping on it fires `EOC_SCP_087_SEAM`
(`effects_on_condition/scp_087_seam.json`), which:

1. increments `scp_087_loops`;
2. computes the arrival tile **once** — `trap_location` +8 z / −18 y → the
   up-stairs at `(11,2)` on z-1 — into the context var `scp_087_arrival`;
3. teleports you there (or ejects, if `loops >= 13`);
4. at exactly `loops == 5`, triggers the point of no return.

> **The offsets (8 / −18 / +1) are intentionally named, not scattered.** They are
> the *only* place the geometry is encoded outside mapgen. If the mapgen changes,
> here is what you update. See the `//5b` comment in `scp_087_seam.json`.

### The escape model — three phases on `scp_087_loops`

`scp_087_loops` is a `global_val` (per-world, survives save/load).

| Phase | `loops` | What happens |
| --- | --- | --- |
| Reversible | 1–4 | Ascending via the `<` up-stairs returns you to the closet. |
| Point of no return | == 5 | `EOC_SCP_087_PHASE5` runs `scp_087_remove_exit` (`t_scp_087_stairs_up` → `t_scp_087_stairs_none`, no `GOES_UP`) and summons the real pursuer. |
| Ejection | >= 13 | `EOC_SCP_087_EJECT` teleports you to the closet, opens the door behind you, applies morale/effect penalties, sets `scp_087_archive = 1`, restores the exit, retires the pursuer, and resets all state. It is **not** an arrival, so it deliberately does not play the entrance beats (`scp_087_just_ejected`). |

**Two things that will bite you if you touch this:**

- **The transform persists.** The loop-5 terrain change is written into the map's
  submaps and mapgen does not re-run on a later visit. That is why ejection runs
  `scp_087_restore_exit` to put the up-stairs back. If you add a new persistent
  change, you must add its inverse at ejection too.
- **State must reset at ejection.** `EOC_SCP_087_EJECT` resets `scp_087_loops`,
  `scp_087_paranoia_at`, `scp_087_knock_heard`, `scp_087_doorway_seen`,
  `scp_087_approach`, `scp_087_1_active`, `scp_087_1_idle`,
  `scp_087_1_lastloop`. Without this the
  counter stays at 13 and a second visit instantly re-ejects. The pursuer is
  **retired** (`EOC_SCP_087_1_RETIRE`) first, because monsters persist in the
  submap spawn list (`src/submap.h:316`) — otherwise a second summon would stack
  a second pursuer.

> **The trap is `AVATAR_ONLY`, and it is not consumed.** Two independent facts
> that both matter:
>
> 1. **Only the player may trigger it.** `map::maybe_trigger_trap` filters null,
>    benign and `AVATAR_ONLY` traps (`src/map.cpp:11318-11348`), and monsters walk
>    onto traps as they move (`src/monmove.cpp:2381`). Without the flag, SCP-087-1
>    chasing you across the seam would fire `EOC_SCP_087_SEAM` **with itself as the
>    alpha talker** and drive `scp_087_loops` — see §8.
> 2. **It re-fires forever.** `trapfunc::eocs` does call `here.remove_trap( p )`
>    (`src/trapfunc.cpp:508`), but `t_scp_087_seam` carries the trap as a *built-in
>    terrain trap*, and `map::tr_at` resolves that from the terrain itself
>    (`src/map.cpp:7065-7068`) while `remove_trap` only clears the submap trap field
>    (`:7185-7196`) — so the call is a no-op and a built-in trap cannot be removed.
>    The loop therefore works, and needs no re-arm mechanism. See §9.

### The pursuer — SCP-087-1

`mon_scp_087_1` (`monsters/scp_087_1.json`), faction `scp_087`, species
`[ "SCP", "SCP_087" ]`. Behavior is split across `scp_087_1_pursuer.json`:

- **Summon** (`EOC_SCP_087_1_SUMMON`) — once, at loop 5. Guarded on
  `scp_087_1_active`.
- **Keeper** (`EOC_SCP_087_1_KEEPER`) — RECURRING every 10 s. Gated at the
  **EOC level** on `scp_087_1_active == 1` so it does nothing when inactive.
  Records the player's anchor + z, tracks the idle grace window, and dispatches
  FOLLOW and CONFINE.
- **Follow** (`EOC_SCP_087_1_FOLLOW`) — per-monster. Re-anchors it to your z if
  you changed level; closes the distance once `scp_087_1_idle >= 18`, i.e. **~3
  minutes**. Read that window carefully: it counts **keeper ticks since the last
  completed loop**, not time since you stopped moving, and only a completed loop or
  a **contact** resets it. Three minutes is roughly the time it takes to walk one
  loop, so it now appears at most about once per loop, and sometimes not at all.
  That is deliberate exposure control (playtest pass 3): at the original 30 seconds
  it closed on the player about once per loop and landed four catches in a single
  descent, and the owner's call is that a pursuer you see that often stops being
  frightening. One unit is 10 seconds (`recurrence: 10`), so **12 = 2 min, 18 = 3
  min, 24 = 4 min** — that number is the whole knob.
- **Confine** (`EOC_SCP_087_1_CONFINE`) — safety net; retires any instance above z0.
- **Retire** (`EOC_SCP_087_1_RETIRE`) — cleanup at ejection.

Damage handling is in `monster_attacks/scp_087_1_attacks.json`:
- `EOC_SCP_087_1_DAMAGE` — **stagger on any hit.** This used to also "full-heal" it,
  which never worked: no JSON primitive can write a monster's HP (that line only
  emitted `invalid body part id "ALL"`). The invulnerability is now the monster's own
  unsurvivable `hp` — see §9.6. **Gated on `SCP_087`** so it can't cross-fire on 173
  (that was a real bug — see §8).
- `EOC_SCP_087_1_CLOSE` — the non-lethal contact payload. It also **resets
  `scp_087_1_idle`**, restarting the grace window: the first playtest logged two
  catches 11 seconds apart, because once the window had lapsed the keeper re-placed
  the monster on the player every tick, so standing up after a knockdown could be
  followed immediately by another one.
- `EOC_SCP_087_SLEEP_INTERRUPT` — wakes you if you sleep inside the shaft.

**Containment** is structural: `t_scp_087_threshold` (the mouth tile, used
**exactly once**) carries `MON_AVOID_STRICT` (`src/monmove.cpp:172-174`), so the
pursuer physically cannot path out. The player walks straight through it. Do not
put `MON_AVOID_STRICT` on the interior stairs — the pursuer must be able to
follow you down.

### Paranoia & crying

`effects_on_condition/scp_087_paranoia_driver.json` holds two RECURRING EOCs,
both gated at the EOC level on `scp_087_loops > 0 && u_val('pos_z') < 0`, plus a
terrain-OR check against the captured position.

> **Why the `pos_z < 0` half matters:** z0 (the surface closet) is floored with
> the *same* `t_scp_087_floor` as the shaft, so a terrain check alone fires while
> you stand in the ordinary closet. `pos_z < 0` pins it to the actual shaft.

Morale uses **`"capped": true`** — load-bearing. Without it, repeated bonuses
stack by root-sum-of-squares and never reach −30 (`src/morale.cpp:195-204`).

> **The step is applied once per loop advance, not once per tick.** The driver
> keeps `scp_087_paranoia_at` (the loop value the morale was last stepped for) and
> only applies `-5 * scp_087_loops` when that differs from `scp_087_loops`. Why:
> with `capped: true` each application *adds* to the point
> (`src/morale.cpp:195-199`), so re-applying every 120 turns made the total scale
> with elapsed time as much as with the loop counter — it pinned to the −99 floor
> in about 40 minutes at loop 1 and the escape model stopped mattering. The ladder
> is now a function of descent alone: −5, −15, −30, −50, −75, floor at loop 6. The
> variable is reset alongside `scp_087_loops` in `EOC_SCP_087_EJECT`.

> **The `scp_087_paranoia` EFFECT lives inside that same guard, and it did not
> always.** It used to sit after the `if`, so it re-applied 4 hours on *every* tick
> while you stood on shaft terrain; durations accumulate on re-application, and the
> first playtest ended with 407,093 turns — 113 hours, or **4.7 in-game days** (my
> own earlier note said "47 days", which was a decimal slip — the second playtest
> measured the fixed figure, below). Nothing was mechanically broken (the effect has
> no `base_mods`; the morale point above is the mechanical half, and it was always
> guarded), but the design intent is "you carry it out of the stairwell for a few
> hours", so it is now stepped once per loop advance like the morale.
> **Measured after that fix** (second end-to-end run, same 13 loops): 191,868 turns =
> ~53 hours, which is 13 × 4 h — exactly once per loop advance, the intended
> behaviour. That confirmed the fix but was *still* not "a few hours", because
> whatever the per-loop duration is, it multiplies by the number of loops.
>
> **So the duration was shortened to 10 minutes** (third pass). A loop takes roughly
> 4 minutes of game time, so 10 minutes keeps the status continuously present for the
> whole descent, while the post-descent stack drops from ~47 h to ~2 h. With the
> ejection's own 6 h that is **~8 h total** — the "a few hours" the design always
> claimed. Only the duration changed; the escalating `min(99, 3 * loops)` intensity
> is untouched, and the effect has nothing mechanical to rebalance (no `base_mods`,
> and nothing in the mod tests for it). The one trade-off: on a long linger the
> status can lapse and re-appear at the next loop — raise it to 30 minutes (total
> ~13 h) if you would rather it never blink. **The mechanical debuff is unaffected by
> any of this** — that is the morale point above, which the ejection sets for 6 hours
> with `capped: true`.
> **If you add anything to this driver, put it inside `scp_087_paranoia_at`'s
> branch unless you have a reason not to.**

### Atmosphere & access

- **The entrance announces itself, in two beats** (`effects_on_condition/scp_087_door.json`):
  - **The knock**, as you walk up to the door — `EOC_SCP_087_DOOR_KNOCK` on
    `avatar_enters_omt`. The door is *one row inside* the overmap tile with a
    passable concrete apron on the boundary row, so stepping onto that apron is an
    OMT transition and the event fires **before** the door is opened. The door
    itself at the boundary would only ever announce itself after you had opened it.
  - **The cry**, on stepping through — `EOC_SCP_087_DOORWAY`, run by the invisible
    `tr_scp_087_doorway` trigger trap on `t_scp_087_door_o`, the **open** door
    variant. There is no event for "a door was opened", so the beat rides the tile.
  Both are once per approach (flags reset at ejection *and* when you walk away), and
  both are pure atmosphere — no effect, no morale, no world sound.
- **Both beats are for ARRIVALS ONLY, and that took two flags to get right**
  (found in the first playtest):
  - `scp_087_just_ejected`, set by `EOC_SCP_087_EJECT` immediately **before** its
    teleport. Being ejected into z0 is an OMT transition, which is exactly what the
    knock listens for, so the log read "You are standing in the closet" followed one
    line later by the knock. The flag lets the knock EOC tell an ejection from an
    approach; it consumes the flag.
  - `scp_087_approach`, armed by the knock EOC on a genuine arrival and consumed by
    the doorway. A tile trap cannot see which way a player crossed, so **direction is
    inferred from the arrival instead**: an outward crossing finds the flag already
    spent and stays silent. Without it the cry fired on the way *out* — the playtest
    heard "the crying starts" from a room it was quitting.
- **Ejection opens the door.** `scp_087_open_door` (`furniture_and_terrain/scp_087_transform.json`)
  turns `t_scp_087_door` into its open variant, aimed at the doorway cell via
  `scp_087_surface` + (x +1, y +20), running after the surface teleport so the
  transform's player-z rule lands on z0. The narration says "the door is open behind
  you", and until this existed that was true only by luck.
- **The door is a real door.** `t_scp_087_door` has an `"open"` link to
  `t_scp_087_door_o` and the `DOOR` flag, and deliberately has **no** `LOCKED`,
  `PICKABLE`, `lockpick_result` or `examine_action`. Two of those omissions are
  load-bearing: without `open`/`DOOR` the `o` key does nothing at all, and
  `OPENCLOSE_INSIDE` would make `map::open_door` refuse because the player is
  *outside* (`src/map.cpp:5291`) — it is the one flag the base game's own
  `t_door_metal_interior_locked` exists to remove. There is also no `prying` or
  `oxytorch` block, so a crowbar or a torch has nothing to act on.
- **Darkness is structural.** Walls are unbreakable (no `bash` block, no
  `MINEABLE`/`DIGGABLE` flags) and there are no lights or windows — the roof is
  solid concrete. A light source is mandatory.
- **Companions are refused, and it is enforced in two stages**
  (`effects_on_condition/scp_087_companion.json`):
  - **The door.** Entering the OMT sets every NPC in range **outside** the
    building — each NPC finds its own `outdoor_only` + `passable_only` tile, and
    the teleport is guarded through the search's `true_eocs` so a failed search
    changes nothing. The narration ("You turn to the others…") is **gated on a
    global flag the per-NPC EOC raises**, so a lone survivor hears nothing: the
    dispatch cannot report whether it found anyone, and "is a follower nearby" is
    not expressible in JSON — following is not an `ally_rule`
    (`src/npc.h:310-328` has no `follow`).
  - **The escort net.** `EOC_SCP_087_COMPANION_ESCORT` is a RECURRING global EOC
    gated at the EOC level on `scp_087_loops > 0 && u_val('pos_z') < 0`. While you
    are below ground it lifts any NPC standing on shaft terrain back up to the
    surface and pushes them outside again, so a follower that walks back in cannot
    stay down there. This is why the refusal is a barrier and not just narration:
    `MON_AVOID_STRICT` is monsters-only (`src/monmove.cpp:172-174`), so **pets**
    are stopped by terrain while **NPCs** need the net.
  Adding a second underground area to this mod? The net's per-NPC terrain test
  (the 6-way `map_terrain_id` OR) is what keeps it from dragging NPCs out of
  unrelated basements — extend it if the shaft ever gains new floor ids.
- **`scp_087_archive`** records that this world has been through 087 once. It is
  read at two points (the seam-turn ambience and the loop-5 reveal) to play the
  `*_again` recurrence cues. It is the hook for "it happens differently the
  second time".

---

## 6. SCP-173 architecture (brief)

Inverted mechanic — because there's no "look away" key (§3), 173 **forces the
observation window open itself**:

- **While seen** → `scp_173_freeze` pins it (`immobilization`, refreshed every
  turn) and rolls a 20%/turn blink chance, applying `blind`. It also sets the
  `scp_173_observed` marker.
- **While blinded/asleep** → `scp_173_lunge` teleports beside you and bashes the
  head.
- **`scp_173_observed` gates invulnerability** (`scp_173_invulnerability.json`):
  damage while observed is undone outright.

> **Do not re-derive sight at damage time.** `Creature::sees` returns true
> unconditionally at range ≤ 1 (`src/creature.cpp:598`), so a fresh check would
> report "observed" even while blinded/asleep — exactly when it matters. The
> marker effect exists precisely to avoid that.

The "only one at a time" sweep is `effects_on_condition/scp_spawn_policy.json`.

---

## 7. How to continue developing

### To add a new SCP (end to end)

1. **Species** — reuse `SCP`; add a narrow child in `species.json` if needed.
2. **Faction** — add to `monster_factions.json`. Give it a `hate` list only, and
   **never** list the faction in its own `by_mood` (see §4 — `emplace` cannot
   overwrite it, so the self-entry silently downgrades friendly-to-self to
   by_mood). `hate` needs no reciprocation.
3. **Monster** — `monsters/<id>.json`. Omit `zombify_into`, `fungalize_into`,
   `burn_into`, `revive_forms` and set `"upgrades": false` (see §8).
4. **Registry** — one line in `monstergroups/scp_registry.json`.
5. **Spawn rate** — low-weight entry in `monstergroups/scp_spawns.json` (if it
   spawns naturally).
6. **Instance cap** — copy the quota sweep EOCs in `scp_spawn_policy.json`,
   change `mtype_ids`.
7. **Mechanics** — EOCs / special attacks / terrain as needed.
8. **Validate** (§10) and update `README.md`.

### To extend SCP-087

Extension points, with where to look:

| Want to… | Touch |
| --- | --- |
| Change how many loops before ejection | `loops >= 13` in `EOC_SCP_087_SEAM` |
| Change the loop-5 threshold | `loops == 5` in `EOC_SCP_087_SEAM` |
| Add a phase (e.g. a beat at loop 9) | add an `if (loops == 9)` branch in `EOC_SCP_087_SEAM`, wire a snippet category |
| Change the shaft geometry | `mapgen/scp_087_mapgen.json` **and** the offsets comment in `scp_087_seam.json` (§5) |
| Retune the grace window | `scp_087_1_idle >= 18` in `EOC_SCP_087_1_FOLLOW` — one unit is 10 s, so 12 = 2 min, 18 = 3 min, 24 = 4 min |
| Change paranoia escalation | the `-5 * scp_087_loops` math in `EOC_SCP_087_PARANOIA` |
| Add pursuer behavior | new EOC + dispatch from `EOC_SCP_087_1_KEEPER` |
| Add prose | new snippet category in `scp_087_snippets.json`, reference with `"snippet": true` |
| Make contact lethal | replace the body of `EOC_SCP_087_1_CLOSE` with one damage call |

### Prose rules

- Every player-facing string is a **snippet** where it can repeat. Do **not** use
  `"same_snippet": true` — it stores on the *beta* talker (`src/npctalk.cpp:5572`),
  which is null for trap and recurring EOCs. See the note in `scp_087_snippets.json`.
- `spawn_message` has **no snippet support** (`src/npctalk.cpp:7623` reads a plain
  `translation`) — use a literal string there.
- Cry/atmosphere is prose, never positional sound (§3).

---

## 8. Gotchas (each cost real debugging time)

**Engine behaviors:**

- **Monsters are transformation-connected by default.** Omit `zombify_into`,
  `fungalize_into`, `burn_into`, `revive_forms` and set `"upgrades": false`, or
  your animate concrete statue becomes a fungal zombie.
- **`Creature::sees` is true at range ≤ 1** regardless of blindness
  (`src/creature.cpp:598`). Any "can it see" check must also exclude
  `blind`/`sleep`.
- **Effect durations decay in `process_turn`, before `move()`**
  (`src/do_turn.cpp:316` vs `:337`). A 1-turn self-freeze expires before the next
  move — use 2 turns.
- **Use `immobilization`, not `stunned`,** to freeze a monster: `stunned` carries
  `NO_SPELLCASTING` and would stop the monster refreshing its own freeze.
- **The blacklist blocks *all* placement,** including EOC spawns (§4).
- **`u_transform_radius` is current-z only** (§3) — aim cross-level with `target_var`.
- **`run_eocs` copies context** to the child dialogue (`src/npctalk.cpp:6584-6585`), so
  a context var set in the parent is visible in the callee. This is how
  `scp_087_arrival` reaches `EOC_SCP_087_EJECT`. `run_eoc_vector` copies too
  (`:501-507`), which is how a guarded `true_eocs` sees the variable its own
  `u_location_variable` just wrote.
- **Traps fire for monsters, not just the player.** `monster::move()` calls
  `here.creature_on_trap( *this )` (`src/monmove.cpp:2381`), and
  `map::maybe_trigger_trap` only filters null, benign and `AVATAR_ONLY` traps
  (`src/map.cpp:11318-11348`). An `action: "eocs"` trap therefore runs its EOC
  **with the triggering creature as alpha** (`src/trapfunc.cpp:504`), so a monster
  can be the actor in an EOC you wrote for the player. Add `AVATAR_ONLY` to any
  trap whose EOC assumes the player. Note `UNDODGEABLE` *removes* the monster's
  only chance to dodge, since it short-circuits the avoidance roll at `:11334`.
- **`range` on a monster special attack only gates when it is greater than 1.**
  `if( range > 1 && !allow_no_target ) { ... rl_dist(...) > range ... }`
  (`src/mattack_actors.cpp:325-330`). `range: 1` is not "melee only" — it is *no
  range check at all*, and `monster::attack_target()` has no adjacency requirement
  (`src/monster.cpp:1643-1659`). Use `2` when you mean contact.
- **`u_location_variable` can fail without writing its output.** It makes 25
  attempts and, on failure, runs `false_eocs` and returns *before* writing
  (`src/npctalk.cpp:4585-4604`). Never hand its output straight to `u_teleport` —
  `read_var_value` returns a default for a missing name (`src/dialogue_helpers.cpp:19-24`)
  and you get an unintended destination. Move the consumer into `true_eocs`.
- **There is no EOC effect for `Character::wake_up()`.** The `wake_up` in
  `src/npctalk.cpp:8630` is an NPC conversation response, not a `u_`/`npc_` EOC
  effect. Waking from an EOC means removing `effect_sleep` by hand — and if you
  do, also remove `lying_down`, which `wake_up()` would have cleared.
- **`avatar_enters_omt` gives you the OMT, not the player.** Its `pos` field is
  `new_abs_omt.raw()` (`src/game.cpp:11522`) — overmap coordinates, useless as a
  teleport target. `oter_id` is the useful part. Note also that z is part of the
  coordinate, so **descending a level is an OMT change** and fires this event with a
  different `oter_id` — gate on the id, not on "we changed OMT".
- **There is no event for "a door was opened".** `event.h` has no door event, and
  `map::open_door` consults nothing but the opener's schizophrenia symptoms. So a
  beat that belongs to the *opening* has to ride the **open variant's tile**
  (a built-in trap, or a check on the tile's terrain) — one step after the handle
  turns. If you need it exactly on the opening, that is an engine change, not JSON.
- **`sound_effect` is the audio hook; sound *ids* come from soundpacks.**
  `{ "sound_effect": "<variant>", "id": "<id>", "volume": N }`
  (`src/npctalk.cpp:5739` → `sfx::play_variant_sound`, `src/sdlsound.cpp:688`) plays
  a soundpack file to the **player only**, creating no world sound — unlike
  `u_make_sound`, which is a real noise monsters hear. An id no soundpack defines is
  skipped **silently** (`src/sdlsound.cpp:704-708`), so a custom id is safe and
  simply stays quiet until someone maps it.
- **`u_hp()` cannot heal a monster, and `u_hp_max('ALL')` is not even a valid id.**
  `hp_eval`/`hp_ass` special-case `'ALL'` (`src/math_parser_diag.cpp:587`, `:597`),
  but `hp_max_eval` does **not** (`:686-691`) — so `u_hp_max('ALL')` reaches
  `generic_factory::convert` and fires `DEBUG : invalid body part id "ALL"`
  (`src/generic_factory.h:541`), and `hp_max` is unassignable anyway (`:1879`).
  Worse, the write half of `u_hp('ALL')` is `set_all_parts_hp_cur`, which walks the
  creature's **body-part map** (`src/creature.cpp:2775-2780`) — while a monster's HP
  is the single `monster::hp` field that `monster::get_hp`/`get_hp_max` read while
  **ignoring the bodypart argument** (`src/monster.cpp:4072-4089`), and `monster`
  does not override `set_part_hp_cur` at all. So **no JSON can restore a monster's
  HP**; the only lever is the monster's own `hp` field at load time. Use
  `u_hp('ALL')` for characters, never for monsters.
- **`looks_like` is cosmetic — and it is how this mod gets tiles.** A grep of `src/`
  finds `looks_like` **only** in the tile drawer (`src/cata_tiles.cpp:1965`, with the
  terrain/furniture/monster cases at `:2018-2026` and the overmap case at `:2033`);
  it never appears in a data loader, so it cannot copy a flag or change behaviour.
  That is exactly why `t_scp_087_stairs_none` can look like a working staircase
  without inheriting `GOES_UP`, and why the seam can look identical to the step
  around it. It is also a *fallback*: if a tileset ever ships art for our own ids,
  that art wins automatically. Two traps when mapping ids: **map tiles and overmap
  tiles come from different tilesets** (here `UltimateCataclysm` for terrain and
  monsters, `Larwick_Overmap` for the overmap), and the overmap target must be a real
  `overmap_terrain` id — `house` and `cabin` do **not** exist as oters, so pointing
  at them resolves to nothing. `copy-from` carries `looks_like` to children, which is
  relied on deliberately in four places (`_landing`/`_seam` inherit the floor's,
  `_stairs_none` the up-stairs', and the nine z-1…z-9 overmap entries the closet's).
- **Mapgen placements are fussier than they look, and every mistake here is fatal.**
  Four rules, all of them learned the hard way while adding the mod's loot:
  1. **An object cannot be items-only.** A mapgen object needs `rows` or a terrain
     map; without one the game fails at load with `format: no terrain map`.
  2. **Coordinate placements go in `place_items`, not `items`.** `items` is the
     *symbol-keyed* form (key = a character in `rows`); handing it an array of x/y
     entries gives `Unread data. Invalid or misplaced field name "items"`.
  3. **`place_items` takes an item GROUP id, not an item id** (`src/mapgen.cpp:2451`,
     `jmapgen_item_group::check`). A single item has to be wrapped in a one-entry
     group to be placed this way.
  4. **One mapgen object serves every oter in its `om_terrain` list.** A placement
     inside layout A appears on z-1, z-3, z-5 *and* z-7. This one is not fatal and is
     the easiest to miss: it is why each loot level in this mod has its own mapgen
     object, and why those objects duplicate layout A's rows verbatim. Change the
     shaft geometry and you must change all of them.
- **`copy-from` copies an identity, not the fields you had in mind.** The field log
  used to `copy-from: paper` purely to inherit weight, volume and material — and
  silently became **`TINDER`** (burnable as fire-lighting tinder, for the one
  guaranteed reward in the mod), claimed `"ammo_type": "paper"`, and filed itself
  under `"category": "spare_parts"`. All three come from the parent's own definition
  (`data/json/items/generic.json:1426-1439`). Before copying from a base item, read
  the *whole* parent; for a simple item, declaring the handful of fields you need is
  five lines and no surprises.
- **`occurrences` means two different things, and the wrong pairing is silently
  fatal.** Without a uniqueness flag it is a *count* — the generator tries to place
  `[min, max]` instances on **every** overmap, which is why the base game's `[0, 2]`
  and `[0, 3]` are ordinary landmarks rather than rarities. With `OVERMAP_UNIQUE` (or
  `GLOBALLY_UNIQUE`) the same pair becomes a **deck** instead: `max` cards, `min` of
  them successes, one card drawn per overmap and reshuffled when it empties
  (`src/overmap.cpp:3296-3314`), so `[1, 12]` means one success per twelve candidate
  overmaps and "Allow exactly one placement attempt on this overmap" (`:3319`) means
  never two in one overmap. **And `OVERMAP_UNIQUE` with `min == 0` erases the special
  from the candidate list outright (`:3285-3288`) — it never spawns anywhere, with no
  error and no warning.** The two uniqueness flags are also not the same thing:
  `OVERMAP_UNIQUE` is one per overmap, `GLOBALLY_UNIQUE` is once per world (`:3276`,
  `:3291`) — and only the latter would fight this mod's recurrence design.

**JSON schema traps:**

- **`special_attacks` needs objects, not strings:** `{ "id": "foo" }`.
- **`u_run_monster_eocs` takes bare string ids.** `{ "id": "X" }` *defines an
  inline EOC* → duplicate-definition error.
- **Talker roles in `u_run_npc_eocs`:** the target NPC is **alpha**, beta is
  **null** (`src/npctalk.cpp:6842`). Use `u_teleport` (alpha), and `message`
  (global) not `u_message` — `f_message` early-returns when the actor is an NPC
  (`src/npctalk.cpp:5542-5544`).
- **`u_z` is not the monster's z.** Use `u_val('pos_z')` (resolves per-talker via
  `f_get_vals`). Bare `u_z` is an undefined character variable → 0.
- **`map_terrain_id` matches ONE id** (`src/condition.cpp:1734`) — OR across
  terrains explicitly.
- **`allow_unlock_doors` is documented but rejected** by the loader
  (`src/monstergenerator.cpp:1196`).

---

## 9. Known open issues / TODOs

1. ~~**The seam trap is consumed on first fire.**~~ **CLOSED — it was never true.**
   `t_scp_087_seam` carries the trap as a *built-in terrain trap*, and
   `map::tr_at` resolves that from the terrain (`src/map.cpp:7065-7068`) while
   `map::remove_trap` only clears the *submap* trap field (`:7185-7196`). The
   `here.remove_trap( p )` in `trapfunc::eocs` (`src/trapfunc.cpp:508`) is
   therefore a no-op for this tile, and a built-in trap cannot be removed at all.
   The seam re-fires on every visit, which is what the loop model needs — no
   re-arm mechanism required. Keeping this item would have cost a lot of time
   designing a fix for a problem that does not exist. **What *was* real here is
   the other half of the same trap: it needed `AVATAR_ONLY`** so that the pursuer
   could not trigger it — see §8.
2. ~~**`EOC_SCP_087_SLEEP_INTERRUPT` gates only on `scp_087_loops > 0`.**~~
   **CLOSED.** It now gates at the **EOC level** on
   `scp_087_loops > 0 && u_val('pos_z') < 0`, matching the paranoia and cry
   drivers, so outside an active descent the body never runs at all.
3. **`scp_087_see` snippet category is defined but unreferenced.** Still open, and
   now documented precisely. The blocker: it cannot simply be wired into the
   hallucination beat, because `spawn_message` accepts a **plain `translation`,
   not a snippet** (`src/npctalk.cpp:7623-7624`), and that beat deliberately only
   speaks when the hallucination was actually *visible* (`:7775-7778` compares
   `visible_spawns`). So the two ways forward are: (a) replace the literal
   `spawn_message` with a preceding `u_message` + `"snippet": true` and accept
   that the line fires even when the shape spawned out of sight, or (b) delete the
   category. This is a content decision, not a defect — the literal currently in
   use is fine.
4. ~~**A companion who is *forced* into the shaft is not auto-returned.**~~
   **CLOSED.** `EOC_SCP_087_COMPANION_ESCORT` now runs while you are below ground
   and lifts any NPC standing on shaft terrain back to the surface and outside, so
   neither a debug teleport nor a follower that walks back down keeps them there.
   It is scoped by the per-NPC terrain test on purpose — an unscoped net would
   drag unrelated NPCs out of basements and subways inside the same reality
   bubble. See §5.
5. **RESOLVED (not a defect in this mod) — the test harness can log submap-load
   errors.** Symptom:
   `tests/cata_test.exe --mods dda,scp_foundation "[monster]"` *can* log
   `ERROR : src/mapbuffer.cpp:386 ... Failed to load submaps from
   test_user_dir/save/Test World 测试世界/maps/0.0.0.zzip, could not open zzip`,
   then `Treating result as failure due to error logged during tests`, while still
   reporting `All tests passed`.
   **What that failure actually is:** `mapbuffer::unserialize_submaps` reaches that
   debugmsg only when `file_exist( zzip_name )` is true *but* `zzip::load` returns
   nullopt (`src/mapbuffer.cpp:374-391`) — a missing file returns false silently.
   `zzip::load` returns nullopt in exactly one place: `mmap_file::map_writeable_file`
   failing (`src/zzip.cpp:355-372`), which on Windows means `CreateFileW` failing
   (`src/mmap_file.cpp:296-340`). The engine already retries three times at 32 ms,
   and the code comment there blames OneDrive or antivirus interfering. That is a
   host-filesystem condition; **no JSON in this mod can influence whether that call
   succeeds**, so there is nothing to "fix" here in data.
   **Evidence gathered:** 4/4 base-game-only runs (`--mods dda`) and **12/12
   consecutive modded runs** after the review fixes were clean — exit 0, no
   `mapbuffer` lines, `All tests passed`; the single historical occurrence came from
   a run that *also* terminated with an access violation (0xC0000005) while every
   clean run exits 0; and it does not reproduce deterministically across six fixed
   `--rng-seed` values (1–6).
   **If you ever see it again:** check the file on disk and re-run before suspecting
   the data — a locked or half-written `.zzip` is the likely cause.
6. **SCP-173 is not actually invulnerable while observed — that mechanism was never
   real (found in playtest).** `EOC_SCP_173_INVULNERABLE_WHILE_SEEN` used
   `u_hp('ALL') = u_hp_max('ALL')` to "undo" a hit. That line fired
   `DEBUG : invalid body part id "ALL"`, and even had the id been right it could not
   have healed anything: the write path touches the creature's body-part map, while a
   monster's HP is the single `monster::hp` field (full proof in §8 and in
   `monster_attacks/scp_087_1_attacks.json` //3b-3d). SCP-087-1 is now made
   unkillable the only way JSON can — an unsurvivable `hp` on the monster itself
   (`monsters/scp_087_1.json` //11) — but **that lever cannot express 173's
   conditional design**: it should be unkillable only while watched and hurtable
   during a blink, and a static number cannot be toggled by an effect. The line has
   been removed (which removes the error) and the narration no longer claims the hit
   was undone, but **173 can currently be damaged while you are looking at it.**
   Options: accept it and drop the "invulnerable" framing; give it an unsurvivable
   `hp` too and lose the blink weakness; or add the missing engine primitive — an
   assignable monster HP (a dialogue-math diag, or an EOC effect that reaches
   `monster::heal( int, bool )`, `src/monster.h:398`).
   **Player-facing impact right now: none** — `disabled_spawning.json` already
   blacklists `mon_scp_173` while its design is unsettled, so it cannot spawn into a
   world at all. This item matters for the day it is switched back on.
7. **Playtest pass 3 — prose and exposure decisions, all settled here.** Three things
   came out of the review round, and all three are closed; recorded so they are not
   re-raised later as defects.
   - **Ambience repetition — fixed by pool size, not by cleverness.** `u_message`
     resolves a *category* through `SNIPPET.random_from_category`
     (`src/npctalk.cpp:5591`), a draw **with replacement**, and the deterministic
     alternative `same_snippet` is unusable here — it stores its pick on the beta
     talker (`:5572`), which our trap and recurring dialogues do not have (the
     header note in `snippets/scp_087_snippets.json` records that). So the three hot
     pools were grown: `scp_087_turn` and `scp_087_turn_again` 4 → 8 lines,
     `scp_087_cry` 5 → 8. Only those three needed it — the turn pools fire once *per
     loop* (thirteen draws in a descent) and the cry pool fires several times; the
     knock, doorway, phase and see categories fire once per visit or once per run.
   - **The ejection's "and it is light" — reworded.** The closet has no light of its
     own (no windows, solid roof), so the claim only held by day. It now reads "the
     world is out there, crawling with things that are not the stairwell", which is
     true whenever you are ejected.
   - **The sleep interrupt — kept as insurance, deliberately not broadened.** See
     `EOC_SCP_087_SLEEP_INTERRUPT` //4 in `monster_attacks/scp_087_1_attacks.json`:
     it needs you asleep *inside the shaft* with the descent live, which neither
     end-to-end run did, and widening it to any sleep during a descent would fire it
     in the ordinary surface closet — the exact false positive the B4 fix removed. It
     stays as the guarantee that nobody is killed in their sleep down there, and is
     accepted as a beat most players will never see.
   - **Exposure, not difficulty:** the same pass raised 087-1's grace window from 30
     seconds to ~3 minutes so it reads as a rare sighting — see `EOC_SCP_087_1_FOLLOW`
     in §5.

---

## 10. Validating changes

There is **no compile step** — it's all JSON. Validate by loading the mods. From
the repo root (MSYS2 DLLs must be on `PATH`):

```sh
PATH="/c/msys64/mingw64/bin:$PATH" ./cataclysm-tiles.exe --check-mods dda scp_foundation
PATH="/c/msys64/mingw64/bin:$PATH" ./tests/cata_test.exe --mods dda,scp_foundation "[monster]"
```

Both must report **no `Json error`, no `ERROR`, and no `debugmsg`** lines
(watch specifically for `invalid species` and snippet errors — those are easy to
miss). The test binary is the more thorough check: it actually loads and
finalizes all the data.

> `cataclysm-tiles.exe` **segfaults on exit** *after* printing its results. This
> reproduces with no mods loaded and is unrelated to this mod. Don't chase it.

Useful greps while working:

```sh
# no stray bare u_z in the JSON (should be empty)
grep -rn --include='*.json' "u_z" data/mods/scp_foundation/ | grep -v pos_z
# every EOC id referenced is also defined somewhere
grep -rho '"EOC_[A-Z0-9_]*"' data/mods/scp_foundation/ | tr -d '"' | sort -u > /tmp/ref.txt
grep -rhoE '"id": *"EOC_[A-Z0-9_]*"' data/mods/scp_foundation/ | sed -E 's/.*"(EOC_[A-Z0-9_]*)".*/\1/' | sort -u > /tmp/def.txt
comm -23 /tmp/ref.txt /tmp/def.txt   # referenced-but-undefined ids; should be empty
```

---

## 11. Attribution

Content is derived from [SCP-173](https://scp-wiki.wikidot.com/scp-173) and
[SCP-087](https://scp-wiki.wikidot.com/scp-087), both licensed **CC BY-SA 3.0**.
See [`README.md`](README.md) for full credits. This mod is for personal use and
is not redistributed.
