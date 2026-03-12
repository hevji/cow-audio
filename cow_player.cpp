/*
 * COW Player — Console audio player for .cow files
 * =================================================
 * Compile (MinGW):
 *   windres resource.rc -o resource.o
 *   g++ -o cow_player.exe cow_player.cpp resource.o -lwinmm
 *
 * Controls:
 *   SPACE       — pause / resume
 *   LEFT arrow  — rewind 5 seconds
 *   RIGHT arrow — forward 5 seconds
 *   UP arrow    — volume up
 *   DOWN arrow  — volume down
 *   Q / ESC     — quit
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <conio.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#pragma comment(lib, "winmm.lib")
#pragma comment(linker, "/subsystem:console")

// ─────────────────────────────────────────────────────────────────────────────
// Layout — all sizes in visible character columns
// ─────────────────────────────────────────────────────────────────────────────
#define INNER   60      // visible chars between the two ║ walls
#define TOTAL   62      // INNER + 2 for the walls

// Colors
#define COL_BOX    (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_TITLE  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_DIM    (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COL_GREEN  (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_RED    (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define COL_CYAN   (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

static HANDLE hCon;
static int    g_volume = 80;   // 0-100
static int    g_ox = 0;        // X offset for centering
static int    g_oy = 0;        // Y offset for centering

// ─────────────────────────────────────────────────────────────────────────────
// Console setup
// ─────────────────────────────────────────────────────────────────────────────
static void con_init()
{
    SetConsoleOutputCP(CP_UTF8);
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("COW Player");

    // Get current console window size
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hCon, &csbi);
    int con_w = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    int con_h = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;

    // Box is TOTAL wide and 14 rows tall
    const int BOX_H = 14;
    g_ox = (con_w - TOTAL) / 2;
    g_oy = (con_h - BOX_H) / 2;
    if (g_ox < 0) g_ox = 0;
    if (g_oy < 0) g_oy = 0;

    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hCon, &ci);
}

static void gotoxy(int x, int y)
{
    COORD c = { (SHORT)(x + g_ox), (SHORT)(y + g_oy) };
    SetConsoleCursorPosition(hCon, c);
}

static void col(WORD a) { SetConsoleTextAttribute(hCon, a); }

// ─────────────────────────────────────────────────────────────────────────────
// Box drawing — uses ASCII fallback so width math is always correct
// We use:  + - | for corners/edges  (1 byte = 1 column, no UTF-8 width issues)
// Then we switch the font chars only for the progress/volume fill blocks.
// ─────────────────────────────────────────────────────────────────────────────

// Print exactly `n` copies of a single-byte char
static void rep(char c, int n) { for (int i = 0; i < n; i++) putchar(c); }

static void box_top(int y)
{
    gotoxy(0, y); col(COL_BOX);
    // Use ASCII so column counts are exact
    putchar('+'); rep('=', INNER); putchar('+');
}

static void box_bot(int y)
{
    gotoxy(0, y); col(COL_BOX);
    putchar('+'); rep('=', INNER); putchar('+');
}

static void box_div(int y)
{
    gotoxy(0, y); col(COL_BOX);
    putchar('+'); rep('-', INNER); putchar('+');
}

// Print a content row: | <content padded to INNER cols> |
static void box_row(int y, WORD c, const char* fmt, ...)
{
    // Format the string first
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    int len = (int)strlen(buf);

    gotoxy(0, y);
    col(COL_BOX);   putchar('|');
    col(c);
    printf("%s", buf);
    // Pad to INNER
    for (int i = len; i < INNER; i++) putchar(' ');
    col(COL_BOX);   putchar('|');
}

static void box_empty(int y)
{
    gotoxy(0, y); col(COL_BOX); putchar('|');
    col(COL_DIM);  rep(' ', INNER);
    col(COL_BOX);  putchar('|');
}

// ─────────────────────────────────────────────────────────────────────────────
// MCI helpers
// ─────────────────────────────────────────────────────────────────────────────
static void mci_cmd(const std::string& cmd)
{
    char err[256] = {};
    mciSendStringA(cmd.c_str(), err, sizeof(err), NULL);
}

static DWORD mci_get_ms(const char* what)
{
    char buf[64] = {};
    mciSendStringA((std::string("status cowtrack ") + what).c_str(),
                   buf, sizeof(buf), NULL);
    return (DWORD)atoi(buf);
}

static void mci_seek(DWORD pos_ms, bool paused)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "seek cowtrack to %lu", (unsigned long)pos_ms);
    mci_cmd(buf);
    if (!paused) mci_cmd("play cowtrack");
}

// Volume via waveOutSetVolume on WAVE_MAPPER
static void set_volume(int vol)   // 0-100
{
    DWORD v = (DWORD)((vol / 100.0) * 0xFFFF);
    DWORD dw = v | (v << 16);
    waveOutSetVolume((HWAVEOUT)(ULONG_PTR)WAVE_MAPPER, dw);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static frame
// ─────────────────────────────────────────────────────────────────────────────
static void draw_static(const std::string& filename, int dur_s)
{
    system("cls");

    box_top(0);
    box_row(1,  COL_YELLOW, "  COW Player  -  Compressed Wave Audio");
    box_div(2);
    box_row(3,  COL_TITLE,  "  %.56s", filename.c_str());
    box_row(4,  COL_DIM,    "  Duration : %d:%02d", dur_s / 60, dur_s % 60);
    box_empty(5);
    box_div(6);
    box_empty(7);   // progress bar
    box_empty(8);   // volume bar
    box_empty(9);   // status
    box_div(10);
    box_row(11, COL_DIM,    "  SPACE Pause/Resume   LEFT/RIGHT Seek 5s");
    box_row(12, COL_DIM,    "  UP/DOWN Volume       Q / ESC   Quit");
    box_bot(13);
}

// ─────────────────────────────────────────────────────────────────────────────
// Dynamic rows
// ─────────────────────────────────────────────────────────────────────────────
static void draw_dynamic(double elapsed, double duration, bool paused, int vol)
{
    // ── Row 7: progress bar ───────────────────────────────────────────────────
    // Layout: "  [####....] 0:00/0:00  "
    // bar width = INNER - 2(pad) - 2(brackets) - 12(time) - 2(spaces) = 42
    const int BAR = 42;
    double pct = (duration > 0) ? elapsed / duration : 0.0;
    if (pct > 1.0) pct = 1.0;
    int filled = (int)(pct * BAR);
    int es = (int)elapsed, ds = (int)duration;

    gotoxy(0, 7);
    col(COL_BOX);   putchar('|');
    col(COL_DIM);   printf("  [");
    if (paused) col(COL_YELLOW); else col(COL_GREEN);
    for (int i = 0; i < BAR; i++) putchar(i < filled ? '#' : '.');
    col(COL_DIM);   putchar(']');
    col(COL_TITLE); printf(" %d:%02d/%d:%02d", es/60, es%60, ds/60, ds%60);
    // pad
    int used = 2 + 1 + BAR + 1 + 1 + 9;  // "  [" bar "] " time
    for (int i = used; i < INNER; i++) putchar(' ');
    col(COL_BOX);   putchar('|');

    // ── Row 8: volume bar ─────────────────────────────────────────────────────
    // Layout: "  Vol [####....] 100%  "
    // vol bar = INNER - 2 - 6 - 2 - 5 - 2 = 43
    const int VBAR = 43;
    int vfill = (int)(vol / 100.0 * VBAR);

    gotoxy(0, 8);
    col(COL_BOX);   putchar('|');
    col(COL_DIM);   printf("  Vol [");
    col(COL_CYAN);
    for (int i = 0; i < VBAR; i++) putchar(i < vfill ? '#' : '.');
    col(COL_DIM);   putchar(']');
    col(COL_TITLE); printf(" %3d%%", vol);
    int vused = 2 + 6 + VBAR + 1 + 4;
    for (int i = vused; i < INNER; i++) putchar(' ');
    col(COL_BOX);   putchar('|');

    // ── Row 9: status ─────────────────────────────────────────────────────────
    gotoxy(0, 9);
    col(COL_BOX); putchar('|');
    if (paused) {
        col(COL_YELLOW);
        const char* s = "  [ PAUSED ]";
        printf("%s", s);
        for (int i = (int)strlen(s); i < INNER; i++) putchar(' ');
    } else {
        col(COL_GREEN);
        const char* s = "  [ PLAYING ]";
        printf("%s", s);
        for (int i = (int)strlen(s); i < INNER; i++) putchar(' ');
    }
    col(COL_BOX); putchar('|');
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static std::string get_exe_dir()
{
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    std::string p(buf);
    auto pos = p.rfind('\\');
    return (pos != std::string::npos) ? p.substr(0, pos) : ".";
}

static int run_cmd(const std::string& cmd)
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    std::string mut = cmd;
    if (!CreateProcessA(NULL, mut.data(), NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return -1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)code;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: cow_player.exe <file.cow>\nPress any key...\n");
        _getch(); return 1;
    }

    std::string cow_path  = argv[1];
    std::string exe_dir   = get_exe_dir();
    std::string py_script = exe_dir + "\\cow_format.py";

    // ── Decode to temp WAV ────────────────────────────────────────────────────
    char tmp_dir[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp_dir);
    std::string tmp_wav = std::string(tmp_dir) + "cow_player_tmp.wav";

    printf("Decoding, please wait...\n");
    std::string dcmd = "python \"" + py_script + "\" decode \"" +
                       cow_path + "\" \"" + tmp_wav + "\"";
    if (run_cmd(dcmd) != 0) {
        fprintf(stderr, "Failed to decode. Ensure Python + cow_format.py are in "
                        "the same folder.\nPress any key...\n");
        _getch(); return 1;
    }

    // ── Read WAV duration ─────────────────────────────────────────────────────
    double duration_sec = 0.0;
    {
        FILE* wf = fopen(tmp_wav.c_str(), "rb");
        if (wf) {
            uint8_t buf[44];
            if (fread(buf, 1, 44, wf) == 44) {
                uint16_t ch        = *(uint16_t*)(buf + 22);
                uint32_t sr        = *(uint32_t*)(buf + 24);
                uint16_t bits      = *(uint16_t*)(buf + 34);
                uint32_t data_size = *(uint32_t*)(buf + 40);
                if (sr && ch && bits)
                    duration_sec = (double)data_size / (sr * ch * (bits / 8));
            }
            fclose(wf);
        }
    }

    // ── Open MCI ──────────────────────────────────────────────────────────────
    mci_cmd("open \"" + tmp_wav + "\" type waveaudio alias cowtrack");
    mci_cmd("set cowtrack time format milliseconds");
    mci_cmd("play cowtrack");

    // Get the waveOut device ID that MCI opened so we can control volume
    char dev_buf[64] = {};
    mciSendStringA("status cowtrack device type", dev_buf, sizeof(dev_buf), NULL);
    set_volume(g_volume);

    // ── Init UI ───────────────────────────────────────────────────────────────
    con_init();

    std::string display_name = cow_path;
    {
        auto p = cow_path.rfind('\\');
        if (p != std::string::npos) display_name = cow_path.substr(p + 1);
    }

    draw_static(display_name, (int)duration_sec);

    bool  paused    = false;
    bool  running   = true;
    DWORD length_ms = mci_get_ms("length");

    while (running) {
        // ── Keyboard ──────────────────────────────────────────────────────────
        if (_kbhit()) {
            int ch = _getch();

            if (ch == 0 || ch == 0xE0) {
                int arrow = _getch();
                DWORD pos = mci_get_ms("position");

                if (arrow == 75) {                              // LEFT rewind
                    mci_seek((pos > 5000) ? pos - 5000 : 0, paused);
                } else if (arrow == 77) {                       // RIGHT forward
                    DWORD np = pos + 5000;
                    if (np < length_ms) mci_seek(np, paused);
                } else if (arrow == 72) {                       // UP vol+
                    g_volume = (g_volume + 5 > 100) ? 100 : g_volume + 5;
                    set_volume(g_volume);
                } else if (arrow == 80) {                       // DOWN vol-
                    g_volume = (g_volume - 5 < 0) ? 0 : g_volume - 5;
                    set_volume(g_volume);
                }

            } else if (ch == ' ' || ch == 'p' || ch == 'P') {
                paused ? mci_cmd("resume cowtrack") : mci_cmd("pause cowtrack");
                paused = !paused;

            } else if (ch == 'q' || ch == 'Q' || ch == 27) {
                running = false;
            }
        }

        // ── Redraw ────────────────────────────────────────────────────────────
        DWORD pos_ms = mci_get_ms("position");
        draw_dynamic(pos_ms / 1000.0, duration_sec, paused, g_volume);

        if (!paused && length_ms > 0 && pos_ms >= length_ms)
            running = false;

        Sleep(100);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    mci_cmd("stop cowtrack");
    mci_cmd("close cowtrack");
    DeleteFileA(tmp_wav.c_str());

    gotoxy(0, 14);
    col(COL_DIM);
    printf("\n");
    return 0;
}