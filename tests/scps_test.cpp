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
#include "item_group.h"
#include "itype.h"
#include "map_helpers.h"
#include "mission.h"
#include "mongroup.h"
#include "mtype.h"
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
static const itype_id itype_scp_494_copper_hammer( "scp_494_copper_hammer" );
static const itype_id itype_scp_494_copper_screwdriver( "scp_494_copper_screwdriver" );
static const itype_id itype_scp_494_copper_bowl( "scp_494_copper_bowl" );
static const itype_id itype_scp_494_wooden_spoon( "scp_494_wooden_spoon" );
static const itype_id itype_scp_494_wooden_syringe( "scp_494_wooden_syringe" );
static const itype_id itype_scp_494_wooden_magnifier( "scp_494_wooden_magnifier" );
static const itype_id itype_scp_494_cotton_bowl( "scp_494_cotton_bowl" );
static const itype_id itype_scp_494_wooden_canteen( "scp_494_wooden_canteen" );
static const itype_id itype_scp_494_wooden_rope( "scp_494_wooden_rope" );
static const itype_id itype_scp_494_wooden_scissors( "scp_494_wooden_scissors" );
static const itype_id itype_scp_494_wooden_golf_ball( "scp_494_wooden_golf_ball" );
static const itype_id itype_scp_494_steel_rope( "scp_494_steel_rope" );
static const itype_id itype_scp_494_steel_bowl( "scp_494_steel_bowl" );
static const itype_id itype_scp_494_cloth_wrench( "scp_494_cloth_wrench" );
static const itype_id itype_scp_494_cloth_scissors( "scp_494_cloth_scissors" );
static const itype_id itype_scp_494_diamond_knife( "scp_494_diamond_knife" );
static const itype_id itype_scp_494_gold_bowl( "scp_494_gold_bowl" );
static const itype_id itype_scp_494_copper_rat( "scp_494_copper_rat" );
static const itype_id itype_scp_494_furry_disk( "scp_494_furry_disk" );
static const itype_id itype_scp_494_lab_rat( "scp_494_lab_rat" );
static const effect_on_condition_id eoc_scp_494_use( "EOC_SCP_494_USE" );

// SCP-701: the manuscript, the Hanged King it summons, and the recital reward.
static const itype_id itype_scp_701( "scp_701" );
static const itype_id itype_scp_701_knife( "scp_701_knife" );
static const flag_id flag_SCP_701( "SCP_701" );
static const mtype_id mon_scp_701_1( "mon_scp_701_1" );
static const efftype_id effect_scp_701_madness( "effect_scp_701_madness" );
static const spell_id spell_scp_701_recital( "spell_scp_701_recital" );
static const effect_on_condition_id eoc_scp_701_perform( "EOC_SCP_701_PERFORM" );
static const effect_on_condition_id eoc_scp_701_event( "EOC_SCP_701_THE_EVENT" );

// SCP-745: the highway hunter pair.
static const mtype_id mon_scp_745( "mon_scp_745" );
static const itype_id itype_scp_745_head( "scp_745_head" );
static const flag_id flag_SCP_745( "SCP_745" );
static const mongroup_id GROUP_SCP_745( "GROUP_SCP_745" );

// SCP-4059: the Chemosh rite and its site predators.
static const mtype_id mon_scp_4059_2( "mon_scp_4059_2" );
static const itype_id itype_scp_4059_journal( "scp_4059_journal" );
static const itype_id itype_scp_4059_death_cap( "scp_4059_death_cap" );
static const itype_id itype_scp_4059_horn( "scp_4059_horn" );
static const flag_id flag_SCP_4059( "SCP_4059" );
static const effect_on_condition_id eoc_scp_4059_read( "EOC_SCP_4059_READ" );
static const effect_on_condition_id eoc_scp_4059_chemosh( "EOC_SCP_4059_CHEMOSH" );
static const itype_id itype_scp_4059_fleshweaver_blade( "scp_4059_fleshweaver_blade" );
static const efftype_id effect_scp_4059_karcist( "effect_scp_4059_karcist" );

