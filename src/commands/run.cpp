#include "../dragon.hpp"

void cmd_run(std::string& configFile) {
    std::string outfile = cmd_build(configFile);
    std::string cmd = outfile;

    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry* root = parser.parse(configFile);
    DragonConfig::CompoundEntry* run = root->getCompound("run");
    
    std::vector<std::string> argv;
    if (run) {
        DragonConfig::ListEntry* args = run->getList("args");
        if (args) {
            for (u_long i = 0; i < args->size(); i++) {
                argv.push_back(args->getString(i)->getValue());
            }
        }
    }

    run_with_args(cmd, argv);
}

void run_with_args(std::string& cmd, std::vector<std::string>& argv) {
    std::string args;
    for (auto arg : argv) {
        args += arg + " ";
    }

    int ret = system(args.c_str());
    if (ret) {
        DRAGON_ERR << "Error running " << cmd << std::endl;
        exit(ret);
    }
}
