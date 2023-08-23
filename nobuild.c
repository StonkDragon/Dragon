#define NOBUILD_IMPLEMENTATION
#include "nobuild.h"

#define CFLAGS "-Wall", "-Wextra"
#define EXE "build/dragon"
#define CC "clang++"
#define SRC "src/dragon.cpp", "src/DragonConfig.cpp", "src/commands/build.cpp", "src/commands/clean.cpp", "src/commands/init.cpp", "src/commands/presets.cpp", "src/commands/run.cpp", "src/commands/package.cpp"

#ifndef _WIN32
int main(int argc, char** argv) {
    GO_REBUILD_URSELF(argc, argv);
#else
int main() {
#endif
    if (!PATH_EXISTS("build")) {
        MKDIRS("build");
    }
    CMD(CC, CFLAGS, SRC, "-o", EXE, "-std=gnu++17");
    return 0;
}
