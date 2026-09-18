# SCP-087 Code Review

Scope: the SCP-087 implementation in `data/mods/scp_foundation/` (16 files), reviewed
against the base-game C++ in `Cataclysm-DDA/src/`. Every claim below was checked against
source; nothing is inferred from the mod's own comments. Where the comments' cited line
numbers have drifted, the true location is given.

**Verdict:** the architecture is sound and most of the load-bearing engine assumptions are
correct. There is **one high-severity bug**, one medium-severity misconfiguration, and
several documentation claims that are wrong in both directions (one "known open issue" is
a false alarm; several cited engine facts are stale).

---

## 1. Summary table

| # | Severity | Finding |
| --- | --- | --- |
| **B1** | **High** | The seam trap fires for SCP-087-1 itself, so the pursuer can drive and reset `scp_087_loops`. |
| **B2** | **Medium-High** | `"range": 1` on `scp_087_1_close` does *not* gate it to melee — the contact payload can fire at range. |
| **B3** | **Medium** | `EOC_SCP_087_COMPANION_GATE` teleports *all* nearby NPCs **into** the closet; it does not "return them to where you came from". |
| **B4** | **Medium-Low** | `EOC_SCP_087_SLEEP_INTERRUPT` uses `u_lose_effect: "sleep"` instead of the engine's wake path, and is the one recurring EOC with **no** EOC-level gate. |
| **B5** | **Medium-Low** | The stagger teleport in `EOC_SCP_087_1_DAMAGE` is unguarded: if `u_location_variable` finds no passable tile, the teleport still runs on an **unset** variable. |
| **B6** | **Low** | `"by_mood": [ <self> ]` on both factions silently overrides the engine's friendly-to-self default. |
| **B7** | **Low** | Documentation accuracy: §9.1 is a false alarm; several cited line numbers/facts are wrong. |

| # | Type | Note |
| --- | --- | --- |
| **N1** | Tuning | Paranoia morale accumulates with *time*, not just loops, so it pins to −99 in ~40 min at any loop count. |
| **N2** | Design | `KEEP_DISTANCE` makes 087-1 *flee* when within 4 tiles, so "closing the distance" is immediately undone. |

**One headline correction in your favour:** DEVELOPING.md §9.1 — *"the seam trap is consumed
on first fire"* — is **not true**. See §3.7. The loop mechanic works as designed.

---

## 2. Method

Source-verified against these files (all paths relative to `Cataclysm-DDA/`):
`src/trapfunc.cpp`, `src/map.cpp`, `src/mapgen.cpp`, `src/monmove.cpp`, `src/monster.cpp`,
`src/mattack_actors.cpp`, `src/npctalk.cpp`, `src/dialogue_helpers.cpp`, `src/teleport.cpp`,
`src/effect_on_condition.cpp`, `src/morale.cpp`, `src/monfaction.cpp`,
`src/overmap_terrain.cpp`, `src/overmap_special.cpp`, `src/math_parser_diag.cpp`,
`src/creature_tracker.cpp`, `src/game.cpp`, `src/character_health.cpp`;
plus `data/json/**` precedents (`traps.json`, `highland_dimension.json`, `special_locations.json`,
`connect_groups.json`, `vision_levels.json`), `doc/JSON/*.md`, and
`tests/monfactions_test.cpp`.

Geometry was verified by indexing the mapgen row strings character by character rather than
trusting the prose.

---

## 3. Findings

### 3.1 B1 (High) — the seam trap fires for SCP-087-1, corrupting the loop counter

**What happens.** `tr_scp_087_seam` is triggerable by *any* creature. The pursuer walks onto
it, and `EOC_SCP_087_SEAM` then runs with **SCP-087-1 as the alpha talker** — so the monster
increments the player's loop counter, and at the thresholds it can run the ejection logic.

**Evidence**

1. `map::maybe_trigger_trap` (`src/map.cpp:11318-11348`) filters only null, benign, and
   `AVATAR_ONLY` traps:
   ```cpp
   if( tr.is_benign() ) { return; }                                        // :11326
   if( tr.has_flag( json_flag_AVATAR_ONLY ) && !c.is_avatar() ) { return; } // :11330-11332
   if( !tr.has_flag( json_flag_UNDODGEABLE ) && may_avoid && c.avoid_trap( pos, tr ) ) { ... } // :11334
   tr.trigger( c.pos_bub(), c );                                           // :11347
   ```
   `tr_scp_087_seam` is `benign: false` and has **no `AVATAR_ONLY`** → it triggers for monsters.
   Because it *is* `UNDODGEABLE`, the avoidance roll at `:11334` is skipped for every creature —
   the flag you added for the player also removes the monster's only chance to dodge.
2. Monsters trigger traps as they move — `monmove.cpp:2381: here.creature_on_trap( *this );`
   inside `monster::move()` (declared `monmove.cpp:976`).
