// Cultivation Framework behavior tests (Lanes 1-5 + on-ramps).
//
// Each test drives a real mod EOC through the engine (direct activation or
// real turn simulation) and asserts D50 hooks / vitamin deltas / traits.
// JSON owns the hooks (design §16.12); this file owns the assertions.
//
// Run: tests/cata_test.exe "[cultivation]" --mods cultivation
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "event.h"
#include "event_bus.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"
#include "autoplay_helpers.h"

namespace
{

void give_sense( avatar &dummy )
{
    dummy.set_mutation( trait_id( "qi_sense" ) );
}

} // namespace

// ---- Lane 1: tick + meditation --------------------------------------------

TEST_CASE( "cultivation/meditate-assigns-trance",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    give_sense( dummy );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_MEDITATE_1H" ) );
    CHECK( autoplay::var_is( dummy, "cult_meditation_start", "yes" ) );
    CHECK( dummy.has_effect( efftype_id( "effect_cult_trance" ) ) );
    CHECK( dummy.has_activity( activity_id( "ACT_CULT_TRANCE" ) ) );
}

TEST_CASE( "cultivation/meditate-refuses-without-sense", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_MEDITATE_1H" ) );
    CHECK( !autoplay::has_var( dummy, "cult_meditation_start" ) );
    CHECK( !dummy.has_effect( efftype_id( "effect_cult_trance" ) ) );
}

TEST_CASE( "cultivation/tick-yields-qi-and-xp",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    give_sense( dummy );
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 0 );

    // Start a REAL trance activity (not just the effect mirror): the tick
    // only yields while an activity is actually running, by design.
    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_MEDITATE_1H" ) );
    REQUIRE( dummy.has_activity( activity_id( "ACT_CULT_TRANCE" ) ) );

    autoplay::seed_recurring_eocs( dummy );
    autoplay::advance_time( dummy, 61_seconds );

    CHECK( autoplay::var_is( dummy, "cult_meditation_yield", "yes" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) > 0 );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_xp_generic" ) ) == 1 );
}

TEST_CASE( "cultivation/tick-clamps-to-soft-cap", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    // Qi Condensation soft-cap is 300 (design §15.2); realm traits are required
    // for the realm rungs of the clamp ladder.
    dummy.set_mutation( trait_id( "cult_realm_qi_condensation" ) );
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 10000 );

    autoplay::seed_recurring_eocs( dummy );
    autoplay::advance_time( dummy, 61_seconds );

    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 300 );
    CHECK( autoplay::var_is( dummy, "cult_meditation_capped", "yes" ) );
}

TEST_CASE( "cultivation/mortal-qi-caps-at-50", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    // A realm-less mortal (qi_sense only) clamps at 50, not the old flat 300.
    give_sense( dummy );
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 1000 );

    autoplay::seed_recurring_eocs( dummy );
    autoplay::advance_time( dummy, 61_seconds );

    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 50 );
    CHECK( autoplay::var_is( dummy, "cult_meditation_capped", "yes" ) );
}

TEST_CASE( "cultivation/wound-caps-qi-150", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    // Wound cap (150) only bites when it is below the realm cap, so use a
    // cultivator (Qi Cond, cap 300) rather than a realm-less mortal (cap 50).
    dummy.set_mutation( trait_id( "cult_realm_qi_condensation" ) );
    dummy.add_effect( efftype_id( "effect_cult_meridian_wound" ), 1_days );
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 1000 );

    autoplay::seed_recurring_eocs( dummy );
    autoplay::advance_time( dummy, 61_seconds );

    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 150 );
}

// ---- Lane 2: breakthroughs -------------------------------------------------

