#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#define DRAGON_UNSUPPORTED_STR "<DRAGON_UNSUPPORTED>"

#ifdef __APPLE__
#define NS_FS_PREF std::__fs
#else
#define NS_FS_PREF std
#endif

namespace Dragon
{
    struct Build {
        std::string compiler;
        std::string target;
        std::string outputDir;
        std::string sourceDir;
        std::string macroPrefix;
        std::string libraryPrefix;
        std::string libraryPathPrefix;
        std::string includePrefix;
        std::string outFilePrefix;
        std::vector<std::string> units;
        std::vector<std::string> flags;
        std::vector<std::string> includes;
        std::vector<std::string> libs;
        std::vector<std::string> libraryPaths;
        std::vector<std::string> preBuild;
        std::vector<std::string> postBuild;
    };

    struct Config {
        static Build parse(const std::string& config);
    };

    Build Config::parse(const std::string& config) {
        int configSize = config.size();
        bool inStr = false;
        std::string data;
        for (int i = 0; i < configSize; ++i) {
            if (!inStr && config.at(i) == ' ') continue;
            if (config.at(i) == '"' && config.at(i - 1) != '\\') {
                inStr = !inStr;
                continue;
            }
            if (config.at(i) == '\n') {
                inStr = false;
                continue;
            }
            char c = config.at(i);
            data += c;
        }
        int dataSize = data.size();
        std::string str;
        Build build;

        for (int i = 0; i < dataSize; i++) {
            char c = data.at(i);

            if (c == ';') {
                str.clear();
                continue;
            }

            if (c == ':') {
                std::string key = str;
                str.clear();
                c = data.at(++i);
                if (c == '[') {
                    std::vector<std::string> values;
                    c = data.at(++i);
                    while (c != ']') {
                        std::string next;
                        while (c != ';') {
                            next += c;
                            c = data.at(++i);
                            if (c == ';') {
                                values.push_back(next);
                                break;
                            }
                        }
                        c = data.at(++i);
                    }
                    if (key == "units") {
                        for (auto& value : values) {
                            build.units.push_back(value);
                        }
                    } else if (key == "flags") {
                        for (auto& value : values) {
                            build.flags.push_back(value);
                        }
                    } else if (key == "includes") {
                        for (auto& value : values) {
                            build.includes.push_back(value);
                        }
                    } else if (key == "libs") {
                        for (auto& value : values) {
                            build.libs.push_back(value);
                        }
                    } else if (key == "defines") {
                        for (auto& value : values) {
                            build.flags.push_back("-D" + value);
                        }
                    } else if (key == "libraryPaths") {
                        for (auto& value : values) {
                            build.libraryPaths.push_back(value);
                        }
                    } else if (key == "preBuild") {
                        for (auto& value : values) {
                            build.preBuild.push_back(value);
                        }
                    } else if (key == "postBuild") {
                        for (auto& value : values) {
                            build.postBuild.push_back(value);
                        }
                        
                    } else if (key == "compiler") {
                        build.compiler = values.at(0);
                    } else if (key == "target") {
                        build.target = values.at(0);
                    } else if (key == "outputDir") {
                        build.outputDir = values.at(0);
                    } else if (key == "sourceDir") {
                        build.sourceDir = values.at(0);
                    } else if (key == "macroPrefix") {
                        build.macroPrefix = values.at(0);
                    } else if (key == "libraryPrefix") {
                        build.libraryPrefix = values.at(0);
                    } else if (key == "libraryPathPrefix") {
                        build.libraryPathPrefix = values.at(0);
                    } else if (key == "includePrefix") {
                        build.includePrefix = values.at(0);
                    } else if (key == "outFilePrefix") {
                        build.outFilePrefix = values.at(0);
                    } else {
                        std::cerr << "[Dragon] " << "Unknown list: " << key << std::endl;
                        exit(1);
                    }
                } else {
                    std::string value = "";
                    while (c != ';') {
                        value += c;
                        c = data.at(++i);
                    }
                    if (key == "compiler") {
                        build.compiler = value;
                    } else if (key == "target") {
                        build.target = value;
                    } else if (key == "outputDir") {
                        build.outputDir = value;
                    } else if (key == "target") {
                        build.target = value;
                    } else if (key == "sourceDir") {
                        build.sourceDir = value;
                    } else if (key == "macroPrefix") {
                        build.macroPrefix = value;
                    } else if (key == "libraryPrefix") {
                        build.libraryPrefix = value;
                    } else if (key == "libraryPathPrefix") {
                        build.libraryPathPrefix = value;
                    } else if (key == "includePrefix") {
                        build.includePrefix = value;
                    } else if (key == "outFilePrefix") {
                        build.outFilePrefix = value;
                    } else {
                        std::cerr << "[Dragon] " << "Unknown key: " << key << std::endl;
                        exit(1);
                    }
                }
                str.clear();
                continue;
            }

            str += c;
        }

        return build;
    }

} // namespace Dragon

