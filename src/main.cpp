/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

// KG: Yes, the above is inaccurate now. It's also a poem, it stays.

// IWYU pragma: no_include <sys/signal.h>
#include <algorithm>
#include <array>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#if defined(_WIN32)
#include "cata_allocator.h"
#include "platform_win.h"
#else
#include <csignal>
#endif

#include <flatbuffers/util.h>

#include "cached_options.h"
#include "cata_allocator.h"
#include "cata_path.h"
#include "color.h"
#include "avatar.h"
#include "compatibility.h"
#include "crash.h"
#include "cursesdef.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "talker.h"
#include "do_turn.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "filesystem.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "get_version.h"
#include "help.h"
#include "input.h"
#include "main_menu.h"
#include "npctalk.h"
#include "mapsharing.h"
#include "memory_fast.h"
#include "options.h"
#include "ordered_static_globals.h"
#include "output.h"
#include "path_info.h"
#include "rng.h"
#include "system_locale.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "worldfactory.h"
#include "cata_imgui.h"
#if defined(MACOSX) || defined(__CYGWIN__)
#   include <unistd.h> // getpid()
#endif

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

#if defined(PREFIX)
#   undef PREFIX
#   include "prefix.h"
#endif

class ui_adaptor;

#if defined(TILES) || defined(SDL_SOUND)
#   include "sdl_version_wrappers.h"
#endif

#if defined(TILES)
#   include "sdltiles.h"
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#include <unistd.h>
#include "sdl_wrappers.h" // for GetAndroidExternalStoragePath(), SDL_main

// Taken from: https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/
// Force Android standard output to adb logcat output

static int pfd[2];
static pthread_t thr;
static const char *tag = "cdda";

static void *thread_func( void * )
{
    ssize_t rdsz;
    char buf[128];
    for( ;; ) {
        if( ( ( rdsz = read( pfd[0], buf, sizeof buf - 1 ) ) > 0 ) ) {
            if( buf[rdsz - 1] == '\n' ) {
                --rdsz;
            }
            buf[rdsz] = 0;  /* add null-terminator */
            __android_log_write( ANDROID_LOG_DEBUG, tag, buf );
        }
    }
    return 0;
}

int start_logger( const char *app_name )
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf( stdout, 0, _IOLBF, 0 );
    setvbuf( stderr, 0, _IONBF, 0 );

    /* create the pipe and redirect stdout and stderr */
    pipe( pfd );
    dup2( pfd[1], 1 );
    dup2( pfd[1], 2 );

    /* spawn the logging thread */
    if( pthread_create( &thr, 0, thread_func, 0 ) == -1 ) {
        return -1;
    }
    pthread_detach( thr );
    return 0;
}

#endif //__ANDROID__

namespace
{

#if defined(_WIN32) and defined(TILES)
// Used only if AttachConsole() works
FILE *CONOUT;
#endif
void exit_handler( int s )
{
    const int old_timeout = inp_mngr.get_timeout();
    inp_mngr.reset_timeout();
    if( s != 2 || query_yn( _( "Really Quit?  All unsaved changes will be lost." ) ) ) {
        deinitDebug();

        int exit_status = 0;
        g.reset();

        catacurses::endwin();

#if defined(__ANDROID__)
        // Avoid capturing SIGABRT on exit on Android in crash report
        // Can be removed once the SIGABRT on exit problem is fixed
        signal( SIGABRT, SIG_DFL );
#endif

        imclient.reset();
        exit( exit_status );
    }
    inp_mngr.set_timeout( old_timeout );
    ui_manager::redraw_invalidated();
    catacurses::doupdate();
}

struct arg_handler {
    //! Handler function to be invoked when this argument is encountered. The handler will be
    //! called with the number of parameters after the flag was encountered, along with the array
    //! of following parameters. It must return an integer indicating how many parameters were
    //! consumed by the call or -1 to indicate that a required argument was missing.
    using handler_method = std::function<int ( int, const char ** )>;

