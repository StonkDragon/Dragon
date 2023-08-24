#include "../dragon.hpp"

std::string vecToString(std::vector<std::string>& vec) {
    std::string ret = "";
    for (auto&& s : vec) {
        ret += s + " ";
    }
    return ret;
}

void build(std::vector<std::string>& cmd) {
    run_with_args(cmd.front(), cmd);
}

std::string build_from_config(DragonConfig::CompoundEntry* buildConfig) {
    if (!buildConfig->getList("units") || buildConfig->getList("units")->size() == 0) {
        DRAGON_ERR << "No compilation units defined!" << std::endl;
        return "";
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
    
    bool incrementalBuild = buildConfig->getStringOrDefault("incrementalBuild", "false")->getValue() == "true";
    bool parallelBuild = parallel && buildConfig->getStringOrDefault("parallelBuild", "false")->getValue() == "true";
    if (parallelBuild && !incrementalBuild) {
        DRAGON_ERR << "Parallel build requires incremental build!" << std::endl;
        return "";
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
            return "";
        }
        std::filesystem::remove_all(
            buildConfig->getStringOrDefault("outputDir", "build")->getValue() +
            std::filesystem::path::preferred_separator +
            buildConfig->getStringOrDefault("target", "main")->getValue()
        );
    }
    try {
        std::filesystem::create_directories(buildConfig->getStringOrDefault("outputDir", "build")->getValue());
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create output directory: " << buildConfig->getStringOrDefault("outputDir", "build")->getValue() << std::endl;
        return "";
    }

    bool sourceDirExists = std::filesystem::exists(buildConfig->getStringOrDefault("sourceDir", "src")->getValue());
    if (!sourceDirExists) {
        DRAGON_ERR << "Source directory does not exist: " << buildConfig->getStringOrDefault("sourceDir", "src")->getValue() << std::endl;
        return "";
    }

#if defined(_WIN32)
#define PRE_BUILD_TAG "preBuildWin"
#define POST_BUILD_TAG "postBuildWin"
#else
#define PRE_BUILD_TAG "preBuild"
#define POST_BUILD_TAG "postBuild"
#endif

    if (buildConfig->getList(PRE_BUILD_TAG)) {
        for (u_long i = 0; i < buildConfig->getList(PRE_BUILD_TAG)->size(); i++) {
            DRAGON_LOG << "Running prebuild command: " << buildConfig->getList(PRE_BUILD_TAG)->getString(i)->getValue() << std::endl;
            int ret = system(buildConfig->getList(PRE_BUILD_TAG)->getString(i)->getValue().c_str());
            if (ret != 0) {
                DRAGON_ERR << "Pre-build command failed: " << buildConfig->getList(PRE_BUILD_TAG)->getString(i)->getValue() << std::endl;
                return "";
            }
        }
    }

    size_t unitSize = customUnits.size();
    std::vector<std::string> units;
    units.reserve(unitSize);

    if (buildConfig->getList("units")) {
        for (u_long i = 0; i < buildConfig->getList("units")->size(); i++) {
            std::string unit = buildConfig->getList("units")->getString(i)->getValue();
            
            if (!incrementalBuild) {
                cmd.push_back(
                    buildConfig->getStringOrDefault("sourceDir", "src")->getValue() +
                    std::filesystem::path::preferred_separator +
                    unit
                );
            }
            units.push_back(
                buildConfig->getStringOrDefault("sourceDir", "src")->getValue() +
                std::filesystem::path::preferred_separator +
                unit
            );
        }
    }

    for (size_t i = 0; i < unitSize; i++) {
        std::string unit = customUnits.at(i);
        
        if (!incrementalBuild) {
            cmd.push_back(
                buildConfig->getStringOrDefault("sourceDir", "src")->getValue() +
                std::filesystem::path::preferred_separator +
                unit
            );
        }
        units.push_back(
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

    size_t sourceDirPrefixLen =
        (buildConfig->getStringOrDefault("sourceDir", "src")->getValue() + std::filesystem::path::preferred_separator).size();

    std::vector<std::thread> threads;

    std::string cachedBuildConfig =
        buildConfig->getStringOrDefault("outputDir", "build")->getValue() +
        std::filesystem::path::preferred_separator +
        "build.drg.cache";

    auto cacheConfig = [cachedBuildConfig, buildConfig]() {
        std::ofstream cacheFile(cachedBuildConfig);
        buildConfig->print(cacheFile);
        cacheFile.close();
    };

    if (!std::filesystem::exists(cachedBuildConfig)) {
        cacheConfig();
    }
    if (file_modified_time(cachedBuildConfig) < file_modified_time(buildConfigFile)) {
        bool fullRebuildOnConfigChange = buildConfig->getStringOrDefault("fullRebuildOnConfigChange", "false")->getValue() == "true";
        if (fullRebuildOnConfigChange) {
            fullRebuild = true;
        }
        cacheConfig();
    }

    auto cacheFile = [](std::string from, std::string to) {
        std::filesystem::create_directories(to.substr(0, to.find_last_of(std::filesystem::path::preferred_separator)));
        std::ifstream src(from, std::ios::binary);
        std::ofstream dst(to, std::ios::binary);
        dst << src.rdbuf();
    };

    DragonConfig::ListEntry* watchRegexes = buildConfig->getList("watch");
    if (watchRegexes) {
        std::filesystem::path sourcePath(buildConfig->getStringOrDefault("sourceDir", "src")->getValue());
        // recurse through source directory
        for (auto& p : std::filesystem::recursive_directory_iterator(sourcePath)) {
            if (std::filesystem::is_regular_file(p)) {
                std::string path = p.path().string();
                for (u_long i = 0; i < watchRegexes->size(); i++) {
                    std::regex regex(watchRegexes->getString(i)->getValue());
                    if (std::regex_search(path, regex)) {
                        std::string cachedFile =
                            buildConfig->getStringOrDefault("outputDir", "build")->getValue() +
                            std::filesystem::path::preferred_separator +
                            replaceAll(path.substr(sourceDirPrefixLen), "/", "@");
                        
                        if (!std::filesystem::exists(cachedFile)) {
                            cacheFile(path, cachedFile);
                        } else {
                            if (file_modified_time(cachedFile) < file_modified_time(path)) {
                                fullRebuild = true;
                                cacheFile(path, cachedFile);
                            }
                        }
                    }
                }
            }
        }
    }

    for (auto&& unit : units) {
        FILE* file = fopen(unit.c_str(), "r");
        size_t len = 1024;
        char* cline = new char[1025];
        ssize_t l;
        while ((l = getline(&cline, &len, file)) != EOF) {
            std::string line(cline, len);
            size_t n;
                                // this string is like this because otherwise
                                // it would be picked up by this here
            if ((n = line.find("// ""TODO")) != std::string::npos) {
                DRAGON_LOG << "Todo: " << line.substr(line.find("TODO") + 5) << std::endl;
            }
        }

        if (!incrementalBuild) {
            continue;
        }

        std::string outFile =
            buildConfig->getStringOrDefault("outputDir", "build")->getValue() +
            std::filesystem::path::preferred_separator +
            replaceAll(unit.substr(sourceDirPrefixLen), "/", "@") +
            ".o";

        auto tmp = new std::vector(cmd);

        tmp->push_back(unit);
        tmp->push_back("-o");
        tmp->push_back(outFile);
        tmp->push_back("-c");

        if (!fullRebuild && std::filesystem::exists(outFile)) {
            if (file_modified_time(unit) < file_modified_time(outFile)) {
                continue;
            }
        }

        std::thread thread([](std::vector<std::string>* tmp){
            DRAGON_LOG << "Started building: " << tmp->at(tmp->size() - 2) << std::endl;
            build(*tmp);
            DRAGON_LOG << "Finished building: " << tmp->at(tmp->size() - 2) << std::endl;
            delete tmp;
        }, tmp);
        if (parallelBuild) {
            threads.push_back(std::move(thread));
        } else {
            thread.join();
        }
    }

    cmd.push_back(buildConfig->getStringOrDefault("outFilePrefix", "-o")->getValue());
    cmd.push_back(outputFile);

    if (incrementalBuild) {
        for (auto&& unit : units) {
            cmd.push_back(
                buildConfig->getStringOrDefault("outputDir", "build")->getValue() +
                std::filesystem::path::preferred_separator +
                replaceAll(unit.substr(sourceDirPrefixLen), "/", "@") +
                ".o"
            );
        }
    }

    for (auto&& thread : threads) {
        thread.join();
    }

    DRAGON_LOG << "Running build command: " << vecToString(cmd) << std::endl;

    build(cmd);

    if (buildConfig->getList(POST_BUILD_TAG)) {
        for (u_long i = 0; i < buildConfig->getList(POST_BUILD_TAG)->size(); i++) {
            DRAGON_LOG << "Running postbuild command: " << buildConfig->getList(POST_BUILD_TAG)->getString(i)->getValue() << std::endl;
            int ret = system(buildConfig->getList(POST_BUILD_TAG)->getString(i)->getValue().c_str());
            if (ret != 0) {
                DRAGON_ERR << "Post-build command failed: " << buildConfig->getList(POST_BUILD_TAG)->getString(i)->getValue() << std::endl;
                exit(ret);
            }
        }
    }

    return outputFile;
}

std::string cmd_build(std::string& configFile, bool waitForInteract) {
    bool configExists = std::filesystem::exists(configFile);
    if (!configExists) {
        DRAGON_ERR << "Config file not found!" << std::endl;
        DRAGON_ERR << "Have you forgot to run 'dragon init'?" << std::endl;
        return "";
    }

    if (waitForInteract) {
        std::string s;
        std::cin >> s;
    }
    
    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry* root = parser.parse(configFile);
    DragonConfig::CompoundEntry* buildConfig = root->getCompound(buildConfigRootEntry);

    if (!buildConfig) {
        DRAGON_ERR << "No build config with name '" << buildConfigRootEntry << "' found!" << std::endl;
        return "";
    }

    return build_from_config(buildConfig);
}