void usage(std::string progName, std::__1::ostream& sink) {
    sink << "Usage: " << progName << " <command> [options]" << std::endl;
    sink << "Commands:" << std::endl;
    sink << "  build     Build the project" << std::endl;
    sink << "  run       Compile and run the project" << std::endl;
    sink << "  init      Initialize a new project. init allows overriding of default values for faster configuration" << std::endl;
    sink << "  clean     Clean all project files" << std::endl;
    sink << "  help      Show this help" << std::endl;
    sink << "  version   Show the version" << std::endl;
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

std::string cmd_build(std::string& configFile) {
    bool configExists = NS_FS_PREF::filesystem::exists(configFile);
    if (!configExists) {
        std::cerr << "[Dragon] " << "Config file not found!" << std::endl;
        std::cerr << "[Dragon] " << "Have you forgot to run 'dragon init'?" << std::endl;
        exit(1);
    }

    FILE* fp = fopen(configFile.c_str(), "r");
    if (!fp) {
        std::cerr << "[Dragon] " << "Failed to open config file: " << configFile << std::endl;
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    std::string config(buf);
    delete[] buf;
    Dragon::Build buildConfig = Dragon::Config::parse(config);
    
    if (buildConfig.units.size() == 0) {
        std::cerr << "[Dragon] " << "No compilation units defined!" << std::endl;
        exit(1);
    }

    if (overrideCompiler) {
        buildConfig.compiler = compiler;
    }
    if (overrideOutputDir) {
        buildConfig.outputDir = outputDir;
    }
    if (overrideTarget) {
        buildConfig.target = target;
    }
    if (overrideSourceDir) {
        buildConfig.sourceDir = sourceDir;
    }
    if (overrideMacroPrefix) {
        buildConfig.macroPrefix = macroPrefix;
    }
    if (overrideLibraryPrefix) {
        buildConfig.libraryPrefix = libraryPrefix;
    }
    if (overrideLibraryPathPrefix) {
        buildConfig.libraryPathPrefix = libraryPathPrefix;
    }
    if (overrideIncludePrefix) {
        buildConfig.includePrefix = includePrefix;
    }
    if (overrideOutFilePrefix) {
        buildConfig.outFilePrefix = outFilePrefix;
    }
    if (buildConfig.macroPrefix.size() == 0) {
        buildConfig.macroPrefix = "-D";
    }
    if (buildConfig.libraryPrefix.size() == 0) {
        buildConfig.libraryPrefix = "-l";
    }
    if (buildConfig.libraryPathPrefix.size() == 0) {
        buildConfig.libraryPathPrefix = "-L";
    }
    if (buildConfig.includePrefix.size() == 0) {
        buildConfig.includePrefix = "-I";
    }
    if (buildConfig.outFilePrefix.size() == 0) {
        buildConfig.outFilePrefix = "-o";
    }
    std::string cmd = buildConfig.compiler;
    cmd += " ";
    for (auto flag : buildConfig.flags) {
        cmd += flag;
        cmd += " ";
    }
    for (auto flag : customFlags) {
        cmd += flag;
        cmd += " ";
    }
    for (auto define : customDefines) {
        if (buildConfig.macroPrefix == DRAGON_UNSUPPORTED_STR) {
            std::cerr << "[Dragon] " << "Macro prefix not supported by compiler!" << std::endl;
        } else {
            cmd += buildConfig.macroPrefix;
            cmd += define;
            cmd += " ";
        }
    }
    for (auto libDir : buildConfig.libraryPaths) {
        cmd += buildConfig.libraryPathPrefix;
        cmd += libDir;
        cmd += " ";
    }
    for (auto libDir : customLibraryPaths) {
        cmd += buildConfig.libraryPathPrefix;
        cmd += libDir;
        cmd += " ";
    }
    for (auto lib : buildConfig.libs) {
        cmd += buildConfig.libraryPrefix;
        cmd += lib;
        cmd += " ";
    }
    for (auto lib : customLibs) {
        cmd += buildConfig.libraryPrefix;
        cmd += lib;
        cmd += " ";
    }
    for (auto include : buildConfig.includes) {
        if (buildConfig.includePrefix == DRAGON_UNSUPPORTED_STR) {
            std::cerr << "[Dragon] " << "Include prefix not supported by compiler!" << std::endl;
        } else {
            cmd += buildConfig.includePrefix;
            cmd += include;
            cmd += " ";
        }
    }
    for (auto include : customIncludes) {
        if (buildConfig.includePrefix == DRAGON_UNSUPPORTED_STR) {
            std::cerr << "[Dragon] " << "Include prefix not supported by compiler!" << std::endl;
        } else {
            cmd += buildConfig.includePrefix;
            cmd += include;
            cmd += " ";
        }
    }
    for (auto unit : buildConfig.units) {
        cmd += buildConfig.sourceDir;
        cmd += NS_FS_PREF::filesystem::path::preferred_separator;
        cmd += unit;
        cmd += " ";
    }
    for (auto unit : customUnits) {
        cmd += buildConfig.sourceDir;
        cmd += NS_FS_PREF::filesystem::path::preferred_separator;
        cmd += unit;
        cmd += " ";
    }
    std::string outputFile = buildConfig.outputDir;
    outputFile += NS_FS_PREF::filesystem::path::preferred_separator;
    outputFile += buildConfig.target;

    // TODO: Make this customizable
    cmd += buildConfig.outFilePrefix;
    cmd += " ";
    cmd += outputFile;
    cmd += " ";

    bool outDirExists = NS_FS_PREF::filesystem::exists(buildConfig.outputDir);
    if (outDirExists) {
        if (buildConfig.outputDir == ".") {
            std::cerr << "[Dragon] " << "Cannot build in current directory" << std::endl;
            exit(1);
        }
        NS_FS_PREF::filesystem::remove_all(buildConfig.outputDir);
    }
    try {
        NS_FS_PREF::filesystem::create_directories(buildConfig.outputDir);
    } catch (NS_FS_PREF::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create output directory: " << buildConfig.outputDir << std::endl;
        exit(1);
    }

    bool sourceDirExists = NS_FS_PREF::filesystem::exists(buildConfig.sourceDir);
    if (!sourceDirExists) {
        std::cerr << "[Dragon] " << "Source directory does not exist: " << buildConfig.sourceDir << std::endl;
        exit(1);
    }
    std::string cmdStr = cmd;
    std::cout << "[Dragon] " << cmdStr << std::endl;
    for (auto preBuild : buildConfig.preBuild) {
        std::cout << "[Dragon] Running Prebuild command: " << preBuild << std::endl;
        int ret = system(preBuild.c_str());
        if (ret != 0) {
            std::cerr << "[Dragon] " << "Pre-build command failed: " << preBuild << std::endl;
            exit(1);
        }
    }
    FILE* cmdPipe = popen(cmdStr.c_str(), "r");
    if (!cmdPipe) {
        std::cerr << "[Dragon] " << "Failed to run command: " << cmdStr << std::endl;
        exit(1);
    }
    char buf2[1024];
    while (fgets(buf2, 1024, cmdPipe)) {
        std::cout << buf2;
    }
    pclose(cmdPipe);
    for (auto postBuild : buildConfig.postBuild) {
        std::cout << "[Dragon] Running Postbuild command: " << postBuild << std::endl;
        int ret = system(postBuild.c_str());
        if (ret != 0) {
            std::cerr << "[Dragon] " << "Post-build command failed: " << postBuild << std::endl;
            exit(1);
        }
    }
    return outputFile;
}

void cmd_init(std::string& configFile) {
    if (NS_FS_PREF::filesystem::exists(configFile)) {
        std::cout << "[Dragon] " << "Config file already exists." << std::endl;
        return;
    }
    std::ofstream config(configFile);
    if (overrideCompiler) {
        config << "compiler: \"" << compiler << "\";" << std::endl;
    } else {
        config << "compiler: \"gcc\";" << std::endl;
    }
    if (overrideOutputDir) {
        config << "outputDir: \"" << outputDir << "\";" << std::endl;
    } else {
        config << "outputDir: \"build\";" << std::endl;
    }
    if (overrideTarget) {
        config << "target: \"" << target << "\";" << std::endl;
    } else {
        config << "target: \"main\";" << std::endl;
    }
    if (overrideSourceDir) {
        config << "sourceDir: \"" << sourceDir << "\";" << std::endl;
    } else {
        config << "sourceDir: \"src\";" << std::endl;
    }
    if (customUnits.size() == 0) {
        config << "units: [];" << std::endl;
    } else {
        config << "units: [" << std::endl;
        for (auto unit : customUnits) {
            config << "    \"" << unit << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    if (customFlags.size() == 0) {
        config << "flags: [];" << std::endl;
    } else {
        config << "flags: [" << std::endl;
        for (auto flag : customFlags) {
            config << "    \"" << flag << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    config << "includes: [" << std::endl;
    if (overrideSourceDir) {
        config << "    \"" << sourceDir << "\";" << std::endl;
    } else {
        config << "    \"src\";" << std::endl;
    }
    for (auto include : customIncludes) {
        config << "    \"" << include << "\";" << std::endl;
    }
    config << "];" << std::endl;
    if (customLibs.size() == 0) {
        config << "libs: [];" << std::endl;
    } else {
        config << "libs: [" << std::endl;
        for (auto lib : customLibs) {
            config << "    \"" << lib << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    if (customLibraryPaths.size() == 0) {
        config << "libraryPaths: [];" << std::endl;
    } else {
        config << "libraryPaths: [" << std::endl;
        for (auto libDir : customLibraryPaths) {
            config << "    \"" << libDir << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    if (customDefines.size() == 0) {
        config << "defines: [];" << std::endl;
    } else {
        config << "defines: [" << std::endl;
        for (auto define : customDefines) {
            config << "    \"" << define << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    if (customPreBuilds.size() == 0) {
        config << "preBuild: [];" << std::endl;
    } else {
        config << "preBuild: [" << std::endl;
        for (auto preBuild : customPreBuilds) {
            config << "    \"" << preBuild << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    if (customPostBuilds.size() == 0) {
        config << "postBuild: [];" << std::endl;
    } else {
        config << "postBuild: [" << std::endl;
        for (auto postBuild : customPostBuilds) {
            config << "    \"" << postBuild << "\";" << std::endl;
        }
        config << "];" << std::endl;
    }
    config << "macroPrefix: \"" << macroPrefix << "\";" << std::endl;
    config << "libraryPrefix: \"" << libraryPrefix << "\";" << std::endl;
    config << "libraryPathPrefix: \"" << libraryPathPrefix << "\";" << std::endl;
    config << "includePrefix: \"" << includePrefix << "\";" << std::endl;
    config << "outFilePrefix: \"" << outFilePrefix << "\";" << std::endl;
    config.close();
    std::cout << "[Dragon] " << "Config file created at " << configFile << std::endl;
}

void cmd_run(std::string& configFile) {
    std::string outfile = cmd_build(configFile);
    std::string cmd = "./" + outfile;
    std::cout << "[Dragon] " << "Running " << cmd << std::endl;
    int ret = system(cmd.c_str());
    std::cout << "[Dragon] " << "Finished with exit code " << ret << std::endl;
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
        NS_FS_PREF::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.c");
        mainFile << "#include <stdio.h>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    printf(\"Hello, World!\\n\");\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "cpp") {
        NS_FS_PREF::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.cpp");
        mainFile << "#include <iostream>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    std::cout << \"Hello, World!\" << std::endl;\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "scale") {
        NS_FS_PREF::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.scale");
        mainFile << "#include \"core.scale\"\n\n";
        mainFile << "function main()\n";
        mainFile << "    \"Hello, World!\" puts\n";
        mainFile << "end" << std::endl;
        mainFile.close();
    } else if (lang == "kotlin") {
        NS_FS_PREF::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.kt");
        mainFile << "fun main() {\n";
        mainFile << "    println(\"Hello, World!\")\n";
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
    presets.push_back("kotlinc-kotlin");
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
    bool configExists = NS_FS_PREF::filesystem::exists(configFile);
    if (!configExists) {
        std::cerr << "[Dragon] " << "Config file not found!" << std::endl;
        std::cerr << "[Dragon] " << "Have you forgot to run 'dragon init'?" << std::endl;
        exit(1);
    }

    FILE* fp = fopen(configFile.c_str(), "r");
    if (!fp) {
        std::cerr << "[Dragon] " << "Failed to open config file: " << configFile << std::endl;
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    std::string config(buf);
    delete[] buf;
    Dragon::Build buildConfig = Dragon::Config::parse(config);

    std::string validation = "dragon-v" + std::string(VERSION);
    std::cout << "\x07";
    std::cout << "[Dragon] " << "Warning: This action is not reversible!" << std::endl;
    std::cout << "[Dragon] " << "All files in the directories below will be deleted:" << std::endl;
    std::cout << "[Dragon] " << "    " << buildConfig.outputDir << std::endl;
    std::cout << "[Dragon] " << "    " << buildConfig.sourceDir << std::endl;
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
    std::string outputDir = buildConfig.outputDir;
    std::string sourceDir = buildConfig.sourceDir;
    NS_FS_PREF::filesystem::remove_all(outputDir);
    NS_FS_PREF::filesystem::remove_all(sourceDir);
    NS_FS_PREF::filesystem::remove("build.drg");
    std::cout << "[Dragon] " << "Cleaned" << std::endl;
}

int main(int argc, const char* argv[])
{
    std::string configFile = "build.drg";

    if (argc < 2) {
        usage(argv[0], std::cout);
        return 0;
    }

    std::string command = argv[1];

    for (int i = 2; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configFile = std::string(argv[++i]);
            } else {
                std::cerr << "[Dragon] " << "No config file specified" << std::endl;
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
                std::cerr << "[Dragon] " << "Presets can only be used to initialize a config file" << std::endl;
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
        } else if (arg == "-h" || arg == "--help") {
            usage(argv[0], std::cout);
            return 0;
        } else {
            std::cerr << "[Dragon] " << "Unknown argument: " << arg << std::endl;
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
    } else {
        std::cerr << "[Dragon] " << "Unknown command: " << command << std::endl;
        usage(std::string(argv[0]), std::cerr);
        exit(1);
    }
    
    return 0;
}