    std::string_view flag;  //!< The commandline parameter to handle (e.g., "--seed").
    std::string_view param_documentation;  //!< Human readable description of this arguments parameter.
    std::string_view documentation;  //!< Human readable documentation for this argument.
    std::string_view help_group; //!< Section of the help message in which to include this argument.
    int num_args; //!< How many further arguments are expected for this parameter (usually 0 or 1).
    handler_method handler;  //!< The callback to be invoked when this argument is encountered.
};

template<typename FirstPassArgs, typename SecondPassArgs>
void printHelpMessage( const FirstPassArgs &first_pass_arguments,
                       const SecondPassArgs &second_pass_arguments )
{
    // Group all arguments by help_group.
    std::multimap<std::string, const arg_handler *> help_map;
    for( const arg_handler &handler : first_pass_arguments ) {
        help_map.emplace( handler.help_group, &handler );
    }
    for( const arg_handler &handler : second_pass_arguments ) {
        help_map.emplace( handler.help_group, &handler );
    }

    std::cout << "Command line parameters:\n";
    std::string current_help_group;
    for( std::pair<const std::string, const arg_handler *> &help_entry : help_map ) {
        if( help_entry.first != current_help_group ) {
            current_help_group = help_entry.first;
            std::cout << "\n" << current_help_group << "\n";
        }

        const arg_handler *handler = help_entry.second;
        std::cout << handler->flag << " " << handler->param_documentation;
        if( !handler->documentation.empty() ) {
            std::cout << "\n    " << handler->documentation << "\n";
        }
    }
    std::cout << std::endl;
}

/**
 * Displays current application version and compile options values
 */
void printVersionMessage()
{
#if defined(TILES)
    const bool hasTiles = true;
#else
    const bool hasTiles = false;
#endif

#if defined(SDL_SOUND)
    const bool hasSound = true;
#else
    const bool hasSound = false;
#endif

    printf( "Cataclysm Dark Days Ahead: %s\n\n"
            "%ctiles, %csound\n\n"
            "data dir: %s\nuser dir: %s\n",
            getVersionString(),
            hasTiles ? '+' : '-',
            hasSound ? '+' : '-',
            PATH_INFO::datadir().c_str(),
            PATH_INFO::user_dir().c_str() );
}

void process_args( const char **argv, int argc, const std::vector<arg_handler> &arg_handlers )
{
    while( argc ) {
        bool arg_handled = false;
        for( const arg_handler &handler : arg_handlers ) {
            if( handler.flag == argv[0] ) {
                argc--;
                argv++;
                if( argc < handler.num_args ) {
                    std::cout << "Missing expected argument to command line parameter " << handler.flag << std::endl;
                    std::exit( 1 );
                }
                int args_consumed = handler.handler( argc, argv );
                if( args_consumed < 0 ) {
                    printf( "Failed parsing parameter '%s'\n", *( argv - 1 ) );
                    std::exit( 1 );
                }
                argc -= args_consumed;
                argv += args_consumed;
                arg_handled = true;
                break;
            }
        }
        // Skip other options.
        if( !arg_handled ) {
            --argc;
            ++argv;
        }
    }
}

struct cli_opts {
    int seed = time( nullptr );
    bool verifyexit = false;
    bool noverify = false;
    bool check_mods = false;
    std::vector<std::string> opts;
    std::string world; /** if set try to load first save in this world on startup */
    std::string save_id; /** with --world, load this specific save instead of the first */
    std::string input_script; /** queue the keys in this file at startup */
    bool dismiss_debug_prompts = false; /** log debugmsgs without their blocking prompts */
    bool dialogue_dump = false; /** write every generated talk topic to mcp_dialogue.json */
    bool selector_dump = false; /** write every run_eoc_selector menu to mcp_selector.json */
    std::string pocket_patch; /** apply this JSON pocket-settings patch after loading */
    std::string var_patch; /** apply this JSON character-state patch after loading */
    std::string selector_run; /** run this selector EOC after loading, for its dump only */
    std::vector<std::string> widget_dump; /** render these widget ids into the snapshot */
    bool new_character = false; /** create a character non-interactively and start */
    bool snapshot = false; /** write a situation snapshot on the first turn */
    bool disable_ascii_art = false;
};

cli_opts parse_commandline( int argc, const char **argv )
{
    cli_opts result;

    constexpr std::string_view section_default;
    constexpr std::string_view section_map_sharing = "Map sharing";
    constexpr std::string_view section_user_directory = "User directories";
    constexpr std::string_view section_accessibility = "Accessibility";
    const std::vector<arg_handler> first_pass_arguments = {{
            {
                "--seed", "<string of letters and or numbers>",
                "Sets the random number generator's seed value",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    const unsigned char *hash_input = reinterpret_cast<const unsigned char *>( params[0] );
                    result.seed = djb2_hash( hash_input );
                    return 1;
                }
            },
            {
                "--jsonverify", {},
                "Checks the CDDA json files and exits",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.verifyexit = true;
                    return 0;
                }
            },
            {
                "--check-mods", "[mod…]",
                "Checks the json files belonging to given CDDA mod and exits",
                section_default,
                1,
                [&result]( int n, const char **params ) -> int {
                    result.check_mods = true;
                    test_mode = true;
                    for( int i = 0; i < n; ++i )
                    {
                        result.opts.emplace_back( params[ i ] );
                    }
                    return 0;
                }
            },
            {
                "--noverify", {},
                "Skips JSON verification",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.noverify = true;
                    return 0;
                }
            },
            {
                "--world", "<name>",
                "Load world",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.world = params[0];
                    return 1;
                }
            },
            {
                "--save", "<save id>",
                "With --world, load this specific save (character) instead of the first",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.save_id = params[0];
                    return 1;
                }
            },
            {
                "--new-character", {},
                "Create a character non-interactively in the --world world and start playing",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.new_character = true;
                    return 0;
                }
            },
            {
                "--input-script", "<path>",
                "Queue key presses from a script file (one key name per line) at startup",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.input_script = params[0];
                    return 1;
                }
            },
            {
                "--dismiss-debug-prompts", {},
                "Write debug messages to the log without showing their blocking prompts (automation)",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.dismiss_debug_prompts = true;
                    return 0;
                }
            },
            {
                "--dialogue-dump", {},
                "Write every generated talk topic (line, shown responses + silently dropped ones) to <world>/mcp_dialogue.json",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.dialogue_dump = true;
                    return 0;
                }
            },
            {
                "--selector-dump", {},
                "Write every run_eoc_selector menu (title, options, which are selectable or hidden) to <world>/mcp_selector.json",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.selector_dump = true;
                    return 0;
                }
            },
            {
                "--selector-run", "<effect_on_condition id>",
                "Run this selector EOC after loading and write its menu to <world>/mcp_selector.json WITHOUT showing it (no cursor, nothing chosen)",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.selector_run = params[0];
                    return 1;
                }
            },
            {
                "--snapshot", {},
                "Write a situation snapshot (mcp_snapshot.json) on the first turn",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.snapshot = true;
                    return 0;
                }
            },
            {
                "--pocket-patch", "<path>",
                "Apply a JSON pocket-settings patch to worn items once the game is loaded",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.pocket_patch = params[0];
                    return 1;
                }
            },
            {
                "--var-patch", "<path>",
                "Apply a JSON character-state patch (values, traits, proficiencies, skills, effects, vitamins) once the game is loaded",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.var_patch = params[0];
                    return 1;
                }
            },
            {
                "--widget-dump", "<comma-separated widget ids>",
                "Render these widgets to text inside the --snapshot (e.g. structured_sidebar)",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    std::string ids( params[0] );
                    std::string cur;
                    for( const char c : ids ) {
                        if( c == ',' ) {
                            if( !cur.empty() ) {
                                result.widget_dump.emplace_back( cur );
                                cur.clear();
                            }
                        } else if( c != ' ' ) {
                            cur += c;
                        }
                    }
                    if( !cur.empty() ) {
                        result.widget_dump.emplace_back( cur );
                    }
                    return 1;
                }
            },
            {
                "--basepath", "<path>",
                "Base path for all game data subdirectories",
                section_default,
                1,
                []( int, const char **params )
                {
                    PATH_INFO::init_base_path( params[0] );
                    PATH_INFO::set_standard_filenames();
                    return 1;
                }
            },
            {
                "--shared", {},
                "Activates the map-sharing mode",
                section_map_sharing,
                0,
                []( int, const char ** ) -> int {
                    MAP_SHARING::setSharing( true );
                    MAP_SHARING::setCompetitive( true );
                    MAP_SHARING::setWorldmenu( false );
                    return 0;
                }
            },
            {
                "--username", "<name>",
                "Instructs map-sharing code to use this name for your character.",
                section_map_sharing,
                1,
                []( int, const char **params ) -> int {
                    MAP_SHARING::setUsername( params[0] );
                    return 1;
                }
            },
            {
                "--addadmin", "<username>",
                "Instructs map-sharing code to use this name for your character and give you "
                "access to the cheat functions.",
                section_map_sharing,
                1,
                []( int, const char **params ) -> int {
                    MAP_SHARING::addAdmin( params[0] );
                    return 1;
                }
            },
            {
                "--adddebugger", "<username>",
                "Informs map-sharing code that you're running inside a debugger",
                section_map_sharing,
                1,
                []( int, const char **params ) -> int {
                    MAP_SHARING::addDebugger( params[0] );
                    return 1;
                }
            },
            {
                "--competitive", {},
                "Instructs map-sharing code to disable access to the in-game cheat functions",
                section_map_sharing,
                0,
                []( int, const char ** ) -> int {
                    MAP_SHARING::setCompetitive( true );
                    return 0;
                }
            },
            {
                "--userdir", "<path>",
                // NOLINTNEXTLINE(cata-text-style): the dot is not a period
                "Base path for user-overrides to files from the ./data directory and named below",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::init_user_dir( params[0] );
                    PATH_INFO::set_standard_filenames();
                    return 1;
                }
            },
            {
                "--disable-ascii-art", {},
                "Disable aesthetic ascii art in menus and descriptions.",
                section_accessibility,
                0,
                [&result]( int, const char ** ) -> int {
                    result.disable_ascii_art = true;
                    return 0;
                }
            }
        }
    };

    // The following arguments are dependent on one or more of the previous flags and are run
    // in a second pass.
    const std::vector<arg_handler> second_pass_arguments = {{
            {
                "--worldmenu", {},
                "Enables the world menu in the map-sharing code",
                section_map_sharing,
                0,
                []( int, const char ** ) -> int {
                    MAP_SHARING::setWorldmenu( true );
                    return true;
                }
            },
            {
                "--datadir", "<directory name>",
                "Sub directory from which game data is loaded",
                {},
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_datadir( params[0] );
                    return 1;
                }
            },
            {
                "--savedir", "<directory name>",
                "Subdirectory for game saves",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_savedir( params[0] );
                    return 1;
                }
            },
            {
                "--configdir", "<directory name>",
                "Subdirectory for game configuration",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_config_dir( params[0] );
                    return 1;
                }
            },
            {
                "--memorialdir", "<directory name>",
                "Subdirectory for memorials",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_memorialdir( params[0] );
                    return 1;
                }
            },
            {
                "--optionfile", "<filename>",
                "Name of the options file within the configdir",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_options( params[0] );
                    return 1;
                }
            },
            {
                "--keymapfile", "<filename>",
                "Name of the keymap file within the configdir",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_keymap( params[0] );
                    return 1;
                }
            },
            {
                "--autopickupfile", "<filename>",
                "Name of the autopickup options file within the configdir",
                {},
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_autopickup( params[0] );
                    return 1;
                }
            },
            {
                "--motdfile", "<filename>",
                "Name of the message of the day file within the motd directory",
                {},
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_motd( params[0] );
                    return 1;
                }
            },
        }
    };

    if( std::count( argv, argv + argc, std::string( "--help" ) ) ) {
        printHelpMessage( first_pass_arguments, second_pass_arguments );
        std::exit( 0 );
    }

    if( std::count( argv, argv + argc, std::string( "--version" ) ) ) {
        printVersionMessage();
        std::exit( 0 );
    }

    // skip program name
    --argc;
    ++argv;

    process_args( argv, argc, first_pass_arguments );
    process_args( argv, argc, second_pass_arguments );

    return result;
}

