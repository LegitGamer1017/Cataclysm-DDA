#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "faction.h"
#include "field_type.h"
#include "game.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "mission.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc_class.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "talker.h"
#include "type_id.h"

// ---------------------------------------------------------------------------------------------
// This file tests the `scps` mod.  `data/mods/cdda-scps` is gitignored in the game repo, so a
// checkout of the engine alone compiles these cases in and they are inert.
//
// DECISION (recorded at the harness's request, 2026-10-11): the absence is made LOUD.
// Every case below early-returns when the mod is not loaded, so without it the suite reads
// "37 cases, 0 assertions, all passed" -- a green tick that proves nothing.  `scps_mod_is_loaded`
// is the tripwire: it is deliberately unguarded and FAILS on any run that does not have `scps`,
// so a hollow green cannot be read as a pass.  If you are seeing that failure, either clone the mod
// or deselect the [scps] tag; do not read the run as coverage.
// ---------------------------------------------------------------------------------------------

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

// SCP-1884: the blind fortune-teller, the mass of hands that is her eyes, and the book she asks for.
static const npc_class_id npc_class_scp_1884_a( "NC_SCP_1884_A" );
static const faction_id faction_scp_1884_custody( "scp_1884_custody" );
static const trait_id trait_scp_1884_blind( "SCP_1884_BLIND" );
static const trait_id trait_scp_1884_no_hands( "SCP_1884_NO_HANDS" );
static const trait_id trait_scp_1884_taken_hand_r( "SCP_1884_TAKEN_HAND_R" );
static const trait_id trait_scp_1884_taken_hand_l( "SCP_1884_TAKEN_HAND_L" );
static const mtype_id mon_scp_1884_b( "mon_scp_1884_b" );
static const efftype_id effect_scp_1884_shared_pain( "effect_scp_1884_shared_pain" );
static const mission_type_id mission_scp_1884_books( "MISSION_SCP_1884_BOOKS" );
static const itype_id itype_scp_1884_shed_skin( "scp_1884_shed_skin" );
static const itype_id itype_note_1884( "scp_foundation_note_1884" );
static const itype_id itype_classic_literature( "classic_literature" );
static const oter_str_id oter_scp_1884_site59( "scp_1884_site59" );
static const effect_on_condition_id eoc_scp_1884_takes_a_hand( "EOC_SCP_1884_TAKES_A_HAND" );
static const effect_on_condition_id eoc_scp_1884_books_recovered( "EOC_SCP_1884_BOOKS_RECOVERED" );

// SCP-9044: the hallway.  The far door, the marker tile the loop returns you to, the loop itself,
// and the keepsakes and gift handed out along the way.
static const oter_str_id oter_scp_9044_house( "scp_9044_house" );
static const ter_str_id ter_scp_9044_door( "t_scp_9044_door" );
static const ter_str_id ter_scp_9044_hall_start( "t_scp_9044_hall_start" );
static const efftype_id effect_scp_9044_cold( "effect_scp_9044_cold" );
static const efftype_id effect_scp_9044_touched( "effect_scp_9044_touched" );
static const itype_id itype_scp_9044_gift( "scp_9044_housewarming_gift" );
static const itype_id itype_scp_9044_card( "scp_9044_housewarming_card" );
static const itype_id itype_scp_9044_album( "scp_9044_photo_album" );
static const itype_id itype_note_9044( "scp_foundation_note_9044" );
static const effect_on_condition_id eoc_scp_9044_step( "EOC_SCP_9044_STEP" );

// SCP-8000: Site-322, the SEEKER machine, and the four things it found.
static const oter_str_id oter_scp_8000_site322( "scp_8000_site322" );
static const ter_str_id ter_scp_8000_seeker( "t_scp_8000_seeker" );
static const furn_str_id furn_scp_8000_console( "f_scp_8000_console" );
static const furn_str_id furn_scp_8000_stabiliser( "f_scp_8000_stabiliser" );
static const itype_id itype_note_8000( "scp_foundation_note_8000" );
static const mtype_id mon_scp_01_seek( "mon_scp_01_seek" );
static const mtype_id mon_scp_04_seek( "mon_scp_04_seek" );
static const mtype_id mon_scp_13_seek( "mon_scp_13_seek" );
static const mtype_id mon_scp_22_seek( "mon_scp_22_seek" );
static const effect_on_condition_id eoc_scp_8000_seeker( "EOC_SCP_8000_SEEKER" );
static const effect_on_condition_id eoc_scp_8000_push( "EOC_SCP_8000_PUSH" );

// SCP-8000, Stage 2: the rift, Memory Lane, Wallace, and the unwritten book.
static const oter_str_id oter_scp_8000_library( "scp_8000_library" );
// (oter_scp_8000_reading_room removed here: the special it named was deleted and no such OMT exists,
// so the declaration was a stale id check that could never have been reached.)
static const ter_str_id ter_scp_8000_rift( "t_scp_8000_rift" );
static const ter_str_id ter_scp_8000_rift_home( "t_scp_8000_rift_home" );
static const ter_str_id ter_scp_8000_library_floor( "t_scp_8000_library_floor" );
static const mtype_id mon_scp_8000_wallace( "mon_scp_8000_wallace" );
static const itype_id itype_unwritten_book( "scp_8000_unwritten_book" );
static const trait_id trait_second_chance_spent( "SCP_8000_SECOND_CHANCE_SPENT" );
static const effect_on_condition_id eoc_scp_8000_rift( "EOC_SCP_8000_RIFT" );
static const effect_on_condition_id eoc_scp_8000_rift_home( "EOC_SCP_8000_RIFT_HOME" );
static const effect_on_condition_id eoc_scp_8000_book_saves_you( "EOC_SCP_8000_BOOK_SAVES_YOU" );

