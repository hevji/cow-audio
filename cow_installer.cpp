/*
 * cow_installer.cpp — COW Audio Format Installer
 * ================================================
 * Compile (MinGW):
 *   windres installer.rc -o installer_res.o
 *   g++ -o cow_installer.exe cow_installer.cpp installer_res.o -lwinmm
 *
 * What this installer does (nothing hidden):
 *   1. Creates C:\COWPlayer\
 *   2. Copies cow_player.exe, cow.exe, cow_format.py, cow_icon.ico
 *      from the folder next to this installer into C:\COWPlayer\
 *   3. Adds C:\COWPlayer to the system PATH (HKLM registry)
 *   4. Registers .cow file association (same as cow_player.reg)
 *   5. Notifies the shell so icons update immediately
 *
 * Requires admin (UAC) for PATH and file association changes.
 * Source code is fully open — read it before running.
 */

#pragma comment(linker, "/subsystem:console")
#pragma comment(lib, "shell32.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Config
// ─────────────────────────────────────────────────────────────────────────────
#define INSTALL_DIR     "C:\\COWPlayer"
#define INSTALL_DIR_W   L"C:\\COWPlayer"

static const char* FILES_TO_COPY[] = {
    "resources\\applications\\cow_player.exe",
    "resources\\applications\\cow.exe",
    "resources\\applications\\cow_format.py",
    nullptr
};

static const char* ICON_PATH_SRC = "resources\\metadata\\cow_icon.ico";

// ─────────────────────────────────────────────────────────────────────────────
// Console colors
// ─────────────────────────────────────────────────────────────────────────────
#define COL_RESET   (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COL_GREEN   (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_RED     (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define COL_YELLOW  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_CYAN    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_WHITE   (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

static HANDLE hCon;
static void col(WORD a) { SetConsoleTextAttribute(hCon, a); }

static void print_step(const char* msg) {
    col(COL_CYAN); printf("  >>  ");
    col(COL_WHITE); printf("%s", msg);
    col(COL_RESET);
}

static void print_ok(const char* msg = "OK") {
    col(COL_GREEN); printf("  [%s]\n", msg);
    col(COL_RESET);
}

static void print_fail(const char* msg) {
    col(COL_RED); printf("  [FAILED] %s\n", msg);
    col(COL_RESET);
}

static void print_info(const char* msg) {
    col(COL_YELLOW); printf("  %s\n", msg);
    col(COL_RESET);
}

