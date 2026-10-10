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
static const oter_str_id oter_scp_8000_reading_room( "scp_8000_reading_room" );
static const ter_str_id ter_scp_8000_rift( "t_scp_8000_rift" );
static const ter_str_id ter_scp_8000_rift_home( "t_scp_8000_rift_home" );
static const ter_str_id ter_scp_8000_library_floor( "t_scp_8000_library_floor" );
static const mtype_id mon_scp_8000_wallace( "mon_scp_8000_wallace" );
static const itype_id itype_unwritten_book( "scp_8000_unwritten_book" );
static const trait_id trait_second_chance_spent( "SCP_8000_SECOND_CHANCE_SPENT" );
static const effect_on_condition_id eoc_scp_8000_rift( "EOC_SCP_8000_RIFT" );
static const effect_on_condition_id eoc_scp_8000_rift_home( "EOC_SCP_8000_RIFT_HOME" );
static const effect_on_condition_id eoc_scp_8000_book_saves_you( "EOC_SCP_8000_BOOK_SAVES_YOU" );

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
    static const itype_id itype_gloves_rubber( "gloves_rubber" );

    clear_map();
    avatar &u = get_avatar();
    u.wear_item( item( itype_gloves_rubber ) );

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
    const tripoint_abs_omt where( get_avatar().pos_abs_omt() + tripoint( 4, 4, 0 ) );
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

    // Down the hall to the front door, then across the drive to the gate in the fence.
    for( int y = 6; y <= 20; y++ ) {
        CHECK( tm.ter( tripoint_omt_ms{ 11, y, 0 } ) != ter_id( "t_wall_wood" ) );
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
    CHECK( count_of( itype_id( "ring_silver" ) ) >= 1 );

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
    CHECK( oter_scp_8000_reading_room.is_valid() );

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

// The book is a carried item plus a PREVENT_DEATH EOC, and the limiter trait is what makes it a
// once.  Driving the death itself headlessly is the one thing this suite cannot do honestly -- the
// engine runs those EOCs from is_game_over, mid-turn, and simulating that faithfully is a
// different kind of test.  What can be proven is the wiring: the book exists, carrying it is what
// the EOC's condition reads, and the limiter it sets is a real trait that the avatar does not
// start with.
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
