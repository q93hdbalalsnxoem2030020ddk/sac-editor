   /*

            Copyright (c) 2025 Sunshine Juhari

     Permission is hereby granted, free of charge, to any person obtaining a copy 
     of this software and associated documentation files (the "Software"), to deal 
     in the Software without restriction, including without limitation the rights 
     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
     copies of the Software, and to permit persons to whom the Software is 
     furnished to do so, subject to the following conditions:

         1. The above copyright notice and this permission notice shall be included 
            in all copies or substantial portions of the Software.

         2. The Software is provided "as is", without warranty of any kind, express 
            or implied, including but not limited to the warranties of merchantability, 
            fitness for a particular purpose, and noninfringement. In no event shall 
            the authors or copyright holders be liable for any claim, damages, or 
            other liability, whether in an action of contract, tort, or otherwise, 
            arising from, out of, or in connection with the Software or the use or 
            other dealings in the Software.

    Author: Sunshine Juhari (sxc_qq1)
    Main Email: sunshinejhr@outlook.com
    Support Email: SAC-service@outlook.com
  */


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

#define MAX_LINE_LENGTH 34600
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
        std::cout << "  \033[1;32mFeedback or Support:\033[0m \033[1;36mSAC-service@outlook.com\033[0m\n\n";
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
	    bool in_comment = false;
	    bool in_single_comment = false;
	    bool in_multi_comment = false;
	    bool in_char = false;bool in_regex = false;
	    bool in_template_string = false;
	    bool in_raw_string = false;
	    bool in_doc_comment = false;
	    char string_char = 0;
	    
	    std::vector<std::string> keywords = {
	        "if", "else", "elif", "for", "while", "do", "switch", "case", "default", "break", "continue",
	        "return", "void", "int", "char", "float", "double", "long", "short", "bool", "true", "false",
	        "const", "static", "extern", "auto", "register", "volatile", "signed", "unsigned", "mutable",
	        "struct", "union", "enum", "typedef", "class", "public", "private", "protected", "operator",
	        "virtual", "inline", "friend", "template", "namespace", "using", "this", "new", "delete",
	        "try", "catch", "throw", "nullptr", "override", "final", "constexpr", "decltype", "noexcept",
	        "function", "var", "let", "const", "async", "await", "import", "export", "from", "default",
	        "def", "class", "self", "import", "from", "as", "lambda", "yield", "with", "global", "nonlocal",
	        "pass", "raise", "assert", "del", "is", "in", "not", "and", "or", "None", "True", "False",
	        "package", "main", "func", "go", "defer", "chan", "select", "interface", "type", "var",
	        "map", "range", "make", "len", "cap", "append", "copy", "close", "goroutine", "fallthrough",
	        "public", "static", "void", "main", "String", "System", "out", "println", "print", "class",
	        "extends", "implements", "super", "abstract", "final", "synchronized", "throws", "native",
	        "fn", "let", "mut", "match", "impl", "trait", "mod", "use", "crate", "pub", "where",
	        "Some", "None", "Ok", "Err", "Vec", "HashMap", "BTreeMap", "Option", "Result", "Box",
	        "require", "module", "exports", "process", "console", "window", "document", "typeof",
	        "instanceof", "prototype", "constructor", "super", "extends", "implements", "interface",
	        "macro", "quote", "unquote", "splice", "gensym", "defmacro", "eval", "apply", "quote",
	        "begin", "cond", "define", "lambda", "quote", "set!", "if", "unless", "when", "case",
	        "SELECT", "FROM", "WHERE", "JOIN", "INNER", "LEFT", "RIGHT", "FULL", "OUTER", "ON",
	        "INSERT", "UPDATE", "DELETE", "CREATE", "ALTER", "DROP", "TABLE", "DATABASE", "INDEX",
	        "PRIMARY", "KEY", "FOREIGN", "REFERENCES", "UNIQUE", "NOT", "NULL", "DEFAULT", "CHECK",
	        "GRANT", "REVOKE", "COMMIT", "ROLLBACK", "TRANSACTION", "BEGIN", "END", "DECLARE",
	        "PROCEDURE", "FUNCTION", "TRIGGER", "VIEW", "CURSOR", "LOOP", "WHILE", "FOR", "IF"
	    };
	    
	    std::vector<std::string> types = {
	        "std::string", "std::vector", "std::map", "std::set", "std::pair", "std::unique_ptr",
	        "std::shared_ptr", "std::weak_ptr", "std::array", "std::deque", "std::list", "std::queue",
	        "std::stack", "std::priority_queue", "std::unordered_map", "std::unordered_set",
	        "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t",
	        "int64_t", "wchar_t", "char16_t", "char32_t", "ptrdiff_t", "intptr_t", "uintptr_t",
	        "string", "String", "Array", "Object", "Number", "Boolean", "Symbol", "BigInt", "undefined",
	        "null", "Promise", "RegExp", "Date", "Error", "Map", "Set", "WeakMap", "WeakSet",
	        "list", "dict", "tuple", "set", "frozenset", "bytearray", "bytes", "memoryview", "slice",
	        "[]int", "[]string", "[]byte", "map[string]int", "chan int", "interface{}", "struct{}",
	        "Integer", "Double", "Float", "Long", "Short", "Byte", "Character", "Boolean", "Object",
	        "ArrayList", "HashMap", "HashSet", "LinkedList", "Stack", "Queue", "Vector", "TreeMap",
	        "i8", "i16", "i32", "i64", "i128", "u8", "u16", "u32", "u64", "u128", "f32", "f64",
	        "usize", "isize", "str", "&str", "String", "Vec", "Box", "Rc", "Arc", "Cell", "RefCell",
	        "INT", "VARCHAR", "CHAR", "TEXT", "BLOB", "REAL", "NUMERIC", "DECIMAL", "BOOLEAN",
	        "DATE", "TIME", "TIMESTAMP", "DATETIME", "YEAR", "BINARY", "VARBINARY", "ENUM",
	        "JSON", "XML", "UUID", "SERIAL", "BIGSERIAL", "SMALLSERIAL", "MONEY", "INET", "CIDR"
	    };
	    
	    std::vector<std::string> constants = {
	        "NULL", "TRUE", "FALSE", "nullptr", "true", "false", "None", "True", "False", "nil",
	        "undefined", "null", "NaN", "Infinity", "Math", "PI", "E", "SQRT2", "LN2", "LOG2E",
	        "MAX_VALUE", "MIN_VALUE", "POSITIVE_INFINITY", "NEGATIVE_INFINITY", "MAX_SAFE_INTEGER",
	        "MIN_SAFE_INTEGER", "EPSILON", "stdout", "stderr", "stdin", "EOF", "EXIT_SUCCESS",
	        "EXIT_FAILURE", "RAND_MAX", "CLOCKS_PER_SEC", "CHAR_BIT", "CHAR_MAX", "CHAR_MIN",
	        "INT_MAX", "INT_MIN", "LONG_MAX", "LONG_MIN", "SHRT_MAX", "SHRT_MIN", "UCHAR_MAX",
	        "UINT_MAX", "ULONG_MAX", "USHRT_MAX", "FLT_MAX", "FLT_MIN", "DBL_MAX", "DBL_MIN"
	    };
	    
	    std::vector<std::string> builtins = {
	        "printf", "scanf", "malloc", "free", "sizeof", "strlen", "strcpy", "strcat", "strcmp",
	        "memcpy", "memset", "memmove", "abs", "fabs", "sqrt", "pow", "sin", "cos", "tan",
	        "log", "exp", "ceil", "floor", "round", "min", "max", "swap", "sort", "reverse",
	        "print", "input", "len", "range", "enumerate", "zip", "map", "filter", "reduce",
	        "sorted", "reversed", "sum", "any", "all", "min", "max", "abs", "round", "pow",
	        "divmod", "bin", "oct", "hex", "ord", "chr", "str", "int", "float", "bool", "list",
	        "dict", "tuple", "set", "frozenset", "type", "isinstance", "issubclass", "hasattr",
	        "getattr", "setattr", "delattr", "dir", "vars", "globals", "locals", "eval", "exec",
	        "compile", "open", "close", "read", "write", "readline", "readlines", "writelines",
	        "fmt", "Println", "Printf", "Sprintf", "len", "make", "append", "copy", "delete",
	        "panic", "recover", "defer", "go", "select", "close", "cap", "new", "reflect",
	        "System", "out", "err", "in", "println", "print", "printf", "Scanner", "BufferedReader",
	        "FileReader", "FileWriter", "ArrayList", "HashMap", "HashSet", "Collections", "Arrays",
	        "String", "Integer", "Double", "Float", "Boolean", "Character", "Math", "Random",
	        "println!", "print!", "format!", "panic!", "assert!", "debug_assert!", "vec!",
	        "macro_rules!", "include!", "include_str!", "include_bytes!", "env!", "option_env!",
	        "concat!", "stringify!", "line!", "column!", "file!", "module_path!", "cfg!"
	    };
	    
	    std::vector<std::string> preprocessor = {
	        "#include", "#define", "#undef", "#ifdef", "#ifndef", "#if", "#else", "#elif",
	        "#endif", "#pragma", "#error", "#warning", "#line", "#import", "#using",
	        "@interface", "@implementation", "@protocol", "@property", "@synthesize",
	        "@dynamic", "@selector", "@encode", "@defs", "@synchronized", "@autoreleasepool",
	        "@try", "@catch", "@finally", "@throw", "@class", "@public", "@private", "@protected"
	    };
	    
	    for (size_t i = 0; i < line.length() && i < (size_t)max_width; i++) {
	        char c = line[i];
	        char next_c = (i + 1 < line.length()) ? line[i + 1] : '\0';
	        char next_next_c = (i + 2 < line.length()) ? line[i + 2] : '\0';
	        
	        if (!in_string && !in_char && !in_comment && !in_single_comment && !in_multi_comment && !in_regex && !in_template_string && !in_raw_string) {
	            if (c == '/' && next_c == '/') {
	                in_single_comment = true;
	                std::cout << "\033[38;5;240m" << c;
	                continue;
	            }
	            if (c == '/' && next_c == '*') {
	                if (next_next_c == '*') {
	                    in_doc_comment = true;
	                    std::cout << "\033[38;5;240m" << c;
	                } else {
	                    in_multi_comment = true;
	                    std::cout << "\033[38;5;240m" << c;
	                }
	                continue;
	            }
	            if (c == '#') {
	                std::string word;
	                size_t start = i;
	                while (i < line.length() && !std::isspace(line[i])) {
	                    word += line[i++];
	                }
	                i--;
	                
	                bool is_preprocessor = false;
	                for (const auto& prep : preprocessor) {
	                    if (word == prep) {
	                        is_preprocessor = true;
	                        break;
	                    }
	                }
	                
	                if (is_preprocessor || c == '#') {
	                    std::cout << "\033[38;5;123m" << word << "\033[0m";
	                } else {
	                    in_single_comment = true;
	                    std::cout << "\033[38;5;240m" << word;
	                }
	                continue;
	            }
	            if (c == '@' && std::isalpha(next_c)) {
	                std::string word = "@";
	                i++;
	                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) {
	                    word += line[i++];
	                }
	                i--;
	                std::cout << "\033[38;5;123m" << word << "\033[0m";
	                continue;
	            }
	        }
	        
	        if (in_single_comment) {
	            std::cout << c;
	            continue;
	        }
	        
	        if (in_multi_comment || in_doc_comment) {
	            std::cout << c;
	            if (c == '*' && next_c == '/') {
	                if (i + 1 < line.length()) {
	                    std::cout << line[++i];
	                }
	                in_multi_comment = false;
	                in_doc_comment = false;
	                std::cout << "\033[0m";
	            }
	            continue;
	        }
	        
	        if (!in_string && !in_char && !in_template_string && !in_raw_string && (c == '"' || c == '\'' || c == '`')) {
	            if (c == '`') {
	                in_template_string = true;
	                std::cout << "\033[38;5;43m" << c;
	            } else if (c == '"') {
	                if (i > 0 && line[i-1] == 'R') {
	                    in_raw_string = true;
	                    std::cout << "\033[38;5;43m" << c;
	                } else {
	                    in_string = true;
	                    std::cout << "\033[38;5;43m" << c;
	                }
	            } else {
	                in_char = true;
	                std::cout << "\033[38;5;43m" << c;
	            }
	            string_char = c;
	        } else if ((in_string && c == '"') || (in_char && c == '\'') || (in_template_string && c == '`')) {
	            if (i == 0 || line[i-1] != '\\') {
	                in_string = false;
	                in_char = false;
	                in_template_string = false;
	                std::cout << c << "\033[0m";
	            } else {
	                std::cout << c;
	            }
	        } else if (in_raw_string && c == ')' && i + 1 < line.length() && line[i+1] == '"') {
	            in_raw_string = false;
	            std::cout << c << line[++i] << "\033[0m";
	        } else if (in_string || in_char || in_template_string || in_raw_string) {
	            if (c == '\\' && next_c != '\0') {
	                std::cout << "\033[38;5;208m" << c << line[++i] << "\033[38;5;43m";
	            } else if (in_template_string && c == '$' && next_c == '{') {
	                std::cout << "\033[38;5;208m" << c << line[++i] << "\033[0m";
	                int brace_count = 1;
	                while (++i < line.length() && brace_count > 0) {
	                    if (line[i] == '{') brace_count++;
	                    else if (line[i] == '}') brace_count--;
	                    
	                    if (brace_count > 0) {
	                        std::cout << line[i];
	                    } else {
	                        std::cout << "\033[38;5;208m" << line[i] << "\033[38;5;43m";
	                    }
	                }
	            } else {
	                std::cout << c;
	            }
	        } else if (c == '/' && next_c != '/' && next_c != '*' && !in_string && !in_char) {
	            if (i > 0 && (line[i-1] == '=' || line[i-1] == '(' || line[i-1] == ',' || line[i-1] == ':' || line[i-1] == '[' || line[i-1] == '!' || line[i-1] == '&' || line[i-1] == '|' || line[i-1] == '?' || line[i-1] == '{' || line[i-1] == '}' || line[i-1] == ';' || line[i-1] == '\n')) {
	                in_regex = true;
	                std::cout << "\033[38;5;123m" << c;
	            } else {
	                std::cout << "\033[38;5;87m" << c << "\033[0m";
	            }
	        } else if (in_regex && c == '/' && (i == 0 || line[i-1] != '\\')) {
	            in_regex = false;
	            std::cout << c;
	            while (i + 1 < line.length() && (line[i+1] == 'g' || line[i+1] == 'i' || line[i+1] == 'm' || line[i+1] == 's' || line[i+1] == 'u' || line[i+1] == 'y')) {
	                std::cout << line[++i];
	            }
	            std::cout << "\033[0m";
	        } else if (in_regex) {
	            if (c == '\\' && next_c != '\0') {
	                std::cout << "\033[38;5;208m" << c << line[++i] << "\033[38;5;123m";
	            } else {
	                std::cout << c;
	            }
	        } else if (c == '<' && !in_string && !in_char) {
	            in_angle = true;
	            std::cout << "\033[38;5;87m" << c;
	        } else if (c == '>' && in_angle) {
	            in_angle = false;
	            std::cout << c << "\033[0m";
	        } else if (in_angle) {
	            std::cout << c;
	        } else if (std::isdigit(c) || (c == '.' && std::isdigit(next_c)) || (c == '0' && (next_c == 'x' || next_c == 'X' || next_c == 'b' || next_c == 'B'))) {
	            std::cout << "\033[38;5;220m";
	            if (c == '0' && (next_c == 'x' || next_c == 'X')) {
	                std::cout << c << line[++i];
	                while (i + 1 < line.length() && (std::isdigit(line[i+1]) || (line[i+1] >= 'a' && line[i+1] <= 'f') || (line[i+1] >= 'A' && line[i+1] <= 'F'))) {
	                    std::cout << line[++i];
	                }
	            } else if (c == '0' && (next_c == 'b' || next_c == 'B')) {
	                std::cout << c << line[++i];
	                while (i + 1 < line.length() && (line[i+1] == '0' || line[i+1] == '1')) {
	                    std::cout << line[++i];
	                }
	            } else {
	                while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.' || line[i] == 'e' || line[i] == 'E' || line[i] == 'f' || line[i] == 'F' || line[i] == 'L' || line[i] == 'l' || line[i] == 'u' || line[i] == 'U')) {
	                    std::cout << line[i++];
	                }
	                i--;
	            }
	            std::cout << "\033[0m";
	        } else if (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']') {
	            std::cout << "\033[38;5;245m" << c << "\033[0m";
	        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '?' || c == ':') {
	            if ((c == '+' && next_c == '+') || (c == '-' && next_c == '-') || (c == '=' && next_c == '=') || (c == '!' && next_c == '=') || (c == '<' && next_c == '=') || (c == '>' && next_c == '=') || (c == '&' && next_c == '&') || (c == '|' && next_c == '|') || (c == '<' && next_c == '<') || (c == '>' && next_c == '>') || (c == '+' && next_c == '=') || (c == '-' && next_c == '=') || (c == '*' && next_c == '=') || (c == '/' && next_c == '=') || (c == '%' && next_c == '=') || (c == '&' && next_c == '=') || (c == '|' && next_c == '=') || (c == '^' && next_c == '=')) {
	                std::cout << "\033[38;5;87m" << c << line[++i] << "\033[0m";
	            } else {
	                std::cout << "\033[38;5;87m" << c << "\033[0m";
	            }
	        } else if (c == ';' || c == ',' || c == '.') {
	            std::cout << "\033[38;5;245m" << c << "\033[0m";
	        } else if (c == '$' && (std::isalpha(next_c) || next_c == '_')) {
	            std::cout << "\033[38;5;208m" << c;
	            while (i + 1 < line.length() && (std::isalnum(line[i+1]) || line[i+1] == '_')) {
	                std::cout << line[++i];
	            }
	            std::cout << "\033[0m";
	        } else if (std::isalpha(c) || c == '_') {
	            std::string word;
	            size_t start = i;
	            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == ':')) {
	                word += line[i++];
	            }
	            i--;
	            
	            bool is_keyword = false;
	            bool is_type = false;
	            bool is_constant = false;
	            bool is_builtin = false;
	            
	            for (const auto& keyword : keywords) {
	                if (word == keyword) {
	                    is_keyword = true;
	                    break;
	                }
	            }
	            
	            if (!is_keyword) {
	                for (const auto& type : types) {
	                    if (word == type) {
	                        is_type = true;
	                        break;
	                    }
	                }
	            }
	            
	            if (!is_keyword && !is_type) {
	                for (const auto& constant : constants) {
	                    if (word == constant) {
	                        is_constant = true;
	                        break;
	                    }
	                }
	            }
	            
	            if (!is_keyword && !is_type && !is_constant) {
	                for (const auto& builtin : builtins) {
	                    if (word == builtin) {
	                        is_builtin = true;
	                        break;
	                    }
	                }
	            }
	            
	            if (is_keyword) {
	                std::cout << "\033[38;5;81m" << word << "\033[0m";
	            } else if (is_type) {
	                std::cout << "\033[38;5;44m" << word << "\033[0m";
	            } else if (is_constant) {
	                std::cout << "\033[38;5;208m" << word << "\033[0m";
	            } else if (is_builtin) {
	                std::cout << "\033[38;5;44m" << word << "\033[0m";
	            } else if (word[0] >= 'A' && word[0] <= 'Z') {
	                std::cout << "\033[38;5;44m" << word << "\033[0m";
	            } else if (next_c == '(' || (i + 1 < line.length() && line[i + 1] == '(')) {
	                std::cout << "\033[38;5;159m" << word << "\033[0m";
	            } else if (word.find("_") != std::string::npos && word == std::string(word.size(), std::toupper(word[0]))) {
	                std::cout << "\033[38;5;208m" << word << "\033[0m";
	            } else {
	                std::cout << "\033[38;5;252m" << word << "\033[0m";
	            }
	        } else {
	            std::cout << c;
	        }
	    }
	    
	    if (in_string || in_char || in_template_string || in_raw_string) std::cout << "\033[0m";
	    if (in_angle) std::cout << "\033[0m";
	    if (in_single_comment || in_multi_comment || in_doc_comment) std::cout << "\033[0m";
	    if (in_regex) std::cout << "\033[0m";
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
            lines = {""};
            cursor_x = cursor_y = top_line = 0;
            modified = false;
            return;
        }

        std::ifstream file(fname, std::ios::in | std::ios::binary);
        if (!file) {
            msg("Cannot open file - permission denied");
            return;
        }

        lines.clear();
        lines.reserve(4096);
        std::string buffer;
        buffer.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        size_t start = 0;
        for (size_t i = 0; i <= buffer.size(); ++i) {
            if (i == buffer.size() || buffer[i] == '\n') {
                std::string line = buffer.substr(start, i - start);
                if (line.length() > MAX_LINE_LENGTH)
                    line.resize(MAX_LINE_LENGTH);
                lines.push_back(std::move(line));
                start = i + 1;
            }
        }

        if (lines.empty())
        lines.push_back("");

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