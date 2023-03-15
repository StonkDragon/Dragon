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
#include <regex>

#ifdef _WIN32
#error "Windows is currently not supported."
#endif

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#define DRAGON_UNSUPPORTED_STR  "<DRAGON_UNSUPPORTED>"

#define DRAGON_LOG              std::cout << "[Dragon] "
#define DRAGON_ERR              std::cerr << "[Dragon] "

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

std::string replaceAll(std::string src, std::string from, std::string to) {
    try {
        return regex_replace(src, std::regex(from), to);
    } catch (std::regex_error& e) {
        return src;
    }
}

std::string cmd_build(std::string& configFile) {
    bool configExists = std::filesystem::exists(configFile);
    if (!configExists) {
        DRAGON_ERR << "Config file not found!" << std::endl;
        DRAGON_ERR << "Have you forgot to run 'dragon init'?" << std::endl;
        exit(1);
    }
    
    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry* root = parser.parse(configFile);
    DragonConfig::CompoundEntry* buildConfig = root->getCompound(buildConfigRootEntry);

    if (!buildConfig->getList("units") || buildConfig->getList("units")->size() == 0) {
        DRAGON_ERR << "No compilation units defined!" << std::endl;
        exit(1);
    }

    if (overrideCompiler) {
        buildConfig->setString("compiler", compiler);
    }
    if (overrideOutputDir) {
        buildConfig->setString("outputDir", outputDir);
    }
    if (overrideTarget) {
        buildConfig->setString("target", target);
    }
    if (overrideSourceDir) {
        buildConfig->setString("sourceDir", sourceDir);
    }
    if (overrideMacroPrefix) {
        buildConfig->setString("macroPrefix", macroPrefix);
    }
    if (overrideLibraryPrefix) {
        buildConfig->setString("libraryPrefix", libraryPrefix);
    }
    if (overrideLibraryPathPrefix) {
        buildConfig->setString("libraryPathPrefix", libraryPathPrefix);
    }
    if (overrideIncludePrefix) {
        buildConfig->setString("includePrefix", includePrefix);
    }
    if (overrideOutFilePrefix) {
        buildConfig->setString("outFilePrefix", outFilePrefix);
    }
    std::vector<std::string> cmd;
    cmd.push_back(buildConfig->getStringOrDefault("compiler", "clang")->getValue());
    if (buildConfig->getList("flags")) {
        for (u_long i = 0; i < buildConfig->getList("flags")->size(); i++) {
            cmd.push_back(buildConfig->getList("flags")->getString(i)->getValue());
        }
    }
    for (auto flag : customFlags) {
        cmd.push_back(flag);
    }
    if (buildConfig->getList("defines")) {
        for (u_long i = 0; i < buildConfig->getList("defines")->size(); i++) {
            if (buildConfig->getStringOrDefault("macroPrefix", "-D")->getValue() == DRAGON_UNSUPPORTED_STR) {
                DRAGON_ERR << "Macro prefix not supported by compiler!" << std::endl;
            } else {
                cmd.push_back(
                    buildConfig->getStringOrDefault("macroPrefix", "-D")->getValue() +
                    buildConfig->getList("defines")->getString(i)->getValue()
                );
            }
        }
    }
    for (auto define : customDefines) {
        if (buildConfig->getStringOrDefault("macroPrefix", "-D")->getValue() == DRAGON_UNSUPPORTED_STR) {
            DRAGON_ERR << "Macro prefix not supported by compiler!" << std::endl;
        } else {
            cmd.push_back(
                buildConfig->getStringOrDefault("macroPrefix", "-D")->getValue() +
                define
            );
        }
    }
    if (buildConfig->getList("libraryPaths")) {
        for (u_long i = 0; i < buildConfig->getList("libraryPaths")->size(); i++) {
            cmd.push_back(
                buildConfig->getStringOrDefault("libraryPathPrefix", "-L")->getValue() +
                buildConfig->getList("libraryPaths")->getString(i)->getValue()
            );
        }
    }
    for (auto libDir : customLibraryPaths) {
        cmd.push_back(
            buildConfig->getStringOrDefault("libraryPathPrefix", "-L")->getValue() +
            libDir
        );
    }
    if (buildConfig->getList("includes")) {
        for (u_long i = 0; i < buildConfig->getList("includes")->size(); i++) {
            if (buildConfig->getStringOrDefault("includePrefix", "-I")->getValue() == DRAGON_UNSUPPORTED_STR) {
                DRAGON_ERR << "Include prefix not supported by compiler!" << std::endl;
            } else {
                cmd.push_back(
                    buildConfig->getStringOrDefault("includePrefix", "-I")->getValue() +
                    buildConfig->getList("includes")->getString(i)->getValue()
                );
            }
        }
    }
    for (auto include : customIncludes) {
        if (buildConfig->getStringOrDefault("includePrefix", "-I")->getValue() == DRAGON_UNSUPPORTED_STR) {
            DRAGON_ERR << "Include prefix not supported by compiler!" << std::endl;
        } else {
            cmd.push_back(
                buildConfig->getStringOrDefault("includePrefix", "-I")->getValue() +
                include
            );
        }
    }

    std::string outputFile = buildConfig->getStringOrDefault("outputDir", "build")->getValue();
    outputFile += std::filesystem::path::preferred_separator;
    outputFile += buildConfig->getStringOrDefault("target", "main")->getValue();

    std::string stdlib = buildConfig->getStringOrDefault("std", "")->getValue();

    if (stdlib.size() > 0) {
        cmd.push_back(
            "-std=" +
            stdlib
        );
    }

    bool outDirExists = std::filesystem::exists(buildConfig->getStringOrDefault("outputDir", "build")->getValue());
    if (outDirExists) {
        if (buildConfig->getStringOrDefault("outputDir", "build")->getValue() == ".") {
            DRAGON_ERR << "Cannot build in current directory" << std::endl;
            exit(1);
        }
        std::filesystem::remove_all(buildConfig->getStringOrDefault("outputDir", "build")->getValue());
    }
    try {
        std::filesystem::create_directories(buildConfig->getStringOrDefault("outputDir", "build")->getValue());
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create output directory: " << buildConfig->getStringOrDefault("outputDir", "build")->getValue() << std::endl;
        exit(1);
    }

    bool sourceDirExists = std::filesystem::exists(buildConfig->getStringOrDefault("sourceDir", "src")->getValue());
    if (!sourceDirExists) {
        DRAGON_ERR << "Source directory does not exist: " << buildConfig->getStringOrDefault("sourceDir", "src")->getValue() << std::endl;
        exit(1);
    }

    if (buildConfig->getList("preBuild")) {
        for (u_long i = 0; i < buildConfig->getList("preBuild")->size(); i++) {
            DRAGON_LOG << "Running prebuild command: " << buildConfig->getList("preBuild")->getString(i)->getValue() << std::endl;
            int ret = system(buildConfig->getList("preBuild")->getString(i)->getValue().c_str());
            if (ret != 0) {
                DRAGON_ERR << "Pre-build command failed: " << buildConfig->getList("preBuild")->getString(i)->getValue() << std::endl;
                exit(1);
            }
        }
    }

    size_t unitSize = customUnits.size();

    if (buildConfig->getList("units")) {
        for (u_long i = 0; i < buildConfig->getList("units")->size(); i++) {
            std::string unit = buildConfig->getList("units")->getString(i)->getValue();
            
            cmd.push_back(
                buildConfig->getStringOrDefault("sourceDir", "src")->getValue() +
                std::filesystem::path::preferred_separator +
                unit
            );
        }
    }

    for (size_t i = 0; i < unitSize; i++) {
        std::string unit = customUnits.at(i);
        
        cmd.push_back(
            buildConfig->getStringOrDefault("sourceDir", "src")->getValue() +
            std::filesystem::path::preferred_separator +
            unit
        );
    }
    
    if (buildConfig->getList("libs")) {
        for (u_long i = 0; i < buildConfig->getList("libs")->size(); i++) {
            cmd.push_back(
                buildConfig->getStringOrDefault("libraryPrefix", "-l")->getValue() +
                buildConfig->getList("libs")->getString(i)->getValue()
            );
        }
    }
    for (auto lib : customLibs) {
        cmd.push_back(
            buildConfig->getStringOrDefault("libraryPrefix", "-l")->getValue() +
            lib
        );
    }

    cmd.push_back(buildConfig->getStringOrDefault("outFilePrefix", "-o")->getValue());
    cmd.push_back(outputFile);

    std::string cmdStr = "";
    for (auto&& s : cmd) {
        cmdStr += s + " ";
    }
    DRAGON_LOG << "Running build command: " << cmdStr << std::endl;
#if 1
// compile with execv()
    int pid = fork();
    if (pid == 0) {
        char** argv = (char**) malloc(sizeof(char*) * (cmd.size() + 1));
        int argc = 0;
        for (auto&& s : cmd) {
            s = replaceAll(s, R"(\\")", "\"");
            argv[argc++] = (char*) s.c_str();
        }
        argv[argc] = nullptr;
        int ret = execvp(argv[0], (char* const*) argv);
        DRAGON_ERR << "Could not run: " << std::string(strerror(errno)) << std::endl;
        DRAGON_ERR << "ret was: " << ret << std::endl;
        exit(ret);
    } else if (pid) {
        waitpid(pid, NULL, 0);
    } else {
        DRAGON_ERR << "Failed to fork child process for compilation! Error: " << std::string(strerror(errno)) << std::endl;
        exit(-1);
    }