// SCP-8001: the island of Last Watch, the Tower of Sunset, the edge itself, and the clockwork
// keeper who has tended the light since before anyone can remember.
static const oter_str_id oter_scp_8001_edge( "scp_8001_edge" );
static const oter_str_id oter_scp_8001_edge_s( "scp_8001_edge_s" );
static const oter_str_id oter_scp_8001_walkway( "scp_8001_walkway" );
static const oter_str_id oter_scp_8001_woods_e( "scp_8001_woods_e" );
static const oter_str_id oter_scp_8001_bay( "scp_8001_bay" );
static const oter_str_id oter_scp_8001_stair( "scp_8001_stair" );
static const oter_str_id oter_scp_8001_hill( "scp_8001_hill" );
static const oter_str_id oter_scp_8001_graves( "scp_8001_graves" );
static const oter_str_id oter_scp_8001_outcrop( "scp_8001_outcrop" );
static const oter_str_id oter_scp_8001_woods( "scp_8001_woods" );
static const oter_str_id oter_scp_8001_wood( "scp_8001_wood" );
static const oter_str_id oter_scp_8001_tower( "scp_8001_tower" );
static const oter_str_id oter_scp_8001_tower_z1( "scp_8001_tower_z1" );
static const oter_str_id oter_scp_8001_tower_z2( "scp_8001_tower_z2" );
static const furn_str_id furn_scp_8001_lamp( "f_scp_8001_lamp" );
static const furn_str_id furn_scp_8001_reflector( "f_scp_8001_reflector" );
static const furn_str_id furn_scp_8001_headstone( "f_scp_8001_headstone" );
static const furn_str_id furn_scp_8001_memorial( "f_scp_8001_memorial" );
static const mtype_id mon_scp_8001_aurelie( "mon_scp_8001_aurelie" );
static const itype_id itype_scp_8001_chart( "scp_8001_chart" );
static const itype_id itype_scp_8001_chart_annotated( "scp_8001_chart_annotated" );
static const itype_id itype_scp_8001_mp3_dead( "scp_8001_mp3_dead" );
static const itype_id itype_scp_8001_lens( "scp_8001_lens" );
static const itype_id itype_scp_8001_record( "scp_8001_record" );
static const field_type_str_id fd_scp_8001_spray( "fd_scp_8001_spray" );
static const effect_on_condition_id eoc_scp_8001_chart( "EOC_SCP_8001_CHART" );
static const effect_on_condition_id eoc_scp_8001_chart_studied( "EOC_SCP_8001_CHART_STUDIED" );
static const effect_on_condition_id eoc_scp_8001_falls( "EOC_SCP_8001_FALLS" );
static const effect_on_condition_id eoc_scp_8001_memorial( "EOC_SCP_8001_MEMORIAL" );

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
static const itype_id itype_scp_3254_grail_letter( "scp_3254_grail_letter" );
static const itype_id itype_scp_3254_grail_tools( "scp_3254_grail_tools" );

// SCP-287: the Meteor Lord's sword, its ghosts, and the two sites.
static const itype_id itype_scp_287( "scp_287" );
static const itype_id itype_scp_287_journal( "scp_287_journal" );
static const flag_id flag_SCP_287( "SCP_287" );
static const mtype_id mon_scp_287_ghost( "mon_scp_287_ghost" );
static const efftype_id effect_scp_287_headache( "effect_scp_287_headache" );
static const spell_id spell_scp_287_rout( "spell_scp_287_rout" );
static const effect_on_condition_id eoc_scp_287_activate( "EOC_SCP_287_ACTIVATE" );

// Every case is a no-op on a checkout that does not have `scps` loaded, so the
// suite can live in the shared test binary without disturbing other runs
// (the same guard style the other SCP cases use).
static bool scps_loaded()
{
    return itype_scp_1499.is_valid();
}

// The tripwire described in the file header.  Unguarded on purpose: this is the one case that must
// NOT early-return, so that a checkout without the mod reports a failure instead of a hollow pass
// with zero assertions in 37 cases.
TEST_CASE( "scps_mod_is_loaded", "[scps]" )
{
    REQUIRE( scps_loaded() );
    CHECK( itype_scp_1499.is_valid() );
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
    // The Grail thread: Katherine's letter and her tools, at her zoo.
    CHECK( itype_scp_3254_grail_letter.is_valid() );
    CHECK( itype_scp_3254_grail_tools.is_valid() );
    CHECK( item_group::group_contains_item( item_group_id( "scp_grail_zoo_loot" ),
                                            itype_scp_3254_grail_tools ) );
}

// SCP-287: the powered Viking sword, the phantom it spawns, the headache it leaves, the rout it
// casts, and its activation EOC must all register, and the barrow must carry the sword.
TEST_CASE( "scps_287_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( itype_scp_287.is_valid() );
    CHECK( itype_scp_287->has_flag( flag_SCP_287 ) );
    CHECK( itype_scp_287_journal.is_valid() );
    CHECK( mon_scp_287_ghost.is_valid() );
    CHECK( effect_scp_287_headache.is_valid() );
    CHECK( spell_scp_287_rout.is_valid() );
    CHECK( eoc_scp_287_activate.is_valid() );
    CHECK( item_group::group_contains_item( item_group_id( "scp_287_mound_loot" ), itype_scp_287 ) );
}

// SCP-287's sword must not be a self-maiming weapon.  HURT_WHEN_WIELDED fires on a melee attack
// (two swings in three) and deals the weapon's own cut damage to the right hand, ignoring gloves
// (src/melee.cpp:2314-2323) -- on this cut-30 sword that is 30 cut to hand_r.  The handling hazard
// belongs in the activation EOC, gated on the article's insulated gloves, and nowhere else.
TEST_CASE( "scps_287_is_not_hurt_when_wielded", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    static const flag_id flag_hurt_when_wielded( "HURT_WHEN_WIELDED" );
    static const flag_id flag_fragile_melee( "FRAGILE_MELEE" );
    static const flag_id flag_no_repair( "NO_REPAIR" );
    static const itype_id itype_battery( "battery" );

    avatar &u = get_avatar();
    item sword( itype_scp_287 );
    CHECK_FALSE( sword.has_flag( flag_hurt_when_wielded ) );
    CHECK( sword.has_flag( flag_fragile_melee ) );
    CHECK( sword.has_flag( flag_no_repair ) );

    // ...and it must not fire on a dead cell: `has_ammo` (ammo_sufficient) is what the EOC's
    // guard reads, and it has to be false empty and true once there is a current.
    CHECK_FALSE( sword.ammo_sufficient( &u ) );
    sword.ammo_set( itype_battery, 1000 );
    CHECK( sword.ammo_remaining() == 1000 );
    CHECK( sword.ammo_sufficient( &u ) );
}

