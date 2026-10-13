#include <string>
#include <string_view>

#include "avatar.h"
#include "cata_catch.h"
#include "condition.h"
#include "dialogue.h"
#include "json.h"
#include "json_loader.h"
#include "npc.h"
#include "type_id.h"

static const npc_template_id npc_template_test_npc_template_vars( "test_npc_template_vars" );

TEST_CASE( "npc_template_variables_applied_on_spawn", "[npc][npc_template]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_template_vars );

    const bool string_var_ok = guy.get_value( "test_template_str" ) == std::string_view( "hello" );
    const bool numeric_var_ok = guy.get_value( "test_template_num" ) == 42.0;
    CHECK( string_var_ok );
    CHECK( numeric_var_ok );
}

TEST_CASE( "npc_has_faction_condition", "[npc][dialogue]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_template_vars );

    dialogue d( get_talker_for( get_avatar() ), get_talker_for( guy ) );

    const conditional_t match =
        conditional_t( json_loader::from_string( R"({ "npc_has_faction": "debuggers" })" ).get_object() );
    const conditional_t no_match =
        conditional_t( json_loader::from_string( R"({ "npc_has_faction": "free_merchants" })" ).get_object() );

    CHECK( match( d ) );
    CHECK_FALSE( no_match( d ) );
}
