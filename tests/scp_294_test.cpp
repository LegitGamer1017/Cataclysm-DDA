#include "cata_catch.h"

#include <algorithm>
#include <string>

#include "calendar.h"
#include "enums.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "regional_settings.h"
#include "scp_294.h"
#include "type_id.h"
#include "units.h"

// The SCP-294 resolver is the one piece of the mod that is code, so it is the one
// piece that can be unit-tested without driving the UI.  These cases pin the
// behaviour the feature promises: whole-catalog liquid matching, case-insensitive,
// literal ids allowed, and the article's "solids never resolve" rule.
TEST_CASE( "scp_294_resolves_liquid_names_case_insensitively", "[scp_294]" )
{
    std::string msg;

    // A liquid's display name resolves, in any case.
    CHECK( scp_294_resolve( "clean water", msg ) == itype_id( "water_clean" ) );
    CHECK( scp_294_resolve( "Clean Water", msg ) == itype_id( "water_clean" ) );
    CHECK( scp_294_resolve( "CLEAN WATER", msg ) == itype_id( "water_clean" ) );

    // A literal item id is accepted too.
    CHECK( scp_294_resolve( "water_clean", msg ) == itype_id( "water_clean" ) );

    // Solids and nonsense can never resolve - this is the article's OUT OF RANGE.
    CHECK( scp_294_resolve( "diamond", msg ).is_null() );
    CHECK( scp_294_resolve( "a rock", msg ).is_null() );
    CHECK( scp_294_resolve( "", msg ).is_null() );
}

// The machine spawns by being added to two base-game map_extra_collections via
// `copy-from` + `extend`.  If that extend silently did nothing, SCP-294 would
// never generate anywhere, so this asserts the membership the placement rests
// on.  (A warning would also be emitted for an unsupported extend, but this is
// the end-to-end check.)  No-op unless the mod is loaded.
TEST_CASE( "scp_294_is_registered_in_its_map_extra_collections", "[scp_294]" )
{
    const map_extra_id scp294( "mx_scp_294" );
    if( !scp294.is_valid() ) {
        return;   // mod not loaded in this run
    }
    // Assert our entry is present AND a known base entry kept its original
    // weight (so `extend` merged, rather than replacing the whole list), and
    // pin the totals the rarity figures are derived from.
    CHECK( map_extra_collection_id( "research_facility_interior" )->values.get_specific_weight(
               scp294 ) == 1 );
    CHECK( map_extra_collection_id( "research_facility_interior" )->values.get_specific_weight(
               map_extra_id( "mx_crater" ) ) == 15000 );
    CHECK( map_extra_collection_id( "research_facility_interior" )->values.get_weight() == 23752 );

    CHECK( map_extra_collection_id( "build" )->values.get_specific_weight( scp294 ) == 1 );
    CHECK( map_extra_collection_id( "build" )->values.get_specific_weight(
               map_extra_id( "mx_house_spider" ) ) == 40 );
    CHECK( map_extra_collection_id( "build" )->values.get_weight() == 221 );
}

// The machine fills the cup with item::fill_with, which caps the pour by whichever
// of the cup's remaining volume or weight runs out first.  Two real bugs lived here
// and this pins both: dense liquids exceeded a 1 kg weight limit ("cup of gold" is
// 2500 g, a mercury pour is several kg) and surfaced as an on-screen debugmsg, and a
// 355 ml cup could not hold an indivisible 400-500 ml charge of ordinary drinks
// (bee's knees, bark tea, three sisters stew).
TEST_CASE( "scp_294_cup_can_pour_what_the_machine_offers", "[scp_294]" )
{
    const itype_id cup_id( "scp_294_cup" );
    if( !cup_id.is_valid() ) {
        return;   // mod not loaded in this run
    }

    // The specific liquids that broke it must stay pourable.
    const itype_id must_pour[] = {
        itype_id( "water_clean" ), itype_id( "coffee" ), itype_id( "beer" ),
        itype_id( "mercury" ), itype_id( "motor_oil" ),
        itype_id( "chem_sulphuric_acid" ), itype_id( "drink_beeknees" ),
        itype_id( "tea_bark" ), itype_id( "soup_three_sisters" ),
        itype_id( "scp_294_molten_gold" )
    };
    for( const itype_id &id : must_pour ) {
        if( !id.is_valid() ) {
            continue;   // not present in this run
        }
        item cup( cup_id, calendar::turn_zero );
        CAPTURE( id.str() );
        CHECK( cup.fill_with( item( id, calendar::turn_zero ) ) > 0 );
    }

    // And the overwhelming majority of the game's liquids must be pourable.
    int pourable = 0;
    int unpourable = 0;
    for( const itype *t : item_controller->all() ) {
        if( t->phase != phase_id::LIQUID ) {
            continue;
        }
        item cup( cup_id, calendar::turn_zero );
        if( cup.fill_with( item( t, calendar::turn_zero ) ) > 0 ) {
            pourable++;
        } else {
            unpourable++;
        }
    }
    CHECK( pourable > 0 );
    // A handful of chemistry batch items (e.g. 10 L per charge) genuinely cannot
    // be poured into a cup; everything else must work.
    CHECK( unpourable * 10 < pourable );
}