// The activation rides on the charge: a loaded sword fills the world with phantoms, a dead one
// does nothing at all.  Insulated gloves are worn so the EOC takes its ghost branch rather than
// the bare-hand shock.
TEST_CASE( "scps_287_scales_phantoms_with_charge", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    static const itype_id itype_battery( "battery" );
    // The mod's OWN insulated gloves, by their namespaced id.  This used to be core `gloves_rubber`,
    // which only satisfied the safety gate because the mod overwrote that core item -- so the test
    // was quietly asserting the bug (287-5) rather than the behaviour.
    static const itype_id itype_scp_287_gloves( "scp_287_gloves" );

    clear_map();
    avatar &u = get_avatar();
    u.wear_item( item( itype_scp_287_gloves ) );

    const auto phantoms = []() {
        int n = 0;
        for( monster &m : g->all_monsters() ) {
            if( m.is_hallucination() ) {
                n++;
            }
        }
        return n;
    };

    const int before = phantoms();

    item_location loaded_loc = u.i_add( item( itype_scp_287 ) );
    loaded_loc->ammo_set( itype_battery, 1000 );
    dialogue full( get_talker_for( u ), get_talker_for( loaded_loc ) );
    eoc_scp_287_activate->activate( full );
    const int after_full = phantoms();
    CHECK( after_full > before );

    item_location dead_loc = u.i_add( item( itype_scp_287 ) );
    dialogue spent( get_talker_for( u ), get_talker_for( dead_loc ) );
    eoc_scp_287_activate->activate( spent );
    CHECK( phantoms() == after_full );
}

// SCP-1884: the blind fortune-teller, the mass of hands that is her eyes, and the book they ask
// for.  She is an NPC that the world cannot make (a blind mutation on an npc_class), he is a
// monster the player can hold a conversation with, and the quest is for a book the game already
// carries in two of its variants -- so the whole anomaly hangs off ids that must exist.
// SCP-1162's one mechanic, and the case that would have caught both of its critical bugs at once.
// The cost used to be free for a stack larger than the request -- u_consume_item_sum has an upstream
// branch that subtracts zero in exactly that case, and paper is charge-counted -- and it could be
// gathered off the floor or out of a vehicle rather than out of the pack.
TEST_CASE( "scps_1162_reaching_in_costs_an_item", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    const itype_id paper( "paper" );
    const itype_id watch( "wristwatch" );
    const effect_on_condition_id take( "EOC_SCP_1162_TAKE" );
    REQUIRE( take.is_valid() );

    // A five-charge paper stack, which is precisely the case that used to cost nothing.
    clear_avatar();
    avatar &you = get_avatar();
    for( int i = 0; i < 5; i++ ) {
        you.i_add( item( paper ) );
    }
    REQUIRE( you.charges_of( paper ) == 5 );
    dialogue d( get_talker_for( you ), nullptr );
    take->activate( d );
    CHECK( you.charges_of( paper ) == 4 );

    // And an unstackable item, so the chain's other branch is exercised too.
    clear_avatar();
    avatar &you2 = get_avatar();
    you2.i_add( item( watch ) );
    REQUIRE( you2.amount_of( watch ) == 1 );
    dialogue d2( get_talker_for( you2 ), nullptr );
    take->activate( d2 );
    CHECK( you2.amount_of( watch ) == 0 );
}

// SCP-4059's ritual IS the SCP: the journal teaches it, and the rite needs the reservoir, at night,
// under a full moon.  Neither half had a case, which is how a 48-tile-wide "at the reservoir" gate and
// a purely decorative night gate both shipped.  The mapgen half is the §7 trap made a regression test:
// a furniture-only palette char inherits the PREDECESSOR floor unless it maps its own terrain.
TEST_CASE( "scps_4059_the_rite_is_learned_and_gated", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    const recipe_id chemosh_id( "recipe_scp_4059_chemosh" );
    const effect_on_condition_id read_eoc( "EOC_SCP_4059_READ" );
    const effect_on_condition_id rite_eoc( "EOC_SCP_4059_CHEMOSH" );
    REQUIRE( chemosh_id.is_valid() );
    const recipe *chemosh = &chemosh_id.obj();

    clear_avatar();
    avatar &you = get_avatar();
    dialogue d( get_talker_for( you ), nullptr );

    // Reading the journal is what teaches the rite.
    CHECK_FALSE( you.knows_recipe( chemosh ) );
    read_eoc->activate( d );
    CHECK( you.knows_recipe( chemosh ) );

    // And the rite cannot be performed away from the reservoir: activate() on an ACTIVATION EOC
    // returns its condition, so this is the gate itself being asserted.
    CHECK_FALSE( rite_eoc->activate( d ) );

    // The clearing's own tiles: even-aligned, because a tinymap's origin is the even corner.
    tripoint_abs_omt base = get_avatar().pos_abs_omt() + point( 40, 40 );
    base.x() -= base.x() % 2;
    base.y() -= base.y() % 2;

    const auto generate = []( const tripoint_abs_omt & where, const std::string & omt,
    tinymap & tm ) {
        overmap_buffer.ter_set( where, oter_id( omt ) );
        tm.load( where, true );
        mapgendata md( where, *tm.cast_to_map(), 0.0f, calendar::turn, nullptr );
        run_mapgen_func( omt, md );
    };

    // The cabin crates stand on the cabin's own boards, not on the predecessor forest floor.
    tinymap ne;
    generate( base + point::east, "scp_4059_clearing_ne", ne );
    CHECK( ne.furn( tripoint_omt_ms{ 13, 7, 0 } ) == furn_id( "f_crate_c" ) );
    CHECK( ne.ter( tripoint_omt_ms{ 13, 7, 0 } ) == ter_id( "t_dirtfloor" ) );

    // And the grave crosses stand in the grave.
    tinymap nw;
    generate( base, "scp_4059_clearing_nw", nw );
    CHECK( nw.furn( tripoint_omt_ms{ 5, 20, 0 } ) == furn_id( "f_battlefield_cross" ) );
    CHECK( nw.ter( tripoint_omt_ms{ 5, 20, 0 } ) == ter_id( "t_pit_corpsed" ) );
}

