#include "../dragon.hpp"

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
        std::filesystem::remove_all(buildConfig->getStringOrDefault("outputDir", "build")->getValue());
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

    if (buildConfig->getList("preBuild")) {
        for (u_long i = 0; i < buildConfig->getList("preBuild")->size(); i++) {
            DRAGON_LOG << "Running prebuild command: " << buildConfig->getList("preBuild")->getString(i)->getValue() << std::endl;
            int ret = system(buildConfig->getList("preBuild")->getString(i)->getValue().c_str());
            if (ret != 0) {
                DRAGON_ERR << "Pre-build command failed: " << buildConfig->getList("preBuild")->getString(i)->getValue() << std::endl;
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
            
            cmd.push_back(
                buildConfig->getStringOrDefault("sourceDir", "src")->getValue() +
                std::filesystem::path::preferred_separator +
                unit
            );
            units.push_back(
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

    cmd.push_back(buildConfig->getStringOrDefault("outFilePrefix", "-o")->getValue());
    cmd.push_back(outputFile);

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
                DRAGON_LOG << "Todo: " << line.substr(n + 7, line.find("\n") - n - 7) << std::endl;
            }
        }
    }

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
