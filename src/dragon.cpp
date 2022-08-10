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

namespace Dragon
{
    struct Build {
        std::string compiler;
        std::string target;
        std::string outputDir;
        std::string sourceDir;
        std::vector<std::string> units;
        std::vector<std::string> flags;
        std::vector<std::string> includes;
        std::vector<std::string> libs;
        std::vector<std::string> libraryPaths;
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
                    } else if (key == "compiler") {
                        build.compiler = values.at(0);
                    } else if (key == "target") {
                        build.target = values.at(0);
                    } else if (key == "outputDir") {
                        build.outputDir = values.at(0);
                    } else if (key == "sourceDir") {
                        build.sourceDir = values.at(0);
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
    sink << "  help      Show this help" << std::endl;
    sink << "  version   Show the version" << std::endl;
    sink << std::endl;
    sink << "Options:" << std::endl;
    sink << "  -c, --config <path>    Path to config file" << std::endl;
    sink << "  -compiler <compiler>   Override compiler" << std::endl;
    sink << "  -outputDir <dir>       Override output directory" << std::endl;
    sink << "  -target <name>         Override output file" << std::endl;
    sink << "  -sourceDir <dir>       Override source directory" << std::endl;
    sink << "  -unit <unit>           Add a unit to the build" << std::endl;
    sink << "  -include <dir>         Add an include directory to the build" << std::endl;
    sink << "  -lib <lib>             Add a library to the build" << std::endl;
    sink << "  -libraryPath <path>    Add a library path to the build" << std::endl;
    sink << "  -define <define>       Add a define to the build" << std::endl;
    sink << "  -flag <flag>           Add a flag to the build" << std::endl;
}

bool overrideCompiler = false;
bool overrideOutputDir = false;
bool overrideTarget = false;
bool overrideSourceDir = false;

std::string compiler = "gcc";
std::string outputDir = "build";
std::string target = "main";
std::string sourceDir = "src";
std::vector<std::string> customUnits;
std::vector<std::string> customIncludes;
std::vector<std::string> customLibs;
std::vector<std::string> customLibraryPaths;
std::vector<std::string> customDefines;
std::vector<std::string> customFlags;

std::string cmd_build(std::string& configFile) {
    bool configExists = std::__fs::filesystem::exists(configFile);
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
        cmd += "-D";
        cmd += define;
        cmd += " ";
    }
    for (auto libDir : buildConfig.libraryPaths) {
        // TODO: Make this prefix customizable
        cmd += "-L";
        cmd += libDir;
        cmd += " ";
    }
    for (auto libDir : customLibraryPaths) {
        // TODO: Make this prefix customizable
        cmd += "-L";
        cmd += libDir;
        cmd += " ";
    }
    for (auto lib : buildConfig.libs) {
        // TODO: Make this prefix customizable
        cmd += "-l";
        cmd += lib;
        cmd += " ";
    }
    for (auto lib : customLibs) {
        // TODO: Make this prefix customizable
        cmd += "-l";
        cmd += lib;
        cmd += " ";
    }
    for (auto include : buildConfig.includes) {
        // TODO: Make this prefix customizable
        cmd += "-I";
        cmd += include;
        cmd += " ";
    }
    for (auto include : customIncludes) {
        // TODO: Make this prefix customizable
        cmd += "-I";
        cmd += include;
        cmd += " ";
    }
    for (auto unit : buildConfig.units) {
        cmd += buildConfig.sourceDir;
        cmd += std::__fs::filesystem::path::preferred_separator;
        cmd += unit;
        cmd += " ";
    }
    for (auto unit : customUnits) {
        cmd += buildConfig.sourceDir;
        cmd += std::__fs::filesystem::path::preferred_separator;
        cmd += unit;
        cmd += " ";
    }
    std::string outputFile = buildConfig.outputDir;
    outputFile += std::__fs::filesystem::path::preferred_separator;
    outputFile += buildConfig.target;

    // TODO: Make this customizable
    cmd += "-o ";
    cmd += outputFile;
    cmd += " ";

    bool outDirExists = std::__fs::filesystem::exists(buildConfig.outputDir);
    if (outDirExists) {
        if (buildConfig.outputDir == ".") {
            std::cerr << "[Dragon] " << "Cannot build in current directory" << std::endl;
            exit(1);
        }
        std::__fs::filesystem::remove_all(buildConfig.outputDir);
    }
    try {
        std::__fs::filesystem::create_directories(buildConfig.outputDir);
    } catch (std::__fs::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create output directory: " << buildConfig.outputDir << std::endl;
        exit(1);
    }

    bool sourceDirExists = std::__fs::filesystem::exists(buildConfig.sourceDir);
    if (!sourceDirExists) {
        std::cerr << "[Dragon] " << "Source directory does not exist: " << buildConfig.sourceDir << std::endl;
        exit(1);
    }
    std::string cmdStr = cmd;
    std::cout << "[Dragon] " << cmdStr << std::endl;
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
    return outputFile;
}

void cmd_init(std::string& configFile) {
    if (std::__fs::filesystem::exists(configFile)) {
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
    config.close();
    std::cout << "[Dragon] " << "Config file created at " << configFile << std::endl;
}

void cmd_run(std::string& configFile) {
    std::string outfile = cmd_build(configFile);
    std::string cmd = "./" + outfile;
    std::cout << "[Dragon] " << "Running " << cmd << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    int ret = system(cmd.c_str());
    auto end = std::chrono::high_resolution_clock::now();
    double runtimeMillis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (ret != 0) {
        std::cerr << "[Dragon] " << "Failed to run " << cmd << std::endl;
        exit(1);
    }
    std::cout << "[Dragon] " << "Finished in " << (runtimeMillis / 1000.0) << " seconds" << std::endl;
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
        std::__fs::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.c");
        mainFile << "#include <stdio.h>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    printf(\"Hello, World!\\n\");\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "cpp") {
        std::__fs::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.cpp");
        mainFile << "#include <iostream>\n\n";
        mainFile << "int main() {\n";
        mainFile << "    std::cout << \"Hello, World!\" << std::endl;\n";
        mainFile << "    return 0;\n";
        mainFile << "}" << std::endl;
        mainFile.close();
    } else if (lang == "scale") {
        std::__fs::filesystem::create_directories(sourceDir);
        std::ofstream mainFile(sourceDir + "/main.cpp");
        mainFile << "#include \"core.scale\"\n\n";
        mainFile << "function main()\n";
        mainFile << "    \"Hello, World!\" puts\n";
        mainFile << "end" << std::endl;
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
    return presets;
}

void load_preset(std::string& identifier) {
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
    } else {
        std::cerr << "[Dragon] " << "Invalid compiler: " << compilerArgument << std::endl;
        exit(1);
    }
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
    } else {
        std::cerr << "[Dragon] " << "Unknown command: " << command << std::endl;
        usage(std::string(argv[0]), std::cerr);
        exit(1);
    }
    
    return 0;
}