3. `trapfunc::eocs` builds the dialogue with the triggering creature as **alpha**
   (`src/trapfunc.cpp:494-508`):
   ```cpp
   dialogue d( get_talker_for( critter ), nullptr );
   write_var_value( var_type::context, "trap_location", &d, trap_location );
   eoc->activate_activation_only( d, "activation", "item's involvement", "item" );
   ...
   here.remove_trap( p );
   ```
   So in `EOC_SCP_087_SEAM`, `u_` is the monster:
   - `{ "math": [ "scp_087_loops += 1" ] }` — **a global counter advanced by the monster's step**;
   - `{ "u_teleport": { "context_val": "scp_087_arrival" } }` — moves the **monster** to z-1;
   - `scp_087_loops == 5` → `EOC_SCP_087_PHASE5` runs with the monster as actor. It summons via
     `EOC_SCP_087_1_SUMMON` (guarded on `scp_087_1_active`, so no duplicate) and transforms the
     terrain around the monster's own position (which it just teleported to z-1 `(11,2)`, so the
     exit still gets sealed — by accident);
   - `scp_087_loops >= 13` → `EOC_SCP_087_EJECT` runs with the monster as actor:
     `u_teleport` moves the **monster** to the surface, `EOC_SCP_087_1_RETIRE` sweeps by mtype
     so the pursuer is genuinely deleted, and the globals are reset
     (`scp_087_archive = 1`, `scp_087_loops = 0`, `scp_087_1_active = 0`) — **while the player is
     still inside the shaft**. The exit is restored, so the player can climb out, but the
     encounter has silently de-synced from its own state.

**Impact.** The player can reach the loop-5 point of no return, or the loop-13 ejection, without
having descended that many times. In the mild case the counter merely drifts and the pacing of
the three phases is wrong; in the severe case the pursuer vanishes and the shaft resets mid-run.
The pursuer is actively herded down the shaft by `EOC_SCP_087_1_FOLLOW`, and the seam sits on
the landing at `(11,20)` — the natural path — so this is likely to occur in normal play, not an
edge case.

**Fix (one word, plus belt-and-braces).** `AVATAR_ONLY` is a real, documented trap flag —
`doc/JSON/JSON_FLAGS.md:1658` ("Only the player character will trigger this trap"),
`doc/JSON/JSON_INFO.md:2743` — and the base game already uses it on an `action: "eocs"` trap:
`data/json/effects_on_condition/nether_eocs/highland_dimension.json:110` →
`"flags": [ "UNDODGEABLE", "AVATAR_ONLY" ]`, exactly the pattern you want.

1. Add `"AVATAR_ONLY"` to `traps/scp_087_traps.json` → `tr_scp_087_seam.flags`.
2. Defence in depth: add `"condition": "u_is_avatar"` to `EOC_SCP_087_SEAM`. The condition
   exists (`src/condition.cpp:2631`, `{"u_is_avatar", "npc_is_avatar", &conditional_fun::f_is_avatar}`)
   and is evaluated before the effect body, so it costs nothing when false.

---

### 3.2 B2 (Medium-High) — `"range": 1` does not gate the contact attack to melee

**The comment says** (`monster_attacks/scp_087_1_attacks.json:3`): *"It fires as a monster special
attack when 087-1 reaches adjacency (range 1)."* That is not what the engine does.

**Evidence.** `mon_eoc_actor::call` (`src/mattack_actors.cpp:301-340`):
```cpp
if( range > 1 && !allow_no_target ) {                     // :325
    if( !mon.sees( here, *target ) ||
        rl_dist( mon.pos_bub(), target->pos_bub() ) > range ) {
        return false;
    }
}
```
With `range == 1` the entire distance/sight gate is **skipped**. The only remaining gates are
`mon.can_act()`, a non-null `mon.attack_target()`, and the `condition`. And
`monster::attack_target()` (`src/monster.cpp:1643-1659`) requires only that the monster has a
destination, that a creature sits on that tile, and that it `sees()` it — **no adjacency**:
```cpp
if( target == nullptr || target == this ||
    attitude_to( *target ) == Attitude::FRIENDLY || !sees( here,  *target ) || ...
```
`sees()` short-circuits true for an `ALWAYS_SEES_YOU` monster tracking the avatar
(`src/creature.cpp:526-529`), bounded only by z-proximity (`:517`) and `MAX_VIEW_DISTANCE`
(`:521-524`). `monster::move()` runs the special-attack loop (`src/monmove.cpp:1022-1054`)
*before* any movement, gated only on `cooldown == 0 && !pacified && !is_hallucination()`.

**Impact.** `EOC_SCP_087_1_CLOSE` — −99 morale, stamina ÷ 4, `downed`, `scp_087_contact` — can be
delivered from any distance on the same z-level where the monster's current destination tile is
the player's tile. The "contact" beat becomes a ranged, repeating debuff (cooldown 1 turn), and
the one attack the design reserved for "it is at your throat" is no longer positional. This also
partly masks N2 below.

**Fix.** `"range": 2` enables the gate (`rl_dist > range` → fail), which yields adjacency
including diagonals. (`range: 1` cannot be used to mean melee — it means "no gate".) If you want
strict 4-way adjacency, add an explicit distance condition instead.

---

### 3.3 B3 (Medium) — the companion gate does the opposite of what it says

**The comment says** (`effects_on_condition/scp_087_companion.json:5`): *"a follower who was
brought to the door is returned to where you came from rather than walking into the shaft."*

**What the code does.** `{ "u_location_variable": { "global_val": "scp_087_exit_here" } }`
captures the **avatar's current position** (`src/npctalk.cpp:4475-4477`:
`talker_pos = target->pos_abs()`), i.e. a tile *inside* the closet. Then `u_run_npc_eocs` with
`npc_range: 20, npc_must_see: false` teleports **every** qualifying NPC to that same point
(`:6842-6845`). The `npc_range`-without-z-filter path additionally requires the same z
(`:6834`), so it is at least confined to the surface floor — but:

- NPCs are relocated **into** the closet, not out of the building or back along your route;
- *all* NPCs within 20 tiles on that floor are affected, including uninvolved bystanders who
  were never following you;