TEST_CASE( "scps_1884_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( mon_scp_1884_b.is_valid() );

    CHECK( trait_scp_1884_blind.is_valid() );
    CHECK( trait_scp_1884_no_hands.is_valid() );
    CHECK( trait_scp_1884_taken_hand_r.is_valid() );
    CHECK( trait_scp_1884_taken_hand_l.is_valid() );

    CHECK( effect_scp_1884_shared_pain.is_valid() );
    CHECK( eoc_scp_1884_takes_a_hand.is_valid() );
    CHECK( eoc_scp_1884_books_recovered.is_valid() );

    CHECK( npc_class_scp_1884_a.is_valid() );
    CHECK( faction_scp_1884_custody.is_valid() );
    CHECK( oter_scp_1884_site59.is_valid() );
    CHECK( itype_scp_1884_shed_skin.is_valid() );
    CHECK( itype_note_1884.is_valid() );

    CHECK( mission_scp_1884_books.is_valid() );
    CHECK( itype_classic_literature.is_valid() );
}

// SCP-9044: the house, the hallway that does not end, and what it hands out along the way.  The
// loop itself is two effects -- a terrain search for the marker tile and a teleport back to it --
// so if the marker terrain, the far door's EOC, or any of the twenty-one stages' effects and
// keepsakes fail to load, the hallway stops being a hallway.
TEST_CASE( "scps_9044_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( oter_scp_9044_house.is_valid() );
    CHECK( ter_scp_9044_hall_start.is_valid() );
    REQUIRE( ter_scp_9044_door.is_valid() );
    CHECK( eoc_scp_9044_step.is_valid() );

    CHECK( effect_scp_9044_cold.is_valid() );
    CHECK( effect_scp_9044_touched.is_valid() );

    CHECK( itype_scp_9044_gift.is_valid() );
    CHECK( itype_scp_9044_card.is_valid() );
    CHECK( itype_scp_9044_album.is_valid() );
    CHECK( itype_note_9044.is_valid() );
}

// SCP-9044's loop, driven for real.  Examining the far door has to count one deeper, put the
// player back on the marker tile at the head of the hallway, and -- at the two depths the log
// marks -- rebuild the standing hallway around them.  Registration alone would not catch a
// broken teleport target or an update mapgen that never applies.
//
// The teleport is checked as "you are standing on the marker", not "you are on this square":
// u_location_variable's terrain search returns the nearest matching tile, which is not the same
// square under every preceding test's map.  Where the player actually ends up relative to the
// update mapgens' fixed squares is what the transform checks below pin down.
TEST_CASE( "scps_9044_step_loops_and_transforms", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    avatar &you = get_avatar();

    const tripoint_bub_ms hall_start( 20, 20, 0 );
    const tripoint_bub_ms far_door( 20, 8, 0 );
    here.ter_set( hall_start, ter_id( "t_scp_9044_hall_start" ) );
    here.ter_set( far_door, ter_id( "t_scp_9044_door" ) );
    you.setpos( here, far_door );

    dialogue d( get_talker_for( you ), nullptr );

    // The loop itself: one deeper, and back on the marker tile at the head of the hallway.
    REQUIRE( eoc_scp_9044_step->activate( d ) );
    CHECK( you.get_value( "scp_9044_depth" ) == 1 );
    CHECK( here.ter( you.pos_bub() ) == ter_id( "t_scp_9044_hall_start" ) );

    // The update mapgens write at fixed squares of the overmap tile the hall is on, so the
    // tiles to inspect are that tile's corner plus the coordinates the update names.
    const tripoint_bub_ms omt_corner = here.get_bub( project_to<coords::ms>( you.pos_abs_omt() ) );
    const tripoint_bub_ms table_pos = omt_corner + tripoint( 11, 9, 0 );
    const tripoint_bub_ms bench_pos = omt_corner + tripoint( 12, 9, 0 );

    // "How cozy" -- the hallway stops being bare.
    you.set_value( "scp_9044_depth", 7 );
    you.setpos( here, far_door );
    REQUIRE( eoc_scp_9044_step->activate( d ) );
    CHECK( you.get_value( "scp_9044_depth" ) == 8 );
    CHECK( here.furn( table_pos ) == furn_id( "f_table" ) );
    CHECK( here.furn( bench_pos ) == furn_id( "f_bench" ) );

    // "It's cold" -- furniture gone, and the boards gone with it.
    you.set_value( "scp_9044_depth", 13 );
    you.setpos( here, far_door );
    REQUIRE( eoc_scp_9044_step->activate( d ) );
    CHECK( you.get_value( "scp_9044_depth" ) == 14 );
    CHECK( here.ter( table_pos ) == ter_id( "t_thconc_floor" ) );
    CHECK( here.furn( table_pos ) == furn_id( "f_null" ) );
}

