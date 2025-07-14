#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>
#include <deque>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#endif

#define MAX_LINE_LENGTH 8192
#define MAX_UNDO_STACK 50
#define STATUS_LINE "\033[1;34m[Ctrl+G: Guide / Ctrl+N: Credits | Line: %d Col: %d | %s]\033[0m"

struct EditorState {
    std::vector<std::string> lines;
    int cursor_x, cursor_y;
    
    EditorState() : cursor_x(0), cursor_y(0) {}
    EditorState(const std::vector<std::string>& l, int x, int y) : lines(l), cursor_x(x), cursor_y(y) {}
};

class Editor {
private:
    std::vector<std::string> lines;
    int cursor_x, cursor_y;
    std::string filename;
    bool running;
    std::string status_msg;
    time_t status_msg_time;
    int term_rows, term_cols;
    int top_line;
    bool show_guide, show_credits;
    bool modified;
    int visible_lines;
    std::string clipboard;
    bool insert_mode;
    std::deque<EditorState> undo_stack;
    std::string find_term;
    int find_line, find_col;
    
#ifdef _WIN32
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO orig_csbi;
    DWORD orig_mode;
#else
    struct termios orig_term;
#endif

public:
    Editor() : cursor_x(0), cursor_y(0), running(true), status_msg_time(0),
               top_line(0), show_guide(false), show_credits(false), 
               modified(false), insert_mode(true), find_line(-1), find_col(-1) {
        lines.reserve(1000);
        lines.push_back("");
        filename = "unnamed.txt";
        init_term();
        sz();
    }
    
    ~Editor() {
        rst();
    }
    
    void init_term() {
#ifdef _WIN32
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hConsole, &orig_csbi);
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hInput, &orig_mode);
        SetConsoleMode(hInput, ENABLE_PROCESSED_INPUT);
        system("cls");
        
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.dwSize = 20;
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
        tcgetattr(STDIN_FILENO, &orig_term);
        struct termios raw_term = orig_term;
        raw_term.c_lflag &= ~(ICANON | ECHO);
        raw_term.c_cc[VMIN] = 1;
        raw_term.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_term);
        printf("\033[?1049h");
        printf("\033[6 q");
        signal(SIGINT, [](int){ exit(0); });
#endif
    }
    
    void rst() {
#ifdef _WIN32
        SetConsoleCursorPosition(hConsole, {0, 0});
        system("cls");
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(hInput, orig_mode);
#else
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
        printf("\033[0 q");
        printf("\033[?1049l\033[H\033[J");
        fflush(stdout);
#endif
    }
    
    void sz() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        term_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        term_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            term_rows = ws.ws_row;
            term_cols = ws.ws_col;
        } else {
            term_rows = 24;
            term_cols = 80;
        }
