#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
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
        std::vector<std::string> libDirs;
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
                    } else if (key == "otherFlags") {
                        for (auto& value : values) {
                            build.flags.push_back(value);
                        }
                    } else if (key == "libraryPaths") {
                        for (auto& value : values) {
                            build.libDirs.push_back(value);
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
    sink << "  init      Initialize a new project" << std::endl;
    sink << "  help      Show this help" << std::endl;
    sink << "  version   Show the version" << std::endl;
    sink << std::endl;
    sink << "Options:" << std::endl;
    sink << "  -c, --config <path>    Path to config file" << std::endl;
    sink << "  -compiler <compiler>   Override compiler" << std::endl;
    sink << "  -outputDir <dir>       Override output directory" << std::endl;
    sink << "  -target <name>         Override output file" << std::endl;
    sink << "  -sourceDir <dir>       Override source directory" << std::endl;
}

bool overrideCompiler = false;
bool overrideOutputDir = false;
bool overrideTarget = false;
bool overrideSourceDir = false;

std::string compiler = "gcc";
std::string outputDir = "build";
std::string target = "main";
std::string sourceDir = "src";

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
    for (auto libDir : buildConfig.libDirs) {
        cmd += "-L";
        cmd += libDir;
        cmd += " ";
    }
    for (auto lib : buildConfig.libs) {
        cmd += "-l";
        cmd += lib;
        cmd += " ";
    }
    for (auto include : buildConfig.includes) {
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
    std::string outputFile = buildConfig.outputDir;
    outputFile += std::__fs::filesystem::path::preferred_separator;
    outputFile += buildConfig.target;

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
    config << "compiler: \"gcc\";" << std::endl;
    config << "outputDir: \"build\";" << std::endl;
    config << "target: \"main\";" << std::endl;
    config << "sourceDir: \"src\";" << std::endl;
    config << "units: [" << std::endl;
    config << "    \"main.c\";" << std::endl;
    config << "];" << std::endl;
    config << "flags: [" << std::endl;
    config << "    \"-Wall\";" << std::endl;
    config << "    \"-Wextra\";" << std::endl;
    config << "    \"-pedantic\";" << std::endl;
    config << "    \"-std=c17\";" << std::endl;
    config << "];" << std::endl;
    config << "includes: [" << std::endl;
    config << "    \"src\";" << std::endl;
    config << "];" << std::endl;
    config << "libs: [" << std::endl;
    config << "    \"m\";" << std::endl;
    config << "];" << std::endl;
    config << "defines: [" << std::endl;
    config << "    \"VERSION=\\\"1.0\\\"\";" << std::endl;
    config << "];" << std::endl;
    config << "otherFlags: [];" << std::endl;
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

int main(int argc, const char* argv[])
{
    std::string configFile = "build.drg";

    if (argc < 2) {
        usage(argv[0], std::cout);
        return 0;
    }

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
        } else {
            std::cerr << "[Dragon] " << "Unknown argument: " << arg << std::endl;
            usage(std::string(argv[0]), std::cerr);
            exit(1);
        }
    }

    std::string command = argv[1];
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