#else
// compile with system()
    int run = system(cmdStr.c_str());
    if (run != 0) {
        DRAGON_ERR << "Failed to run command: " << cmdStr << std::endl;
        exit(run);
    }
#endif
    if (buildConfig->getList("postBuild")) {
        for (u_long i = 0; i < buildConfig->getList("postBuild")->size(); i++) {
            DRAGON_LOG << "Running postbuild command: " << buildConfig->getList("postBuild")->getString(i)->getValue() << std::endl;
            int ret = system(buildConfig->getList("postBuild")->getString(i)->getValue().c_str());
            if (ret != 0) {
                DRAGON_ERR << "Post-build command failed: " << buildConfig->getList("postBuild")->getString(i)->getValue() << std::endl;
                exit(ret);
            }
        }
    }

    return outputFile;
}

void cmd_init(std::string& configFile) {
    if (std::filesystem::exists(configFile)) {
        DRAGON_LOG << "Config file already exists." << std::endl;
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
    if (overrideOutFilePrefix)
        def << "  outFilePrefix: \"" << outFilePrefix << "\";\n";
    if (overrideLibraryPathPrefix)
        def << "  libraryPathPrefix: \"" << libraryPathPrefix << "\";\n";
    if (overrideLibraryPrefix)
        def << "  libraryPrefix: \"" << libraryPrefix << "\";\n";
    if (overrideMacroPrefix)
        def << "  macroPrefix: \"" << macroPrefix << "\";\n";
    if (overrideIncludePrefix)
        def << "  includePrefix: \"" << includePrefix << "\";\n";

    if (customLibraryPaths.size() > 0) {
        def << "  libraryPaths: [\n";
        for (auto libraryPath : customLibraryPaths) {
            def << "    \"" << libraryPath << "\";\n";
        }
        def << "  ];\n";    
    } else {
    }
    if (customLibs.size() > 0) {
        def << "  libs: [\n";
        for (auto library : customLibs) {
            def << "    \"" << library << "\";\n";
        }
        def << "  ];\n";    
    } else {
    }
    if (customDefines.size() > 0) {
        def << "  defines: [\n";
        for (auto define : customDefines) {
            def << "    \"" << define << "\";\n";
        }
        def << "  ];\n";    
    } else {
    }
    if (customFlags.size() > 0) {
        def << "  flags: [\n";
        for (auto flag : customFlags) {
            def << "    \"" << flag << "\";\n";
        }
        def << "  ];\n";    
    } else {
    }
    if (customPreBuilds.size() > 0) {
        def << "  preBuild: [\n";
        for (auto preBuild : customPreBuilds) {
            def << "    \"" << preBuild << "\";\n";
        }
        def << "  ];\n";    
    } else {
    }
    if (customPostBuilds.size() > 0) {
        def << "  postBuild: [\n";
        for (auto postBuild : customPostBuilds) {
            def << "    \"" << postBuild << "\";\n";
        }
        def << "  ];\n";    
    } else {
    }
    def << "};" << std::endl;

    def.close();
    DRAGON_LOG << "Config file created at " << configFile << std::endl;
}

void cmd_run(std::string& configFile) {
    std::string outfile = cmd_build(configFile);
    std::string cmd = outfile;

    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry* root = parser.parse(configFile);
    DragonConfig::CompoundEntry* run = root->getCompound("run");
    
    std::vector<std::string> argv;
    std::vector<std::string> envs;
    argv.push_back(cmd);
    if (run) {
        DragonConfig::ListEntry* args = run->getList("args");
        DragonConfig::ListEntry* env = run->getList("env");
        if (args) {
            for (u_long i = 0; i < args->size(); i++) {
                argv.push_back(args->getString(i)->getValue());
            }
        }
        if (env) {
            for (u_long i = 0; i < env->size(); i++) {
                envs.push_back(env->getString(i)->getValue());
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

    DRAGON_LOG << "Running " << cmd << std::endl;

    int child = fork();
    if (child == 0) {
        int ret = execve(cmd.c_str(), argv_c, envs_c);
        if (ret == -1) {
            DRAGON_LOG << "Error: " << strerror(errno) << std::endl;
            exit(1);
        }
    } else if (child > 0) {
        wait(NULL);
        DRAGON_LOG << "Finished!" << std::endl;
    } else {
        DRAGON_ERR << "Error forking a child process!" << std::endl;
        DRAGON_ERR << "Error: " << strerror(errno) << std::endl;
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
    std::filesystem::create_directories(sourceDir);
    if (lang == "c") {
        std::ofstream mainFile(sourceDir + std::filesystem::path::preferred_separator + "main.c");
        mainFile << "#include <stdio.h>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    printf(\"Hello, World!\\n\");\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "cpp") {
        std::ofstream mainFile(sourceDir + std::filesystem::path::preferred_separator + "main.cpp");
        mainFile << "#include <iostream>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    std::cout << \"Hello, World!\" << std::endl;\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "scale") {
        std::ofstream mainFile(sourceDir + std::filesystem::path::preferred_separator + "main.scale");
        mainFile << "function main(): none\n";
        mainFile << "    \"Hello, World!\" puts\n";
        mainFile << "end" << std::endl;
        mainFile.close();
    } else if (lang == "objc") {
        std::ofstream mainFile(sourceDir + std::filesystem::path::preferred_separator + "main.m");
        mainFile << "#import <Foundation/Foundation.h>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    @autoreleasepool {\n";
        mainFile << "        NSLog(@\"Hello, World!\");\n";
        mainFile << "    }\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "swift") {
        std::ofstream mainFile(sourceDir + std::filesystem::path::preferred_separator + "main.swift");
        mainFile << "print(\"Hello, World!\")" << std::endl;
        mainFile.close();
    } else {
        DRAGON_ERR << "Unknown language: " << lang << std::endl;
        exit(1);
    }
}

std::vector<std::string> get_presets() {
    std::vector<std::string> presets;
    presets.push_back("clang-c");
    presets.push_back("gcc-c");
    presets.push_back("tcc-c");
    presets.push_back("clang-cpp");
    presets.push_back("gcc-cpp");
    presets.push_back("sclc-scale");
    presets.push_back("clang-objc");
    presets.push_back("gcc-objc");
    presets.push_back("swiftc-swift");
    return presets;
}

void load_preset(std::string& identifier) {
    DRAGON_LOG << "Loading preset: " << identifier << std::endl;
    std::vector<std::string> tokens = split(identifier, '-');
    if (tokens.size() != 2) {
        DRAGON_ERR << "Invalid preset identifier: " << identifier << std::endl;
        DRAGON_ERR << "Expected format: <compiler>-<lang>" << std::endl;
        DRAGON_ERR << "Available presets: " << std::endl;
        std::vector<std::string> presets = get_presets();
        for (auto preset : presets) {
            DRAGON_ERR << "    " << preset << std::endl;
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
        compiler = "gcc";
        target = "main";
        sourceDir = "src";
        outputDir = "build";
        if (lang == "c") {
            customUnits.push_back("main.c");
            generate_generic_main("c");
        } else if (lang == "c++" || lang == "cpp" || lang == "cxx") {
            compiler = "g++";
            customUnits.push_back("main.cpp");
            generate_generic_main("cpp");
        } else if (lang == "objc") {
            customFlags.push_back("-framework Foundation");
            customUnits.push_back("main.m");
            generate_generic_main("objc");
        } else {
            DRAGON_ERR << "Unsupported language with gcc: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "clang") {
        compiler = "clang";
        target = "main";
        sourceDir = "src";
        outputDir = "build";
        if (lang == "c") {
            customUnits.push_back("main.c");
            generate_generic_main("c");
        } else if (lang == "c++" || lang == "cpp" || lang == "cxx") {
            compiler += "++";
            customUnits.push_back("main.cpp");
            generate_generic_main("cpp");
        } else if (lang == "objc") {
            customFlags.push_back("-framework Foundation");
            customUnits.push_back("main.m");
            generate_generic_main("objc");
        } else {
            DRAGON_ERR << "Unsupported language with clang: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "tcc") {
        compiler = "tcc";
        if (lang == "c") {
            customUnits.push_back("main.c");
            generate_generic_main("c");
        } else {
            DRAGON_ERR << "Unsupported language with tcc: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "sclc") {
        compiler = "sclc";
        if (lang == "scale") {
            target += ".scl";
            customUnits.push_back("main.scale");
            generate_generic_main("scale");
        } else {
            DRAGON_ERR << "Unsupported language with sclc: " << lang << std::endl;
            exit(1);
        }
    } else if (compilerArgument == "swiftc") {
        compiler = "swiftc";
        if (lang == "swift") {
            customUnits.push_back("main.swift");
            generate_generic_main("swift");
        } else {
            DRAGON_ERR << "Unsupported language with swiftc: " << lang << std::endl;
            exit(1);
        }
    } else {
        DRAGON_ERR << "Invalid compiler: " << compilerArgument << std::endl;
        exit(1);
    }
    DRAGON_LOG << "Loaded preset" << std::endl;
}

void cmd_clean(std::string& configFile) {
    bool configExists = std::filesystem::exists(configFile);
    if (!configExists) {
        DRAGON_ERR << "Config file not found!" << std::endl;
        DRAGON_ERR << "Have you forgot to run 'dragon init'?" << std::endl;
        exit(1);
    }

    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry* root = parser.parse(configFile);

    std::string validation = std::filesystem::current_path().filename().string();
    std::cout << "\x07";
    DRAGON_LOG << "Warning: This action is not reversible!" << std::endl;
    DRAGON_LOG << "All files in the directories below will be deleted:" << std::endl;
    DRAGON_LOG << "    " << root->getStringOrDefault("outputDir", "build")->getValue() << std::endl;
    DRAGON_LOG << "    " << root->getStringOrDefault("sourceDir", "src")->getValue() << std::endl;
    DRAGON_LOG << "Are you sure you want to continue?" << std::endl;
    DRAGON_LOG << "Please type '" << validation << "' to confirm" << std::endl;
    DRAGON_LOG << "> ";
    std::string answer;
    std::cin >> answer;

    if (answer != validation) {
        DRAGON_ERR << "Invalid validation code! Aborting..." << std::endl;
        return;
    }

    DRAGON_LOG << "Cleaning..." << std::endl;
    std::string outputDir = root->getStringOrDefault("outputDir", "build")->getValue();
    std::string sourceDir = root->getStringOrDefault("sourceDir", "src")->getValue();
    std::filesystem::remove_all(outputDir);
    std::filesystem::remove_all(sourceDir);
    std::filesystem::remove("build.drg");
    DRAGON_LOG << "Cleaned" << std::endl;
}

#if __has_attribute(noreturn)
__attribute__((noreturn))
#endif
void handle_signal(int signal) {
    DRAGON_LOG << "Caught signal " << signal << std::endl;
    if (errno)
        DRAGON_LOG << "Error: " << strerror(errno) << std::endl;
    exit(signal);
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
        } else if (arg == "-h" || arg == "--help") {
            usage(argv[0], std::cout);
            return 0;
        } else {
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
        root->print(std::cout);
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
