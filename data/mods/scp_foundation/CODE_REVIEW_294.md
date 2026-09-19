# Code Review — SCP-294

Scope: the SCP-294 implementation — `src/scp_294.cpp`, `src/scp_294.h`, the
`iexamine`/`init` wiring, `tests/scp_294_test.cpp`, and the mod's
`scp_294_*` JSON — reviewed against the base game source.

Method: every load-bearing claim below is checked against the actual code and
cited by `file:line`. Nothing is reported as a bug without a source reference.
Where the implementation's own comments or docs disagree with what the code does,
that is called out explicitly.

Status at review time: builds clean (`build-029`, exit 0); `[scp_294]`,
`[monster]` and `[item]` test specs pass against `dda,scp_foundation`. Two
defects found during development are already fixed and re-verified (see §5).

---

## 0. Resolution status (fix pass)

Fixed in this pass, and re-verified by build + tests (see §9):

| Finding | Status |
| --- | --- |
| **B1** failed `put_in` charges for an empty cup | **Fixed** — return checked; refuse without charging |
| **B2** `charges = 1` pours 1 ml for ammo-subtype liquids | **Fixed** — pour is sized with `charges_per_volume( 355_ml )` |
| **B3** duplicate `water` display name | **Fixed** — renamed to `boiling water` |
| **R1** unknown `item` id fails silently | **Fixed** — `check_scp_294_requests()` in the consistency phase |
| **R2** dead `scp_294_requests()` accessor | **Fixed** — used by a new unit test |
| DOC1 stale `modinfo.json` | **Fixed** — description + version |
| DOC2 overstated "Feature-complete" | **Fixed** — now "Implemented, not yet playtested" |
| R3, D1–D5, DOC3, DOC4 | **Open** — see §2/§3/§4; these are a benign note, design decisions, and a stale external plan, not defects |

---

## 1. Findings that are real defects

### B1 — A failed `put_in` logs an error and still charges the player  *(fixed)*

`src/scp_294.cpp`:

```cpp
cup.put_in( liquid, pocket_type::CONTAINER );
you.i_add_or_drop( cup );
you.use_charges( itype_cash_card, scp_294_cost );
```

`item::put_in` is declared (`src/item.h:1055-1057`) as:

```cpp
ret_val<void> put_in( const item &payload, pocket_type pk_type,
                      bool unseal_pockets = false, Character *carrier = nullptr,
                      bool quiet = false );
```

and its own doc comment (`src/item.h:1052-1053`) states: *"When `quiet` is true,
failure returns silently instead of triggering a debugmsg."* With the default
`quiet = false`, a payload that does not fit **fires a debugmsg** (which is an
error, and fails the test harness under "error logged during initialization"),
and on top of that the handler ignores the return value and charges the player
50 cents for an empty cup.

**Impact:** any liquid whose per-charge volume exceeds the cup's 355 ml triggers
an error and a paid-for empty cup. Rare with the current item set, but the
handler must not be able to charge for nothing.

**Fix:** check `put_in(...).success()` (`src/ret_val.h:13`) and, on failure, hand
the liquid to the player (or refuse) **without** charging; move
`use_charges` after the success check.

---

### B2 — `charges = 1` dispenses one *charge*, not a cupful  *(fixed)*

`src/scp_294.cpp`:

```cpp
if( liquid.count_by_charges() ) {
    liquid.charges = 1;
}
```

`count_by_charges()` is true for anything with `stackable_ || ammo ||
( comestible && phase != SOLID )` (`src/itype.h:1729-1731`). For a normal drink
that is one 250 ml serving, so `charges = 1` is 250 ml. But liquid **ammo** is
defined per `stack_size`, not per charge. `motor_oil`
(`data/json/items/fuel.json:199-222`) is `"subtypes": [ "AMMO" ]` with
`"volume": "200ml"` and `"stack_size": 200` — i.e. **1 charge = 1 ml**. So "a
cup of motor oil" pours **1 ml** into a 355 ml cup.