- an NPC dropped next to you is then permanently barred from the shaft by
  `MON_AVOID_STRICT` (correct, but it means they stand in the closet from then on);
- it re-fires on **every** entry to the OMT (`"required_event": "avatar_enters_omt"`).

**Impact.** Medium — narratively the refusal beat reads as "they refuse at the door", but the
mechanical effect is "everyone nearby is yanked to you and trapped in the room". In a populated
area this is visible and odd.

**Fix.** Store a *previous* surface location and teleport NPCs there, or push them away from the
door rather than onto the player (e.g. a location variable offset outward), and narrow the
selection to your actual allies/followers rather than any NPC in range.

---

### 3.4 B4 (Medium-Low) — the sleep interrupt doesn't use the engine's wake path, and has no L3 gate

**Evidence.** `EOC_SCP_087_SLEEP_INTERRUPT` (`monster_attacks/scp_087_1_attacks.json`) uses
`{ "u_lose_effect": "sleep" }`. The canonical wake path is `Character::wake_up()`
(`src/character_health.cpp:2444-2470`), which does considerably more:
- it **bails out entirely** if the character has `effect_narcosis` (`:2447-2449`) — a component of
  your "you get the reveal, never a silent death" promise that `remove_effect` bypasses;
- it defers the sleep-effect removal by setting the duration to 0 rather than removing it inline
  (`:2456-2457`, deliberately, to avoid invalidating an effect iterator mid-`process_effects`);
- it removes `effect_lying_down`, clears the alarm effect, calls `recalc_sight_limits()`, and
  forces the movement mode out of prone (`:2460-2469`).

So a player who lay down to sleep, or who is under anaesthesia, may not be cleanly woken, and
sight/movement-mode state is left stale.

**Also** (this is your DEVELOPING.md §9.2, confirmed): this EOC is the **only** recurring EOC with
no EOC-level `condition`. Every other one (`EOC_SCP_087_1_KEEPER`, `EOC_SCP_087_PARANOIA`,
`EOC_SCP_087_CRY`) correctly gates at the EOC level so idles cost nothing
(`src/effect_on_condition.cpp:110-112` + the dispatch in `:275-303`). This one evaluates
`scp_087_loops > 0` and `u_has_effect("sleep")` every 60 turns for the entire game.

**Fix.** Add the same gate used elsewhere —
`"condition": { "and": [ { "math": [ "scp_087_loops > 0" ] }, { "math": [ "u_val('pos_z') < 0" ] } ] }` —
and consider whether an effect exists that routes through `wake_up()`; if not, at minimum also
remove `lying_down` so the player is not left prone in the dark.

---

### 3.5 B5 (Medium-Low) — unguarded teleport after a failed `u_location_variable`

**Evidence.** In `EOC_SCP_087_1_DAMAGE` (`monster_attacks/scp_087_1_attacks.json`):
```json
{ "u_location_variable": { "context_val": "scp_087_1_stagger" }, "min_radius": 2, "max_radius": 3, "passable_only": true },
{ "u_teleport": { "context_val": "scp_087_1_stagger" }, "force_safe": true }
```
`f_location_variable` (`src/npctalk.cpp:4585-4604`) tries **25** random points and, if none
satisfies `passable_only` within `[min_radius, max_radius]`, runs `false_eocs` and **returns
without writing the variable**:
```cpp
if( !found ) {
    run_eoc_vector( false_eocs, d );
    return;
}
```
The mod supplies no `false_eocs`, so the variable can remain unwritten — and the following
`u_teleport` is unconditional. `read_var_value` on a missing name returns a static default
(`src/dialogue_helpers.cpp:19-24`), which for a location resolves to the zero position, and
`f_teleport` then hands that to `teleport::teleport_to_point` (`src/npctalk.cpp:8160-8164`).

**Impact.** Low-Medium: when it triggers, the stagger intends to nudge 087-1 2–3 tiles but instead
acts on an unset location — the monster is displaced to an unintended place rather than staying
put. I verified the failure path and the missing guard; I did **not** pin down the exact runtime
symptom (relocation to the map origin vs. a debugmsg), because that depends on how a
default-constructed `diag_value` degrades — worth one in-game confirmation.

**Fix.** Make the teleport conditional on the search succeeding, e.g. move it into the
`u_location_variable`'s `true_eocs`, or give the `u_location_variable` a `false_eocs` that skips
the teleport. (Note `u_teleport` with `force_safe: true` already handles the *occupied* case —
that part is fine.)

---

### 3.6 B6 (Low) — faction self-`by_mood` overrides friendly-to-self

**Evidence.** `monfaction::load` (`src/monfaction.cpp:185-209`) fills the attitude map from the
JSON lists and then:
```cpp
for( const mfaction_str_id &mfac : _att_by_mood ) { attitude_map[mfac] = MFA_BY_MOOD; } // :194-196
...
// by default faction is friendly to itself (don't overwrite if explicitly specified)
attitude_map.emplace( id, MFA_FRIENDLY );                                                  // :208
```
`emplace` does **not** overwrite an existing key. Because both factions list *themselves* in
`by_mood` (`monster_factions.json`), the self-attitude is `MFA_BY_MOOD`, not the engine's
`MFA_FRIENDLY`. This also contradicts the comment's own rationale: the reciprocity test you cite
(`tests/monfactions_test.cpp:64-94`) only complains about `friendly`/`neutral`, so `hate` needs no
reciprocation — but neither does anything else here, making the self-`by_mood` entry
unnecessary *and* harmful.

