#include "../dragon.hpp"

void pkg_help() {
    DRAGON_LOG << "Usage: dragon package <command> [options]" << std::endl;
    DRAGON_LOG << "Commands:" << std::endl;
    DRAGON_LOG << "  help        Display this help message." << std::endl;
    DRAGON_LOG << "  install     Install a package." << std::endl;
}

std::string build_from_config(DragonConfig::CompoundEntry* buildConfig);

// layout: 
//   dragon package install StonkDragon/Scale
//   dragon package install StonkDragon/Scale v23.7
//   dragon package install github.com@StonkDragon/Scale
//   dragon package install github.com@StonkDragon/Scale v23.7
int pkg_install(std::vector<std::string> args) {
    std::string url = "https://github.com/";
    if (args.size() == 1) {
        DRAGON_ERR << "'package install' requires a package name." << std::endl;
        return 1;
    }
    std::string package = args[1];
    if (package.find("@") != std::string::npos) {
        std::vector<std::string> parts = split(package, '@');
        if (parts.size() != 2) {
            DRAGON_ERR << "Invalid package name '" << package << "'." << std::endl;
            return 1;
        }
        url = "https://" + parts[0] + "/" + parts[1];
    }
    std::string version = "";
    if (args.size() > 2) {
        version = args[2];
    }

    std::string packageDir = "/opt/dragon/den/";
    if (package.find("@") != std::string::npos) {
        packageDir += package.substr(0, package.find("@"));
    } else {
        packageDir += package;
    }

    std::string command = "git clone " + url + package;
    if (version.size()) {
        command += " --branch " + version;
    }
    command += " --recurse-submodules " + packageDir + " > /dev/null 2> /dev/null";
    
    int ret;

    if (std::filesystem::exists(packageDir)) {
        std::filesystem::remove_all(packageDir);
    }
    std::filesystem::create_directories(std::filesystem::path(packageDir).parent_path());

    DRAGON_LOG << command << std::endl;
    ret = system(command.c_str());
    if (ret != 0) {
        DRAGON_ERR << "Failed to install package '" << package << "'." << std::endl;
        return ret;
    }

    std::string configFile = packageDir + "/build.drg";
    if (!std::filesystem::exists(configFile)) {
        DRAGON_ERR << "Invalid package '" << package << "'." << std::endl;
        return 1;
    }
    using namespace DragonConfig;
    ConfigParser parser;
    CompoundEntry* root = parser.parse(configFile);
    if (!root) {
        DRAGON_ERR << "Failed to parse package config for '" << package << "'." << std::endl;
        return 1;
    }
    CompoundEntry* install = root->getCompound("install");
    if (!install) {
        DRAGON_ERR << "No 'install' section in package config for '" << package << "'." << std::endl;
        return 1;
    }
    auto pwd = std::filesystem::current_path();
    std::filesystem::current_path(packageDir);
    
    std::string builtFile = build_from_config(install);
    std::filesystem::current_path(pwd);

    if (builtFile.size() == 0) {
        DRAGON_ERR << "Failed to install package '" << package << "'." << std::endl;
        return 1;
    }

    DRAGON_LOG << "Successfully installed package '" << package << "'." << std::endl;
    return 0;
}

int cmd_package(std::vector<std::string> args) {
    if (args.size() == 0) {
        DRAGON_ERR << "'package' requires a subcommand." << std::endl;
        return 1;
    }
    std::string command = args[0];
    if (command == "help") {
        pkg_help();
        return 0;
    } else if (command == "install") {
        return pkg_install(args);
    } else {
        DRAGON_ERR << "Unknown subcommand '" << command << "'." << std::endl;
        return 1;
    }
}
