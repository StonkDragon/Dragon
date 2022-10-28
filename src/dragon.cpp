#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#ifdef _WIN32
#error "Windows is currently not supported."
#endif

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#define DRAGON_UNSUPPORTED_STR "<DRAGON_UNSUPPORTED>"

#ifdef __APPLE__
// macOS is annoying and doesn't have std::filesystem
#define filesystem __fs::filesystem
// macOS is also annoying and doesn't have std::ostream
#define ostream __1::ostream
#endif

#ifdef __linux__
const std::string HOSTNAME = "linux";
#elif _WIN32
const std::string HOSTNAME = "windows";
#elif __APPLE__
const std::string HOSTNAME = "macos";
#else
const std::string HOSTNAME = "unknown";
#endif

bool isOSSupported(const std::string& os) {
    return HOSTNAME == os;
}

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

std::string cmd_build(std::string& configFile) {
    bool configExists = std::filesystem::exists(configFile);
    if (!configExists) {
        std::cerr << "[Dragon] " << "Config file not found!" << std::endl;
        std::cerr << "[Dragon] " << "Have you forgot to run 'dragon init'?" << std::endl;
        exit(1);
    }
    
    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry root = parser.parse(configFile);
    DragonConfig::CompoundEntry buildConfig = root.getCompound(buildConfigRootEntry);

    DragonConfig::ListEntry targetOS = buildConfig.getList("osTarget");
    bool osIsSupported = false;
    for (u_long i = 0; i < targetOS.size(); i++) {
        std::string os = targetOS.get(i);
        if (isOSSupported(os)) {
            osIsSupported = true;
            break;
        }
    }
    if (targetOS.isEmpty()) osIsSupported = true;
    if (!osIsSupported) {
        std::string msg = buildConfig.getStringOrDefault("wrongOsMsg", "OS not supported!").getValue();
        std::cerr << "[Dragon] " << msg << std::endl;
        exit(1);
    }
    
    if (buildConfig.getList("units").size() == 0) {
        std::cerr << "[Dragon] " << "No compilation units defined!" << std::endl;
        exit(1);
    }

    if (overrideCompiler) {
        buildConfig.setString("compiler", compiler);
    }
    if (overrideOutputDir) {
        buildConfig.setString("outputDir", outputDir);
    }
    if (overrideTarget) {
        buildConfig.setString("target", target);
    }
    if (overrideSourceDir) {
        buildConfig.setString("sourceDir", sourceDir);
    }
    if (overrideMacroPrefix) {
        buildConfig.setString("macroPrefix", macroPrefix);
    }
    if (overrideLibraryPrefix) {
        buildConfig.setString("libraryPrefix", libraryPrefix);
    }
    if (overrideLibraryPathPrefix) {
        buildConfig.setString("libraryPathPrefix", libraryPathPrefix);
    }
    if (overrideIncludePrefix) {
        buildConfig.setString("includePrefix", includePrefix);
    }
    if (overrideOutFilePrefix) {
        buildConfig.setString("outFilePrefix", outFilePrefix);
    }
    std::string cmd = buildConfig.getStringOrDefault("compiler", "clang").getValue();
    cmd += " ";
    for (u_long i = 0; i < buildConfig.getList("flags").size(); i++) {
        cmd += buildConfig.getList("flags").get(i);
        cmd += " ";
    }
    for (auto flag : customFlags) {
        cmd += flag;
        cmd += " ";
    }
    for (u_long i = 0; i < buildConfig.getList("defines").size(); i++) {
        cmd += buildConfig.getStringOrDefault("macroPrefix", "-D").getValue();
        cmd += buildConfig.getList("defines").get(i);
        cmd += " ";
    }
    for (auto define : customDefines) {
        if (buildConfig.getStringOrDefault("macroPrefix", "-D").getValue() == DRAGON_UNSUPPORTED_STR) {
            std::cerr << "[Dragon] " << "Macro prefix not supported by compiler!" << std::endl;
        } else {
            cmd += buildConfig.getStringOrDefault("macroPrefix", "-D").getValue();
            cmd += define;
            cmd += " ";
        }
    }
    for (u_long i = 0; i < buildConfig.getList("libraryPaths").size(); i++) {
        cmd += buildConfig.getStringOrDefault("libraryPathPrefix", "-L").getValue();
        cmd += buildConfig.getList("libraryPaths").get(i);
        cmd += " ";
    }
    for (auto libDir : customLibraryPaths) {
        cmd += buildConfig.getStringOrDefault("libraryPathPrefix", "-L").getValue();
        cmd += libDir;
        cmd += " ";
    }
    for (u_long i = 0; i < buildConfig.getList("includes").size(); i++) {
        if (buildConfig.getStringOrDefault("includePrefix", "-I").getValue() == DRAGON_UNSUPPORTED_STR) {
            std::cerr << "[Dragon] " << "Include prefix not supported by compiler!" << std::endl;
        } else {
            cmd += buildConfig.getStringOrDefault("includePrefix", "-I").getValue();
            cmd += buildConfig.getList("includes").get(i);
            cmd += " ";
        }
    }
    for (auto include : customIncludes) {
        if (buildConfig.getStringOrDefault("includePrefix", "-I").getValue() == DRAGON_UNSUPPORTED_STR) {
            std::cerr << "[Dragon] " << "Include prefix not supported by compiler!" << std::endl;
        } else {
            cmd += buildConfig.getStringOrDefault("includePrefix", "-I").getValue();
            cmd += include;
            cmd += " ";
        }
    }

    std::string outputFile = buildConfig.getStringOrDefault("outputDir", "build").getValue();
    outputFile += std::filesystem::path::preferred_separator;
    outputFile += buildConfig.getStringOrDefault("target", "main").getValue();

    std::string stdlib = buildConfig.getString("std").getValue();

    if (stdlib.size() > 0) {
        cmd += "-std=";
        cmd += stdlib;
        cmd += " ";
    }

    bool outDirExists = std::filesystem::exists(buildConfig.getStringOrDefault("outputDir", "build").getValue());
    if (outDirExists) {
        if (buildConfig.getStringOrDefault("outputDir", "build").getValue() == ".") {
            std::cerr << "[Dragon] " << "Cannot build in current directory" << std::endl;
            exit(1);
        }
        std::filesystem::remove_all(buildConfig.getStringOrDefault("outputDir", "build").getValue());
    }
    try {
        std::filesystem::create_directories(buildConfig.getStringOrDefault("outputDir", "build").getValue());
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create output directory: " << buildConfig.getStringOrDefault("outputDir", "build").getValue() << std::endl;
        exit(1);
    }

    bool sourceDirExists = std::filesystem::exists(buildConfig.getStringOrDefault("sourceDir", "src").getValue());
    if (!sourceDirExists) {
        std::cerr << "[Dragon] " << "Source directory does not exist: " << buildConfig.getStringOrDefault("sourceDir", "src").getValue() << std::endl;
        exit(1);
    }

    for (u_long i = 0; i < buildConfig.getList("preBuild").size(); i++) {
        std::cout << "[Dragon] Running prebuild command: " << buildConfig.getList("preBuild").get(i) << std::endl;
        int ret = system(buildConfig.getList("preBuild").get(i).c_str());
        if (ret != 0) {
            std::cerr << "[Dragon] " << "Pre-build command failed: " << buildConfig.getList("preBuild").get(i) << std::endl;
            exit(1);
        }
    }

    size_t unitSize = customUnits.size();

    for (u_long i = 0; i < buildConfig.getList("units").size(); i++) {
        std::string unit = buildConfig.getList("units").get(i);
        
        cmd += buildConfig.getStringOrDefault("sourceDir", "src").getValue();
        cmd += std::filesystem::path::preferred_separator;
        cmd += unit;
        cmd += " ";
    }

    for (size_t i = 0; i < unitSize; i++) {
        std::string unit = customUnits.at(i);
        
        cmd += buildConfig.getStringOrDefault("sourceDir", "src").getValue();
        cmd += std::filesystem::path::preferred_separator;
        cmd += unit;
        cmd += " ";
    }
    
    for (u_long i = 0; i < buildConfig.getList("libs").size(); i++) {
        cmd += buildConfig.getStringOrDefault("libraryPrefix", "-l").getValue();
        cmd += buildConfig.getList("libs").get(i);
        cmd += " ";
    }
    for (auto lib : customLibs) {
        cmd += buildConfig.getStringOrDefault("libraryPrefix", "-l").getValue();
        cmd += lib;
        cmd += " ";
    }

    cmd += buildConfig.getStringOrDefault("outFilePrefix", "-o").getValue();
    cmd += " ";
    cmd += outputFile;
    cmd += " ";

    std::string cmdStr = cmd;
    std::cout << "[Dragon] Running build command: " << cmdStr << std::endl;
    int run = system(cmdStr.c_str());
    if (run != 0) {
        std::cerr << "[Dragon] " << "Failed to run command: " << cmdStr << std::endl;
        exit(run);
    }
    for (u_long i = 0; i < buildConfig.getList("postBuild").size(); i++) {
        std::cout << "[Dragon] Running postbuild command: " << buildConfig.getList("postBuild").get(i) << std::endl;
        int ret = system(buildConfig.getList("postBuild").get(i).c_str());
        if (ret != 0) {
            std::cerr << "[Dragon] " << "Post-build command failed: " << buildConfig.getList("postBuild").get(i) << std::endl;
            exit(1);
        }
    }

    for (u_long i = 0; i < buildConfig.getList("units").size(); i++) {
        std::string unit;
        unit = buildConfig.getStringOrDefault("outputDir", "build").getValue();
        unit += std::filesystem::path::preferred_separator;
        unit += buildConfig.getList("units").get(i);
        unit += ".o";
        remove(unit.c_str());
    }

    for (auto unit : customUnits) {
        std::string unit2 = buildConfig.getStringOrDefault("outputDir", "build").getValue();
        unit2 += std::filesystem::path::preferred_separator;
        unit2 += unit;
        unit2 += ".o";
        remove(unit2.c_str());
    }

    return outputFile;
}

