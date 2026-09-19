# Code Review — SCP-294 (pass 2)

Scope: everything implemented for SCP-294 to date — `src/scp_294.cpp` / `.h`, the
`iexamine`/`init` wiring, `tests/scp_294_test.cpp`, and the mod's SCP-294 JSON —
reviewed against the base game source. Pass 1 lives in `CODE_REVIEW_294.md`; this
pass re-checks it and covers everything added since (the `fill_with` fix, the cup
resize, the gold cooling, and the gold lump).

Method: every load-bearing claim is checked against the actual code and cited by
`file:line`. Nothing is reported as a bug without a source reference. Where the
implementation's comments or docs disagree with what the code does, that is
called out.

Verified state at review time: `build-039` (tests) and `build-040` (tiles) exit 0;
`[scp_294]` 41 assertions / 5 cases PASS; `[item]` 164,289 assertions PASS;
`--check-mods scp_foundation` CLEAN.

---

## 0. Resolution status (fix pass)

| Finding | Status |
| --- | --- |
| **F1** the lump's literal weight vs the computed pour | **Fixed** — the machine now pours one serving, and `scp_294_gold_lump_weighs_what_was_poured` mirrors it (cup + serving + `fill_with`), sums the mass poured and asserts it equals the lump's weight, so a desync fails loudly. The lump is 2500 g, one 250 ml charge. |
| **F2** a pour was up to two servings | **Fixed** — the pour is capped at `scp_294_serving_volume = 250_ml` via `fill_with`'s `amount` argument. The cup stays 500 ml so an indivisible 400–500 ml charge still pours (as its single charge). |
| **F4** player/dev docs behind the code | **Fixed** — README now says one serving and documents the cooling + gold lump; `DEVELOPING.md` §11 gains "The pour, the cup, and the cooling gold" and the new tunable. |
| **F7** *(new, from a live save)* `SPAWN_ACTIVE` on molten gold was redundant, and the comment/doc claiming it was load-bearing were wrong | **Fixed** — the flag is removed. `has_temperature()` is `is_comestible() && !NO_TEMP` (`src/item.cpp:3190-3192`) and the item ctor sets `active = true` on that branch (`:257-259`), so a comestible is active from construction; that is what lets the expiry inside `if( active )` (`:4770-4785`) run. `tests/scp_294_test.cpp` now asserts `molten.is_active()`, so making the gold non-comestible (or adding `NO_TEMP`) can no longer silently stop the cooling. The save's other comestibles stored as `active = true` with no such flag are what exposed the wrong assumption. |
| **F8** *(new, from the log)* the mod's art-less items had no `looks_like` | **Fixed** — all 12 (the 10 SCP-294 liquids, the gold lump, the field log) now point at base tiles (`water_clean` / `blood` / `cola` / `mercury` / `motor_oil` / `gold_small` / `paper`). Items *do* honour `looks_like` (`cata_tiles.cpp:2086-2097`) — a case the mod's own doc omitted. **Correction to the earlier assumption:** the log's `has no 'unknown' tile defined!` is a **load-time tileset check** (`tileset_loader.cpp:476-479`), a property of UltimateCataclysm, **not** caused by our items — so `looks_like` does not silence it, and the mod is not at fault for it. |
| **F3, F5, F6** | **Open** — the cosmetic `" (active)"` suffix, the uncraft gating nit, and the items carried from pass 1 (D1 is still the one with worldgen risk). |

Re-verified after the fix: `build-041` (tests) and `build-042` (tiles) exit 0;
`build-043` reported **"Nothing to be done"**, which confirms the F7/F8 pass is
**data-only** (no C++ changed, so no relink was needed); `build-044` (tests) exit 0
for the new `is_active()` assertion. `[scp_294]` **47 assertions / 6 cases** PASS;
`[item]` 164,289 assertions PASS; `--check-mods scp_foundation` CLEAN; JSON
validate and format clean.

---

## 1. What changed since pass 1