// SCP-9044's way out.  The loop teleports the player to the head of the hallway, and the SCP's
// whole bargain is that turning around from there works.  This builds the real house from the
// mod's mapgen -- forcing a map tile to scp_9044_house and loading it runs the mapgen -- and
// checks the route at the mapgen's own coordinates: the marker tile the loop searches for, the
// front door, the chain gate, and no wall anywhere along the column between them.  Doors and
// gates open; walls do not, so "not a wall" is precisely what makes the escape possible.  This is
// also the only test of the fence gate: without it the lot is sealed and the SCP is unreachable.
TEST_CASE( "scps_9044_has_a_way_out", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    // A tinymap is 2x2 OMTs and load() takes its origin from the EVEN-aligned corner, so the tile
    // coordinates below are only the OMT we think they are when `where` is already even.  Rounding it
    // here is what keeps this case independent of where earlier cases left the avatar -- it used to
    // pass alone and fail in-suite for exactly that reason.
    tripoint_abs_omt where = get_avatar().pos_abs_omt() + tripoint( 4, 4, 0 );
    where.x() -= where.x() % 2;
    where.y() -= where.y() % 2;
    // Force the terrain as well as the load: `load` on a tile nothing has generated can hand back a
    // default OMT, and the mapgen then writes over someone else's predecessor -- which is how this
    // case read back t_wall_wood instead of the door on a run where the avatar started elsewhere.
    overmap_buffer.ter_set( where, oter_id( "scp_9044_house" ) );
    tinymap tm;
    tm.load( where, true );
    // tinymap::load() gives a default OMT; the terrain's own mapgen has to be run explicitly.
    // This is the sequence tests/mapgen_helpers.cpp uses for nested mapgens.
    mapgendata md( where, *tm.cast_to_map(), 0.0f, calendar::turn, nullptr );
    run_mapgen_func( oter_scp_9044_house.str(), md );

    // The tiles the loop and the escape depend on, at the layout the mapgen writes.
    CHECK( tm.ter( tripoint_omt_ms{ 11, 5, 0 } ) == ter_id( "t_scp_9044_door" ) );
    CHECK( tm.ter( tripoint_omt_ms{ 11, 17, 0 } ) == ter_id( "t_scp_9044_hall_start" ) );
    CHECK( tm.ter( tripoint_omt_ms{ 11, 18, 0 } ) == ter_id( "t_door_c" ) );
    CHECK( tm.ter( tripoint_omt_ms{ 11, 21, 0 } ) == ter_id( "t_chaingate_c" ) );

    // Down the hall to the front door, then across the drive to the gate in the fence.  "Not a wall"
    // was not enough: t_tree and t_chainfence satisfy that too and are every bit as impassable, which
    // is how a gate opening into the tree border went unnoticed.  The row now runs to 23, the map edge.
    for( int y = 6; y <= 23; y++ ) {
        const ter_id t = tm.ter( tripoint_omt_ms{ 11, y, 0 } );
        CHECK( t != ter_id( "t_wall_wood" ) );
        CHECK( t != ter_id( "t_chainfence" ) );
        CHECK( t != ter_id( "t_tree" ) );
    }
}

// The end of the walk, which nothing else exercises: the touch at nineteen, the gift at
// twenty-two, and -- the reason this needs its own case -- that twenty-two is the *last* lap that
// pays.  The counter keeps climbing, and at twenty-three the door still does not open and the
// house says nothing at all.  Before this, the reward was the only part of the SCP that no test
// had ever produced.
TEST_CASE( "scps_9044_reaches_the_end_once", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    avatar &you = get_avatar();

    const tripoint_bub_ms hall_start( 20, 20, 0 );
    const tripoint_bub_ms far_door( 20, 8, 0 );
    here.ter_set( hall_start, ter_id( "t_scp_9044_hall_start" ) );
    here.ter_set( far_door, ter_id( "t_scp_9044_door" ) );

    dialogue d( get_talker_for( you ), nullptr );

    // u_spawn_item drops the item at your feet rather than into your pack -- verified here the
    // first time this case ran, when an inventory-only count read 0 for the keepsake.  Count both
    // places, so the assertion is about the thing existing and not about where it landed.
    const auto count_of = [&]( const itype_id &what ) {
        int n = you.amount_of( what );
        for( const item &it : here.i_at( you.pos_bub() ) ) {
            if( it.typeId() == what ) {
                n++;
            }
        }
        return n;
    };
    const auto gifts = [&]() {
        return count_of( itype_scp_9044_gift );
    };

    // Nineteen: "Feel my fingers."  Something takes hold of your arm and does not let go.
    you.set_value( "scp_9044_depth", 18 );
    you.setpos( here, far_door );
    REQUIRE( eoc_scp_9044_step->activate( d ) );
    CHECK( you.get_value( "scp_9044_depth" ) == 19 );
    CHECK( you.has_effect( effect_scp_9044_touched ) );
    // The id here has to be the REAL one.  It used to read `ring_silver`, which does not exist: the
    // engine's missing-item fallback fabricates an item carrying whatever id was asked for, so this
    // assertion passed while every player at depth 19 got an `undefined-ring_silver` and a logged
    // error.  Asserting a real id is the whole point of the check.
    CHECK( count_of( itype_id( "silver_ring" ) ) >= 1 );

    // Twenty-two: "Welcome home."  The gift, on the lap it belongs to.
    you.set_value( "scp_9044_depth", 21 );
    you.setpos( here, far_door );
    REQUIRE( eoc_scp_9044_step->activate( d ) );
    CHECK( you.get_value( "scp_9044_depth" ) == 22 );
    CHECK( gifts() == 1 );

    // Twenty-three: the door does not open, and this time the house says nothing at all.  Walking
    // on must not pay again -- the gift is for reaching the end, not for pacing it.
    you.setpos( here, far_door );
    REQUIRE( eoc_scp_9044_step->activate( d ) );
    CHECK( you.get_value( "scp_9044_depth" ) == 23 );
    CHECK( gifts() == 1 );
}

// SCP-8000: the machine, the site it sits in, and the four things it found.  The console's EOC is
// what makes the chain work, so the registration here is not decoration -- if the interface or the
// stabiliser lose their examine_action, or a find fails to load, the site is a room with a box in
// it and nothing else.
TEST_CASE( "scps_8000_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( oter_scp_8000_site322.is_valid() );

    REQUIRE( ter_scp_8000_seeker.is_valid() );
    CHECK( furn_scp_8000_console.is_valid() );
    CHECK( furn_scp_8000_stabiliser.is_valid() );

    CHECK( eoc_scp_8000_seeker.is_valid() );
    CHECK( eoc_scp_8000_push.is_valid() );
    CHECK( itype_note_8000.is_valid() );

    CHECK( mon_scp_01_seek.is_valid() );
    CHECK( mon_scp_04_seek.is_valid() );
    CHECK( mon_scp_13_seek.is_valid() );
    CHECK( mon_scp_22_seek.is_valid() );
}