// "Cup of gold" cools: the dispensed molten gold is a timed transform (it carries
// SPAWN_ACTIVE, which calls item::activate() at creation, plus a countdown), and
// the cooled result is a separate gold item.  This pins that the pair exists and
// that the molten form is actually timed - the exact delay is playtest territory.
TEST_CASE( "scp_294_molten_gold_is_a_timed_transform", "[scp_294]" )
{
    const itype_id molten( "scp_294_molten_gold" );
    if( !molten.is_valid() ) {
        return;   // mod not loaded in this run
    }
    CHECK( item::type_is_defined( itype_id( "scp_294_gold_lump" ) ) );
    CHECK( molten->countdown_interval > 0_seconds );

    // The countdown only runs for an ACTIVE item (src/item.cpp:4770-4785), and a
    // comestible is active from construction because has_temperature() is true
    // (src/item.cpp:257-259, :3190-3192).  Assert that, so making the gold
    // non-comestible - or adding NO_TEMP to it - cannot silently stop it cooling.
    const item molten_sample( molten, calendar::turn_zero );
    CHECK( molten_sample.is_active() );
}

// The lump's weight is a literal in JSON, while the pour is computed at runtime
// (one serving, via item::fill_with).  Those two can silently desync: change the
// serving size, the cup, or the molten gold's own weight and the lump would keep
// claiming its old mass.  This mirrors the machine exactly - cup, one serving,
// fill_with - sums the mass actually poured, and requires it to equal the lump.
TEST_CASE( "scp_294_gold_lump_weighs_what_was_poured", "[scp_294]" )
{
    const itype_id cup_id( "scp_294_cup" );
    const itype_id molten( "scp_294_molten_gold" );
    const itype_id lump( "scp_294_gold_lump" );
    if( !molten.is_valid() ) {
        return;   // mod not loaded in this run
    }
    REQUIRE( cup_id.is_valid() );
    REQUIRE( lump.is_valid() );

    item liquid( molten, calendar::turn_zero );
    REQUIRE( liquid.count_by_charges() );

    // Mirror src/scp_294.cpp: pour one serving, capped by the cup.
    const int serving = std::max( 1, liquid.charges_per_volume( 250_ml, true ) );
    item cup( cup_id, calendar::turn_zero );
    const int poured = cup.fill_with( liquid, serving );
    REQUIRE( poured > 0 );

    // What the machine poured must weigh exactly what the cooled lump declares.
    item poured_gold( molten, calendar::turn_zero );
    poured_gold.charges = poured;
    const item cooled( lump, calendar::turn_zero );
    CAPTURE( poured, poured_gold.weight(), cooled.weight() );
    CHECK( poured_gold.weight() == cooled.weight() );
}

// The scripted table is only populated when the mod is loaded (the test run
// passes --mods), so this is a no-op without it and a real check with it: every
// item id the table names must resolve.  This is the runtime counterpart to the
// consistency check in src/scp_294.cpp.
TEST_CASE( "scp_294_special_requests_reference_real_items", "[scp_294]" )
{
    for( const scp_294_request &req : scp_294_requests() ) {
        if( !req.item.is_null() ) {
            CAPTURE( req.id );
            CHECK( req.item.is_valid() );
        }
    }
}