| Change | Where |
| --- | --- |
| `put_in` failure no longer charges and no longer logs | `scp_294.cpp` |
| Pour sized with `item::fill_with` (volume **and** weight) | `scp_294.cpp` |
| Cup resized 355 ml → 500 ml, weight 1 kg → 10 kg | `items/scp_294_items.json` |
| Molten gold cools into a gold lump | `items/scp_294_items.json` |
| `scp_294_gold_lump` + its uncraft into `gold_small` | `items/scp_294_items.json`, `recipes/scp_294_recipes.json` |
| Two new test cases (liquid pourability, timed transform) | `tests/scp_294_test.cpp` |

---

## 2. Verified-correct register (new mechanisms)

The cooling path is the riskiest new thing, so it was traced end to end rather
than assumed:

| Claim | Evidence |
| --- | --- |
| `SPAWN_ACTIVE` activates the item at construction | `src/item.cpp:204-206` calls `activate()` |
| `activate()` is trivial and safe on a comestible | `src/item.cpp:479-488` — sets `active = true`, no other effect |
| The copy made inside `fill_with` keeps `active`/countdown | copy ctor is `= default` (`src/item.cpp:386`), so members copy |
| A nested item is still processed | `item::process` recurses into `contents.process(...)` (`src/item.cpp:4724-4727`); `Character::process_items` walks top-level items (`character_inventory.cpp:2908-2924`) |
| The countdown only runs while active | the expiry is inside `if( active )` (`src/item.cpp:4770-4785`) |
| A comestible is processed ~every 10 minutes | `processing_speed()` returns 10 minutes for comestibles (`src/item.cpp:4183-4184`) |
| The transform replaces the item **in place** inside the cup | `item_transformation::transform` calls `it.convert( target )` (`item_transformation.cpp:53-60`) |
| The cooled item is not left stale-active | `active = false` is set *before* the action (`src/item.cpp:4786`) |
| Molten gold is still drinkable while cooling | no `active` guard exists in the consume path (`src/consumption.cpp`) |
| The lump fits back in the cup it formed in | 5000 g ≤ 10 kg and 260 ml ≤ 500 ml |
| Trading uses the post-apoc price | `item::price( bool practical )` sums `price_no_contents(practical)` (`src/item.cpp:1921-1928`) |
| Gold is *meant* to be worthless | `gold_small` `price_postapoc: "0 cent"` ("scrap gold is actually worthless", `metal.json:114-116`); `1l_gold` `"1 USD"` ("novelty item, absolutely useless", `:767-769`) |
| Deconstructing a gold form into `gold_small` is the base pattern | `data/json/uncraft/resources/metal.json:190-197` |
| A mod cannot extend a recipe's `components` | `src/item_components.cpp` has no `extend`/`was_loaded` handling — so the uncraft route was correct, not a shortcut |

---

## 3. Findings

### F1 — The gold lump's mass is a hard-coded 5000 g, decoupled from the pour  *(fixed)*

> Resolved: the pour is now one serving (250 ml, so 2500 g of molten gold) and the
> lump is 2500 g, with a test that fails if the two ever diverge. The text below
> records the original finding.

The pour is now **computed**: `fill_with` decides how many charges fit the 500 ml
cup (`scp_294.cpp`), which for molten gold is 2 × 2500 g = 5000 g. But
`scp_294_gold_lump` declares `"weight": "5000 g"` as a literal, and
`item_transformation` creates the target from its own definition — it does not
carry the source mass across. So the two are only equal **by coincidence of the
current numbers**.

Change the cup's capacity, the molten gold's `volume`/`weight`, or the cup's
weight limit, and the lump silently keeps claiming 5000 g. Nothing catches it —
the `.//2` comment records the arithmetic but does not enforce it.

**Recommendation:** add a test that mirrors the machine (cup + `fill_with` molten
gold), sums the poured mass, and asserts it equals the lump's weight. That turns
a silent desync into a red test. Cheap, and it belongs with the existing
pourability case.

### F2 — A pour is now up to 500 ml, i.e. two servings  *(fixed)*

> Resolved: the machine pours `scp_294_serving_volume` (250 ml) and never more.

