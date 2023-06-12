#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include "runtime/engine.h"

#include "editor/include/editor.h"

// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html

int main(int argc, char** argv)
{
    std::filesystem::path executable_path(argv[0]);
    std::filesystem::path config_file_path = executable_path.parent_path() / "MoYuEditor.ini";

    MoYu::PilotEngine* engine = new MoYu::PilotEngine();

    engine->startEngine(config_file_path.generic_string());
    engine->initialize();

    MoYu::PilotEditor* editor = new MoYu::PilotEditor();
    editor->initialize(engine);

    editor->run();

    editor->clear();

    engine->clear();
    engine->shutdownEngine();

    return 0;
}