**Impact.** Low for 087-1 (one instance is intended at a time), but it is the documented pattern in
DEVELOPING.md §4 and will be copied by every new SCP: two same-faction anomalies would treat each
other by mood rather than as allies, and could brawl.

**Fix.** Delete `"by_mood": [ "foundation" ]` and `"by_mood": [ "scp_087" ]`; leave the factions
with only their `hate` lists. Fix the DEVELOPING.md line that recommends "only `by_mood` toward
itself".

---

### 3.7 B7 (Low) — documentation accuracy

Worth fixing because these comments are the mod's own design contract, and two of them are
load-bearing mistakes.

**False alarm (correct it in the other direction):**

- **DEVELOPING.md §9.1 / `scp_087_traps.json` and §5 all claim the seam trap is consumed on first
  fire, "load-bearing for the loop model". It is not.** The trap is a **built-in terrain trap**
  (`t_scp_087_seam` → `"trap": "tr_scp_087_seam"`, loaded at `src/mapdata.cpp:1368`, resolved at
  `:1131-1135`, registered into `traplocs` on `ter_set` at `src/map.cpp:2589-2591`).
  `map::tr_at` returns the terrain's trap *first* (`src/map.cpp:7065-7068`), while
  `map::remove_trap` only clears the **submap** trap field (`:7185-7196`, `tid = current_submap->get_trap(l)`
  → `tr_null` for a built-in trap → the whole block is skipped). So `trapfunc::eocs`'s
  `here.remove_trap( p )` (`src/trapfunc.cpp:508`) is a **no-op** here, and a built-in trap cannot
  be removed. The seam re-fires on every step, which is what the loop design requires.
  This also survives `ERASE_ALL_BEFORE_PLACING_TERRAIN` — that flag calls `remove_trap` too
  (`src/mapgen.cpp:3002`, `:3076-3077`), with the same no-op result.

**Stale or wrong citations / claims:**

| Claim | Reality |
| --- | --- |
| `validate_species_ids` debugmsg at `src/monstergenerator.cpp:1412` (species.json `//`, DEVELOPING.md §4) | It is at **`src/monstergenerator.cpp:726`**, called at `:372`. Behavior claim correct. |
| `"range": 1` = adjacency (B2) | Wrong — see 3.2. |
| `MAN_MADE` "marks it as a constructed place" (overmap_special) | The flag is documented but **not referenced anywhere in `src/`**; inert in core. Harmless, but it buys nothing. |
| `NOT_HALLUCINATION` on `mon_scp_087_1` with the comment implying it guards the spawn | The flag is only consulted when building the **random** hallucination pool (`src/monstergenerator.cpp:480-484`, used by `get_valid_hallucination()` at `:1265-1268`). The explicit `hallucination_count` path ignores it entirely (`src/npctalk.cpp:7703-7731` → `game::spawn_hallucination(p, mt, lifespan)` at `src/game.cpp:4709-4729`). The hallucination layer therefore works — the flag is just not doing what the comment implies. |
| `tracking_distance` default (per `doc/JSON/MONSTERS.md:135`: 3) | Code default is **8**, min 3 (`src/monstergenerator.cpp:904-905`). The mod sets 4 explicitly, so no behavior impact — but don't rely on the doc. |
| `"sound": true` on the cry, with the comment's "never positional sound" reasoning | `sound` is a **display gate**, not a sound event: it suppresses the message when the target is asleep or deaf (`src/npctalk.cpp:5602-5615`). It does *not* create a world noise, so the "no horde attraction" claim holds — but for a different reason than stated. Side effect worth knowing: a **deaf** player never hears the crying. |
| `same_snippet` storing on the beta talker, `src/npctalk.cpp:5572` | Correct (`:5572-5577`), and the `f_message` NPC early-return you cite at `:5536-5538` is correct (`:5542-5544`). Good catch, correctly avoided. |
| `u_run_npc_eocs` roles, `src/npctalk.cpp:6842` | Correct: `dialogue newDialog( get_talker_for( target ), nullptr, ... )` — target is alpha, **beta is null** (`:6842`). Your `u_teleport` + global `message` are the right choices. |
| `run_eocs` copies context, `src/npctalk.cpp:6583` | Correct, at **`:6584-6585`**: `dialogue newDialog{ ..., d.get_conditionals(), d.get_context() }`. `get_talker` also clones the parent's alpha when no `alpha_talker`/`alpha_loc` is given (`:6480-6485`), which is why the bare `{ "run_eocs": [...] }` form works here. |
| morales, `src/morale.cpp:195-204` | Correct. `capped` → `get_net_bonus() + new_bonus` (linear, `:199`); uncapped → root-sum-of-squares (`:200-204`). `normalize_bonus` (`:237-241`) clamps the point to `max_bonus`. |

---

## 4. Design notes (not bugs)

### N1 — paranoia accumulates with time, not with loops

The morale point is a single accumulating instance (`src/morale.cpp:328-360` finds the existing
matching point and calls `m.add(...)`), and `capped: true` makes each application *add* rather
than root-sum-square. The driver re-applies `bonus = -5 * scp_087_loops` every 120 turns with a
4-hour duration, and `morale_point::add` **resets the duration on every application**
(`src/morale.cpp:184-190`, `new_cap` → `duration = new_duration`).

So the total grows as `-5 × loops × applications`: at `loops == 1` it reaches the −99 floor after
~20 applications (~40 minutes) and then stops mattering. The intended "escalates with the loop
counter" reading is therefore only visible in the first half-hour, whatever the player does. If
the escalation is meant to track *descent*, drive the bonus from the loop counter without letting
the application count dominate — e.g. re-apply less often, or set the bonus so the loop term is
the only meaningful axis.

