#include "dragon.hpp"

bool strendswith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string dirnameFromPath(const std::string& path) {
    std::string directory;
    const size_t last_slash_idx = path.rfind('/');
    if (std::string::npos != last_slash_idx) {
        directory = path.substr(0, last_slash_idx);
    }
    return directory;
}

#if __has_attribute(noreturn)
__attribute__((noreturn))
#endif
void handle_signal(int signal) {
    DRAGON_LOG << "Caught signal " << signal << std::endl;
    if (errno)
        DRAGON_LOG << "Error: " << strerror(errno) << std::endl;
#if !defined(_WIN32) && !defined(__wasm__)
    void* array[64];
    char** strings;
    int size, i;

    size = backtrace(array, 64);
    strings = backtrace_symbols(array, size);
    if (strings != NULL) {
        for (i = 0; i < size; i++)
            printf("  %s\n", strings[i]);
    }

    free(strings);
#endif
    exit(signal);
}

#include "DragonConfig.hpp"

void usage(std::string progName, std::ostream& sink) {
    sink << "Usage: " << progName << " <command> [options]" << std::endl;
    sink << "Commands:" << std::endl;
    sink << "  build     Build the project" << std::endl;
    sink << "  run       Compile and run the project" << std::endl;
    sink << "  init      Initialize a new project. init allows overriding of default values for faster configuration" << std::endl;
    sink << "  clean     Clean all project files" << std::endl;
    sink << "  help      Show this help" << std::endl;
    sink << "  version   Show the version" << std::endl;
    sink << "  config    Show the current config" << std::endl;
    sink << "  presets   List the available presets" << std::endl;
    sink << "  package   Run the 'package' subcommand" << std::endl;
    sink << std::endl;
    sink << "Options:" << std::endl;
    sink << "  -c, --config <path>         Path to config file" << std::endl;
    sink << "  -compiler <compiler>        Override compiler" << std::endl;
    sink << "  -outputDir <dir>            Override output directory" << std::endl;
    sink << "  -target <name>              Override output file" << std::endl;
    sink << "  -sourceDir <dir>            Override source directory" << std::endl;
    sink << "  -unit <unit>                Add a unit to the build" << std::endl;
    sink << "  -include <dir>              Add an include directory to the build" << std::endl;
    sink << "  -lib <lib>                  Add a library to the build" << std::endl;
    sink << "  -libraryPath <path>         Add a library path to the build" << std::endl;
    sink << "  -define <define>            Add a define to the build" << std::endl;
    sink << "  -flag <flag>                Add a flag to the build" << std::endl;
    sink << "  -preBuild <command>         Run a command before the build" << std::endl;
    sink << "  -postBuild <command>        Run a command after the build" << std::endl;
    sink << "  -macroPrefix <prefix>       Compiler prefix for macros" << std::endl;
    sink << "  -libraryPrefix <prefix>     Compiler prefix for libraries" << std::endl;
    sink << "  -libraryPathPrefix <prefix> Compiler prefix for library paths" << std::endl;
    sink << "  -includePrefix <prefix>     Compiler prefix for includes" << std::endl;
    sink << "  -outputPrefix <prefix>      Compiler prefix for output files" << std::endl;
    sink << "  -preset <preset>            Use a preset for initialization (only works with the 'init' command)" << std::endl;
    sink << "  -conf <key>                 Use key as the root key for build configuration" << std::endl;
}

bool overrideCompiler = false;
bool overrideOutputDir = false;
bool overrideTarget = false;
bool overrideSourceDir = false;
bool overrideMacroPrefix = false;
bool overrideLibraryPrefix = false;
bool overrideLibraryPathPrefix = false;
bool overrideIncludePrefix = false;
bool overrideOutFilePrefix = false;

std::string compiler = "gcc";
std::string outputDir = "build";
std::string target = "main";
std::string sourceDir = "src";

std::string macroPrefix = "-D";
std::string libraryPrefix = "-l";
std::string libraryPathPrefix = "-L";
std::string includePrefix = "-I";
std::string outFilePrefix = "-o";

std::vector<std::string> customUnits;
std::vector<std::string> customIncludes;
std::vector<std::string> customLibs;
std::vector<std::string> customLibraryPaths;
std::vector<std::string> customDefines;
std::vector<std::string> customFlags;
std::vector<std::string> customPreBuilds;
std::vector<std::string> customPostBuilds;

std::string buildConfigRootEntry = "build";

std::string replaceAll(std::string src, std::string from, std::string to) {
    try {
        return regex_replace(src, std::regex(from), to);
    } catch (std::regex_error& e) {
        return src;
    }
}