// The chain, driven for real.  The counter is what every step of the reveal reads, so it is the
// thing worth asserting: one scan advances it by one, the stabiliser pages it ahead (or lets
// something through instead), and past the last entry scanning again is what cracks the machine.
// The reveals themselves point at real overmap specials, which a test world may not have placed --
// that is why the assertion is on the counter and not on the located tile.
TEST_CASE( "scps_8000_seeker_scans_the_chain", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    avatar &you = get_avatar();
    dialogue d( get_talker_for( you ), nullptr );

    REQUIRE( eoc_scp_8000_seeker->activate( d ) );
    CHECK( you.get_value( "scp_8000_scan" ) == 1 );

    REQUIRE( eoc_scp_8000_seeker->activate( d ) );
    CHECK( you.get_value( "scp_8000_scan" ) == 2 );

    // The stabiliser either pages ahead or leaks; over ten pulls it cannot plausibly do neither.
    // diag_value compares for equality against a number and nothing else, so this asks "did it
    // move" rather than "did it advance by at least one" -- over ten pulls that is the same claim.
    for( int i = 0; i < 10; i++ ) {
        eoc_scp_8000_push->activate( d );
    }
    CHECK_FALSE( you.get_value( "scp_8000_scan" ) == 2 );

    // Past the last entry: the scan runs, the counter passes ten, and the bay is ruined underfoot.
    you.set_value( "scp_8000_scan", 10 );
    REQUIRE( eoc_scp_8000_seeker->activate( d ) );
    CHECK( you.get_value( "scp_8000_scan" ) == 11 );
}

// SCP-8000 Stage 2: the rift, Memory Lane, Wallace, and the book he gives you.  The registration
// matters more than usual here, because half of Stage 2 lives in a dimension -- if the library
// layer's terrain or its reading room fail to resolve, the rift sends the player into a world of
// nothing at all.
TEST_CASE( "scps_8000_wallace_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( oter_scp_8000_library.is_valid() );

    CHECK( ter_scp_8000_rift.is_valid() );
    CHECK( ter_scp_8000_rift_home.is_valid() );
    CHECK( ter_scp_8000_library_floor.is_valid() );

    REQUIRE( mon_scp_8000_wallace.is_valid() );
    CHECK( itype_unwritten_book.is_valid() );
    CHECK( trait_second_chance_spent.is_valid() );

    CHECK( eoc_scp_8000_rift.is_valid() );
    CHECK( eoc_scp_8000_rift_home.is_valid() );
    CHECK( eoc_scp_8000_book_saves_you.is_valid() );
}

// The one behaviour the plan flagged as possibly untestable, and it turns out to be testable:
// the engine's prevent-death pass is a public entry point, so a test can kill the vitals, run it,
// and see whether the book paid.  That is exactly what the EOC is for -- it fires from
// is_game_over, mid-turn, with the vitals already at zero, and cancelling the death is the whole
// of its job.  The avatar is put back on its feet at the end so the case cannot poison the ones
// that follow it.
TEST_CASE( "scps_8000_book_pays_for_one_death", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    avatar &you = get_avatar();
    const bodypart_id torso( "torso" );

    // Without the book, the death simply stands.
    you.set_part_hp_cur( torso, 0 );
    REQUIRE( you.is_dead_state() );
    effect_on_conditions::prevent_death();
    CHECK( you.is_dead_state() );

    // With it, the book pays.
    you.set_part_hp_cur( torso, 40 );
    you.i_add( item( itype_unwritten_book ) );
    you.set_part_hp_cur( torso, 0 );
    REQUIRE( you.is_dead_state() );
    effect_on_conditions::prevent_death();
    CHECK_FALSE( you.is_dead_state() );
    CHECK( you.has_trait( trait_second_chance_spent ) );

    // ...and having paid once, it will not pay again.
    you.set_part_hp_cur( torso, 0 );
    REQUIRE( you.is_dead_state() );
    effect_on_conditions::prevent_death();
    CHECK( you.is_dead_state() );

    you.set_part_hp_cur( torso, 40 );
    CHECK_FALSE( you.is_dead_state() );
}

// The book is a carried item plus a PREVENT_DEATH EOC, and the limiter trait is what makes it a
// once.  This case proves the wiring the EOC's condition reads: the book exists, carrying it is
// what the condition checks, and the limiter is a real trait the avatar does not start with.
TEST_CASE( "scps_8000_book_is_a_carried_lifeline", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    avatar &you = get_avatar();

    CHECK_FALSE( you.has_trait( trait_second_chance_spent ) );
    CHECK( you.amount_of( itype_unwritten_book ) == 0 );

    you.i_add( item( itype_unwritten_book ) );
    CHECK( you.amount_of( itype_unwritten_book ) == 1 );
}

// Stage 2's load-bearing mechanic, and the one the audit marks not-assertable: the rift actually
// changes dimensions.  u_travel_to_dimension is the only way into Memory Lane, so if it does not
// fire the library is somewhere that cannot be reached -- and no other case here would notice,
// because every other Stage 2 case checks content rather than travel.  The avatar is left back in
// the main world, so the case cannot poison the ones after it.
TEST_CASE( "scps_8000_rift_actually_travels", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    // The SCP-1499 cases run before this one and leave the player in their dimension, so the
    // starting layer has to be asserted rather than assumed -- the same trap the 9044 case hit.
    if( g->get_dimension_prefix() != dimension_id( "default" ) ) {
        g->travel_to_dimension( dimension_id( "default" ), {}, {}, std::nullopt );
    }
    avatar &you = get_avatar();
    dialogue d( get_talker_for( you ), nullptr );

    const dimension_id home( g->get_dimension_prefix() );
    CHECK( home == dimension_id( "default" ) );

    // In through the tear.
    REQUIRE( eoc_scp_8000_rift->activate( d ) );
    CHECK( g->get_dimension_prefix() == dimension_id( "scp_8000_library" ) );

    // And back out the far side of it.
    REQUIRE( eoc_scp_8000_rift_home->activate( d ) );
    CHECK( g->get_dimension_prefix() == home );
}

