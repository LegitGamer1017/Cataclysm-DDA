#pragma once
#ifndef CATA_TESTS_AUTOPLAY_HELPERS_H
#define CATA_TESTS_AUTOPLAY_HELPERS_H

// Turn-simulation helpers for headless mod-behavior tests.
//
// The game's real loop advances the calendar, then processes the avatar's
// per-turn state (`Character::process_turn()`), which fires queued recurring
// EOCs whose next-fire time has arrived (`effect_on_conditions::
// process_effect_on_conditions`). Tests normally bypass this by calling an EOC
// directly, which cannot exercise recurring ticks or time-based conditions.
//
// These helpers drive the REAL engine paths headlessly:
//   * seed_recurring_eocs()  - (re)queue every loaded recurring EOC for the
//                              avatar from "now", giving a deterministic start.
//   * advance_turn()         - one avatar turn: process_turn() then advance
//                              the calendar by 1 turn (the pattern used by the
//                              engine's enchantments/character tests).
//   * advance_time()         - jump the calendar forward a duration, then let
//                              process_turn() fire any EOC now due.
//   * activate_eoc()         - fire a specific EOC immediately (deterministic
//                              unit checks that don't depend on timing).
//
// Together these let a test script N real turns (recurring ticks, meditation
// activities, break/cap hooks) and assert D50 hook variables / vitamin deltas
// afterwards - the "playtest without a GUI" surface.

#include <string>
#include <string_view>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "math_parser_diag_value.h"
#include "talker.h"

namespace autoplay
{

/** Clear and re-queue every loaded recurring non-global EOC for the avatar
 *  starting from the current calendar time. Gives a deterministic start point
 *  for turn-simulation tests that depend on recurring ticks. */
inline void seed_recurring_eocs( avatar &guy )
{
    effect_on_conditions::clear( guy );
    effect_on_conditions::load_new_character( guy );
}

/** Advance the avatar exactly one turn the way the engine does in tests:
 *  run process_turn() (fires due recurring EOCs, replenishes moves, updates
 *  body/morale/etc.), then increment the calendar. */
inline void advance_turn( avatar &guy )
{
    guy.process_turn();
    calendar::turn += 1_turns;
}

/** Advance `n` single-turn steps (see advance_turn). */
inline void advance_turns( avatar &guy, int n )
{
    for( int i = 0; i < n; ++i ) {
        advance_turn( guy );
    }
}

/** Jump the calendar forward by `dur`, then run one process_turn() so any
 *  recurring EOC whose next-fire time is now <= calendar::turn fires exactly
 *  once. Use this for long-interval ticks (e.g. a 60-second Qi tick) without
 *  looping 60 one-second turns. */
inline void advance_time( avatar &guy, const time_duration &dur )
{
    calendar::turn += dur;
    guy.process_turn();
}

/** Fire a specific EOC by string id immediately (deterministic; does not
 *  depend on queue timing). Returns the EOC's activation result. */
inline bool activate_eoc( avatar &guy, const std::string &eoc_id )
{
    dialogue d( get_talker_for( guy ), std::make_unique<talker>() );
    return effect_on_condition_id( eoc_id )->activate( d );
}

/** True when the avatar has a per-character variable (a D50 u_add_var hook)
 *  equal to `expect`. `get_value` returns a diag_value; comparing against a
 *  string_view avoids the type-mismatch debugmsg that .str() would emit when
 *  the variable is unset (stored as an empty/monostate value). */
inline bool var_is( const avatar &guy, const std::string &var, const std::string_view expect )
{
    return guy.get_value( var ) == expect;
}

/** True when the avatar has the variable at all (set to anything non-empty). */
inline bool has_var( const avatar &guy, const std::string &var )
{
    const diag_value &v = guy.get_value( var );
    return !v.is_empty() && ( v.is_str() && !v.str().empty() );
}

} // namespace autoplay

#endif // CATA_TESTS_AUTOPLAY_HELPERS_H