void cmd_init(std::string& configFile) {
    if (std::filesystem::exists(configFile)) {
        std::cout << "[Dragon] " << "Config file already exists." << std::endl;
        return;
    }
    std::ofstream def(configFile);

    def << "build: {\n";
    def << "  compiler: \"" << (overrideCompiler ? compiler : "gcc") << "\";\n";
    def << "  sourceDir: \"" << (overrideSourceDir ? sourceDir : "src") << "\";\n";
    def << "  outputDir: \"" << (overrideOutputDir ? outputDir : "build") << "\";\n";
    def << "  target: \"" << (overrideTarget ? target : "main") << "\";\n";
    if (customIncludes.size() > 0) {
        def << "  includes: [\n";
        for (auto include : customIncludes) {
            def << "    \"" << include << "\";\n";
        }
        def << "];\n";
    } else {
        def << "  includes: [];\n";
    }
    if (customUnits.size() > 0) {
        def << "  units: [\n";
        for (auto unit : customUnits) {
            def << "    \"" << unit << "\";\n";
        }
        def << "  ];\n";
    } else {
        def << "  units: [];\n";
    }
    def << "  outFilePrefix: \"" << (overrideOutFilePrefix ? outFilePrefix : "-o") << "\";\n";
    def << "  libraryPathPrefix: \"" << (overrideLibraryPathPrefix ? libraryPathPrefix : "-L") << "\";\n";
    def << "  libraryPrefix: \"" << (overrideLibraryPrefix ? libraryPrefix : "-l") << "\";\n";
    def << "  macroPrefix: \"" << (overrideMacroPrefix ? macroPrefix : "-D") << "\";\n";
    def << "  includePrefix: \"" << (overrideIncludePrefix ? includePrefix : "-I") << "\";\n";

    if (customLibraryPaths.size() > 0) {
        def << "  libraryPaths: [\n";
        for (auto libraryPath : customLibraryPaths) {
            def << "    \"" << libraryPath << "\";\n";
        }
        def << "  ];\n";    
    } else {
        def << "  libraryPaths: [];\n";
    }
    if (customLibs.size() > 0) {
        def << "  libs: [\n";
        for (auto library : customLibs) {
            def << "    \"" << library << "\";\n";
        }
        def << "  ];\n";    
    } else {
        def << "  libs: [];\n";
    }
    if (customDefines.size() > 0) {
        def << "  defines: [\n";
        for (auto define : customDefines) {
            def << "    \"" << define << "\";\n";
        }
        def << "  ];\n";    
    } else {
        def << "  defines: [];\n";
    }
    if (customFlags.size() > 0) {
        def << "  flags: [\n";
        for (auto flag : customFlags) {
            def << "    \"" << flag << "\";\n";
        }
        def << "  ];\n";    
    } else {
        def << "  flags: [];\n";
    }
    if (customPreBuilds.size() > 0) {
        def << "  preBuild: [\n";
        for (auto preBuild : customPreBuilds) {
            def << "    \"" << preBuild << "\";\n";
        }
        def << "  ];\n";    
    } else {
        def << "  preBuild: [];\n";
    }
    if (customPostBuilds.size() > 0) {
        def << "  postBuild: [\n";
        for (auto postBuild : customPostBuilds) {
            def << "    \"" << postBuild << "\";\n";
        }
        def << "  ];\n";    
    } else {
        def << "  postBuild: [];\n";
    }
    def << "};" << std::endl;

    def.close();
    std::cout << "[Dragon] " << "Config file created at " << configFile << std::endl;
}

