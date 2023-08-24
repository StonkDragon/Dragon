#if !defined(DRAGON_HPP)
#define DRAGON_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <functional>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#if defined(__wasm__)
#error "Why are you trying to compile this for wasm? It's not supported."
#endif

#ifdef _WIN32
#warning "Windows support is experimental and may not work."
#endif

#if !defined(_WIN32)
#include <execinfo.h>
#include <sys/stat.h>
#endif

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#define DRAGON_UNSUPPORTED_STR  "<DRAGON_UNSUPPORTED>"

#define DRAGON_LOG              std::cout << "[Dragon] "
#define DRAGON_ERR              std::cerr << "[Dragon] "

#include "DragonConfig.hpp"

extern bool overrideCompiler;
extern bool overrideOutputDir;
extern bool overrideTarget;
extern bool overrideSourceDir;
extern bool overrideMacroPrefix;
extern bool overrideLibraryPrefix;
extern bool overrideLibraryPathPrefix;
extern bool overrideIncludePrefix;
extern bool overrideOutFilePrefix;

extern bool fullRebuild;
extern bool parallel;

extern std::string compiler;
extern std::string outputDir;
extern std::string target;
extern std::string sourceDir;
extern std::string macroPrefix;
extern std::string libraryPrefix;
extern std::string libraryPathPrefix;
extern std::string includePrefix;
extern std::string outFilePrefix;

extern std::string buildConfigFile;

extern std::vector<std::string> customUnits;
extern std::vector<std::string> customIncludes;
extern std::vector<std::string> customLibs;
extern std::vector<std::string> customLibraryPaths;
extern std::vector<std::string> customDefines;
extern std::vector<std::string> customFlags;
extern std::vector<std::string> customPreBuilds;
extern std::vector<std::string> customPostBuilds;

extern std::string buildConfigRootEntry;

std::string cmd_build(std::string& configFile, bool waitForInteract = false);
void cmd_init(std::string& configFile);
void cmd_run(std::string& configFile);
std::vector<std::string> get_presets();
void generate_generic_main(std::string lang);
void load_preset(std::string& identifier);
void cmd_clean(std::string& configFile);
int cmd_package(std::vector<std::string> args);
int pkg_install(std::vector<std::string> args);
void run_with_args(std::string& cmd, std::vector<std::string>& args);

std::string replaceAll(std::string src, std::string from, std::string to);
bool strstarts(const std::string& str, const std::string& prefix);
std::vector<std::string> split(const std::string& str, char delim);
std::filesystem::file_time_type file_modified_time(const std::string& path);

#endif