// SCP-3254: the cybernetic bear and her plush children.
static const mtype_id mon_scp_3254( "mon_scp_3254" );
static const mtype_id mon_scp_3254_1( "mon_scp_3254_1" );
static const mtype_id mon_scp_3254_1_adult( "mon_scp_3254_1_adult" );
static const itype_id itype_scp_foundation_note_3254( "scp_foundation_note_3254" );
static const itype_id itype_teddy_bear( "teddy_bear" );
static const spell_id spell_scp_3254_mend( "scp_3254_mend" );
static const effect_on_condition_id eoc_scp_3254_birthday( "EOC_SCP_3254_BIRTHDAY" );

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
    CHECK( itype_scp_494_copper_hammer.is_valid() );
    CHECK( itype_scp_494_copper_screwdriver.is_valid() );
    CHECK( itype_scp_494_copper_bowl.is_valid() );
    CHECK( itype_scp_494_wooden_spoon.is_valid() );
    CHECK( itype_scp_494_wooden_syringe.is_valid() );
    CHECK( itype_scp_494_wooden_magnifier.is_valid() );
    CHECK( itype_scp_494_cotton_bowl.is_valid() );
    CHECK( itype_scp_494_wooden_canteen.is_valid() );
    CHECK( itype_scp_494_wooden_rope.is_valid() );
    CHECK( itype_scp_494_wooden_scissors.is_valid() );
    CHECK( itype_scp_494_wooden_golf_ball.is_valid() );
    CHECK( itype_scp_494_steel_rope.is_valid() );
    CHECK( itype_scp_494_steel_bowl.is_valid() );
    CHECK( itype_scp_494_cloth_wrench.is_valid() );
    CHECK( itype_scp_494_cloth_scissors.is_valid() );
    CHECK( itype_scp_494_diamond_knife.is_valid() );
    CHECK( itype_scp_494_gold_bowl.is_valid() );
    CHECK( itype_scp_494_copper_rat.is_valid() );
    CHECK( itype_scp_494_furry_disk.is_valid() );
    CHECK( itype_scp_494_lab_rat.is_valid() );
    CHECK( eoc_scp_494_use.is_valid() );
}

// SCP-494 must be findable as a rare random find: injected into the science and mutagen-lab loot
// groups.  A future edit that dropped the injection (or a silent overwrite of the group) would make
// the gloves unobtainable short of wishing, so this pins it.
TEST_CASE( "scps_494_spawns_as_rare_lab_loot", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( item_group::group_contains_item( item_group_id( "science" ), itype_scp_494 ) );
    CHECK( item_group::group_contains_item( item_group_id( "mut_lab" ), itype_scp_494 ) );
}

// SCP-701: the manuscript, its flag, the Hanged King it summons, the madness effect, the recital
// reward spell, and the performance EOCs must all register when the mod is loaded.
TEST_CASE( "scps_701_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( itype_scp_701.is_valid() );
    CHECK( itype_scp_701->has_flag( flag_SCP_701 ) );
    CHECK( itype_scp_701_knife.is_valid() );
    CHECK( mon_scp_701_1.is_valid() );
    CHECK( effect_scp_701_madness.is_valid() );
    CHECK( spell_scp_701_recital.is_valid() );
    CHECK( eoc_scp_701_perform.is_valid() );
    CHECK( eoc_scp_701_event.is_valid() );
}

// The manuscript must turn up in the world: the library bookcase and household shelf groups carry
// it, so a copy can be found rather than only wished up.
TEST_CASE( "scps_701_spawns_in_library_loot", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( item_group::group_contains_item( item_group_id( "SUS_fiction_bookcase" ), itype_scp_701 ) );
    CHECK( item_group::group_contains_item( item_group_id( "homebooks" ), itype_scp_701 ) );
}

// SCP-745: the luminous highway predator, its trophy head, and its nocturnal pair group must all
// register when the mod is loaded.
TEST_CASE( "scps_745_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( mon_scp_745.is_valid() );
    CHECK( mon_scp_745->luminance > 0 );
    CHECK( itype_scp_745_head.is_valid() );
    CHECK( itype_scp_745_head->has_flag( flag_SCP_745 ) );
    CHECK( GROUP_SCP_745.is_valid() );
}

// SCP-4059: the hircine predator, the journal, the death caps, the horn, the Chemosh rite, and its
// EOCs must all register when the mod is loaded.
TEST_CASE( "scps_4059_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( mon_scp_4059_2.is_valid() );
    CHECK( itype_scp_4059_journal.is_valid() );
    CHECK( itype_scp_4059_journal->has_flag( flag_SCP_4059 ) );
    CHECK( itype_scp_4059_death_cap.is_valid() );
    CHECK( itype_scp_4059_horn.is_valid() );
    CHECK( eoc_scp_4059_read.is_valid() );
    CHECK( eoc_scp_4059_chemosh.is_valid() );
    CHECK( itype_scp_4059_fleshweaver_blade.is_valid() );
    CHECK( itype_scp_4059_fleshweaver_blade->has_flag( flag_SCP_4059 ) );
    CHECK( effect_scp_4059_karcist.is_valid() );
    CHECK( item_group::group_contains_item( item_group_id( "scp_4059_foundation_cache" ),
                                            itype_scp_4059_fleshweaver_blade ) );
}

// SCP-3254: the docile cybernetic bear, her juvenile that grows up, the containment file, and the
// healing aura she casts on her brood must all register.
TEST_CASE( "scps_3254_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( mon_scp_3254.is_valid() );
    CHECK( mon_scp_3254_1.is_valid() );
    CHECK( mon_scp_3254_1_adult.is_valid() );
    CHECK( itype_scp_foundation_note_3254.is_valid() );
    CHECK( spell_scp_3254_mend.is_valid() );
    CHECK( eoc_scp_3254_birthday.is_valid() );
    CHECK( item_group::group_contains_item( item_group_id( "scp_3254_enclosure_loot" ),
                                            itype_teddy_bear ) );
}
