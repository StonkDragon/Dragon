#include "../dragon.hpp"

void cmd_run(std::string& configFile) {
    std::string outfile = cmd_build(configFile);
    std::string cmd = outfile;

    DragonConfig::ConfigParser parser;
    DragonConfig::CompoundEntry* root = parser.parse(configFile);
    DragonConfig::CompoundEntry* run = root->getCompound("run");
    
    std::vector<std::string> argv;
    std::vector<std::string> envs;
    argv.push_back(cmd);
    if (run) {
        DragonConfig::ListEntry* args = run->getList("args");
        DragonConfig::ListEntry* env = run->getList("env");
        if (args) {
            for (u_long i = 0; i < args->size(); i++) {
                argv.push_back(args->getString(i)->getValue());
            }
        }
        if (env) {
            for (u_long i = 0; i < env->size(); i++) {
                envs.push_back(env->getString(i)->getValue());
            }
        }
    }

    char** envs_c = NULL;
    envs_c = (char**) malloc(envs.size() * sizeof(char*) + 1);
    for (u_long i = 0; i < envs.size(); i++) {
        envs_c[i] = (char*)envs[i].c_str();
    }
    envs_c[envs.size()] = NULL;

    char** argv_c = NULL;
    argv_c = (char**) malloc(argv.size() * sizeof(char*) + 1);
    for (u_long i = 0; i < argv.size(); i++) {
        argv_c[i] = (char*)argv[i].c_str();
    }
    argv_c[argv.size()] = NULL;

    DRAGON_LOG << "Running " << cmd << std::endl;

    int child = fork();
    if (child == 0) {
        int ret = execve(cmd.c_str(), argv_c, envs_c);
        if (ret == -1) {
            DRAGON_LOG << "Error: " << strerror(errno) << std::endl;
            exit(1);
        }
    } else if (child > 0) {
        wait(NULL);
        DRAGON_LOG << "Finished!" << std::endl;
    } else {
        DRAGON_ERR << "Error forking a child process!" << std::endl;
        DRAGON_ERR << "Error: " << strerror(errno) << std::endl;
        exit(1);
    }
}