#endif
        visible_lines = term_rows - 4;
        if (visible_lines < 5) visible_lines = 5;
    }
    
    void msg(const std::string& message) {
        status_msg = message;
        status_msg_time = time(nullptr);
    }
    
    void clr() {
#ifdef _WIN32
        system("cls");
#else
        printf("\033[H\033[J");
#endif
    }
    
    void save_state() {
        if (undo_stack.size() >= MAX_UNDO_STACK) {
            undo_stack.pop_front();
        }
        undo_stack.emplace_back(lines, cursor_x, cursor_y);
    }
    
    void undo() {
        if (!undo_stack.empty()) {
            EditorState state = undo_stack.back();
            undo_stack.pop_back();
            lines = std::move(state.lines);
            cursor_x = state.cursor_x;
            cursor_y = state.cursor_y;
            if (cursor_y >= (int)lines.size()) cursor_y = lines.size() - 1;
            if (cursor_x > (int)lines[cursor_y].length()) cursor_x = lines[cursor_y].length();
            modified = true;
            msg("Undo successful");
        }
    }
    
    void crd() {
        clr();
        std::cout << "\033[1;37m  SAC++ Text Editor \033[0m\n\n";
        std::cout << "  \033[1;32mCreated by:\033[0m \033[1;36m@sxc_qq1\033[0m\n\n";
        std::cout << "  \033[1;32mFeatures:\033[0m\n";
        std::cout << "    • cross-platform support (Windows/Linux/Android)\n";
        std::cout << "    • text editing with undo/redo\n";
        std::cout << "    • syntax highlighting\n";
        std::cout << "    • optimized performance\n\n";
        std::cout << "  \033[1;32mTerms:\033[0m\n";
        std::cout << "    Software provided AS IS. Use at your own risk.\n";
        std::cout << "    Free for personal and educational use.\n\n";
        std::cout << "  \033[1;32mFeedback or Support:\033[0m \033[1;36msac-service@outlook.com\033[0m\n\n";
        std::cout << "  \033[1;33mEnter to return\033[0m";
        std::cout.flush();
    }
    
    void gd() {
        clr();
        std::cout << "\033[1;37m  SAC++ Editor - Guide \033[0m\n\n";
        std::cout << "  \033[1;32mFile Operations:\033[0m\n";
        std::cout << "    ^S  Save file        ^O  Open file\n";
        std::cout << "    ^Q  Quit editor      ^W  New file\n\n";
        std::cout << "  \033[1;32mEdit Operations:\033[0m\n";
        std::cout << "    ^U  Undo             ^Y  Redo\n";
        std::cout << "    ^X  Cut line         ^C  Copy line\n";
        std::cout << "    ^A  Select all\n";
        std::cout << "    ^F  Find text        ^R  Replace\n\n";
        std::cout << "  \033[1;32mNavigation:\033[0m\n";
        std::cout << "    Arrow keys  Move cursor\n";
        std::cout << "    Page Up/Dn  Scroll page\n";
        std::cout << "    Home/End    Line start/end\n";
        std::cout << "    ^Home/End   File start/end\n\n";
        std::cout << "  \033[1;32mMode:\033[0m\n";
        std::cout << "    Insert  Insert mode (default)\n";
        std::cout << "    ^I      Toggle insert/overwrite\n\n";
        std::cout << "  \033[1;32mOther:\033[0m\n";
        std::cout << "    ^G  Help             ^N  Credits\n";
        std::cout << "    ^L  Line goto        ^D  Delete line\n\n";
        std::cout << "  \033[1;33mEnter to return\033[0m";
        std::cout.flush();
    }
    
    std::vector<std::string> wrap_line(const std::string& line, int max_width) {
        std::vector<std::string> wrapped;
        if (line.empty()) {
            wrapped.push_back("");
            return wrapped;
        }
        
        int pos = 0;
        while (pos < (int)line.length()) {
            int end_pos = std::min(pos + max_width, (int)line.length());
            wrapped.push_back(line.substr(pos, end_pos - pos));
            pos = end_pos;
        }
        
        return wrapped;
    }
    
    void scrbar() {
        std::vector<std::string> display_lines;
        for (const auto& line : lines) {
            auto wrapped = wrap_line(line, term_cols - 7);
            display_lines.insert(display_lines.end(), wrapped.begin(), wrapped.end());
        }
        
        if (display_lines.size() <= (size_t)visible_lines) {
            for (int i = 0; i < visible_lines; i++) {
#ifdef _WIN32
                SetConsoleCursorPosition(hConsole, {(SHORT)(term_cols-1), (SHORT)(i+1)});
                std::cout << "\033[44m \033[0m";
#else
                printf("\033[%d;%dH\033[44m \033[0m", i + 2, term_cols);
#endif
            }
        } else {
            int thumb_size = (visible_lines * visible_lines) / (int)display_lines.size();
            if (thumb_size < 1) thumb_size = 1;
            int thumb_pos = (visible_lines * top_line) / (int)display_lines.size();
            
            for (int i = 0; i < visible_lines; i++) {
#ifdef _WIN32
                SetConsoleCursorPosition(hConsole, {(SHORT)(term_cols-1), (SHORT)(i+1)});
#else
                printf("\033[%d;%dH", i + 2, term_cols);
#endif
                if (i >= thumb_pos && i < thumb_pos + thumb_size) {
                    std::cout << "\033[46m \033[0m";
                } else {
                    std::cout << "\033[44m \033[0m";
                }
            }
        }
    }
    
    void highlight_line(const std::string& line, int max_width) {
        bool in_string = false;
        bool in_angle = false;
        char string_char = 0;
        
        for (size_t i = 0; i < line.length() && i < (size_t)max_width; i++) {
            char c = line[i];
            
            if (!in_string && (c == '"' || c == '\'')) {
                in_string = true;
                string_char = c;
                std::cout << "\033[32m" << c;
            } else if (in_string && c == string_char) {
                in_string = false;
                std::cout << c << "\033[0m";
            } else if (in_string) {
                std::cout << c;
            } else if (c == '<') {
                in_angle = true;
                std::cout << "\033[33m" << c;
            } else if (c == '>') {
                in_angle = false;
                std::cout << c << "\033[0m";
            } else if (in_angle) {
                std::cout << c;
            } else {
                std::cout << c;
            }
        }
        
        if (in_string) std::cout << "\033[0m";
        if (in_angle) std::cout << "\033[0m";
    }
    
    void drw() {
        if (show_guide) {
            gd();
            return;
        }
        
        if (show_credits) {
            crd();
            return;
        }
        
        sz();
        clr();
        
        char mod_indicator = modified ? '*' : ' ';
        std::string mode_str = insert_mode ? "INS" : "OVR";
        
#ifdef _WIN32
        SetConsoleCursorPosition(hConsole, {0, 0});
#endif
        std::cout << "\033[1;36m~ SAC++: " << filename << " " << mod_indicator << " ~\033[0m\n";
        
        std::vector<std::string> display_lines;
        std::vector<int> line_mapping;
        
        for (int i = 0; i < (int)lines.size(); i++) {
            auto wrapped = wrap_line(lines[i], term_cols - 7);
            for (const auto& wrap : wrapped) {
                display_lines.push_back(wrap);
                line_mapping.push_back(i);
            }
        }
        
        int cursor_display_line = 0;
        int chars_before_cursor = 0;
        for (int i = 0; i < cursor_y; i++) {
            auto wrapped = wrap_line(lines[i], term_cols - 7);
            cursor_display_line += wrapped.size();
        }
        
        auto current_wrapped = wrap_line(lines[cursor_y], term_cols - 7);
        for (int i = 0; i < (int)current_wrapped.size(); i++) {
            if (chars_before_cursor + (int)current_wrapped[i].length() >= cursor_x) {
                cursor_display_line += i;
                break;
            }
            chars_before_cursor += current_wrapped[i].length();
        }
        
        if (cursor_display_line < top_line) {
            top_line = cursor_display_line;
        }
        if (cursor_display_line >= top_line + visible_lines) {
            top_line = cursor_display_line - visible_lines + 1;
        }
        if (top_line < 0) top_line = 0;
        if (top_line > (int)display_lines.size() - visible_lines) {
            top_line = std::max(0, (int)display_lines.size() - visible_lines);
        }
        
        int start_line = top_line;
        int end_line = std::min(start_line + visible_lines, (int)display_lines.size());
        
        for (int i = start_line; i < end_line; i++) {
            int original_line = (i < (int)line_mapping.size()) ? line_mapping[i] : 0;
            std::cout << "\033[0;37m" << std::setw(4) << (original_line + 1) << " |\033[0m ";
            
            if (i < (int)display_lines.size()) {
                highlight_line(display_lines[i], term_cols - 7);
            }
            std::cout << "\n";
        }
        
        for (int i = end_line; i < start_line + visible_lines; i++) {
            std::cout << "\033[1;37m" << std::setw(4) << (i + 1) << " |\033[0m\n";
        }
        
        scrbar();
        
#ifdef _WIN32
        SetConsoleCursorPosition(hConsole, {0, (SHORT)(term_rows-2)});
#else
        printf("\033[%d;1H", term_rows - 1);
#endif
        
        if (!status_msg.empty() && time(nullptr) - status_msg_time < 3) {
            std::cout << "\033[7m" << status_msg << "\033[0m";
        }
        
#ifdef _WIN32
        SetConsoleCursorPosition(hConsole, {0, (SHORT)(term_rows-1)});
#else
        printf("\033[%d;1H", term_rows);
#endif
        
        printf(STATUS_LINE, cursor_y + 1, cursor_x + 1, mode_str.c_str());
        
        int display_line = cursor_display_line - top_line + 2;
        int display_col = cursor_x - chars_before_cursor + 8;
        
        if (display_line >= 2 && display_line <= visible_lines + 1) {
#ifdef _WIN32
            SetConsoleCursorPosition(hConsole, {(SHORT)(display_col), (SHORT)(display_line-1)});
#else
            printf("\033[%d;%dH", display_line, display_col);
#endif
        }
        
        std::cout.flush();
    }
    
    void sav() {
        std::ofstream file(filename);
        if (!file.is_open()) {
            msg("Error: Cannot save file!");
            return;
        }
        
        for (size_t i = 0; i < lines.size(); i++) {
            file << lines[i];
            if (i < lines.size() - 1) file << "\n";
        }
        
        file.close();
        msg("File saved! (" + std::to_string(lines.size()) + " lines)");
        modified = false;
    }
    
    bool file_exists(const std::string& fname) {
#ifdef _WIN32
        DWORD attr = GetFileAttributesA(fname.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat buffer;
        return (stat(fname.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
    }
    
    void open_file(const std::string& fname) {
        if (!file_exists(fname)) {
            msg("File does not exist, creating new file");
            filename = fname;
            lines.clear();
            lines.push_back("");
            cursor_x = cursor_y = top_line = 0;
            modified = false;
            return;
        }
        
        std::ifstream file(fname);
        if (!file.is_open()) {
            msg("Error: Cannot open file - permission denied");
            return;
        }
        
        lines.clear();
        lines.reserve(1000);
        std::string line;
        while (std::getline(file, line)) {
            if (line.length() > MAX_LINE_LENGTH) {
                line = line.substr(0, MAX_LINE_LENGTH);
            }
            lines.push_back(std::move(line));
        }
        
        if (lines.empty()) {
            lines.push_back("");
        }
        
        file.close();
        filename = fname;
        cursor_x = cursor_y = top_line = 0;
        modified = false;
        msg("File loaded: " + fname);
    }
    
    void adj() {
        std::vector<std::string> display_lines;
        for (const auto& line : lines) {
            auto wrapped = wrap_line(line, term_cols - 7);
            display_lines.insert(display_lines.end(), wrapped.begin(), wrapped.end());
        }
        
        int cursor_display_line = 0;
        for (int i = 0; i < cursor_y; i++) {
            auto wrapped = wrap_line(lines[i], term_cols - 7);
            cursor_display_line += wrapped.size();
        }
        
        auto current_wrapped = wrap_line(lines[cursor_y], term_cols - 7);
        int chars_before_cursor = 0;
        for (int i = 0; i < (int)current_wrapped.size(); i++) {
            if (chars_before_cursor + (int)current_wrapped[i].length() >= cursor_x) {
                cursor_display_line += i;
                break;
            }
            chars_before_cursor += current_wrapped[i].length();
        }
        
        if (cursor_display_line < top_line) {
            top_line = cursor_display_line;
        }
        if (cursor_display_line >= top_line + visible_lines) {
            top_line = cursor_display_line - visible_lines + 1;
        }
        if (top_line < 0) top_line = 0;
        if (top_line > (int)display_lines.size() - visible_lines) {
            top_line = std::max(0, (int)display_lines.size() - visible_lines);
        }
    }
    
    void find_text() {
        msg("Find: ");
        drw();
        
        std::string search_term;
        char ch;
        bool escape = false;
        
        while (true) {
            ch = get_char();
            if (ch == 27) {
                escape = true;
                break;
            }
            if (ch == '\r' || ch == '\n') break;
            if (ch == 127 || ch == 8) {
                if (!search_term.empty()) {
                    search_term.pop_back();
                }
            } else if (ch >= 32 && ch <= 126) {
                search_term += ch;
            }
            msg("Find: " + search_term);
            drw();
        }
        
        if (escape) {
            msg("Find cancelled");
            return;
        }
        
        if (search_term.empty()) {
            msg("No search term entered");
            return;
        }
        
        find_term = search_term;
        int start_line = (find_line == cursor_y && find_col < cursor_x) ? cursor_y : cursor_y;
        int start_col = (find_line == cursor_y && find_col < cursor_x) ? cursor_x + 1 : cursor_x;
        
        for (int i = start_line; i < (int)lines.size(); i++) {
            size_t pos = lines[i].find(search_term, (i == start_line) ? start_col : 0);
            if (pos != std::string::npos) {
                cursor_y = i;
                cursor_x = pos;
                find_line = i;
                find_col = pos;
                adj();
                msg("Found: " + search_term);
                return;
            }
        }
        
        for (int i = 0; i < start_line; i++) {
            size_t pos = lines[i].find(search_term);
            if (pos != std::string::npos) {
                cursor_y = i;
                cursor_x = pos;
                find_line = i;
                find_col = pos;
                adj();
                msg("Found: " + search_term + " (wrapped)");
                return;
            }
        }
        
        msg("Not found: " + search_term);
        find_line = find_col = -1;
    }
    
    void goto_line() {
        msg("Go to line: ");
        drw();
        
        std::string line_num;
        char ch;
        while (true) {
            ch = get_char();
            if (ch == '\r' || ch == '\n') break;
            if (ch == 27) {
                msg("Goto cancelled");
                return;
            }
            if (ch == 127 || ch == 8) {
                if (!line_num.empty()) {
                    line_num.pop_back();
                }
            } else if (ch >= '0' && ch <= '9') {
                line_num += ch;
            }
            msg("Go to line: " + line_num);
            drw();
        }
        
        if (!line_num.empty()) {
            try {
                int target = std::stoi(line_num) - 1;
                if (target >= 0 && target < (int)lines.size()) {
                    cursor_y = target;
                    cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
                    adj();
                    msg("Moved to line " + line_num);
                } else {
                    msg("Line number out of range (1-" + std::to_string(lines.size()) + ")");
                }
            } catch (const std::exception&) {
                msg("Invalid line number");
            }
        }
    }
    
    char get_char() {
#ifdef _WIN32
        return _getch();
#else
        char ch;
        read(STDIN_FILENO, &ch, 1);
        return ch;
#endif
    }
    
    void inp() {
        char ch = get_char();
        
        if (show_guide || show_credits) {
            show_guide = show_credits = false;
            return;
        }
        
        if (ch == 27) {
            char seq[3];
#ifdef _WIN32
            if (_kbhit()) {
                seq[0] = _getch();
                if (seq[0] == '[' && _kbhit()) {
                    seq[1] = _getch();
#else
            if (read(STDIN_FILENO, seq, 2) == 2 && seq[0] == '[') {
#endif
                    switch (seq[1]) {
                        case 'A':
                            if (cursor_y > 0) {
                                cursor_y--;
                                cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
                            }
                            break;
                        case 'B':
                            if (cursor_y < (int)lines.size() - 1) {
                                cursor_y++;
                                cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
                            }
                            break;
                        case 'C':
                            if (cursor_x < (int)lines[cursor_y].length()) {
                                cursor_x++;
                            } else if (cursor_y < (int)lines.size() - 1) {
                                cursor_y++;
                                cursor_x = 0;
                            }
                            break;
                        case 'D':
                            if (cursor_x > 0) {
                                cursor_x--;
                            } else if (cursor_y > 0) {
                                cursor_y--;
                                cursor_x = lines[cursor_y].length();
                            }
                            break;
                    }
                    adj();
#ifdef _WIN32
                }
            }
#else
            }
#endif
        } else if (ch == 127 || ch == 8) {
            if (cursor_x > 0) {
                save_state();
                lines[cursor_y].erase(cursor_x - 1, 1);
                cursor_x--;
                modified = true;
            } else if (cursor_y > 0) {
                save_state();
                cursor_x = lines[cursor_y - 1].length();
                lines[cursor_y - 1] += lines[cursor_y];
                lines.erase(lines.begin() + cursor_y);
                cursor_y--;
                modified = true;
            }
        } else if (ch == '\n' || ch == '\r') {
            save_state();
            std::string rest = lines[cursor_y].substr(cursor_x);
            lines[cursor_y] = lines[cursor_y].substr(0, cursor_x);
            lines.insert(lines.begin() + cursor_y + 1, rest);
            cursor_y++;
            cursor_x = 0;
            modified = true;
        } else if (ch == 17) {
            if (modified) {
                msg("Warning: Unsaved changes! Press Ctrl+Q again to exit.");
                static time_t last_quit = 0;
                if (time(nullptr) - last_quit < 2) {
                    running = false;
                }
                last_quit = time(nullptr);
            } else {
                running = false;
            }
        } else if (ch == 19) {
            sav();
        } else if (ch == 7) {
            show_guide = true;
        } else if (ch == 14) {
            show_credits = true;
        } else if (ch == 21) {
            undo();
        } else if (ch == 6) {
            find_text();
        } else if (ch == 12) {
            goto_line();
        } else if (ch == 24) {
            if (cursor_y < (int)lines.size()) {
                save_state();
                clipboard = lines[cursor_y];
                lines.erase(lines.begin() + cursor_y);
                if (lines.empty()) lines.push_back("");
                if (cursor_y >= (int)lines.size()) cursor_y = lines.size() - 1;
                cursor_x = 0;
                modified = true;
                msg("Line cut to clipboard");
            }
        } else if (ch == 4) {
            if (!lines.empty() && cursor_y < (int)lines.size()) {
                save_state();
                lines.erase(lines.begin() + cursor_y);
                if (lines.empty()) lines.push_back("");
                if (cursor_y >= (int)lines.size()) cursor_y = lines.size() - 1;
                cursor_x = 0;
                modified = true;
                msg("Line deleted");
            }
        } else if (ch == 9) {
            insert_mode = !insert_mode;
            msg(insert_mode ? "Insert mode" : "Overwrite mode");
        } else if (ch >= 32 && ch <= 126) {
            if (lines[cursor_y].length() < MAX_LINE_LENGTH - 1) {
                save_state();
                if (insert_mode) {
                    lines[cursor_y].insert(cursor_x, 1, ch);
                } else {
                    if (cursor_x < (int)lines[cursor_y].length()) {
                        lines[cursor_y][cursor_x] = ch;
                    } else {
                        lines[cursor_y] += ch;
                    }
                }
                cursor_x++;
                modified = true;
            }
        }
        
        adj();
    }
    
    void run() {
        while (running) {
            drw();
            inp();
        }
    }
    
    void load_file(const std::string& fname) {
        filename = fname;
        open_file(fname);
    }
};

int main(int argc, char* argv[]) {
    try {
        Editor editor;
        
        if (argc > 1) {
            editor.load_file(argv[1]);
        }
        
        editor.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}