static void draw_header() {
    system("cls");
    col(COL_GREEN);
    printf(" +============================================================+\n");
    printf(" |           COW Audio Format  -  Installer                   |\n");
    printf(" |           github.com/hevji/cow-audio                       |\n");
    printf(" +============================================================+\n");
    col(COL_RESET);
    printf("\n");
    print_info("This installer will:");
    print_info("  - Create C:\\COWPlayer\\");
    print_info("  - Copy cow_player.exe, cow.exe, cow_format.py, cow_icon.ico");
    print_info("  - Add C:\\COWPlayer to your system PATH");
    print_info("  - Register .cow file association");
    printf("\n");
    col(COL_YELLOW);
    printf("  Source: github.com/hevji/cow-audio/blob/main/cow_installer.cpp\n");
    col(COL_RESET);
    printf("\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// Get directory of this exe
// ─────────────────────────────────────────────────────────────────────────────
static std::string get_exe_dir()
{
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    std::string p(buf);
    auto pos = p.rfind('\\');
    return (pos != std::string::npos) ? p.substr(0, pos) : ".";
}

// ─────────────────────────────────────────────────────────────────────────────
// Step 1: Create install directory
// ─────────────────────────────────────────────────────────────────────────────
static bool step_create_dir()
{
    print_step("Creating " INSTALL_DIR " ...");

    if (CreateDirectoryA(INSTALL_DIR, NULL)) {
        print_ok("Created");
        return true;
    }

    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS) {
        print_ok("Already exists");
        return true;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Could not create directory (error %lu)", err);
    print_fail(msg);
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Step 2: Copy files
// ─────────────────────────────────────────────────────────────────────────────
static bool step_copy_files(const std::string& src_dir)
{
    bool all_ok = true;

    // Copy main files
    for (int i = 0; FILES_TO_COPY[i]; i++) {
        std::string src  = src_dir + "\\" + FILES_TO_COPY[i];

        // Destination is just the filename, not the subfolder
        std::string fname = FILES_TO_COPY[i];
        auto slash = fname.rfind('\\');
        if (slash != std::string::npos) fname = fname.substr(slash + 1);

        std::string dst  = std::string(INSTALL_DIR) + "\\" + fname;
        std::string msg  = std::string("Copying ") + fname + " ...";
        print_step(msg.c_str());

        if (!CopyFileA(src.c_str(), dst.c_str(), FALSE)) {
            DWORD err = GetLastError();
            char emsg[256];
            snprintf(emsg, sizeof(emsg), "error %lu — is the file next to the installer?", err);
            print_fail(emsg);
            all_ok = false;
        } else {
            print_ok();
        }
    }

    // Copy icon
    std::string icon_src = src_dir + "\\" + ICON_PATH_SRC;
    std::string icon_dst = std::string(INSTALL_DIR) + "\\cow_icon.ico";
    print_step("Copying cow_icon.ico ...");
    if (!CopyFileA(icon_src.c_str(), icon_dst.c_str(), FALSE)) {
        print_fail("Could not copy icon");
        all_ok = false;
    } else {
        print_ok();
    }

    return all_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Step 3: Add to system PATH
// ─────────────────────────────────────────────────────────────────────────────
static bool step_add_to_path()
{
    print_step("Adding " INSTALL_DIR " to system PATH ...");

    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
        0, KEY_READ | KEY_WRITE, &hKey);

    if (res != ERROR_SUCCESS) {
        print_fail("Could not open registry key — are you running as admin?");
        return false;
    }

    // Read current PATH
    char cur_path[32767] = {};
    DWORD type, size = sizeof(cur_path);
    RegQueryValueExA(hKey, "Path", NULL, &type, (LPBYTE)cur_path, &size);

    // Check if already in PATH
    std::string path_str(cur_path);
    if (path_str.find(INSTALL_DIR) != std::string::npos) {
        RegCloseKey(hKey);
        print_ok("Already in PATH");
        return true;
    }

    // Append
    if (!path_str.empty() && path_str.back() != ';')
        path_str += ";";
    path_str += INSTALL_DIR;

    res = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                         (const BYTE*)path_str.c_str(),
                         (DWORD)path_str.size() + 1);
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS) {
        print_fail("Could not write to registry");
        return false;
    }

    // Broadcast environment change so new terminals pick it up
    DWORD_PTR result;
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, &result);

    print_ok();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Step 4: Register .cow file association
// ─────────────────────────────────────────────────────────────────────────────
static bool step_register_association()
{
    print_step("Registering .cow file association ...");

    const char* exe_path = INSTALL_DIR "\\cow_player.exe";
    const char* icon_path = INSTALL_DIR "\\cow_icon.ico";

    // HKCR\.cow -> COWAudioFile
    HKEY hKey;
    RegCreateKeyExA(HKEY_CLASSES_ROOT, ".cow", 0, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExA(hKey, "", 0, REG_SZ,
                   (const BYTE*)"COWAudioFile", 13);
    RegSetValueExA(hKey, "Content Type", 0, REG_SZ,
                   (const BYTE*)"audio/x-cow", 12);
    RegCloseKey(hKey);

    // HKCR\COWAudioFile
    RegCreateKeyExA(HKEY_CLASSES_ROOT, "COWAudioFile", 0, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExA(hKey, "", 0, REG_SZ,
                   (const BYTE*)"COW Compressed Wave Audio", 26);
    RegCloseKey(hKey);

    // DefaultIcon
    RegCreateKeyExA(HKEY_CLASSES_ROOT, "COWAudioFile\\DefaultIcon", 0, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExA(hKey, "", 0, REG_SZ,
                   (const BYTE*)icon_path, (DWORD)strlen(icon_path) + 1);
    RegCloseKey(hKey);

    // shell\open\command
    RegCreateKeyExA(HKEY_CLASSES_ROOT, "COWAudioFile\\shell\\open\\command",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    std::string cmd = std::string("\"") + exe_path + "\" \"%1\"";
    RegSetValueExA(hKey, "", 0, REG_SZ,
                   (const BYTE*)cmd.c_str(), (DWORD)cmd.size() + 1);
    RegCloseKey(hKey);

    // Notify shell of association change
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    print_ok();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Uninstaller
// ─────────────────────────────────────────────────────────────────────────────
static void uninstall()
{
    printf("\n");
    col(COL_YELLOW);
    printf("  Uninstalling COW Audio...\n\n");
    col(COL_RESET);

    // Remove files
    print_step("Removing files ...");
    for (int i = 0; FILES_TO_COPY[i]; i++) {
        std::string fname = FILES_TO_COPY[i];
        auto slash = fname.rfind('\\');
        if (slash != std::string::npos) fname = fname.substr(slash + 1);
        std::string f = std::string(INSTALL_DIR) + "\\" + fname;
        DeleteFileA(f.c_str());
    }
    DeleteFileA(INSTALL_DIR "\\cow_icon.ico");
    DeleteFileA(INSTALL_DIR "\\cow_installer.exe");
    RemoveDirectoryA(INSTALL_DIR);
    print_ok();

    // Remove from PATH
    print_step("Removing from PATH ...");
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
        0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char cur_path[32767] = {};
        DWORD type, size = sizeof(cur_path);
        RegQueryValueExA(hKey, "Path", NULL, &type, (LPBYTE)cur_path, &size);
        std::string p(cur_path);
        // Remove all occurrences
        std::string target = std::string(";") + INSTALL_DIR;
        size_t pos;
        while ((pos = p.find(target)) != std::string::npos) p.erase(pos, target.size());
        target = std::string(INSTALL_DIR) + ";";
        while ((pos = p.find(target)) != std::string::npos) p.erase(pos, target.size());
        target = INSTALL_DIR;
        while ((pos = p.find(target)) != std::string::npos) p.erase(pos, target.size());
        RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                       (const BYTE*)p.c_str(), (DWORD)p.size() + 1);
        RegCloseKey(hKey);
        print_ok();
    } else {
        print_fail("Could not open registry — are you admin?");
    }

    // Remove file association
    print_step("Removing file association ...");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, ".cow");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "COWAudioFile\\shell\\open\\command");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "COWAudioFile\\shell\\open");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "COWAudioFile\\shell");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "COWAudioFile\\DefaultIcon");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "COWAudioFile");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    print_ok();

    printf("\n");
    col(COL_GREEN);
    printf("  COW Audio has been uninstalled.\n");
    col(COL_RESET);
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("COW Audio Installer");

    // Hide cursor
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(hCon, &ci);

    // Check for /uninstall flag
    if (argc > 1 && (strcmp(argv[1], "/uninstall") == 0 ||
                     strcmp(argv[1], "-uninstall") == 0 ||
                     strcmp(argv[1], "--uninstall") == 0)) {
        draw_header();
        uninstall();
        printf("\n  Press any key to exit...\n");
        getchar();
        return 0;
    }

    // If re-launched elevated, skip straight to install
    bool skip_prompt = (argc > 1 && strcmp(argv[1], "/elevated") == 0);

    draw_header();

    if (!skip_prompt) {
        col(COL_WHITE);
        printf("  Press ENTER to install, or close this window to cancel.\n");
        col(COL_RESET);
        printf("  To uninstall later: cow_installer.exe /uninstall\n\n");
        getchar();
    }

    // Check if already admin
    BOOL is_admin = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elev;
        DWORD size;
        if (GetTokenInformation(token, TokenElevation, &elev, sizeof(elev), &size))
            is_admin = elev.TokenIsElevated;
        CloseHandle(token);
    }

    // If not admin, re-launch elevated
    if (!is_admin) {
        char exe_path[MAX_PATH];
        GetModuleFileNameA(NULL, exe_path, MAX_PATH);
        SHELLEXECUTEINFOA sei = {};
        sei.cbSize       = sizeof(sei);
        sei.lpVerb       = "runas";
        sei.lpFile       = exe_path;
        sei.lpParameters = "/elevated";
        sei.nShow        = SW_SHOW;
        if (!ShellExecuteExA(&sei)) {
            col(COL_RED);
            printf("\n  Admin access denied. Installation cancelled.\n");
            col(COL_RESET);
            printf("  Press any key to exit...\n");
            getchar();
            return 1;
        }
        return 0; // elevated copy will continue
    }

    printf("\n");
    std::string src_dir = get_exe_dir();

    bool ok = true;
    ok &= step_create_dir();
    ok &= step_copy_files(src_dir);
    ok &= step_add_to_path();
    ok &= step_register_association();

    printf("\n");
    if (ok) {
        col(COL_GREEN);
        printf(" +============================================================+\n");
        printf(" |   Installation complete!                                    |\n");
        printf(" |                                                             |\n");
        printf(" |   Open a new terminal and type:  cow encode mysong.wav     |\n");
        printf(" |   Or just double-click any .cow file to play it.           |\n");
        printf(" +============================================================+\n");
        col(COL_RESET);
    } else {
        col(COL_RED);
        printf(" +============================================================+\n");
        printf(" |   Installation completed with errors.                       |\n");
        printf(" |   Check the messages above and try running as admin.        |\n");
        printf(" +============================================================+\n");
        col(COL_RESET);
    }

    printf("\n  Press any key to exit...\n");
    getchar();
    return ok ? 0 : 1;
}