### N2 — `KEEP_DISTANCE` fights the "closes the distance" beat

`monster::attitude()` returns `MATT_FLEE` when a `KEEP_DISTANCE` monster is *inside*
`tracking_distance` (`src/monster.cpp:1916-1919`), and `monster::move()` sets `moves = 0` and
returns early when a `KEEP_DISTANCE` monster is within `tracking_distance` and not already
fleeing (`src/monmove.cpp:1143-1151`). With `tracking_distance: 4`, `EOC_SCP_087_1_FOLLOW`'s
"teleport adjacent once idle" is therefore followed by the monster immediately backing away to
~4 tiles. The pacing works; the *approach* does not stick. Worth a playtest to decide whether
that is the intended "it is always just behind you, never arriving" — if so, the FOLLOW
teleport's own comment ("ie it closes") oversells it.

---

## 5. Verified-correct register (do not "fix" these)

Independent of the findings above, these assumptions all check out. Recorded so a future
refactor doesn't churn them.

**Geometry and coordinates**

- `z_adjust` operates on **absolute z**: `get_abs`/`get_bub` pass z through unchanged
  (`src/map.cpp:10855-10863`), and bub-z *is* the absolute z-level. So the seam's `+8 / −18` from
  z-9 `(11,20)` yields z-1 `(11,2)`, exactly as documented. `u_val('pos_z') < 0` is a valid
  "below the surface" test.
- Row-string indexing confirms z0's threshold at `(11,2)`, layout A `<`=11,2 `>`=11,20, layout B
  mirrored, z-9's seam `Q` at `(11,20)` — so the stair chaining holds. `game::find_stairs`
  (`src/game.cpp:9950-9960`) prefers the same x,y tile and only then searches the OMT rectangle.
- **Cross-z teleport works.** `map::inbounds` accepts any z in `[-OVERMAP_DEPTH, OVERMAP_HEIGHT]`
  regardless of the loaded level (`src/map.cpp:10235-10244`), and the map stores every z-level
  (submap index `indexz + (x + y*MAPSIZE) * OVERMAP_LAYERS`, `src/map.cpp:10959-10961`). So the
  z-9 → z-1 hop and the z-9 → z0 ejection take the in-bubble fast path.
- `map::transform_radius` (`src/map.cpp:5387-5399`) transforms on the **target location's** z
  (`get_bub(p)`), so `u_transform_radius` with `target_var: scp_087_arrival` really does restore
  z-1's up-stairs while the player is still at z-9. `target_var` is honored
  (`src/npctalk.cpp:4686-4687`). Radius 12 also stays inside the 132×132 bubble for the shaft's
  central OMT, so no `"transform_radius called for area out of bounds"` debugmsg.
- `trap_location` is written as a true location (`tripoint_abs_ms`, `src/trapfunc.cpp:502-505`)
  and read by `f_location_variable_adjust` via `read_var_value(...).tripoint()`
  (`src/npctalk.cpp:4649`), with `output_var` supported (`:4646`). The `EOC_TELEPAD` precedent
  you cite is real.
- Non-rotatable overmap terrains register with the **bare** id (`oter_t(*this)`,
  `src/overmap_terrain.cpp:694-695` + `:748-752`), and `get_mapgen_id()` returns the bare type id
  (`:770-775`). So `NO_ROTATE` is genuinely load-bearing: it is what makes `scp_087_z0` correct
  both as the special's `overmap` value and as the `avatar_enters_omt` `oter_id` in the companion
  gate's `compare_string`. The flag and the id form are consistent.

**EOC / talker / math semantics**

- EOC-level `condition` is parsed separately from `effect` and evaluated before the body
  (`src/effect_on_condition.cpp:110-114`), so the three gates you added are real optimizations.
- `z_min`/`z_max` on `u_run_monster_eocs` compare the monster's absolute `posz()`
  (`src/npctalk.cpp:6917-6918`) — the keeper's per-level dispatch and the `-10..10` "all levels"
  sweep are both correct.
- `u_val('pos_z')` is implemented for **monsters** (`talker_monster.h:41`,
  `talker_monster.cpp:38-41` → `Creature::posz()`, absolute), so FOLLOW's z-anchor and CONFINE's
  `pos_z > 0` test are valid. (The DEVELOPING.md warning against bare `u_z` is well founded.)
- In a `monster_attack` EOC, alpha is the **monster** and beta the **victim**
  (`src/mattack_actors.cpp:316-318`, `:332-338`), so `n_val('stamina')`, `npc_message` and
  `npc_add_effect` / `npc_add_morale` in `EOC_SCP_087_1_CLOSE` target the player correctly.
- `monster_takes_damage` sends alpha = the damaged monster, beta = the source
  (`src/monster.cpp:2519-2524`, `send_with_talker`). When damage has **no** source the event
  carries no talkers and the alpha falls back to the avatar — which is exactly why the
  `{ "and": [ "has_alpha", { "u_has_species": "SCP_087" }, "u_is_monster" ] }` guard on
  `EOC_SCP_087_1_DAMAGE` is necessary and correct. Good defensive writing.