**Impact:** inconsistent, visibly wrong amounts for ammo-subtype liquids
(motor oil, and anything else declared that way). The base game has the perfect
helper — `item::charges_per_volume` (`src/item.h:2056-2063`, *"Number of
(charges of) this item that fit into the given volume"*).

**Fix:** `liquid.charges = std::max( 1, liquid.charges_per_volume( 355_ml ) );`
then B1's success check. This also makes the amount correct for multi-charge
drinks (e.g. spirits at ~36 ml/charge fill ~320 ml rather than 36 ml).

---

### B3 — `scp_294_boiling_water` duplicates the base `water` display name  *(fixed)*

`items/scp_294_items.json` gives the item `"name": { "str_sp": "water" }`. The
base game already has an item with exactly that name
(`data/json/items/comestibles/drink.json:1638-1642`, `"id": "water"`, `"name":
{ "str_sp": "water" }`).

The resolver's exact-name branch will match **both**, and the tie is broken by
`better_liquid`, which compares only "is a drink" and name length — both equal —
so the winner is whichever comes first in `item_controller->all()`
(`src/item_factory.cpp:5319-5338`, which returns insertion order, base data
before mod data). It currently lands on the base `water`, which is correct *by
load order*, not by design.

**Impact:** latent wrong-item bug that would flip if load order ever changed; and
two different items answering to "water" is a smell. The item is only reachable
via the `surprise me` special anyway, so it does not need to be named "water".

**Fix:** give it a distinct name (e.g. `"boiling water"`); the special already
maps the phrase, so nothing else changes.

---

## 2. Robustness gaps

### R1 — A typo'd `scp_294_request.item` fails silently at runtime

The loader (`src/scp_294.cpp`) reads `item` as a plain `itype_id` and never
validates it. An unknown id therefore becomes a null id, and the handler turns
that into `OUT OF RANGE` — a missing drink looks like a deliberate refusal, and
`--check-mods` will not catch it. All ids currently referenced **do** exist
(verified: `wine_cabernet` `data/json/items/comestibles/alcohol.json:125`,
`milkshake_choc` `.../frozen.json:46`, `juice` `.../drink.json:778`, `lye`
`data/json/items/chemicals_and_resources.json:1446`), but the next edit is not
protected.

**Fix (applied):** `check_scp_294_requests()` now runs in the consistency-check
phase, registered in `DynamicDataLoader::check_consistency()`
(`src/init.cpp:955-1016`). Note the original review said to use an
`add( type, load, check )` overload — **that was wrong**; `add()` has only loader
overloads (`src/init.h:82-90`). Load-time validation inside the loader would also
be unsafe, because item ids may not be finalised yet at that point.

### R2 — `scp_294_requests()` is dead API  *(fixed)*

Declared (`src/scp_294.h`) and defined (`src/scp_294.cpp`) but referenced nowhere.
Resolved by using it: `tests/scp_294_test.cpp` now walks the table and asserts
every referenced item id resolves.

### R3 — `get_state` silently coerces non-numeric globals to 0

`get_state` returns 0 unless `is_dbl()`. That is fine for keys only this code
writes, but it means a key collision with a JSON `global_val` of another type
would read as "0 pours" with no diagnostic. Low risk; noting it.

---

## 3. Design risks (not bugs)

### D1 — The placement overlay forces a floor tile at a fixed coordinate

`scp_294_mapgen_extra.json` uses an `update_mapgen` with
`place_terrain: t_floor` and `place_furniture: f_scp_294` at `(11, 11)`. Both
apply unconditionally (`src/mapgen.cpp:2953-2963` for terrain, `:2918-2924` for
furniture, registered `:4772-4773`). Because it attaches to a whole extras
collection (`research_facility_interior` for labs, `build` for offices), the tile
at `(11, 11)` is whatever that lab/office variant puts there — so the overlay can
**overwrite a wall (breaching it), a staircase, or existing furniture**. This is
the reason the tile is forced to floor (reachability), but it trades one problem
for another; `t_floor` is also not the lab's usual floor id.

**Options:** (a) accept and retune the coordinate against the layouts actually
used; (b) attach the extra to a narrower, hand-picked oter whose `(x, y)` is
reliably open; (c) drop `place_terrain` and accept that a machine occasionally
sits in a wall (cosmetic, not breaking). Worth an explicit decision — the current
code silently picks (a)-with-a-floor-hole.

### D2 — Charging only on a successful dispense

The article implies coin-first-then-type; the handler charges only when a cup is
actually produced (`scp_294.cpp`). This is a deliberate, documented choice
(README §"How it works"), but it is a deviation from canon and should be a
conscious one.

### D3 — Bashing degrades the anomaly into ordinary furniture

`f_scp_294`'s `bash` sets `furn_set: f_scp_294_dead`; `f_scp_294_dead`'s `bash`
sets `furn_set: f_vending_o`. So the anomaly can be beaten into the base game's
ordinary broken vending machine. Thematically odd; harmless mechanically.

### D4 — Three of the drink statuses are purely descriptive

`scp_294_rhythm`, `scp_294_memory`, `scp_294_medical` carry no `base_mods`. The
article has the medical drink actually letting the subject practise medicine and
the music drink changing how they move. This matches the mod's existing
precedent (`scp_087_contact` is descriptive-only), but it is a content decision,
not an accident — confirm it is intended.

### D5 — Hard-coded tuning constants

`scp_294_cost = 50`, `scp_294_pours_per_cycle = 50`, `scp_294_restock_turns = 90*60`,
`scp_294_max_cycles = 3`. Fine as constants, but they are the balance surface and
nothing surfaces them to the player or to JSON.

---

## 4. Documentation discrepancies

### DOC1 — `modinfo.json` was not updated  *(fixed)*

Its description listed only SCP-173 and SCP-087 and the version was still `0.2`.
Both updated (SCP-294 paragraph added; version `0.3`).

### DOC2 — `DEVELOPING.md §1` marks SCP-294 "Feature-complete"  *(fixed)*

The table said *Feature-complete* for all three. For SCP-294 that rested on a
clean build and loader/test validation only — **no end-to-end playtest of the
examine flow, money, restock, jam or placement has been run**. Now reads
"Implemented, not yet playtested", per the mod's own distinction between
"proven in play" and "implemented".

### DOC3 — The approved plan and the implementation diverged on matching

The plan (`~/.commandcode/plans/scp-294-implementation.md` §A3) specified
`debug_menu::fuzzy_score` with a `SCORE_THRESHOLD`; the implementation uses
`lcmatch` exact-then-substring with no threshold. The **shipped** docs
(`DEVELOPING.md §11`) match the code, so only the plan is stale — noting it so
the two do not drift.

### DOC4 — Registry note  *(fixed)*

`DEVELOPING.md §2` described `monstergroups/scp_registry.json` (`GROUP_SCP_ALL`)
as "the canonical index of every SCP". SCP-294 has no entry — correct, since it
is an object and the registry is a monstergroup. The wording now says the
registry indexes SCPs that exist as monsters, and that objects such as SCP-294
are deliberately not listed.

---

## 5. Already fixed during development (re-verified)

- **Static-init `furn_id`.** `const furn_id furn_scp_294_dead("f_scp_294_dead")`
  resolved an `int_id` before furniture data existed, producing
  `invalid (null) id "f_scp_294_dead"` at load. Now a `furn_str_id`, converted at
  the point of use. (`int_id`-from-string is the factory lookup; `string_id`
  construction is not — which is why the `itype_id` constants were unaffected.)
- **Invalid color.** `"color": "light_purple"` on the perfect drink — not a
  defined color (`src/color.cpp` knows `pink`/`magenta`, not `light_purple`).
  Now `pink`.
- **Density.** `scp_294_liquid_carbon` (2.4 g/ml) and `scp_294_life_story`
  (1.6 g/ml) exceeded their material's density and failed
  `item_material_density_sanity_check` (`tests/item_test.cpp:1025-1045`, which
  compares item density to the declared materials'). Weights corrected to 250 g.

---

## 6. Style and performance

- **astyle:** the new C++ files are clean under the repo's rules (run with the
  option set the installed astyle supports; **no lines exceed 100 chars**, all
  one-line conditionals are braced). Note the repo's `.astylerc` names
  `indent-preprocessor` and `add-brackets`, which this astyle build rejects, so
  `make astyle` cannot run as-is in this environment — an environmental issue,
  not a defect in the new code.
- **Copy-with-style:** the C++ follows the surrounding conventions
  (`#pragma once` + include guard, `snake_case` free functions, 4-space indent,
  `if( x )` spacing, `debugmsg`/`add_msg` usage).
- **Performance:** the resolver iterates `item_controller->all()` but skips any
  non-liquid before doing any work, and the vector is cached, so the cost is a
  few hundred cheap comparisons on an examine — negligible. Minor: for liquid
  candidates it lowercases the name and then `lcmatch` lowercases it again; not
  worth changing.
- The three `state_key` `string_format` calls per examine are also negligible.

---

## 7. Verified-correct register (assumptions checked against source)

| Assumption | Evidence |
| --- | --- |
| Named `examine_action` registration + failure mode | `src/iexamine.cpp:7475` map, `:7481` failure debugmsg |
| Furniture `examine_action` is a JSON string | `src/mapdata.cpp:1252` |
| `map::furn` returns `furn_id`; `furn_set` takes it | `src/map.h:969`, `:977` |
| `Character::charges_of` / `use_charges` signatures | `src/character.h:4079`, `:3137` |
| `item::put_in` + `ret_val::success` | `src/item.h:1055`, `src/ret_val.h:13` |
| Input popup API (`query`/`cancelled`/`set_label`/`set_description`) | `src/input_popup.h:49-55`, `:89` |
| Item enumeration, liquid phase, display name | `src/item_factory.cpp:5319`, `src/itype.h:1590`, `:1722` |
| `lcmatch` is case-insensitive substring | `src/cata_utility.h:115` |
| Globals are shared with JSON `global_val`, per-world, serialized | `src/global_vars.h:15/28/71`, `src/game.cpp:11620`, `src/savegame.cpp:164/325` |
| `consumption_effect_on_conditions` accepts bare string ids | `data/json/items/comestibles/alien.json:21` |
| `u_deal_damage` shape; effect/damage ids valid | `src/npctalk.cpp:7534/8496`; `nausea` `data/json/effects.json:1429`; heat/cold `data/json/damage_types.json:168` |
| `map_extra` `generator_id` → `update_mapgen`; `place_terrain`/`place_furniture` keys | `data/json/overmap/map_extras.json:255`; `src/mapgen.cpp:2953-2963`, `:2918-2924`, `:4772-4773` |
| `map_extra_collection` copy-from + extend works | `data/mods/CrazyCataclysm/crazy_mapgen.json:34-39` |
| Specials' referenced base item ids exist | `wine_cabernet` (alcohol.json:125), `milkshake_choc` (frozen.json:46), `juice` (drink.json:778), `lye` (chemicals_and_resources.json:1446) |
| No `src/*.cpp` build-list edit needed | `src/CMakeLists.txt:16`, `Makefile:1042` (globs) |

---

## 8. Recommended order to address

1. **B1** (charged for an empty cup / error log) — smallest change, removes a real
   failure mode.
2. **B2** (dispense a cupful, not one charge) — same code block as B1; do together.
3. **B3** (duplicate "water" name) — one-line content fix.
4. **R1** (validate `scp_294_request.item`) — prevents a whole class of silent
   content bugs.
5. **D1** (placement tile) — decide the approach; then DOC1/DOC2.
6. **R2, D3–D5, DOC3–DOC4** — cleanups and content decisions.

Items 1–4 and DOC1/DOC2/DOC4 were done in the fix pass (§0). Item 5's **D1** and
D3–D5 remain open on purpose: they are design decisions, not defects. **DOC3**
refers to the approved plan file outside the repo and is left as-is.

---

## 9. Fix-pass verification

Everything changed in this pass was compiled and exercised; nothing here is
asserted from reading alone.

- **Build:** `build-030` (preset `game-fast`) — **exit 0**, no warnings.
- **Tests** (`tests/cata_test.exe --mods dda,scp_foundation`):
  - `[scp_294]` → **passed** — 2 test cases, 21 assertions. The new case walks
    every scripted request and asserts its item id resolves.
  - `[item]` → **passed** — 126 test cases, 164,286 assertions.
  - `[monster]` → **passed** — 50 test cases, 61,461 assertions.
- **R1 is proven to fire, not merely registered.** With one request's `item`
  temporarily pointed at a bogus id, a direct run logged
  `ERROR : src/scp_294.cpp:129 [void check_scp_294_requests()] SCP-294 request
  "scp_294_req_superconductor" references unknown item "scp_294_intentionally_bogus"`
  and the run failed; the id was reverted and the suite re-passed.
  `SKIP_VERIFICATION` is set only by the `--no-verify` CLI flag
  (`src/main.cpp:822-823`), so the check runs under normal play, `--check-mods`,
  and the test harness.
- **JSON:** `cdda_validate_json` clean (34 files); `cdda_format_check` clean
  (0 dirty / 0 errors).
- **Style:** the new C++ is clean under astyle (run with the subset of options
  the installed astyle accepts — the repo `.astylerc` names `indent-preprocessor`
  and `add-brackets`, which this build rejects); no line exceeds 100 characters.

**Not verified — end-to-end playtest.** The examine flow, the money gate, the
restock/jam cycle, the pour sizing, and world placement are exercised by neither
the build nor the tests. They remain a manual step, and SCP-294 is marked
"Implemented, not yet playtested" for exactly that reason.