`fill_with` fills the cup, and the cup is 500 ml, so a normal drink now yields
**two charges**: `water` → 500 ml, spirits ~14 charges, motor oil 500 charges.
Before the resize it was one 250 ml serving. This is the price of making
400–500 ml-per-charge drinks (bee's knees, bark tea, three sisters stew) pourable
at all — but it doubles the machine's food/water value and was not called out as
a balance decision.

**Recommendation:** decide deliberately. If one serving is wanted, pour
`min( cup capacity, one serving )` — the container can still be 500 ml, so the
big-charge drinks still fit; only the *amount poured* changes.

### F3 — Cooling gold displays as " (active)"  *(cosmetic)*

`item_tname.cpp:613-624` appends `" (active)"` to any item with `active` set and a
`countdown_interval`, unless its id ends in `_on`. So the cup's contents read
**"molten gold (active)"** while cooling. That is arguably a useful tell, but it
is a visible oddity on a liquid and should be a conscious choice.

### F4 — Player/dev docs have fallen behind  *(fixed)*

> Resolved: README and `DEVELOPING.md` §11 now cover the serving size, the cup,
> the cooling and the gold lump.

`README.md` §SCP-294 still lists "molten gold" among the dispensable liquids and
**never says it cools**; neither README nor `DEVELOPING.md` §11 mentions the gold
lump, its uncraft, the cup's 500 ml capacity, or the shift from
`charges_per_volume` to `fill_with`. The in-file `"//"` comments are thorough; the
prose docs are now the stale ones. (This is exactly the drift pass 1's DOC1/DOC2
fixed once already.)

### F5 — Uncraft gating copied from the ingot  *(content nit)*

The lump uncrafts to 250 `gold_small` (4750 g from 5000 g — a deliberate ~5%
loss, consistent with deconstruction) but requires `HAMMER 2` + `SAW_M 2`, copied
from the 1 L ingot recipe. A poured, soft lump probably wants less than a
machine-sawn ingot. Minor.

### F6 — Still open from pass 1

| Item | Status |
| --- | --- |
| **D1** placement forces `t_floor` at a fixed `(11,11)` — can overwrite a wall or a feature | Open; unchanged |
| **D3** bashing the anomaly degrades it to `f_vending_o` | Open; unchanged |
| **D4** `music` / `life_story` / `medical_knowledge` are descriptive-only, and `liquid_carbon` is fully inert | Open; now a design question you have raised |
| **D5** hard-coded constants (`cost`, `pours_per_cycle`, `restock_turns`, `max_cycles`) | Open; unchanged |
| **R3** `get_state` coerces a non-numeric global to 0 | Open; benign |
| **DOC3** the approved plan file (outside the repo) still specifies `fuzzy_score` | Open; the shipped docs match the code |

---

## 4. Verification run

- `build-039` (preset `game-fast`) — exit 0; `build-040` (preset `game`, tiles) —
  exit 0. Note **no relink was needed** for the last change: it is data-only, and
  JSON is loaded at runtime, so the 16:05 `cataclysm-tiles.exe` (which already
  contains every C++ fix) runs it.
- `[scp_294]` — PASS, 41 assertions in 5 cases (resolver; map-extra membership;
  every-liquid pourability; molten-gold-is-timed; request-table item ids).
- `[item]` — PASS, 164,289 assertions (includes the lump's density: 5000 g /
  260 ml against gold's material density).
- `--check-mods scp_foundation` against the fresh tiles binary — CLEAN.
- `cdda_validate_json` clean (35 files); `cdda_format_check` clean.

**Not verified:** the cooling *timing* in live play (expected 10–20 minutes given
the 10-minute comestible processing cadence), and whether the transform message
reads well in a container. Both are one playtest away.

---

## 5. Recommended order

1. **F1** — the mass-coupling test. It is the only latent defect here, and it is
   cheap.
2. **F2** — decide "fill the cup" vs "one serving"; it is a balance call, not a bug.
3. **F4** — bring README/DEVELOPING up to date with the cooling + lump + cup size.
4. **F3, F5** — cosmetic/content nits.
5. **D1, D3–D5, R3** — carried from pass 1; D1 remains the one with real
   world-generation risk.
