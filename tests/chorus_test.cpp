// Behavior tests for The Chorus.
//
// These drive the mod's EOCs through the real engine paths headlessly and
// assert their D50 hook variables (see tests/autoplay_helpers.h).
//
// Run: tests/cata_test.exe "[the_chorus]" --mods the_chorus
#include <memory>
#include <string>
#include <string_view>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "game.h"
#include "global_vars.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "autoplay_helpers.h"

TEST_CASE( "the_chorus/seed-awaken-hook", "[the_chorus][autoplay]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    REQUIRE_FALSE( autoplay::has_var( dummy, "chorus_awakened" ) );
    CHECK( autoplay::activate_eoc( dummy, "EOC_CHORUS_SEED" ) );
    CHECK( autoplay::var_is( dummy, "chorus_awakened", "yes" ) );
    CHECK( get_globals().get_global_value( "chorus_active" ) == 1.0 );
}

TEST_CASE( "the_chorus/join-grants-trait", "[the_chorus][autoplay]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    REQUIRE_FALSE( dummy.has_trait( trait_id( "CHORUS_BOUND" ) ) );
    CHECK( autoplay::activate_eoc( dummy, "EOC_CHORUS_JOIN" ) );
    CHECK( dummy.has_trait( trait_id( "CHORUS_BOUND" ) ) );
    CHECK( autoplay::var_is( dummy, "chorus_joined", "yes" ) );
}

TEST_CASE( "the_chorus/unfold-hook", "[the_chorus][autoplay]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    // The unfold is driven by the character_dies event for any character carrying
    // the marker; fire it for a marked avatar to assert the hook deterministically.
    dummy.set_value( "chorus_infected", "yes" );
    CHECK( autoplay::activate_eoc( dummy, "EOC_CHORUS_UNFOLD" ) );
    CHECK( get_globals().get_global_value( "chorus_unfold_proof" ) == 1.0 );
}

TEST_CASE( "the_chorus/npc-death-unfolds-member", "[the_chorus][autoplay]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    // End-to-end F1: a seeded infiltrator template carries the marker, so killing
    // it must fire the character_dies event and unfold a mimic. (Converted NPCs use
    // the same event path; a template is the deterministic fixture here.)
    const character_id id = get_map().place_npc( dummy.pos_bub().xy() + point( 1, 0 ),
                             npc_template_id( "chorus_infiltrator_wanderer" ) );
    npc *guy = g->find_npc( id );
    REQUIRE( guy != nullptr );
    REQUIRE( guy->get_value( "chorus_infected" ) == std::string_view( "yes" ) );

    get_globals().set_global_value( "chorus_unfold_proof", 0.0 );
    guy->die( &get_map(), nullptr );
    CHECK( get_globals().get_global_value( "chorus_unfold_proof" ) == 1.0 );
}