void cmd_run(std::string& configFile) {
    std::string outfile = cmd_build(configFile);
    std::string cmd = outfile;

    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry root = parser.parse(configFile);
    DragonConfig::CompoundEntry run = root.getCompound("run");
    
    std::vector<std::string> argv;
    std::vector<std::string> envs;
    argv.push_back(cmd);
    if (run != DragonConfig::CompoundEntry::Empty) {
        DragonConfig::ListEntry args = run.getList("args");
        DragonConfig::ListEntry env = run.getList("env");
        if (args != DragonConfig::ListEntry::Empty) {
            for (u_long i = 0; i < args.size(); i++) {
                argv.push_back(args.get(i));
            }
        }
        if (env != DragonConfig::ListEntry::Empty) {
            for (u_long i = 0; i < env.size(); i++) {
                envs.push_back(env.get(i));
            }
        }
    }

    char** envs_c = NULL;
    envs_c = (char**) malloc(envs.size() * sizeof(char*) + 1);
    for (u_long i = 0; i < envs.size(); i++) {
        envs_c[i] = (char*)envs[i].c_str();
    }
    envs_c[envs.size()] = NULL;

    char** argv_c = NULL;
    argv_c = (char**) malloc(argv.size() * sizeof(char*) + 1);
    for (u_long i = 0; i < argv.size(); i++) {
        argv_c[i] = (char*)argv[i].c_str();
    }
    argv_c[argv.size()] = NULL;

    std::cout << "[Dragon] " << "Running " << cmd << std::endl;

    int child = fork();
    if (child == 0) {
        int ret = execve(cmd.c_str(), argv_c, envs_c);
        if (ret == -1) {
            std::cout << "[Dragon] " << "Error: " << strerror(errno) << std::endl;
            exit(1);
        }
    } else if (child > 0) {
        wait(NULL);
        std::cout << "[Dragon] " << "Finished!" << std::endl;
    } else {
        std::cerr << "[Dragon] " << "Error forking a child process!" << std::endl;
        std::cerr << "[Dragon] " << "Error: " << strerror(errno) << std::endl;
        exit(1);
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

void generate_generic_main(std::string lang) {
    if (lang == "c") {
        std::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.c");
        mainFile << "#include <stdio.h>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    printf(\"Hello, World!\\n\");\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "cpp") {
        std::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.cpp");
        mainFile << "#include <iostream>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    std::cout << \"Hello, World!\" << std::endl;\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "scale") {
        std::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.scale");
        mainFile << "#include \"core.scale\"\n\n";
        mainFile << "function main()\n";
        mainFile << "    \"Hello, World!\" puts\n";
        mainFile << "end" << std::endl;
        mainFile.close();
    } else if (lang == "kotlin") {
        std::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.kt");
        mainFile << "fun main() {\n";
        mainFile << "    println(\"Hello, World!\")\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "objc") {
        std::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.m");
        mainFile << "#import <Foundation/Foundation.h>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    @autoreleasepool {\n";
        mainFile << "        NSLog(@\"Hello, World!\");\n";
        mainFile << "    }\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else {
        std::cerr << "[Dragon] " << "Unknown language: " << lang << std::endl;
        exit(1);
    }
}

std::vector<std::string> get_presets() {
    std::vector<std::string> presets;
    presets.push_back("clang-c");
    presets.push_back("clang-cpp");
    presets.push_back("gcc-c");
    presets.push_back("gcc-cpp");
    presets.push_back("tcc-c");
    presets.push_back("sclc-scale");
    presets.push_back("kotlin-kotlin");
    presets.push_back("clang-objc");
    presets.push_back("gcc-objc");
    return presets;
}

void load_preset(std::string& identifier) {
    std::cout << "[Dragon] " << "Loading preset: " << identifier << std::endl;
    std::vector<std::string> tokens = split(identifier, '-');
    if (tokens.size() != 2) {
        std::cerr << "[Dragon] " << "Invalid preset identifier: " << identifier << std::endl;
        std::cerr << "[Dragon] " << "Expected format: <compiler>-<lang>" << std::endl;
        std::cerr << "[Dragon] " << "Available presets: " << std::endl;
        std::vector<std::string> presets = get_presets();
        for (auto preset : presets) {
            std::cerr << "[Dragon] " << "    " << preset << std::endl;
        }
        exit(1);
    }
    std::string compilerArgument = tokens[0];
    std::string lang = tokens[1];
    overrideCompiler = true;
    overrideOutputDir = true;
    overrideSourceDir = true;
    overrideTarget = true;
    if (compilerArgument == "gcc") {
        if (lang == "c") {
            compiler = "gcc";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customUnits.push_back("main.c");
            generate_generic_main("c");
        } else if (lang == "c++" || lang == "cpp" || lang == "cxx") {
            compiler = "g++";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customUnits.push_back("main.cpp");
            generate_generic_main("cpp");
        } else if (lang == "objc") {
            compiler = "gcc";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customFlags.push_back("-framework Foundation");
            customUnits.push_back("main.m");
            generate_generic_main("objc");
        } else {
            std::cerr << "[Dragon] " << "Unsupported language with gcc: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "clang") {
        if (lang == "c") {
            compiler = "clang";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customUnits.push_back("main.c");
            generate_generic_main("c");
        } else if (lang == "c++" || lang == "cpp" || lang == "cxx") {
            compiler = "clang++";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customUnits.push_back("main.cpp");
            generate_generic_main("cpp");
        } else if (lang == "objc") {
            compiler = "clang";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customFlags.push_back("-framework Foundation");
            customUnits.push_back("main.m");
            generate_generic_main("objc");
        } else {
            std::cerr << "[Dragon] " << "Unsupported language with clang: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "tcc") {
        if (lang == "c") {
            compiler = "tcc";
            target = "main";
            sourceDir = "src";
            outputDir = "build";
            customUnits.push_back("main.c");
            generate_generic_main("c");
        } else {
            std::cerr << "[Dragon] " << "Unsupported language with tcc: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "sclc" || compilerArgument == "scale") {
        if (lang == "scale") {
            compiler = "sclc";
            target = "main.scl";
            sourceDir = "src";
            outputDir = "build";
            customUnits.push_back("main.scale");
            generate_generic_main("scale");
        } else {
            std::cerr << "[Dragon] " << "Unsupported language with sclc: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "kotlin") {
        if (lang == "kotlin") {
            compiler = "kotlinc-native";
            target = "main.kexe";
            sourceDir = "src";
            outputDir = "build";
            libraryPathPrefix = "-repo ";
            libraryPrefix = "-library ";
            macroPrefix = DRAGON_UNSUPPORTED_STR;
            includePrefix = DRAGON_UNSUPPORTED_STR;
            overrideMacroPrefix = true;
            overrideLibraryPathPrefix = true;
            overrideLibraryPrefix = true;
            overrideIncludePrefix = true;
            customUnits.push_back("main.kt");
            generate_generic_main("kotlin");
        } else {
            std::cerr << "[Dragon] " << "Unsupported language with kotlin: " << lang << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "[Dragon] " << "Invalid compiler: " << compilerArgument << std::endl;
        exit(1);
    }
    std::cout << "[Dragon] " << "Loaded preset" << std::endl;
}

void cmd_clean(std::string& configFile) {
    bool configExists = std::filesystem::exists(configFile);
    if (!configExists) {
        std::cerr << "[Dragon] " << "Config file not found!" << std::endl;
        std::cerr << "[Dragon] " << "Have you forgot to run 'dragon init'?" << std::endl;
        exit(1);
    }

    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry root = parser.parse(configFile);

    std::string validation = "dragon-v" + std::string(VERSION);
    std::cout << "\x07";
    std::cout << "[Dragon] " << "Warning: This action is not reversible!" << std::endl;
    std::cout << "[Dragon] " << "All files in the directories below will be deleted:" << std::endl;
    std::cout << "[Dragon] " << "    " << root.getStringOrDefault("outputDir", "build").getValue() << std::endl;
    std::cout << "[Dragon] " << "    " << root.getStringOrDefault("sourceDir", "src").getValue() << std::endl;
    std::cout << "[Dragon] " << "Are you sure you want to continue?" << std::endl;
    std::cout << "[Dragon] " << "Please type '" << validation << "' to confirm" << std::endl;
    std::cout << "[Dragon] " << "> ";
    std::string answer;
    std::cin >> answer;

    if (answer != validation) {
        std::cerr << "[Dragon] " << "Invalid validation code! Aborting..." << std::endl;
        return;
    }

    std::cout << "[Dragon] " << "Cleaning..." << std::endl;
    std::string outputDir = root.getStringOrDefault("outputDir", "build").getValue();
    std::string sourceDir = root.getStringOrDefault("sourceDir", "src").getValue();
    std::filesystem::remove_all(outputDir);
    std::filesystem::remove_all(sourceDir);
    std::filesystem::remove("build.drg");
    std::cout << "[Dragon] " << "Cleaned" << std::endl;
}

void handle_signal(int signal) {
    std::cout << "[Dragon] " << "Caught signal " << signal << std::endl;
    std::cout << "[Dragon] " << "Error: " << strerror(errno) << std::endl;
    exit(0);
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

    for (int i = 2; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        if (arg == "-c" || arg == "--buildConfig") {
            if (i + 1 < argc) {
                configFile = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No buildConfig file specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-compiler") {
            overrideCompiler = true;
            if (i + 1 < argc) {
                compiler = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No compiler specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-outputDir") {
            overrideOutputDir = true;
            if (i + 1 < argc) {
                outputDir = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No output directory specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-target") {
            overrideTarget = true;
            if (i + 1 < argc) {
                target = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No output file specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-sourceDir") {
            overrideSourceDir = true;
            if (i + 1 < argc) {
                sourceDir = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No source directory specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-unit") {
            if (i + 1 < argc) {
                customUnits.push_back(std::string(argv[++i]));
            } else {
                std::cerr << "[Dragon] " << "No unit specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-include") {
            if (i + 1 < argc) {
                customIncludes.push_back(std::string(argv[++i]));
            } else {
                std::cerr << "[Dragon] " << "No include specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-lib") {
            if (i + 1 < argc) {
                customLibs.push_back(std::string(argv[++i]));
            } else {
                std::cerr << "[Dragon] " << "No library specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-libraryPath") {
            if (i + 1 < argc) {
                customLibraryPaths.push_back(std::string(argv[++i]));
            } else {
                std::cerr << "[Dragon] " << "No library directory specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-define") {
            if (i + 1 < argc) {
                customDefines.push_back(std::string(argv[++i]));
            } else {
                std::cerr << "[Dragon] " << "No define specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-flag") {
            if (i + 1 < argc) {
                customFlags.push_back(std::string(argv[++i]));
            } else {
                std::cerr << "[Dragon] " << "No flag specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-preset") {
            if (command != "init") {
                std::cerr << "[Dragon] " << "Presets can only be used to initialize a buildConfig file" << std::endl;
                exit(1);
            }
            if (i + 1 < argc) {
                std::string preset = std::string(argv[++i]);
                load_preset(preset);
                cmd_init(configFile);
                return 0;
            } else {
                std::cerr << "[Dragon] " << "No preset specified" << std::endl;
                std::cerr << "[Dragon] " << "Available presets: " << std::endl;
                std::vector<std::string> presets = get_presets();
                for (auto preset : presets) {
                    std::cerr << "[Dragon] " << "    " << preset << std::endl;
                }
                exit(1);
            }
        } else if (arg == "-macroPrefix") {
            overrideMacroPrefix = true;
            if (i + 1 < argc) {
                macroPrefix = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No macro prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-libraryPrefix") {
            overrideLibraryPrefix = true;
            if (i + 1 < argc) {
                libraryPrefix = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No library prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-libraryPathPrefix") {
            overrideLibraryPathPrefix = true;
            if (i + 1 < argc) {
                libraryPathPrefix = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No library path prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-includePrefix") {
            overrideIncludePrefix = true;
            if (i + 1 < argc) {
                includePrefix = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No include prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-outputPrefix") {
            overrideOutFilePrefix = true;
            if (i + 1 < argc) {
                outFilePrefix = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No output prefix specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-conf") {
            if (i + 1 < argc) {
                buildConfigRootEntry = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No build Configuration specified" << std::endl;
                exit(1);
            }
        } else if (arg == "-h" || arg == "--help") {
            usage(argv[0], std::cout);
            return 0;
        } else {
            std::cerr << "[Dragon] " << "Unknown argument: " << arg << std::endl;
            usage(std::string(argv[0]), std::cerr);
            exit(1);
        }
    }

    // TODO: Add `platform` compound to build config
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
        DragonConfig::CompoundEntry root = parser.parse(configFile);
        root.print(std::cout);
    } else if (command == "presets") {
        std::vector<std::string> presets = get_presets();
        std::cout << "Available presets: " << std::endl;
        for (auto preset : presets) {
            std::cout << "  " << preset << std::endl;
        }
    } else {
        std::cerr << "[Dragon] " << "Unknown command: " << command << std::endl;
        usage(std::string(argv[0]), std::cerr);
        exit(1);
    }
    
    return 0;
}