bool assure_essential_dirs_exist()
{
    using namespace PATH_INFO;
    std::vector<std::string> essential_paths{
        config_dir(),
        savedir(),
        templatedir(),
        user_font(),
        user_sound().get_unrelative_path().u8string(),
        user_gfx().get_unrelative_path().u8string()
    };
    for( const std::string &path : essential_paths ) {
        if( !assure_dir_exist( path ) ) {
            popup( _( "Unable to make directory \"%s\".  Check permissions." ), path );
            return false;
        }
    }
    return true;
}

} // namespace

#if defined(EMSCRIPTEN)
EM_ASYNC_JS( void, mount_idbfs, (), {
    console.log( "Mounting IDBFS for persistence..." );
    FS.mkdir( '/home/web_user/.cataclysm-dda' );
    FS.mount( IDBFS, {}, '/home/web_user/.cataclysm-dda' );
    await new Promise( function( resolve, reject )
    {
        FS.syncfs( true, function( err ) {
            if( err ) {
                reject( err );
            } else {
                console.log( "Successfully mounted IDBFS." );
                resolve();
            }
        } );
    } );

    let fsNeedsSync = false;
    window.setFsNeedsSync = function setFsNeedsSync()
    {
        if( !fsNeedsSync ) {
            requestAnimationFrame( syncFs );
        }
        fsNeedsSync = true;
    };

    function syncFs()
    {
        console.log( "Persisting to IDBFS..." );
        FS.syncfs( false, function( err ) {
            fsNeedsSync = false;
            if( err ) {
                console.error( err );
            } else {
                console.log( "Successfully persisted to IDBFS..." );
            }
        } );
    }
} );
#endif

