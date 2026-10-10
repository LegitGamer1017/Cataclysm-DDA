#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map_helpers.h"
#include "mongroup.h"
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