- `u_add_morale.bonus` accepts a math expression (`get_dbl_or_var`, `src/npctalk.cpp:7406`).
- ~~`u_hp('ALL')` / `u_hp_max('ALL')` are valid and assignable~~ — **WRONG, corrected
  during playtest.** `u_hp('ALL')` is valid and assignable (`hp_eval`/`hp_ass`
  special-case the string, `src/math_parser_diag.cpp:587`, `:597`), but
  **`u_hp_max('ALL')` is not**: `hp_max_eval` builds the bodypart straight from the
  string with no special case (`:686-691`), so it hits
  `generic_factory::convert` and fires `DEBUG : invalid body part id "ALL"`
  (`src/generic_factory.h:541`). And for a **monster** neither half is useful: the
  write path is `set_all_parts_hp_cur` (body-part map) while a monster's HP is the
  single `monster::hp` field, which `monster::get_hp`/`get_hp_max` read while
  ignoring the part argument (`src/monster.cpp:4072-4089`). The claim was recorded
  from the docs rather than from the code — exactly the failure mode this review was
  supposed to avoid. See the appendix.
- `u_die` accepts `remove_corpse` and `supress_message` (misspelling intentional)
  (`src/npctalk.cpp:5928-5940`).
- `u_location_variable` accepts `passable_only` (`src/npctalk.cpp:4415`).
- `u_spawn_monster`'s `real_count` and `hallucination_count` both default to **0**
  (`src/npctalk.cpp:7594-7595`), so `EOC_SCP_087_PARANOIA`'s `hallucination_count: 1` spawns a
  hallucination and **not** an extra real pursuer. Placement goes through
  `find_nearby_spawn_point`, which requires a passable, floored, unoccupied tile
  (`src/game.cpp:4626-4630` → `can_place_monster` → `will_move_to`), so 087-1 cannot spawn inside
  the shaft walls even though radius 6–14 overlaps them.
- `avatar_enters_omt` carries `oter_id` (`src/event.h:230-236`) — the right variable name.

**Monsters, terrain, factions**

- Every monster flag used is real: `SEES`, `ALWAYS_SEES_YOU`, `KEEP_DISTANCE`, `STUN_IMMUNE`,
  `NEVER_WANDER`, `NO_BREATHE`, `NOHEAD`, `NOGIB`, `NOT_HALLUCINATION`
  (`src/mtype.h:87,153,161,163-166,208`; `src/mtype.cpp:185-304`).
- `"harvest": "exempt"` is a real harvest list (`data/json/harvest.json:7-11`).
- `MON_AVOID_STRICT` is checked in `monster::will_move_to` (`src/monmove.cpp:172-174`) — monsters
  only, never the player. The threshold containment works for *walking*. (Caveat: it is not
  consulted by `teleport::teleport_to_point`, so a teleport — including `u_teleport` — can place a
  monster on that tile; nothing in the current design does so from below.)
- All terrain flags/fields parse, and `"WALL"`/`"INDOORFLOOR"` are connect **group** ids, not
  terrain ids (`data/json/connect_groups.json:4`, `:140`). `examine_action: "locked_object"` is
  real (`src/iexamine.cpp:7536`).
- The multi-z overmap special pattern is correct, including `occurrences` being mandatory
  (`src/overmap_special.cpp:344`), the placement origin being z=0 (`src/overmap.cpp:3154-3155`),
  `locations: ["land"]` (`data/json/overmap/special_locations.json:37-41`) and `SAFE_AT_WORLDGEN`
  being actually consumed (`src/overmap.cpp:3000`, `:3055`). `MAN_MADE` is inert (see B7).
- `overmap_terrain` `see_cost: "full_high"` and `vision_levels: "underground_dirt"` are valid
  literals (`src/overmap_terrain.cpp:582-586`, `:617/654-655`;
  `data/json/overmap/vision_levels.json:183-188`), as are `NO_ROTATE`/`KNOWN_UP`/`KNOWN_DOWN`
  (`src/overmap_terrain.cpp:378-387`).
- `map_terrain_id` matches one id per use and `loc` is effectively mandatory
  (`src/condition.cpp:1725-1745`) — the six-way OR is the right workaround. `u_has_species` exists
  and reads the actor's species list (`src/condition.cpp:518-526`).
- Species `copy-from` works and an unknown species debugmsgs (`src/monstergenerator.cpp:726`), so
  the `SCP_087` child species is both valid and correctly motivated.
- The `MONSTER_BLACKLIST` warning is right: enforcement is at `creature_tracker::add`
  (`src/creature_tracker.cpp:93-95`), which every path goes through, including
  `u_spawn_monster` (`src/npctalk.cpp:7743-7746` → `game::place_critter_at` at
  `src/game.cpp:4492-4499`) and hallucinations (`src/game.cpp:4725`). Not blacklisting 087-1 is
  correct.
- `ERASE_ALL_BEFORE_PLACING_TERRAIN` is real and preserves built-in traps
  (`src/mapgen.cpp:3000-3004`, `:3076-3077`).

---

## 6. Remediation plan (recommended order)

1. **B1** — add `"AVATAR_ONLY"` to `tr_scp_087_seam.flags`; add `"condition": "u_is_avatar"` to
   `EOC_SCP_087_SEAM`. *(One-line change; fixes the highest-impact defect.)*
2. **B2** — change `scp_087_1_close` to `"range": 2` and correct the comment.
3. **B4** — add the EOC-level condition to `EOC_SCP_087_SLEEP_INTERRUPT`; review the wake
   behaviour.
4. **B5** — guard the stagger teleport (`true_eocs` / `false_eocs` on the location variable).
5. **B3** — rework the companion gate to send NPCs away from the door, restricted to allies.
6. **B6** — drop the self-`by_mood` entries and fix the DEVELOPING.md guidance.
7. **B7** — correct §9.1 (close it as a non-issue), fix the `:1412` citation, and soften the
   `MAN_MADE` / `NOT_HALLUCINATION` / `sound` comments.
