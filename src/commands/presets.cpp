#include "../dragon.hpp"

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