// SCP-8001: the edge of the world.  The island is eleven overmap tiles and the tower is three
// levels of one of them, so what is worth checking is that the pieces carrying the mechanic exist
// at all -- the mist that hides the drop, the lamp that makes the tower a lighthouse, the headstones
// and the memorial that name the island, and the keeper standing in the middle of it.
TEST_CASE( "scps_8001_is_registered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( oter_scp_8001_edge.is_valid() );
    CHECK( oter_scp_8001_edge_s.is_valid() );
    CHECK( oter_scp_8001_walkway.is_valid() );
    CHECK( oter_scp_8001_woods_e.is_valid() );
    CHECK( oter_scp_8001_bay.is_valid() );
    CHECK( oter_scp_8001_stair.is_valid() );
    CHECK( oter_scp_8001_hill.is_valid() );
    CHECK( oter_scp_8001_graves.is_valid() );
    CHECK( oter_scp_8001_outcrop.is_valid() );
    CHECK( oter_scp_8001_woods.is_valid() );
    CHECK( oter_scp_8001_wood.is_valid() );
    CHECK( oter_scp_8001_tower.is_valid() );
    CHECK( oter_scp_8001_tower_z1.is_valid() );
    CHECK( oter_scp_8001_tower_z2.is_valid() );

    REQUIRE( furn_scp_8001_lamp.is_valid() );
    CHECK( furn_scp_8001_reflector.is_valid() );
    CHECK( furn_scp_8001_headstone.is_valid() );
    CHECK( furn_scp_8001_memorial.is_valid() );

    REQUIRE( mon_scp_8001_aurelie.is_valid() );
    CHECK( itype_scp_8001_chart.is_valid() );
    CHECK( itype_scp_8001_chart_annotated.is_valid() );
    CHECK( itype_scp_8001_mp3_dead.is_valid() );
    CHECK( itype_scp_8001_lens.is_valid() );
    CHECK( itype_scp_8001_record.is_valid() );

    CHECK( fd_scp_8001_spray.is_valid() );
    CHECK( eoc_scp_8001_chart.is_valid() );
    CHECK( eoc_scp_8001_chart_studied.is_valid() );
    CHECK( eoc_scp_8001_falls.is_valid() );
    CHECK( eoc_scp_8001_memorial.is_valid() );
}

// The spray is the whole reason the edge reads as an edge -- it is what hides whatever is past the
// rail.  A field with a half_life would thin out and clear an hour into a run, so the claims worth
// testing are that this one never ages, that it renews itself out of fd_fog rather than decaying
// away, and that it is opaque, because a transparent field would leave the drop in plain sight.
TEST_CASE( "scps_8001_the_spray_never_clears", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( fd_scp_8001_spray.is_valid() );

    // Resolve through the *string* id, as every other field test in the tree does.  `field_type_id`
    // is the enum-backed id: for a field declared without a `legacy_enum_id` it quietly yields
    // fd_null and `is_valid()` still says true, so a test written against it passes while reading a
    // completely different field.  Asserting the resolved object's own id is the guard against that.
    const field_type &spray = *fd_scp_8001_spray;
    CHECK( spray.id.str() == "fd_scp_8001_spray" );
    CHECK( spray.get_intensity_level( 0 ).name.translated() == "spray" );
    CHECK( spray.half_life == 0_turns );
    CHECK_FALSE( spray.wandering_field.is_null() );
    REQUIRE_FALSE( spray.intensity_levels.empty() );
    CHECK_FALSE( spray.intensity_levels.front().transparent );
}

// The tower is a lighthouse before it is anything else, and core's own lighthouse light emits no
// light at all -- it is scenery.  240 is the practical ceiling: the light cache is one reality
// bubble across, so a bigger number buys nothing the player can actually see.
TEST_CASE( "scps_8001_the_lamp_is_the_light", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( furn_scp_8001_lamp.is_valid() );
    CHECK( furn_scp_8001_lamp->light_emitted == 240 );
}

// Aurélie is the fifth thing in this mod that would rather talk than fight, after the bear, Luana,
// Spaghetti Jones and Wallace.  Her conversation is her content: strip the topics and she is a piece
// of clockwork scenery standing in a library, which is a different and much smaller story.
TEST_CASE( "scps_8001_aurelie_would_rather_talk", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    REQUIRE( mon_scp_8001_aurelie.is_valid() );
    CHECK_FALSE( mon_scp_8001_aurelie->chat_topics.empty() );
}

// SCP-8001 hangs off SCP-8000: the SEEKER can be asked about the edge of the world as well, which
// makes the island an eleventh catalogue entry and pushes the machine's collapse one scan further
// out.  The counter is what every branch of the chain reads, so it is what this drives -- the
// reveal itself points at a special a test world may never have placed.
TEST_CASE( "scps_8001_seeker_knows_about_the_edge", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    avatar &you = get_avatar();
    dialogue d( get_talker_for( you ), nullptr );

    you.set_value( "scp_8000_scan", 10 );
    REQUIRE( eoc_scp_8000_seeker->activate( d ) );
    CHECK( you.get_value( "scp_8000_scan" ) == 11 );

    // The ruin used to sit at eleven; with the island in the catalogue it has moved to twelve.
    REQUIRE( eoc_scp_8000_seeker->activate( d ) );
    CHECK( you.get_value( "scp_8000_scan" ) == 12 );
}

// The chart is the way in: studying it reveals the island and swaps itself for the annotated copy.
// `transform_item` wants an item context that a bare dialogue does not have, so what this proves is
// that the effect chain executes and resolves -- the reveal half is covered by the loader and by the
// SEEKER case above.
TEST_CASE( "scps_8001_the_chart_can_be_studied", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    clear_avatar();
    avatar &you = get_avatar();
    dialogue d( get_talker_for( you ), nullptr );

    // Only the *studied* half is driven.  EOC_SCP_8001_CHART runs a `target_params` search for the
    // island, and the engine logs `Unable to find and assign mission target` (a blocking error screen
    // in a real game, and a hard test failure here) whenever the special cannot be found or created
    // within its 540-tile radius -- which a test world, having no ocean in range, never can.  The
    // search itself is content that only resolves in a full world; what is checkable here is that the
    // annotated-copy effect chain executes.
    CHECK( eoc_scp_8001_chart_studied->activate( d ) );
}

// Aurélie's request is gated on three modern books *by name*.  If any of them ever leaves core her
// request becomes unanswerable, and nothing else in the mod would notice -- so this is the guard
// that the offer can still be completed at all.  (The response itself is dialogue, which a test
// cannot drive; those three ids plus the two things she hands back are what is checkable.)
TEST_CASE( "scps_8001_aurelie_can_still_be_answered", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    CHECK( itype_id( "textbook_robots" ).is_valid() );
    CHECK( itype_id( "SICP" ).is_valid() );
    CHECK( itype_id( "emergency_book" ).is_valid() );

    CHECK( itype_scp_8001_lens.is_valid() );
    CHECK( itype_scp_8001_record.is_valid() );
}