8. **N1 / N2** — decide in playtest; both are tuning, not defects.

## 7. How to verify

No compile step — all JSON.

```sh
PATH="/c/msys64/mingw64/bin:$PATH" ./cataclysm-tiles.exe --check-mods dda scp_foundation
PATH="/c/msys64/mingw64/bin:$PATH" ./tests/cata_test.exe --mods dda,scp_foundation "[monster]"
```

Both must show no `Json error`, no `ERROR`, no `debugmsg`. Then specifically:

- **B1**: debug-spawn `mon_scp_087_1` on the seam tile and watch `scp_087_loops` (debug
  variables view) — it must **not** change. Repeat with the player stepping on it: it must
  increment exactly once per step.
- **B2**: park 087-1 ~10 tiles away on the same z with no obstruction and see whether the
  `scp_087_contact` effect / −99 morale / `downed` still land. With `range: 2` they should not
  until it is adjacent.
- **B5**: force the failure by standing where the 2–3 tile ring is impassable; confirm 087-1 does
  not move to an unintended location (and check the message log for a debugmsg).
- **B6**: run the faction test — `./tests/cata_test.exe --mods dda,scp_foundation "[monfactions]"`
  — after removing the self-`by_mood` entries to confirm nothing regresses.

---

## 8. Resolution status (added after the fixes were implemented)

Everything in §1 was addressed. Every edit was re-verified against the base-game
source listed in §2 before being written, and two extra items were found and fixed
while verifying (marked **+**).

| # | Status | What changed |
| --- | --- | --- |
| B1 | **Fixed** | `traps/scp_087_traps.json`: added `AVATAR_ONLY` to the seam trap's flags, and `effects_on_condition/scp_087_seam.json`: `"condition": "u_is_avatar"` on `EOC_SCP_087_SEAM` as a second line of defence. |
| B2 | **Fixed** | `monster_attacks/scp_087_1_attacks.json`: `scp_087_1_close` `"range": 1` → `2`, which is the minimum value that makes `mon_eoc_actor::call` gate on distance at all. |
| B3 | **Fixed** | `effects_on_condition/scp_087_companion.json`, now in **two stages**. (a) *The door*: each NPC finds its own `outdoor_only` + `passable_only` tile (teleport guarded through `true_eocs`), `npc_range` 20 → 8, and the avatar-position capture is gone. (b) *The barrier*: the new `EOC_SCP_087_COMPANION_ESCORT` (RECURRING, EOC-gated on `scp_087_loops > 0 && u_val('pos_z') < 0`) dispatches `EOC_SCP_087_NPC_ESCORT`, which lifts any NPC standing on **shaft terrain** back to the surface (`location_variable_adjust` + `z_override`) and then pushes them outside — so neither a debug teleport nor a follower that walks back down stays down there. The per-NPC 6-way `map_terrain_id` test is what stops the net from dragging NPCs out of unrelated basements, and `z_override` means no stored surface anchor is needed (so no unset-anchor hazard of the B5 kind). |
| B4 | **Fixed** | Sleep interrupt now gates at the **EOC level** on `scp_087_loops > 0 && u_val('pos_z') < 0`, and also clears `lying_down` (mirroring what `Character::wake_up()` would have done; no EOC effect exposes `wake_up()`). |
| B5 | **Fixed** | The stagger teleport moved into a new `EOC_SCP_087_1_STAGGER`, reached only via the `u_location_variable`'s `true_eocs`, so a failed search leaves the monster where it is. |
| B6 | **Fixed** | `monster_factions.json`: both self-`by_mood` entries removed; the "never list a faction in its own `by_mood`" rule is now in DEVELOPING §4 and §7. |
| B7 | **Fixed** | DEVELOPING §9.1 closed as a non-issue (the trap is a built-in terrain trap; `remove_trap` is a no-op); §9.2 closed; `validate_species_ids` :1412→:726 (also in `species.json`); `run_eocs` :6583→:6584-6585; `f_message` :5536-5538→:5542-5544; `MAN_MADE` marked inert; `NOT_HALLUCINATION` documented accurately in `scp_087_1.json`; `tracking_distance` default corrected in the comments; `sound` documented as a display gate; six new engine-behaviour gotchas added to §8. |
| N1 | **Fixed** | `scp_087_paranoia_driver.json`: the morale step is applied once per loop advance (tracked by the new `scp_087_paranoia_at`, reset in `EOC_SCP_087_EJECT`), so the ladder is −5/−15/−30/−50/−75 then the floor, driven by descent rather than elapsed time. |
| N2 | **Resolved** | No code change — the retreat is correct. The over-claiming comment was replaced with the verified `KEEP_DISTANCE` mechanics and an explicit "the approach does not stick, and that is intended". |
| **+1** | **Fixed** | `traps/scp_087_traps.json`: `"visibility": -1` → `99`. A **negative** visibility is inverted — `trap::can_see` is `return visibility < 0 || ...` (`src/trap.cpp:314`) and `detect_trap` is `return roll > visibility` (`:301`) — so `-1` means *always seen and always detected*, masked only by `always_invisible`. |
| **+2** | **Resolved — not a mod defect** | The intermittent `mapbuffer::unserialize_submaps` log error was traced to its only possible trigger: `mapbuffer.cpp:374-391` reaches that debugmsg *only* when `file_exist(zzip)` is true but `zzip::load` returns nullopt, and `zzip::load` returns nullopt in exactly one place — `mmap_file::map_writeable_file` failing (`src/zzip.cpp:355-372`), i.e. a Win32 `CreateFileW`/mmap failure (`src/mmap_file.cpp:296-340`) on a file that exists. The engine already retries 3× at 32 ms and its own comment blames OneDrive/antivirus interference. **No JSON in this mod can influence whether that call succeeds**, so there is nothing to change in data. Closed with evidence rather than patched over: 4/4 base-game-only runs and **12/12 consecutive modded runs** clean (exit 0, zero `mapbuffer` lines, `All tests passed`); the single historical occurrence came from a run that *also* died at exit with an access violation (0xC0000005) while every clean run exits 0; and six fixed `--rng-seed` values (1–6) do not reproduce it. |

