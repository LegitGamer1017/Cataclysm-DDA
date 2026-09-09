// Reproduction for the Secluded Practitioner start-kit bug (empty inventory
// on spawn). The real flow grants profession items in game::start_game() via
// Character::add_profession_items(). These tests drive that exact function
// headlessly against the cultivation profession to discriminate between a
// data-side failure (our kit never resolves) and a game-flow failure (the
// function never runs / profession not applied when starting).
//
// Build: cdda_build_start preset='tests'
// Run:   tests/cata_test.exe "[cultivation][profession]" --mods cultivation
//        (or cdda_run_tests spec='[cultivation]' mods='cultivation')
#include "avatar.h"
#include "catch/catch.hpp"
#include "character.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "profession.h"
#include "type_id.h"

static const profession_id cult_secluded_prof( "cult_secluded" );

// Test 1: the cultivation profession's start kit resolves through the exact
// function the game calls at spawn (add_profession_items).
TEST_CASE( "cultivation/profession-start-kit-granted", "[cultivation][profession]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    // apply our profession the way character creation does
    dummy.prof = &cult_secluded_prof.obj();
    REQUIRE_FALSE( dummy.prof->is_hobby() );

    dummy.add_profession_items();

    CHECK( dummy.amount_of( itype_id( "cult_jade_slip_menu" ) ) >= 1 );
    CHECK( dummy.amount_of( itype_id( "cult_manual_faded" ) ) >= 1 );
    CHECK( dummy.amount_of( itype_id( "mortar_pestle" ) ) >= 1 );
    // the kit carries two spirit grass blades
    CHECK( dummy.amount_of( itype_id( "cult_herb_grass" ) ) >= 2 );
}

// Test 2 (control): the default/unemployed profession's baseline items flow
// through the same function. If this passes but Test 1 fails, the problem is
// profession-specific data; if both fail, the grant mechanism itself is at
// fault in the flow (matching the player's completely-empty inventory).
TEST_CASE( "cultivation/baseline-profession-items-granted", "[cultivation][profession]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    dummy.prof = profession::generic();
    REQUIRE_FALSE( dummy.prof->is_hobby() );

    dummy.add_profession_items();

    // unemployed (generic) carries clothing (jeans) and starts with at least
    // one wearable / held item in the real game.
    int total = 0;
    total += dummy.amount_of( itype_id( "jeans" ) );
    total += dummy.amount_of( itype_id( "tshirt" ) );
    total += dummy.amount_of( itype_id( "socks" ) );
    CHECK( total >= 1 );
}