namespace
{
void initialize_runtime()
{
    cata::init_allocator();

    ordered_static_globals();
    init_crash_handlers();
    reset_floating_point_mode();
#if defined(FLATBUFFERS_LOCALE_INDEPENDENT) && (FLATBUFFERS_LOCALE_INDEPENDENT > 0)
    flatbuffers::ClassicLocale::Get();
#endif
}

void initialize_platform_output()
{
#if defined(_WIN32) and defined(TILES)
    const HANDLE std_output { GetStdHandle( STD_OUTPUT_HANDLE ) }, std_error { GetStdHandle( STD_ERROR_HANDLE ) };
    if( std_output != INVALID_HANDLE_VALUE and std_error != INVALID_HANDLE_VALUE ) {
        if( AttachConsole( ATTACH_PARENT_PROCESS ) ) {
            if( std_output == nullptr ) {
                freopen_s( &CONOUT, "CONOUT$", "w", stdout );
            }
            if( std_error == nullptr ) {
                freopen_s( &CONOUT, "CONOUT$", "w", stderr );
            }
        }
    }
#endif
#if defined(__ANDROID__)
    // Start the standard output logging redirector
    start_logger( "cdda" );
#endif
}

void initialize_default_paths()
{
#if defined(__ANDROID__)
    // On Android first launch, we copy all data files from the APK into the app's writeable folder so std::io stuff works.
    // Use the external storage so it's publicly modifiable data (so users can mess with installed data, save games etc.)
    std::string external_storage_path( GetAndroidExternalStoragePath() );

    PATH_INFO::init_base_path( external_storage_path );
#else
    // Set default file paths
#if defined(PREFIX)
    PATH_INFO::init_base_path( std::string( PREFIX ) );
#else
    PATH_INFO::init_base_path( "" );
#endif
#endif

#if defined(__ANDROID__)
    PATH_INFO::init_user_dir( external_storage_path );
#else
#   if defined(USE_HOME_DIR) || defined(USE_XDG_DIR) || defined(EMSCRIPTEN)
    PATH_INFO::init_user_dir( "" );
#   else
    PATH_INFO::init_user_dir( "." );
#   endif
#endif
    PATH_INFO::set_standard_filenames();
}

void validate_directories()
{
    if( !dir_exist( PATH_INFO::datadir() ) ) {
        printf( "Fatal: Can't find data directory \"%s\"\nPlease ensure the current working directory is correct or specify data directory with --datadir.  Perhaps you meant to start \"cataclysm-launcher\"?\n",
                PATH_INFO::datadir().c_str() );
        exit( 1 );
    }

    if( !assure_dir_exist( PATH_INFO::user_dir() ) ) {
        printf( "Can't open or create %s. Check permissions.\n",
                PATH_INFO::user_dir().c_str() );
        exit( 1 );
    }
}

void initialize_debugging()
{
#if defined(EMSCRIPTEN)
    setupDebug( DebugOutput::std_err );
#else
    setupDebug( DebugOutput::file );
#endif
    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    json_error_output_colors = json_error_output_colors_t::color_tags;
}

void initialize_locale()
{
    /**
     * OS X does not populate locale env vars correctly (they usually default to
     * "C") so don't bother trying to set the locale based on them.
     */
#if !defined(MACOSX)
    if( setlocale( LC_ALL, "" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '').";
    } else {
#endif
        try {
            std::locale::global( std::locale( "" ) );
        } catch( const std::exception & ) {
            // if user default locale retrieval isn't implemented by system
            try {
                // default to basic C locale
                std::locale::global( std::locale::classic() );
            } catch( const std::exception &err ) {
                debugmsg( "%s", err.what() );
                exit_handler( -999 );
            }
        }
#if !defined(MACOSX)
    }
#endif

    DebugLog( D_INFO, DC_ALL ) << "[main] C locale set to " << setlocale( LC_ALL, nullptr );
    DebugLog( D_INFO, DC_ALL ) << "[main] C++ locale set to " << std::locale().name();
}

void log_sdl_versions()
{
#if defined(TILES) || defined(SDL_SOUND)
    const SDLVersionInfo compiled = GetCompiledSDLVersion();
    DebugLog( D_INFO, DC_ALL ) << "SDL version used during compile is "
                               << compiled.major << "."
                               << compiled.minor << "."
                               << compiled.patch;

    const SDLVersionInfo linked = GetLinkedSDLVersion();
    DebugLog( D_INFO, DC_ALL ) << "SDL version used during linking and in runtime is "
                               << linked.major << "."
                               << linked.minor << "."
                               << linked.patch;
#endif
}

bool initialize_interface( const cli_opts &cli )
{
#if !defined(TILES)
    get_options().init();
    get_options().load();
#endif

    // in test mode don't initialize curses to avoid escape sequences being inserted into output stream
    if( !test_mode ) {
        try {
            // set minimum FULL_SCREEN sizes
            FULL_SCREEN_WIDTH = EVEN_MINIMUM_TERM_WIDTH;
            FULL_SCREEN_HEIGHT = EVEN_MINIMUM_TERM_HEIGHT;
            catacurses::init_interface();
        } catch( const std::exception &err ) {
            // can't use any curses function as it has not been initialized
            std::cerr << "Error while initializing the interface: " << err.what() << std::endl;
            DebugLog( D_ERROR, DC_ALL ) << "Error while initializing the interface: " << err.what() << "\n";
            return false;
        }
    } else if( cli.check_mods ) {
        get_options().init();
        get_options().load();
    }

    return true;
}

void load_static_game_data( const cli_opts &cli )
{
    // First load and initialize everything that does not
    // depend on the mods.
    try {
        g->load_static_data();
        if( cli.verifyexit ) {
            exit_handler( 0 );
        }
        if( cli.check_mods ) {
            init_colors();
            const std::vector<mod_id> mods( cli.opts.begin(), cli.opts.end() );
            exit( g->check_mod_data( mods ) && !debug_has_error_been_observed() ? 0 : 1 );
        }
    } catch( const std::exception &err ) {
        debugmsg( "%s", err.what() );
        exit_handler( -999 );
    }
}

void initialize_imgui()
{
    // Load the colors of ImGui to match the colors set by the user.
    cataimgui::init_colors();

    // set decimal point for float input widgets
    // uses system locale, because that's what imgui uses to parse and display floats
    ImGui::GetPlatformIO().Platform_LocaleDecimalPoint =
        static_cast<unsigned char>( *localeconv()->decimal_point );
}

void apply_runtime_cli_overrides( const cli_opts &cli )
{
    // Override existing settings from cli options
    if( cli.disable_ascii_art ) {
        get_options().get_option( "ENABLE_ASCII_ART" ).setValue( "false" );
        get_options().get_option( "ENABLE_ASCII_TITLE" ).setValue( "false" );
    }

    if( cli.noverify ) {
        get_options().get_option( "SKIP_VERIFICATION" ).setValue( "true" );
    }
}

void configure_curses_cursor()
{
#if defined(DEBUG_CURSES_CURSOR)
    catacurses::curs_set( 2 );
#else
    // I have no clue what this comment is on about
    // Any value works well enough for debugging at least
    catacurses::curs_set( 0 ); // Invisible cursor here, because MAPBUFFER.load() is crash-prone
#endif
}

void install_signal_handlers()
{
#if !defined(_WIN32)
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset( &sigIntHandler.sa_mask );
    sigIntHandler.sa_flags = 0;
    sigaction( SIGINT, &sigIntHandler, nullptr );
#endif
}

void select_initial_language()
{
#if defined(LOCALIZE)
    if( get_option<std::string>( "USE_LANG" ).empty() && !SystemLocale::Language().has_value() ) {
#if defined(TILES)
        display_buffer_draw_scope draw_scope;
        if( !display_buffer_scope_is_invalid() ) {
#endif
            imclient->new_frame(); // we have to prime the pump, because of reasons
            imclient->end_frame();
            const std::string lang = select_language();
            get_options().get_option( "USE_LANG" ).setValue( lang );
            set_language_from_options();
#if defined(TILES)
        }
#endif
    }
#endif
}

// Queue a scripted key sequence: one key name per line, '#' comments and blank lines
// ignored.  Keys are resolved with the game's own keycode tables *for the active keyboard
// mode*, so "F9", "j" and "RETURN" all work and look like real keypresses.  The queue is
// served by input_manager::get_input_event() ahead of real input, which makes a driven run
// reproducible.
static void load_input_script( const std::string &path )
{
    std::ifstream fin( path );
    if( !fin.is_open() ) {
        debugmsg( "input script: cannot read %s", path );
        return;
    }
    std::vector<std::string> keys;
    std::string line;
    while( std::getline( fin, line ) ) {
        while( !line.empty() && ( line.back() == '\r' || line.back() == ' ' ||
                                  line.back() == '\t' ) ) {
            line.pop_back();
        }
        const size_t start = line.find_first_not_of( " \t" );
        if( start == std::string::npos || line[start] == '#' ) {
            continue;
        }
        line = line.substr( start );
        // Keep the *name*: input_manager resolves it per keyboard mode, so a scripted key
        // looks like a real keypress wherever it is consumed.
        keys.push_back( line );
    }
    if( !keys.empty() ) {
        inp_mngr.inject_input_events( keys );
    }
}

void run_game_loop( cli_opts &cli )
{
    main_menu::queued_world_to_load = std::move( cli.world );
    main_menu::queued_save_id_to_load = std::move( cli.save_id );

    // NOTE: the input script is already queued in main(), before data loading, so that it
    // can also answer the buffered debugmsg prompts replayed during startup.

    if( cli.new_character ) {
        // Non-interactive character creation: this is what automation uses instead of
        // driving the main menu (and it skips world selection entirely).
        // NOTE: the world factory is only initialised inside main_menu::opening_screen()
        // normally, and we are deliberately skipping that, so do it here.
        world_generator->init();
        WORLD *w = world_generator->get_world( main_menu::queued_world_to_load );
        if( w == nullptr ) {
            debugmsg( "--new-character needs --world (no world named '%s')",
                      main_menu::queued_world_to_load );
            return;
        }
        world_generator->set_active_world( w );
        if( !g->start_new_character() ) {
            return;
        }
        if( !cli.pocket_patch.empty() ) {
            g->apply_pocket_patch( cli.pocket_patch );
        }
        if( !cli.var_patch.empty() ) {
            g->apply_var_patch( cli.var_patch );
        }
        if( !cli.widget_dump.empty() ) {
            g->set_widget_dump_ids( cli.widget_dump );
        }
        if( !cli.selector_run.empty() ) {
            // Run a selector EOC for its dump alone.  This is the only deterministic way to see a
            // menu that is normally reached by activating a mutation: the mutation list sorts by
            // id, and RETURN activates the *selected* entry, so a key script cannot pick one.
            set_selector_dry_run( true );
            set_selector_dump_enabled( true );
            const effect_on_condition_id eoc( cli.selector_run );
            // A marker, so a missing dump can be told apart from "no selector was built": it is
            // written whenever the hook is *reached*, and mcp_selector.json appears only if the
            // EOC actually ran a selector.  Absent marker = the hook never ran (a UI blocked the
            // load first), which reads very differently from a gated EOC.
            try {
                const cata_path marker =
                    PATH_INFO::world_base_save_path() / "mcp_selector.run.json";
                std::ofstream mout( marker.get_unrelative_path(), std::ios::trunc );
                if( mout.is_open() ) {
                    JsonOut mjout( mout );
                    mjout.start_object();
                    mjout.member( "eoc", cli.selector_run );
                    mjout.member( "valid", eoc.is_valid() );
                    mjout.end_object();
                }
            } catch( const std::exception & ) {
                // diagnostics are best effort
            }
            if( eoc.is_valid() ) {
                dialogue d( get_talker_for( get_avatar() ), nullptr );
                eoc->activate( d );
            } else {
                debugmsg( "selector-run: no effect_on_condition with id '%s'",
                          cli.selector_run );
            }
        }
        if( cli.snapshot ) {
            g->request_snapshot();
        }
        while( !g->do_turn() ) {}
        return;
    }

    while( true ) {
        main_menu menu;
        if( !menu.opening_screen() ) {
            break;
        }

        if( !cli.pocket_patch.empty() ) {
            g->apply_pocket_patch( cli.pocket_patch );
        }
        if( !cli.var_patch.empty() ) {
            g->apply_var_patch( cli.var_patch );
        }
        if( !cli.widget_dump.empty() ) {
            g->set_widget_dump_ids( cli.widget_dump );
        }
        if( !cli.selector_run.empty() ) {
            // Run a selector EOC for its dump alone.  This is the only deterministic way to see a
            // menu that is normally reached by activating a mutation: the mutation list sorts by
            // id, and RETURN activates the *selected* entry, so a key script cannot pick one.
            set_selector_dry_run( true );
            set_selector_dump_enabled( true );
            const effect_on_condition_id eoc( cli.selector_run );
            // A marker, so a missing dump can be told apart from "no selector was built": it is
            // written whenever the hook is *reached*, and mcp_selector.json appears only if the
            // EOC actually ran a selector.  Absent marker = the hook never ran (a UI blocked the
            // load first), which reads very differently from a gated EOC.
            try {
                const cata_path marker =
                    PATH_INFO::world_base_save_path() / "mcp_selector.run.json";
                std::ofstream mout( marker.get_unrelative_path(), std::ios::trunc );
                if( mout.is_open() ) {
                    JsonOut mjout( mout );
                    mjout.start_object();
                    mjout.member( "eoc", cli.selector_run );
                    mjout.member( "valid", eoc.is_valid() );
                    mjout.end_object();
                }
            } catch( const std::exception & ) {
                // diagnostics are best effort
            }
            if( eoc.is_valid() ) {
                dialogue d( get_talker_for( get_avatar() ), nullptr );
                eoc->activate( d );
            } else {
                debugmsg( "selector-run: no effect_on_condition with id '%s'",
                          cli.selector_run );
            }
        }
        if( cli.snapshot ) {
            g->request_snapshot();
        }

        shared_ptr_fast<ui_adaptor> ui = g->create_or_get_main_ui_adaptor();
        get_event_bus().send<event_type::game_begin>( getVersionString() );
        while( !g->do_turn() ) {}
    }
}

}  // namespace

