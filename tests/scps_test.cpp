#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "faction.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map_helpers.h"
#include "mission.h"
#include "mongroup.h"
#include "npc_class.h"
#include "player_helpers.h"
#include "talker.h"
#include "type_id.h"

// Content ids introduced by the `scps` mod (SCP-173 / SCP-1499 / SCP-1162).
static const species_id species_SCP( "SCP" );

static const itype_id itype_scp_1499( "scp_1499" );
static const itype_id itype_note_173( "scp_foundation_note_173" );

static const flag_id flag_SCP_1499( "SCP_1499" );

static const mtype_id mon_scp_173( "mon_scp_173" );
static const mtype_id mon_scp_1499_1( "mon_scp_1499_1" );
static const mtype_id mon_zombie_scientist( "mon_zombie_scientist" );

static const spell_id spell_scp_173_freeze( "scp_173_freeze" );
static const spell_id spell_scp_173_los_check( "scp_173_los_check" );

static const efftype_id effect_scp_1499_dimension( "effect_scp_1499_dimension" );

static const mongroup_id GROUP_LAB( "GROUP_LAB" );
static const mongroup_id GROUP_SCP_1499_CATHEDRAL( "GROUP_SCP_1499_CATHEDRAL" );

static const oter_str_id oter_scp_1499_recovery_site( "scp_1499_recovery_site" );
static const oter_str_id oter_scp_1162_office( "scp_1162_office" );
static const oter_str_id oter_scp_1499_wasteland( "scp_1499_wasteland" );

static const effect_on_condition_id eoc_scp_1499_enter( "EOC_SCP_1499_ENTER" );
static const effect_on_condition_id eoc_scp_1162_use( "EOC_SCP_1162_USE" );

// Endgame foundation (see FOUNDATION_DESIGN.md): the Foundation outpost and its ruined variant,
// the remnant faction / sentry class, and the starter quest.
static const oter_str_id oter_scp_foundation_outpost( "scp_foundation_outpost" );
static const oter_str_id oter_scp_foundation_outpost_ruined( "scp_foundation_outpost_ruined" );
static const faction_id faction_scp_foundation_remnants( "scp_foundation_remnants" );
static const npc_class_id npc_class_scp_foundation_sentry( "NC_SCP_FOUNDATION_SENTRY" );
static const mission_type_id mission_scp_foundation_log( "MISSION_SCP_FOUNDATION_LOG" );
static const effect_on_condition_id eoc_scp_foundation_log_recovered( "EOC_SCP_FOUNDATION_LOG_RECOVERED" );

// SCP-494: the gloves and the material trades they enable.
static const itype_id itype_scp_494( "scp_494" );
static const flag_id flag_SCP_494( "SCP_494" );
static const itype_id itype_scp_494_plastic_diamond( "scp_494_plastic_diamond" );
static const itype_id itype_scp_494_diamond_sphere( "scp_494_diamond_sphere" );
static const itype_id itype_scp_494_steel_keypad_box( "scp_494_steel_keypad_box" );
static const itype_id itype_scp_494_micro_phone( "scp_494_micro_phone" );
static const itype_id itype_scp_494_gold_wristwatch( "scp_494_gold_wristwatch" );
static const itype_id itype_scp_494_gold_spoon( "scp_494_gold_spoon" );
static const itype_id itype_scp_494_gold_radio( "scp_494_gold_radio" );
static const itype_id itype_scp_494_silver_scissors( "scp_494_silver_scissors" );
static const itype_id itype_scp_494_silver_wrench( "scp_494_silver_wrench" );
static const itype_id itype_scp_494_lead_canteen( "scp_494_lead_canteen" );
static const itype_id itype_scp_494_stone_flashlight( "scp_494_stone_flashlight" );
static const itype_id itype_scp_494_bone_knife( "scp_494_bone_knife" );
static const itype_id itype_scp_494_glass_bottle( "scp_494_glass_bottle" );
static const itype_id itype_scp_494_diamond_spoon( "scp_494_diamond_spoon" );
static const effect_on_condition_id eoc_scp_494_use( "EOC_SCP_494_USE" );

// Every case is a no-op on a checkout that does not have `scps` loaded, so the
// suite can live in the shared test binary without disturbing other runs
// (the same guard style the other SCP cases use).
static bool scps_loaded()
{
    return itype_scp_1499.is_valid();
}