TEST_CASE( "cultivation/breakthrough-qi-cond-succeeds",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    give_sense( dummy );
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 60 );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_BREAK_QI_COND" ) );
    CHECK( dummy.has_trait( trait_id( "cult_realm_qi_condensation" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_breakthrough_qi_cond_success", "yes" ) );
    // The realm teaches its signature art through the normal trait path.
    CHECK( dummy.magic->knows_spell( spell_id( "spell_cult_sword_qi" ) ) );
}

TEST_CASE( "cultivation/breakthrough-foundation-fail-wounds", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.set_mutation( trait_id( "cult_realm_qi_condensation" ) );
    dummy.vitamin_set( vitamin_id( "cult_xp_generic" ), 800 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 700 );
    dummy.i_add( item( itype_id( "cult_pill_foundation" ) ) );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_BREAK_FOUNDATION" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_xp_generic" ) ) == 400 );
    CHECK( dummy.has_effect( efftype_id( "effect_cult_meridian_wound" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_breakthrough_foundation_fail", "yes" ) );
    CHECK( !dummy.has_trait( trait_id( "cult_realm_foundation" ) ) );
}

TEST_CASE( "cultivation/breakthrough-foundation-success",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.set_mutation( trait_id( "cult_realm_qi_condensation" ) );
    dummy.vitamin_set( vitamin_id( "cult_xp_generic" ), 600 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 0 );
    dummy.i_add( item( itype_id( "cult_pill_foundation" ) ) );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_BREAK_FOUNDATION" ) );
    CHECK( dummy.has_trait( trait_id( "cult_realm_foundation" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_breakthrough_foundation_success", "yes" ) );
}

TEST_CASE( "cultivation/breakthrough-golden-arms-minor",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.set_mutation( trait_id( "cult_realm_foundation" ) );
    dummy.vitamin_set( vitamin_id( "cult_xp_generic" ), 2100 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 0 );
    dummy.i_add( item( itype_id( "cult_pill_core" ) ) );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_BREAK_GOLDEN" ) );
    CHECK( dummy.has_trait( trait_id( "cult_realm_golden_core" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_tribulation_minor", "yes" ) );
    CHECK( autoplay::var_is( dummy, "cult_breakthrough_golden_success", "yes" ) );
}

TEST_CASE( "cultivation/breakthrough-nascent-arms-major",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.set_mutation( trait_id( "cult_realm_golden_core" ) );
    dummy.vitamin_set( vitamin_id( "cult_xp_generic" ), 6100 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 0 );
    dummy.i_add( item( itype_id( "cult_pill_nascent" ) ) );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_BREAK_NASCENT" ) );
    CHECK( dummy.has_trait( trait_id( "cult_realm_nascent_soul" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_tribulation_major", "yes" ) );
    CHECK( autoplay::var_is( dummy, "cult_breakthrough_nascent_success", "yes" ) );
}

// ---- Lane 3: tribulation + deviation ---------------------------------------

TEST_CASE( "cultivation/tribulation-hooks-fire", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_TRIB_ZAP" ) );
    CHECK( autoplay::var_is( dummy, "cult_trib_zap_fired", "yes" ) );
    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_TRIB_MINOR" ) );
    CHECK( autoplay::var_is( dummy, "cult_trib_minor_fired", "yes" ) );
    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_TRIB_MAJOR" ) );
    CHECK( autoplay::var_is( dummy, "cult_trib_major_fired", "yes" ) );
}

TEST_CASE( "cultivation/deviation-300-taxes-silently", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 100 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 400 );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_DEVIATION" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 99 );
    CHECK( !autoplay::has_var( dummy, "cult_deviation_zap" ) );
}

TEST_CASE( "cultivation/deviation-600-self-zaps", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 100 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 700 );

    // 1-in-20 per fire; 500 fires make a miss astronomically unlikely.
    for( int i = 0; i < 500 && !autoplay::has_var( dummy, "cult_deviation_zap" ); ++i ) {
        autoplay::activate_eoc( dummy, "EOC_CULT_DEVIATION" );
    }
    CHECK( autoplay::has_var( dummy, "cult_deviation_zap" ) );
}

TEST_CASE( "cultivation/deviation-900-crisis",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 100 );
    dummy.vitamin_set( vitamin_id( "cult_impure_qi" ), 950 );

    // 1-in-10 per fire; bounded loop keeps the suite fast and near-certain.
    for( int i = 0; i < 300 && !autoplay::has_var( dummy, "cult_deviation_crisis" ); ++i ) {
        autoplay::activate_eoc( dummy, "EOC_CULT_DEVIATION" );
    }
    REQUIRE( autoplay::has_var( dummy, "cult_deviation_crisis" ) );
    const bool scarred = dummy.has_trait( trait_id( "SCHIZOPHRENIC" ) ) ||
                         dummy.has_trait( trait_id( "INSOMNIA" ) );
    CHECK( scarred );
}

// ---- Lane 4: Qi tax --------------------------------------------------------

TEST_CASE( "cultivation/tax-debits-with-fuel", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 100 );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_SWORD_QI_TAX" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 85 );
    CHECK( autoplay::var_is( dummy, "cult_tax_fired", "yes" ) );
}

TEST_CASE( "cultivation/tax-refuses-without-fuel", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    dummy.vitamin_set( vitamin_id( "cult_qi" ), 10 );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_SWORD_QI_TAX" ) );
    CHECK( !autoplay::has_var( dummy, "cult_tax_fired" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 10 );
}

// ---- Lane 5: debug kit ------------------------------------------------------

TEST_CASE( "cultivation/debug-gated-grants", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    // Gated: without the debug trait the EOC refuses.
    CHECK( !autoplay::activate_eoc( dummy, "EOC_CULT_DEBUG_GRANT_QI" ) );
    CHECK( !autoplay::has_var( dummy, "cult_debug_qi" ) );

    dummy.set_mutation( trait_id( "cult_debug_mode" ) );
    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_DEBUG_GRANT_QI" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 1000 );
    CHECK( autoplay::var_is( dummy, "cult_debug_qi", "yes" ) );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_DEBUG_SET_FOUNDATION" ) );
    CHECK( dummy.has_trait( trait_id( "cult_realm_foundation" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_debug_realm", "yes" ) );
}

// ---- On-ramps ----------------------------------------------------------------

TEST_CASE( "cultivation/fruit-burns-unsensed", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_EAT_SPIRIT_FRUIT" ) );
    // Realm-less mortal: the +300 grant clamps to the Mortal soft-cap (50),
    // not 300 (design §15.4 "grants to cap"; regression for the 300/50 bug).
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 50 );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_impure_qi" ) ) == 100 );
    // Progression head-start (design §15.6): unsensed pays less than sensed.
    CHECK( dummy.vitamin_get( vitamin_id( "cult_xp_generic" ) ) == 100 );
    // The fruit scorches but must never leave you burning.
    CHECK( !dummy.has_effect( efftype_id( "onfire" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_onramp_fruit", "yes" ) );
}

TEST_CASE( "cultivation/fruit-feeds-sensed", "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();
    give_sense( dummy );
    // A cultivator (Qi Cond, cap 300): the same +300 grant now fills to 300.
    dummy.set_mutation( trait_id( "cult_realm_qi_condensation" ) );

    REQUIRE( autoplay::activate_eoc( dummy, "EOC_CULT_EAT_SPIRIT_FRUIT" ) );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_qi" ) ) == 300 );
    CHECK( dummy.vitamin_get( vitamin_id( "cult_impure_qi" ) ) == 30 );
    // The controlled path pays more progression XP than the wild one (200 vs 100).
    CHECK( dummy.vitamin_get( vitamin_id( "cult_xp_generic" ) ) == 200 );
    CHECK( autoplay::var_is( dummy, "cult_onramp_fruit", "yes" ) );
}

TEST_CASE( "cultivation/manual-awakens-sense",
           "[cultivation][behavior]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &dummy = get_avatar();

    // Drive the real reads_book event through the bus with our manual as the
    // item, exactly as the read activity sends it in live play.
    get_event_bus().send<event_type::reads_book>( dummy.getID(),
            itype_id( "cult_manual_faded" ) );
    CHECK( dummy.has_trait( trait_id( "qi_sense" ) ) );
    CHECK( autoplay::var_is( dummy, "cult_onramp_manual", "yes" ) );
}