#if defined(USE_WINMAIN)
int APIENTRY WinMain( _In_ HINSTANCE /* hInstance */, _In_opt_ HINSTANCE /* hPrevInstance */,
                      _In_ LPSTR /* lpCmdLine */, _In_ int /* nCmdShow */ )
{
    int argc = __argc;
    char **argv = __argv;
#elif defined(__ANDROID__)
extern "C" int SDL_main( int argc, char **argv ) {
#else
int main( int argc, const char *argv[] )
{
#endif

    initialize_runtime();

#if defined(EMSCRIPTEN)
    mount_idbfs();
#endif

    on_out_of_scope json_member_reporting_guard{ [] {
            // Disable reporting unvisited members if stack unwinding leaves main early.
            Json::globally_report_unvisited_members( false );
        } };

    initialize_platform_output();
    initialize_default_paths();

    MAP_SHARING::setDefaults();

    cli_opts cli = parse_commandline( argc, const_cast<const char **>( argv ) );

    // Queue scripted keys as early as possible: data loading buffers its debugmsg prompts
    // and replays them during startup, and each prompt blocks until a key arrives, so a
    // script queued later can never answer them and the run stalls on the prompt instead of
    // playing.  input_manager::inject_input_events() makes sure the keycode tables the keys
    // resolve against exist even this early.
    if( !cli.input_script.empty() ) {
        load_input_script( cli.input_script );
    }

    // An unattended run must not block on a developer-facing prompt.  Data loading buffers
    // debugmsg prompts (a duplicate mod id, a JSON error in data/mods, ...) and replays them
    // during startup; each one waits for a key, which would stall the run before it plays.
    // The reports still reach the debug log, where cdda_tail_debug can read them.
    if( cli.dismiss_debug_prompts ) {
        set_debug_prompts_auto_dismiss( true );
    }
    if( cli.dialogue_dump ) {
        set_dialogue_dump_enabled( true );
    }
    if( cli.selector_dump ) {
        set_selector_dump_enabled( true );
    }

    validate_directories();
    initialize_debugging();
    initialize_locale();
    log_sdl_versions();

    if( !initialize_interface( cli ) ) {
        return 1;
    }

    set_language_from_options();
    rng_set_engine_seed( cli.seed );
    game_ui::init_ui();

    g = std::make_unique<game>();
    load_static_game_data( cli );

    initialize_imgui();
    apply_runtime_cli_overrides( cli );

    // Now we do the actual game.
    configure_curses_cursor();
    install_signal_handlers();

    if( !assure_essential_dirs_exist() ) {
        exit_handler( -999 );
        return 0;
    }

    select_initial_language();
    replay_buffered_debugmsg_prompts();

    run_game_loop( cli );

    exit_handler( -999 );
    return 0;
}