### Post-review change, and what it moves

After this review, the closet door was deliberately rebuilt as an **ordinary openable
door** (it was `LOCKED`/`PICKABLE` with a `locked_object` examine) — see the mod's
README and DEVELOPING §5. One thing that follows from it and belongs on the record
here: while the door was locked it was a **hard structural barrier for NPCs** as well
as the player — `move_cost: 0`, no `open` link, `doors::can_unlock_door` handling
vehicles only (`gates.cpp:638-651`), and `str_min: 40` above a typical NPC's smash
ability. **That is no longer true**: an NPC can now open the door and walk in. B3's
escort net (`EOC_SCP_087_COMPANION_ESCORT`) is therefore now the *only* thing that
keeps companions out of the shaft, which is exactly why it was made a recurring,
terrain-scoped barrier rather than a one-shot nudge.

### Playtest round 1 — two defects found in live play, both fixed

| # | Defect | Root cause | Status |
| --- | --- | --- | --- |
| **P1** | The companion refusal played "You turn to the others. 'We go alone'…" **with no companions anywhere near** — the mod narrating a group that did not exist. | `EOC_SCP_087_COMPANION_GATE` emitted the `u_message` unconditionally on entering the tile. It never asked whether anyone was there, because it cannot: `u_run_npc_eocs` runs its child EOC zero or more times and reports nothing back (`src/npctalk.cpp:6840-6845`), and "a follower is nearby" is not expressible — following is not an `ally_rule` at all (`src/npc.h:310-328` has no `follow` entry), so `u_has_ai_rule` cannot test for it. | **Fixed** — the per-NPC EOC raises a global flag and the narration is gated on it. It must be a global, not a context variable: the dialogue is **copied** per NPC (`src/npctalk.cpp:6842`), so context writes inside the child are discarded. |
| **P2** | `DEBUG : invalid body part id "ALL"` (`src/generic_factory.h:541`) whenever the player hit SCP-087-1 — the report that produced this round. | `u_hp_max('ALL')` in `EOC_SCP_087_1_DAMAGE`. `hp_max_eval` builds the bodypart straight from the string with **no `'ALL'` case** (`src/math_parser_diag.cpp:686-691`) while `hp_eval`/`hp_ass` do special-case it (`:587`, `:597`); `hp_max` is unassignable (`:1879`). The failure path is `convert` → debugmsg → `null_id` (`src/generic_factory.h:534-544`). | **Fixed** — line removed. |
| **P2b** | *(found while verifying P2, worse than the error itself)* **087-1 and 173 were never actually invulnerable.** The heal was a no-op on monsters, so the debugmsg was the only visible symptom of a defence that did not exist. | The write half of `u_hp('ALL')` is `set_all_parts_hp_cur`, which walks the creature's **body-part map** (`src/creature.cpp:2775-2780`). A monster's HP is the single `monster::hp` field: `monster::get_hp`/`get_hp_max` read it while **ignoring the bodypart argument** (`src/monster.cpp:4072-4089`), and `monster` does not override `set_part_hp_cur` at all. So the written value never reached the HP the engine damages. No dialogue-math entry can assign monster HP — the diag table has only `hp` (parts) and read-only `hp_max` (`src/math_parser_diag.cpp:1878-1879`). | **087-1 fixed** by declaring an unsurvivable `hp` (`monsters/scp_087_1.json` //11) — the only lever JSON has. **173 cannot be fixed this way** (its invulnerability is conditional) and is now recorded as an open gap: DEVELOPING §9.6. |

Note on how P2b was missed the first time: the original claim that
`u_hp('ALL')`/`u_hp_max('ALL')` were "valid and assignable" was taken from the
documentation instead of the code path. Same class of error the review was written to
catch. The corrected entry is in §5's verified-correct register.


### Validation evidence

- All **26 JSON files parse** (byte-level check, 0 failures), no duplicate object
  keys anywhere, and the EOC id sweep is clean: **26 referenced / 26 defined**, no
  undefined references, no duplicates — this covers the two EOCs and one EOC
  condition added by the fixes.
- `tests/cata_test.exe --mods dda,scp_foundation "[monster]"` — **12 consecutive
  runs**, every one: **exit code 0**, `All tests passed (61450 assertions in 50
  test cases)`, `SCP Foundation pack load time:: …` then `Game data loaded`, and
  **no** `Json error`, `invalid species`, `debugmsg`, `ERROR` or `mapbuffer` line.
  (The `[monster]` tag includes the faction-reciprocity test, so B6 is covered.)
- Six of those runs used fixed `--rng-seed` values (1–6) to make worldgen
  deterministic — all clean, which is also the evidence that `+2` is not
  worldgen-dependent.
- Baseline `--mods dda "[monster]"`, 4 runs: 60270 assertions, exit 0, zero errors
  — so the mod adds 1180 assertions of coverage and no failures.