// The lamp room is one tile, so its mapgen can be generated directly -- and that is worth doing,
// because "the tower is a lighthouse" is carried entirely by two furniture pieces landing on a real
// map.  The island itself is a 4x3 multi-tile mapgen whose tiles the loader already proves have
// mapgen; this is the floor the beam actually comes off.  (The mapgen is written for the tower's
// first floor and is run here at z0 purely to read its content back.)
TEST_CASE( "scps_8001_the_lamp_room_generates", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    // Even-aligned, or the tinymap's origin is not `where` and these coordinates read the wrong tile.
    tripoint_abs_omt where = get_avatar().pos_abs_omt() + tripoint( 8, 8, 0 );
    where.x() -= where.x() % 2;
    where.y() -= where.y() % 2;
    // Forcing the terrain before loading is what makes this case independent of where world generation
    // happened to put the avatar: the base is derived from the avatar's position, the seed is fresh
    // every run (tests/Makefile `--rng-seed time`), and without this the tinymap could inherit an
    // earlier case's tile and read back someone else's map.  This is the flake the harness described.
    overmap_buffer.ter_set( where, oter_id( "field" ) );
    tinymap tm;
    tm.load( where, true );
    mapgendata md( where, *tm.cast_to_map(), 0.0f, calendar::turn, nullptr );
    run_mapgen_func( oter_scp_8001_tower_z1.str(), md );

    CHECK( tm.furn( tripoint_omt_ms{ 11, 8, 0 } ) == furn_id( "f_scp_8001_lamp" ) );
    CHECK( tm.furn( tripoint_omt_ms{ 12, 12, 0 } ) == furn_id( "f_scp_8001_reflector" ) );
    CHECK( tm.ter( tripoint_omt_ms{ 11, 8, 0 } ) == ter_id( "t_floor" ) );
    CHECK( tm.ter( tripoint_omt_ms{ 7, 6, 0 } ) == ter_id( "t_wall" ) );
    // Everything outside the tower footprint is open air, not a floor floating in the sky.
    CHECK( tm.ter( tripoint_omt_ms{ 2, 2, 0 } ) == ter_id( "t_open_air" ) );
    CHECK( tm.ter( tripoint_omt_ms{ 11, 21, 0 } ) == ter_id( "t_open_air" ) );
}

// The island itself, generated for real.  It is a single 4x3 multi-tile mapgen, so every tile in it
// is a *slice* of one 96x72 drawing -- and whether those slices land where they should is the one
// thing no loader check can tell you.  Forcing the overmap terrain before generating matters: the
// mapgen picks its slice from the terrain `mapgendata` was built against, so it has to be the tile
// we mean.  Runs at the player's position + 24 so it cannot collide with anything else in the suite.
TEST_CASE( "scps_8001_the_island_generates_and_the_walkway_ends_over_a_drop", "[scps]" )
{
    if( !scps_loaded() ) {
        return;
    }
    // Even-aligned so the tinymap's origin IS each `where` (see the note on the 9044 mapgen case).
    tripoint_abs_omt base = get_avatar().pos_abs_omt() + point( 24, 0 );
    base.x() -= base.x() % 2;
    base.y() -= base.y() % 2;

    const auto generate = []( const tripoint_abs_omt & where, const std::string & omt,
    tinymap & tm ) {
        overmap_buffer.ter_set( where, oter_id( omt ) );
        tm.load( where, true );
        mapgendata md( where, *tm.cast_to_map(), 0.0f, calendar::turn, nullptr );
        run_mapgen_func( omt, md );
    };

    // The void tile west of the outcrop, and the outcrop the walkway starts from.
    tinymap edge;
    generate( base + point::west * 2, "scp_8001_walkway", edge );
    // The walkway is decking across island row 36, which is local row 12 of these two slices.
    CHECK( edge.ter( tripoint_omt_ms{ 12, 12, 0 } ) == ter_id( "t_dock_deep" ) );
    CHECK( edge.ter( tripoint_omt_ms{ 6, 12, 0 } ) == ter_id( "t_dock_deep" ) );
    // ...and past its westernmost board there is nothing at all.  This is the edge of the world.
    CHECK( edge.ter( tripoint_omt_ms{ 5, 12, 0 } ) == ter_id( "t_open_air" ) );
    CHECK( edge.ter( tripoint_omt_ms{ 0, 0, 0 } ) == ter_id( "t_open_air" ) );
    // A drop rather than a step: open air has no floor, so walking onto it is a fall.
    CHECK( edge.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, tripoint_omt_ms{ 0, 0, 0 } ) );

    tinymap outcrop;
    generate( base + point::west, "scp_8001_outcrop", outcrop );
    CHECK( outcrop.ter( tripoint_omt_ms{ 2, 12, 0 } ) == ter_id( "t_dock_deep" ) );
    CHECK( outcrop.ter( tripoint_omt_ms{ 0, 12, 0 } ) == ter_id( "t_dock_deep" ) );

    // The Tower of Sunset: walls, the east door, and the shelves standing on a floor -- which is the
    // assertion that would have caught the library being built on grass.
    tinymap tower;
    generate( base, "scp_8001_tower", tower );
    CHECK( tower.ter( tripoint_omt_ms{ 16, 6, 0 } ) == ter_id( "t_wall" ) );
    CHECK( tower.ter( tripoint_omt_ms{ 16, 11, 0 } ) == ter_id( "t_door_c" ) );
    CHECK( tower.furn( tripoint_omt_ms{ 11, 7, 0 } ) == furn_id( "f_bookcase" ) );
    CHECK( tower.ter( tripoint_omt_ms{ 11, 7, 0 } ) == ter_id( "t_floor" ) );

    // The graveyard, and the collapsed marker that named the island by accident.
    tinymap graves;
    generate( base + point::east, "scp_8001_graves", graves );
    CHECK( graves.furn( tripoint_omt_ms{ 4, 4, 0 } ) == furn_id( "f_scp_8001_headstone" ) );

    tinymap stair;
    generate( base + point::east * 2, "scp_8001_stair", stair );
    CHECK( stair.furn( tripoint_omt_ms{ 13, 10, 0 } ) == furn_id( "f_scp_8001_memorial" ) );
}