bool strstarts(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, const char* argv[])
{
    signal(SIGSEGV, handle_signal);
    signal(SIGABRT, handle_signal);
    signal(SIGILL, handle_signal);
    signal(SIGFPE, handle_signal);

    std::string configFile = "build.drg";

    if (argc < 2) {
        usage(argv[0], std::cout);
        return 0;
    }

    std::string command = argv[1];

    if (command == "package") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) {
            args.push_back(std::string(argv[i]));
        }
        return cmd_package(args);
    }

    std::string key = "";

    for (int i = 2; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        if (arg == "-c" || arg == "--buildConfig") {
            if (i + 1 < argc) {
                configFile = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No buildConfig file specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-compiler") {
            overrideCompiler = true;
            if (i + 1 < argc) {
                compiler = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No compiler specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-outputDir") {
            overrideOutputDir = true;
            if (i + 1 < argc) {
                outputDir = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No output directory specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-target") {
            overrideTarget = true;
            if (i + 1 < argc) {
                target = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No output file specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-sourceDir") {
            overrideSourceDir = true;
            if (i + 1 < argc) {
                sourceDir = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No source directory specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-unit") {
            if (i + 1 < argc) {
                customUnits.push_back(std::string(argv[++i]));
            } else {
                DRAGON_ERR << "No unit specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-include") {
            if (i + 1 < argc) {
                customIncludes.push_back(std::string(argv[++i]));
            } else {
                DRAGON_ERR << "No include specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-lib") {
            if (i + 1 < argc) {
                customLibs.push_back(std::string(argv[++i]));
            } else {
                DRAGON_ERR << "No library specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-libraryPath") {
            if (i + 1 < argc) {
                customLibraryPaths.push_back(std::string(argv[++i]));
            } else {
                DRAGON_ERR << "No library directory specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-define") {
            if (i + 1 < argc) {
                customDefines.push_back(std::string(argv[++i]));
            } else {
                DRAGON_ERR << "No define specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-flag") {
            if (i + 1 < argc) {
                customFlags.push_back(std::string(argv[++i]));
            } else {
                DRAGON_ERR << "No flag specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-preset") {
            if (command != "init") {
                DRAGON_ERR << "Presets can only be used to initialize a buildConfig file" << std::endl;
                exit(1);
            }
            if (i + 1 < argc) {
                std::string preset = std::string(argv[++i]);
                load_preset(preset);
                cmd_init(configFile);
                return 0;
            } else {
                DRAGON_ERR << "No preset specified" << std::endl;
                DRAGON_ERR << "Available presets: " << std::endl;
                std::vector<std::string> presets = get_presets();
                for (auto preset : presets) {
                    DRAGON_ERR << "    " << preset << std::endl;
                }
                exit(1);
            }
        } else if (arg == "-macroPrefix") {
            overrideMacroPrefix = true;
            if (i + 1 < argc) {
                macroPrefix = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No macro prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-libraryPrefix") {
            overrideLibraryPrefix = true;
            if (i + 1 < argc) {
                libraryPrefix = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No library prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-libraryPathPrefix") {
            overrideLibraryPathPrefix = true;
            if (i + 1 < argc) {
                libraryPathPrefix = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No library path prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-includePrefix") {
            overrideIncludePrefix = true;
            if (i + 1 < argc) {
                includePrefix = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No include prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-outputPrefix") {
            overrideOutFilePrefix = true;
            if (i + 1 < argc) {
                outFilePrefix = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No output prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-conf") {
            if (i + 1 < argc) {
                buildConfigRootEntry = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No build Configuration specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-get-key" && command == "config") {
            if (i + 1 < argc) {
                key = std::string(argv[++i]);
            } else {
                DRAGON_ERR << "No key specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-h" || arg == "--help") {
            usage(argv[0], std::cout);
            return 0;
        } else if (command != "package") {
            DRAGON_ERR << "Unknown argument: " << arg << std::endl;
            usage(std::string(argv[0]), std::cerr);
            exit(1);
        }
    }

    if (command == "init") {
        cmd_init(configFile);
    } else if (command == "build") {
        cmd_build(configFile);
    } else if (command == "help") {
        usage(argv[0], std::cout);
    } else if (command == "version") {
        std::cout << "Dragon version " << VERSION << std::endl;
    } else if (command == "run") {
        cmd_run(configFile);
    } else if (command == "clean") {
        cmd_clean(configFile);
    } else if (command == "config") {
        DragonConfig::ConfigParser parser;
        DragonConfig::CompoundEntry* root = parser.parse(configFile);
        if (key.size()) {
            DragonConfig::StringEntry* entry = root->getStringByPath(key);
            if (entry) {
                std::cout << entry->getValue() << std::endl;
            } else {
                DRAGON_ERR << "Path '" << key << "' not found" << std::endl;
                exit(-1);
            }
        } else {
            root->print(std::cout);
        }
    } else if (command == "presets") {
        std::vector<std::string> presets = get_presets();
        DRAGON_LOG << "Available presets: " << std::endl;
        for (auto preset : presets) {
            std::cout << "  " << preset << std::endl;
        }
    } else {
        DRAGON_ERR << "Unknown command: " << command << std::endl;
        usage(std::string(argv[0]), std::cerr);
        exit(1);
    }
    
    return 0;
}
