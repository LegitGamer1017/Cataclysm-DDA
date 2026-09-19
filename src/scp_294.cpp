#include "scp_294.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "global_vars.h"
#include "iexamine.h"
#include "input_popup.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "map.h"
#include "messages.h"
#include "output.h"
#include "pocket_type.h"
#include "popup.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

// SCP-294 states:
//   * every successful pour is one cup, charged fifty cents;
//   * the machine holds fifty cups, then goes quiet for ~ninety minutes;
//   * it restocks a limited number of times, and after that jams for good.
//
// Its per-machine state lives in the world globals (the same store JSON
// `global_val` uses), keyed by the machine's absolute position so two machines
// in two different labs do not share one counter.  The permanent dead state is
// a furniture id (f_scp_294_dead), which is per-tile and save-safe by itself.

namespace
{

std::vector<scp_294_request> scp_294_request_table;

const itype_id itype_cash_card( "cash_card" );
const itype_id itype_scp_294_cup( "scp_294_cup" );
// A furn_str_id, not a furn_id: furn_id is an int_id and resolving it here would
// run before furniture data exists (static initialization order), producing an
// "invalid (null) id" load error.  The int_id is resolved at the point of use.
const furn_str_id furn_scp_294_dead( "f_scp_294_dead" );

constexpr int scp_294_cost = 50;                 // cents
constexpr int scp_294_pours_per_cycle = 50;      // article: "~fifty uses"
constexpr int scp_294_restock_turns = 90 * 60;   // article: "~90 minutes", 1 turn == 1 s
constexpr int scp_294_max_cycles = 3;            // then it jams for good

// One serving - the base game's standard drink measure (250 ml).  The machine
// pours this much, not a full cup.  The CUP is deliberately larger (500 ml) so
// that a liquid whose single charge is 400-500 ml (bee's knees, bark tea, three
// sisters stew) can be poured at all; those simply pour their one indivisible
// charge, which is still roughly a serving.
const units::volume scp_294_serving_volume = 250_ml;

std::string state_key( const char *base, const tripoint_abs_ms &p )
{
    return string_format( "%s@%d_%d_%d", base, p.x(), p.y(), p.z() );
}

int get_state( const std::string &key )
{
    diag_value const *v = get_globals().maybe_get_global_value( key );
    return ( v != nullptr && v->is_dbl() ) ? static_cast<int>( v->dbl() ) : 0;
}

void set_state( const std::string &key, int value )
{
    get_globals().set_global_value( key, value );
}

// A liquid type is preferred over a chemical for the same query, and the
// shortest matching name wins ties, so "water" lands on water rather than a
// longer compound name that merely contains the word.
bool better_liquid( const itype *cand, const itype *best )
{
    const bool cand_drink = cand->comestible && cand->comestible->comesttype == "DRINK";
    const bool best_drink = best->comestible && best->comestible->comesttype == "DRINK";
    if( cand_drink != best_drink ) {
        return cand_drink;
    }
    return cand->nname( 1 ).size() < best->nname( 1 ).size();
}

} // namespace

void load_scp_294_request( const JsonObject &jo, const std::string & )
{
    scp_294_request req;
    jo.read( "id", req.id );

    if( jo.has_array( "match" ) ) {
        for( const std::string phrase : jo.get_array( "match" ) ) {
            req.match.push_back( to_lower_case( trim( phrase ) ) );
        }
    } else if( jo.has_string( "match" ) ) {
        req.match.push_back( to_lower_case( trim( jo.get_string( "match" ) ) ) );
    }

    if( jo.has_string( "item" ) ) {
        req.item = itype_id( jo.get_string( "item" ) );
    }
    jo.read( "response", req.response );
    jo.read( "message", req.message );

    scp_294_request_table.push_back( req );
}

void reset_scp_294_request()
{
    scp_294_request_table.clear();
}

void check_scp_294_requests()
{
    // Runs in the consistency-check phase (init.cpp), after items are finalised.
    // Without this an unknown item id in the table would silently become a null
    // id and read as an ordinary OUT OF RANGE at runtime.
    for( const scp_294_request &req : scp_294_request_table ) {
        if( !req.item.is_null() && !req.item.is_valid() ) {
            debugmsg( "SCP-294 request \"%s\" references unknown item \"%s\"",
                      req.id.c_str(), req.item.str().c_str() );
        }
    }
}

const std::vector<scp_294_request> &scp_294_requests()
{
    return scp_294_request_table;
}

