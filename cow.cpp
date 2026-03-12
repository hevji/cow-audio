/*
 * cow.exe — COW format CLI tool
 * ==============================
 * Compile:
 *   g++ -o cow.exe cow.cpp -mconsole
 *
 * Usage:
 *   cow encode  <file.wav>   — encodes to .cow in same folder as input
 *   cow decode  <file.cow>   — decodes to .wav in same folder as input
 *   cow play    <file.cow>   — plays the file
 *   cow info    <file.cow>   — shows file info
 *
 * cow_format.py is always loaded from C:\COWPlayer\cow_format.py
 */

#pragma comment(linker, "/subsystem:console")

#include <windows.h>
#include <cstdio>
#include <string>

#define COW_DIR "C:\\COWPlayer"
#define PY_SCRIPT COW_DIR "\\cow_format.py"
#define PLAYER_EXE COW_DIR "\\cow_player.exe"

// Get the folder of a file path  e.g. "C:\music\song.wav" -> "C:\music"
static std::string get_folder(const std::string& path)
{
    auto pos = path.rfind('\\');
    if (pos == std::string::npos) pos = path.rfind('/');
    if (pos == std::string::npos) {
        // No separator — it's a bare filename, use current directory
        char cwd[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, cwd);
        return std::string(cwd);
    }
    return path.substr(0, pos);
}

// Get filename without extension  e.g. "song.wav" -> "song"
static std::string strip_ext(const std::string& path)
{
    auto sep = path.rfind('\\');
    if (sep == std::string::npos) sep = path.rfind('/');
    std::string name = (sep == std::string::npos) ? path : path.substr(sep + 1);
    auto dot = name.rfind('.');
    if (dot == std::string::npos) return name;
    return name.substr(0, dot);
}

// Make an absolute path from a possibly-relative one
static std::string make_absolute(const std::string& path)
{
    char buf[MAX_PATH];
    GetFullPathNameA(path.c_str(), MAX_PATH, buf, NULL);
    return std::string(buf);
}

static int run(const std::string& cmd)
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    std::string mut = cmd;
    if (!CreateProcessA(NULL, mut.data(), NULL, NULL, FALSE,
                        0, NULL, NULL, &si, &pi))
        return -1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)code;
}

static void usage()
{
    printf("\nCOW Audio Tool\n");
    printf("==============\n");
    printf("  cow encode  <file.wav>    encode to .cow (same folder)\n");
    printf("  cow decode  <file.cow>    decode to .wav (same folder)\n");
    printf("  cow play    <file.cow>    play the file\n");
    printf("  cow info    <file.cow>    show file info\n\n");
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        usage();
        return 1;
    }

    std::string cmd      = argv[1];
    std::string inp      = make_absolute(argv[2]);
    std::string folder   = get_folder(inp);
    std::string basename = strip_ext(inp);
    // Output goes in same folder as input
    std::string out_cow  = folder + "\\" + basename + ".cow";
    std::string out_wav  = folder + "\\" + basename + "_decoded.wav";

    if (cmd == "encode") {
        std::string c = "python \"" PY_SCRIPT "\" encode \"" + inp + "\"";
        return run(c);

    } else if (cmd == "decode") {
        std::string c = "python \"" PY_SCRIPT "\" decode \"" + inp + "\"";
        return run(c);

    } else if (cmd == "play") {
        std::string c = "\"" PLAYER_EXE "\" \"" + inp + "\"";
        return run(c);

    } else if (cmd == "info") {
        std::string c = "python \"" PY_SCRIPT "\" info \"" + inp + "\"";
        return run(c);

    } else {
        printf("Unknown command: %s\n", cmd.c_str());
        usage();
        return 1;
    }
}
