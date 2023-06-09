#include "../dragon.hpp"

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