itype_id scp_294_resolve( const std::string &request, std::string &out_message )
{
    out_message.clear();
    const std::string query = to_lower_case( trim( request ) );
    if( query.empty() ) {
        return itype_id::NULL_ID();
    }

    // 1) scripted requests win outright
    for( const scp_294_request &req : scp_294_request_table ) {
        for( const std::string &phrase : req.match ) {
            if( phrase == query ) {
                out_message = req.message;
                if( !req.response.empty() ) {
                    return itype_id::NULL_ID();
                }
                return req.item;
            }
        }
    }

    // 2) a literal item id, if it names a liquid
    const itype_id direct( query );
    if( direct.is_valid() && direct->phase == phase_id::LIQUID ) {
        return direct;
    }

    // 3) a liquid's display name - exact match first, then a case-insensitive
    //    substring match, best candidate wins
    const itype *best = nullptr;
    bool best_exact = false;
    for( const itype *t : item_controller->all() ) {
        if( t->phase != phase_id::LIQUID ) {
            continue;   // solids such as "diamond" can never resolve
        }
        const bool exact = to_lower_case( t->nname( 1 ) ) == query;
        if( !exact && !lcmatch( t->nname( 1 ), query ) ) {
            continue;
        }
        if( best == nullptr || ( exact != best_exact ? exact : better_liquid( t, best ) ) ) {
            best = t;
            best_exact = exact;
        }
    }

    return best != nullptr ? best->get_id() : itype_id::NULL_ID();
}

void iexamine::scp_294( Character &you, const tripoint_bub_ms &examp )
{
    map &here = get_map();
    const tripoint_abs_ms pos = here.get_abs( examp );

    const std::string key_pours = state_key( "scp_294_pours", pos );
    const std::string key_empty = state_key( "scp_294_empty_at", pos );
    const std::string key_cycles = state_key( "scp_294_cycles", pos );

    if( here.furn( examp ) == furn_id( furn_scp_294_dead ) ) {
        add_msg( m_bad, _( "The machine is dead.  The touchpad does not light." ) );
        return;
    }

    int pours = get_state( key_pours );
    int empty_at = get_state( key_empty );
    int cycles = get_state( key_cycles );
    const int now = to_turn<int>( calendar::turn );

    if( empty_at > 0 ) {
        if( now < empty_at ) {
            add_msg( m_info, _( "The machine shows OUT OF ORDER." ) );
            return;
        }
        cycles += 1;
        if( cycles >= scp_294_max_cycles ) {
            here.furn_set( examp, furn_id( furn_scp_294_dead ) );
            add_msg( m_bad, _( "Something inside grinds, seizes, and stops for good." ) );
            return;
        }
        pours = 0;
        empty_at = 0;
        set_state( key_pours, pours );
        set_state( key_empty, empty_at );
        set_state( key_cycles, cycles );
        add_msg( m_info, _( "Something inside clunks and settles.  The machine is ready again." ) );
    }

    if( you.charges_of( itype_cash_card ) < scp_294_cost ) {
        popup( _( "The coin slot waits.  It wants fifty cents." ) );
        return;
    }

    // The prompt goes in the window TITLE, not a label: input_popup draws a label
    // on the same line as the text field (src/input_popup.cpp:110-114), so a long
    // one squeezes the field into a narrow strip.  This matches how the base game
    // handles a longer prompt (src/debug_menu.cpp:1874).
    string_input_popup_imgui pad( 65, "", _( "Enter the name of any liquid" ) );
    pad.set_description( _( "The touchpad is warm under your fingers." ) );
    const std::string request = pad.query();
    if( pad.cancelled() || request.empty() ) {
        return;
    }

    std::string message;
    const itype_id result = scp_294_resolve( request, message );

    if( result.is_null() || !result.is_valid() ) {
        if( !message.empty() ) {
            popup( message );
        } else {
            popup( _( "OUT OF RANGE" ) );
        }
        return;
    }

    if( !itype_scp_294_cup.is_valid() ) {
        debugmsg( "SCP-294: the cup item 'scp_294_cup' is not defined" );
        return;
    }

    item cup( itype_scp_294_cup, calendar::turn );
    item liquid( result, calendar::turn );

    // item::fill_with is the engine's own "fill this container" helper: it caps
    // what goes in by the cup's remaining VOLUME *and* remaining WEIGHT
    // (item_container.cpp:831-840) and returns the charges it took.  Its `amount`
    // argument is the cap that turns "fill the cup" into "pour one serving", so
    // the machine hands over a 250 ml drink rather than a 500 ml one.  A liquid
    // whose single charge is bigger than a serving still fits, because `amount` is
    // floored at 1 and the cup itself is 500 ml.  A return of 0 means nothing fits
    // at all - a chemistry batch item defined at litres per charge - which is
    // refused without charging, and silently.
    const int serving = std::max( 1, liquid.charges_per_volume( scp_294_serving_volume, true ) );
    if( cup.fill_with( liquid, serving ) <= 0 ) {
        add_msg( m_bad, _( "The machine grinds, chokes, and produces nothing." ) );
        return;
    }
    you.i_add_or_drop( cup );
    you.use_charges( itype_cash_card, scp_294_cost );

    pours += 1;
    if( pours >= scp_294_pours_per_cycle ) {
        empty_at = now + scp_294_restock_turns;
        add_msg( m_info, _( "The machine dispenses its last cup, then goes quiet." ) );
    }
    set_state( key_pours, pours );
    set_state( key_empty, empty_at );

    if( !message.empty() ) {
        add_msg( m_info, message );
    }
}