TEST_CASE( "scps_content_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( species_SCP.is_valid() );
    CHECK( mon_scp_173.is_valid() );
    CHECK( mon_scp_1499_1.is_valid() );
    CHECK( itype_scp_1499.is_valid() );
    CHECK( itype_note_173.is_valid() );
    CHECK( flag_SCP_1499.is_valid() );
    CHECK( spell_scp_173_freeze.is_valid() );
    CHECK( spell_scp_173_los_check.is_valid() );
    CHECK( effect_scp_1499_dimension.is_valid() );
    CHECK( GROUP_SCP_1499_CATHEDRAL.is_valid() );
    CHECK( oter_scp_1499_recovery_site.is_valid() );
    CHECK( oter_scp_1162_office.is_valid() );
    CHECK( oter_scp_1499_wasteland.is_valid() );
    CHECK( eoc_scp_1499_enter.is_valid() );
    CHECK( eoc_scp_1162_use.is_valid() );
}

// The mod adds SCP-173 to the vanilla lab spawn group by re-declaring GROUP_LAB
// with no `copy-from`.  Monster groups merge on a repeat id (src/mongroup.cpp),
// so the core spawns must survive.  A future edit that adds "override": true or
// renames the group would silently erase the vanilla lab population; this pins
// the additive behaviour that the mod depends on.
TEST_CASE( "scps_scp_173_merges_into_lab_spawns", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( GROUP_LAB.is_valid() );
    CHECK( MonsterGroupManager::IsMonsterInGroup( GROUP_LAB, mon_scp_173 ) );
    // Core lab populations must still be present: merge, not replace.
    CHECK( MonsterGroupManager::IsMonsterInGroup( GROUP_LAB, mon_zombie_scientist ) );
}

// The SCP-1499 mask drives its dimension mechanic through the SCP_1499 flag:
// the wear/takeoff events gate on `u_has_worn_with_flag: SCP_1499`.  Dropping
// the flag from the item would silently break the whole mechanic.
TEST_CASE( "scps_1499_mask_carries_its_wear_flag", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( itype_scp_1499.is_valid() );
    CHECK( itype_scp_1499->has_flag( flag_SCP_1499 ) );
}

// Behaviour: activating EOC_SCP_1499_ENTER applies the dimension effect that
// marks the wearer as displaced.  The full round trip also involves overmap
// travel and map generation (covered by the data-level cases above); this pins
// the observable side effect the EOC promises on activation.
TEST_CASE( "scps_1499_enter_applies_dimension_effect", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    clear_map_without_vision();
    avatar &you = get_avatar();
    dialogue d( get_talker_for( you ), nullptr );

    // Pre-set the build guard so the EOC skips its map-generation block: the
    // effect assertion does not depend on the generated entry building.
    you.set_value( "scp_1499_entry_built", "yes" );

    CHECK( eoc_scp_1499_enter->activate( d ) );
    CHECK( you.has_effect( effect_scp_1499_dimension ) );
}

// The endgame foundation must all register when the mod is loaded: the outpost and its ruined
// variant, the remnant faction, the sentry class, the starter quest, and its completion EOC.
// A rename or a broken cross-reference here would silently drop the whole facilities/factions/
// quests layer, so this pins it.
TEST_CASE( "scps_foundation_slice_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( oter_scp_foundation_outpost.is_valid() );
    CHECK( oter_scp_foundation_outpost_ruined.is_valid() );
    CHECK( faction_scp_foundation_remnants.is_valid() );
    CHECK( npc_class_scp_foundation_sentry.is_valid() );
    CHECK( mission_scp_foundation_log.is_valid() );
    CHECK( eoc_scp_foundation_log_recovered.is_valid() );
}

// SCP-494's gloves must register with their flag and the four trade results, and the activation
// EOC must exist.  A rename or a broken copy-from would silently drop the anomaly.
TEST_CASE( "scps_494_gloves_and_trades_are_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( itype_scp_494.is_valid() );
    CHECK( itype_scp_494->has_flag( flag_SCP_494 ) );
    CHECK( itype_scp_494_plastic_diamond.is_valid() );
    CHECK( itype_scp_494_diamond_sphere.is_valid() );
    CHECK( itype_scp_494_steel_keypad_box.is_valid() );
    CHECK( itype_scp_494_micro_phone.is_valid() );
    CHECK( itype_scp_494_gold_wristwatch.is_valid() );
    CHECK( itype_scp_494_gold_spoon.is_valid() );
    CHECK( itype_scp_494_gold_radio.is_valid() );
    CHECK( itype_scp_494_silver_scissors.is_valid() );
    CHECK( itype_scp_494_silver_wrench.is_valid() );
    CHECK( itype_scp_494_lead_canteen.is_valid() );
    CHECK( itype_scp_494_stone_flashlight.is_valid() );
    CHECK( itype_scp_494_bone_knife.is_valid() );
    CHECK( itype_scp_494_glass_bottle.is_valid() );
    CHECK( itype_scp_494_diamond_spoon.is_valid() );
    CHECK( eoc_scp_494_use.is_valid() );
}
