#include "../dragon.hpp"

void cmd_clean(std::string& configFile) {
    bool configExists = std::filesystem::exists(configFile);
    if (!configExists) {
        DRAGON_ERR << "Config file not found!" << std::endl;
        DRAGON_ERR << "Have you forgot to run 'dragon init'?" << std::endl;
        return;
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
