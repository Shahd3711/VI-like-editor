#include "editor.h"
#include <ncurses.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <thread>
#include <cctype>

using namespace std;

Editor::Editor()
    : cy(0), cx(0), top_line(0), mode(MODE_NORMAL),
      status_msg("Welcome to mini-vi (press i to insert, :w to save, :q to quit)"),
      undo_cx(0), undo_cy(0) {
    buf.clear();
    buf.push_back(std::string());
}

Editor::~Editor() {
    end_ncurses();
}

void Editor::init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    start_color();
    use_default_colors();
}

void Editor::end_ncurses() {
    if (isendwin() == FALSE) {
        curs_set(1);
        endwin();
    }
}

void Editor::run(const string& fname) {
    init_ncurses();
    if (!fname.empty()) {
        open_file(fname);
    }
    draw();

    int ch;
    while (true) {
        draw();
        // The getch() function is used to wait for user input
        ch = getch(); 

        // Handle different modes
        if (mode == MODE_NORMAL) {
            handle_normal(ch);
        } else if (mode == MODE_INSERT) {
            handle_insert(ch);
        } else if (mode == MODE_COMMAND) {
            handle_command();
        } else if (mode == MODE_SEARCH) {
            handle_search();
        }
        // The quit check is handled within command mode (:q)
    }
}

// File operations
void Editor::open_file(const string& fname) {
    ifstream f(fname);
    if (!f.is_open()) {
        // If file doesn't exist or cannot be opened for reading, start with an empty buffer
        set_status("File not found, starting new file: " + fname);
        filename = fname;
        buf.clear();
        buf.push_back(string());
        cy = cx = top_line = 0;
        return;
    }
    buf.clear();
    string line;
    // Load file content into buffer
    while (getline(f, line)) {
        if (line.size() > MAX_LINE_LEN) line = line.substr(0, MAX_LINE_LEN);
        buf.push_back(line);
        if (buf.size() >= MAX_LINES) break;
    }
    if (buf.empty()) buf.push_back(string());
    filename = fname;
    cy = cx = top_line = 0;
    set_status("Opened: " + fname + " (" + to_string(buf.size()) + " lines)");
}

bool Editor::save_file(const string& fname) {
    ofstream f(fname);
    if (!f.is_open()) {
        set_status("Error: cannot write to " + fname);
        return false;
    }
    // Write all lines to the file
    for (size_t i = 0; i < buf.size(); ++i) {
        f << buf[i];
        if (i + 1 < buf.size()) f << '\n';
    }
    filename = fname;
    set_status("Saved: " + fname + " (" + to_string(buf.size()) + " lines)");
    return true;
}

// Drawing
void Editor::draw() {
    clear();
    draw_buffer();
    draw_status();
    refresh();
}

void Editor::draw_buffer() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    // leave one row for status
    int avail = rows - 1;
    // Scroll view if cursor moves out of range
    center_view_on_cursor();

    for (int i = 0; i < avail; ++i) {
        size_t line_no = top_line + i;
        move(i, 0);
        clrtoeol(); // Clear to end of line for safety
        if (line_no < buf.size()) {
            // print line number with 4 width and space
            char lnbuf[16];
            snprintf(lnbuf, sizeof(lnbuf), "%4zu ", line_no + 1);
            addstr(lnbuf);
            
            string disp = buf[line_no];
            // clip buffer content to screen width minus line number space (5 chars)
            int maxchars = cols - 5;
            if ((int)disp.size() > maxchars) disp = disp.substr(0, maxchars);
            addnstr(disp.c_str(), maxchars);
        } else {
            // Draw tildes (~) for empty lines beyond buffer end
            addstr("~");
        }
    }
    
    // position cursor relative to screen
    int screen_y = (int)(cy - top_line);
    int screen_x = (int)(cx + 5); // accounting for "#### "
    if (screen_y >= 0 && screen_y < avail) {
        move(screen_y, screen_x);
    } else {
        // Hide cursor if out of visible area
        move(rows - 1, cols - 1); 
    }
}

void Editor::draw_status() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    move(rows - 1, 0);
    clrtoeol();
    
    // Format mode string
    string mode_str;
    if (mode == MODE_INSERT) mode_str = "-- INSERT --";
    else if (mode == MODE_NORMAL) mode_str = "-- NORMAL --";
    else if (mode == MODE_COMMAND) mode_str = "-- COMMAND --";
    else mode_str = "-- SEARCH --";
    
    string filepart = filename.empty() ? "[No Name]" : filename;
    
    // Format position string
    char posbuf[64];
    snprintf(posbuf, sizeof(posbuf), " Ln %zu/%zu, Col %zu ", cy + 1, buf.size(), cx + 1);
    
    string status = mode_str + " | " + filepart + posbuf;
    
    // Print status bar content
    addnstr(status.c_str(), min((int)status.size(), cols - 1));
    
    // Print message at right, prioritizing status content
    int msg_start_col = max(0, cols - (int)status_msg.size() - 1);
    if (msg_start_col > (int)status.size()) {
        move(rows - 1, msg_start_col);
        addnstr(status_msg.c_str(), cols - msg_start_col - 1);
    }
}

// Input handlers
void Editor::handle_normal(int ch) {
    switch (ch) {
        case 'i': cmd_i(); break;
        case 'a': cmd_a(); break;
        case 'A': cmd_A(); break;
        case 'o': cmd_o(); break;
        case 'O': cmd_O(); break;
        
        // Deletion
        case 'x': cmd_x(); break;

        // Movements (h, j, k, l and arrow keys)
        case 'h': 
        case KEY_LEFT: cmd_move_left(); break;
        case 'j': 
        case KEY_DOWN: cmd_move_down(); break;
        case 'k': 
        case KEY_UP: cmd_move_up(); break;
        case 'l': 
        case KEY_RIGHT: cmd_move_right(); break;

        // Vi-like Movement Commands
        case '0':
        case '^': cmd_move_to_bol(); break;
        case '$': cmd_move_to_eol(); break;
        case 'G': cmd_move_to_eof(); break;
        
        // Multi-key commands
        case 'g': {
            int c2 = getch();
            if (c2 == 'g') cmd_move_to_bof();
            else { set_status("Unknown command g" + string(1,(char)c2)); }
            break;
        }
        case 'd': {
            int c2 = getch();
            if (c2 == 'd') cmd_dd();
            else { set_status("Unknown command d" + string(1,(char)c2)); }
            break;
        }
        case 'y': {
            int c2 = getch();
            if (c2 == 'y') cmd_yy();
            else { set_status("Unknown command y" + string(1,(char)c2)); }
            break;
        }

        // Undo and Paste
        case 'u': cmd_u(); break;
        case 'p': cmd_p(); break;

        // Modes
        case '/': mode = MODE_SEARCH; set_status("/ Search: "); break;
        case ':': mode = MODE_COMMAND; set_status(": Command: "); break;
        
        case KEY_BACKSPACE:
        case 127:
            // In normal mode, backspace typically moves left
            cmd_move_left();
            break;
        default:
            // Ignore unknown commands
            break;
    }
    ensure_cursor_in_bounds();
}

void Editor::handle_insert(int ch) {
    if (ch == 27) { // ESC
        mode = MODE_NORMAL;
        // Move cursor back one position after exiting insert mode (vi standard)
        if (cx > 0) cx--; 
        set_status("-- NORMAL --");
        ensure_cursor_in_bounds();
        return;
    }
    if (ch == KEY_BACKSPACE || ch == 127) {
        // backspace behavior
        if (cx > 0) {
            snapshot_undo();
            string &line = buf[cy];
            line.erase(cx - 1, 1);
            cx--;
        } else if (cy > 0) {
            // Join with previous line if at BOL
            snapshot_undo();
            size_t prevlen = buf[cy - 1].size();
            buf[cy - 1] += buf[cy];
            buf.erase(buf.begin() + cy);
            cy--;
            cx = prevlen;
        }
    } else if (ch == '\n' || ch == KEY_ENTER) {
        snapshot_undo();
        split_line_at_cursor();
    } else if (isprint(ch)) {
        snapshot_undo();
        insert_char((char)ch);
    }
    ensure_cursor_in_bounds();
}

void Editor::handle_command() {
    // Command input is blocking and happens inside prompt_command
    string cmdline = prompt_command(":");
    
    // Commands implementation
    if (cmdline.empty()) {
        // Do nothing if command is empty
    } else if (cmdline == "q") {
        end_ncurses();
        exit(0);
    } else if (cmdline == "w") {
        if (filename.empty()) {
            string fn = prompt_input("Filename: ");
            if (!fn.empty()) save_file(fn);
        } else {
            save_file(filename);
        }
    } else if (cmdline.rfind("w ", 0) == 0) {
        string fn = cmdline.substr(2);
        save_file(fn);
    } else if (cmdline == "wq" || cmdline == "x") {
        if (filename.empty()) {
            string fn = prompt_input("Filename: ");
            if (!fn.empty()) save_file(fn);
            end_ncurses();
            exit(0);
        } else {
            save_file(filename);
            end_ncurses();
            exit(0);
        }
    } else {
        set_status("Unknown command: " + cmdline);
    }
    
    // Always return to normal mode after command execution
    mode = MODE_NORMAL;
}

void Editor::handle_search() {
    // Search input is blocking and happens inside prompt_command
    std::string pattern = prompt_command("/");
    
    if (pattern.empty()) {
        mode = MODE_NORMAL;
        return;
    }
    // Start search from next character
    ssize_t found_line = find_next(pattern, cy, cx + 1); 
    if (found_line >= 0) {
        // move cursor to found occurrence
        cy = (size_t)found_line;
        size_t pos = buf[cy].find(pattern);
        if (pos != string::npos) cx = pos;
        set_status("Found: " + pattern);
    } else {
        set_status("Pattern not found: " + pattern);
    }
    mode = MODE_NORMAL;
}

// Commands implementations
void Editor::cmd_i() { mode = MODE_INSERT; set_status("-- INSERT --"); }
void Editor::cmd_a() {
    // move right if possible then insert
    if (cx < buf[cy].size()) cx++;
    mode = MODE_INSERT;
    set_status("-- INSERT --");
}
void Editor::cmd_A() {
    cx = buf[cy].size();
    mode = MODE_INSERT;
    set_status("-- INSERT --");
}
void Editor::cmd_o() {
    snapshot_undo();
    // Insert new line below and move cursor to it
    buf.insert(buf.begin() + cy + 1, string());
    cy++;
    cx = 0;
    mode = MODE_INSERT;
    set_status("-- INSERT --");
}
void Editor::cmd_O() {
    snapshot_undo();
    // Insert new line above and move cursor to it
    buf.insert(buf.begin() + cy, string());
    cx = 0;
    mode = MODE_INSERT;
    set_status("-- INSERT --");
}

void Editor::cmd_x() {
    if (is_buf_empty()) return;
    
    string &line = buf[cy];
    // Delete character under cursor if it exists
    if (cx < line.size()) {
        snapshot_undo();
        line.erase(cx, 1);
        set_status("Deleted char");
    }
    ensure_cursor_in_bounds();
}

void Editor::cmd_dd() {
    if (is_buf_empty()) return;
    
    snapshot_undo();
    yank_buffer.clear();
    yank_buffer.push_back(buf[cy]); // Save line to yank buffer
    
    buf.erase(buf.begin() + cy); // Delete the line
    
    if (buf.empty()) buf.push_back(string()); // Ensure buffer is never empty
    
    // Adjust cursor position
    if (cy >= buf.size()) cy = buf.size() - 1;
    cx = min(cx, buf[cy].size());
    set_status("Deleted line");
}

void Editor::cmd_yy() {
    if (is_buf_empty()) return;
    yank_buffer.clear();
    yank_buffer.push_back(buf[cy]);
    set_status("Yanked line");
}

void Editor::cmd_p() {
    if (yank_buffer.empty()) {
        set_status("Nothing to paste");
        return;
    }
    snapshot_undo();
    
    // Paste yanked lines as new lines after the current line (cy)
    // Inserts at cy + 1
    buf.insert(buf.begin() + cy + 1, yank_buffer.begin(), yank_buffer.end());
    
    // Move cursor to the first pasted line
    cy = cy + 1;
    cx = 0;
    set_status("Pasted");
}

void Editor::cmd_u() {
    if (undo_buf.empty()) {
        set_status("Nothing to undo");
        return;
    }
    buf = undo_buf;
    cx = undo_cx;
    cy = undo_cy;
    undo_buf.clear();
    set_status("Undo successful");
}

// Moves
void Editor::cmd_move_left() {
    if (cx > 0) cx--;
    else if (cy > 0) {
        // Move to the end of the previous line
        cy--;
        cx = buf[cy].size();
    }
}
void Editor::cmd_move_right() {
    if (cx < buf[cy].size()) cx++;
    else if (cy + 1 < buf.size()) {
        // Move to the beginning of the next line
        cy++;
        cx = 0;
    }
}
void Editor::cmd_move_up() {
    if (cy > 0) {
        cy--;
        // Maintain column position, but clip if line is shorter
        cx = min(cx, buf[cy].size());
    }
}
void Editor::cmd_move_down() {
    if (cy + 1 < buf.size()) {
        cy++;
        // Maintain column position, but clip if line is shorter
        cx = min(cx, buf[cy].size());
    }
}

void Editor::cmd_move_to_bol() {
    cx = 0; // Move to beginning of line
}

void Editor::cmd_move_to_eol() {
    cx = buf[cy].size(); // Move to position after last character
}

void Editor::cmd_move_to_bof() {
    cy = 0; // Move to the first line
    cx = 0; // Move to beginning of line
}

void Editor::cmd_move_to_eof() {
    if (buf.size() > 0) {
        cy = buf.size() - 1; // Move to the last line
        cx = 0; // Move to beginning of line (vi standard for 'G')
    }
}


// Editing primitives
void Editor::insert_char(char c) {
    if (buf[cy].size() < MAX_LINE_LEN) {
        buf[cy].insert(buf[cy].begin() + cx, c);
        cx++;
    } else {
        set_status("Line length limit reached (MAX_LINE_LEN)");
    }
}

void Editor::delete_char() {
    // delete_char is not directly used by current commands, but kept for completeness
    if (cx < buf[cy].size()) {
        buf[cy].erase(cx, 1);
    } else if (cy + 1 < buf.size()) {
        join_with_next_line();
    }
}

void Editor::split_line_at_cursor() {
    string rest = buf[cy].substr(cx);
    buf[cy].erase(cx);
    buf.insert(buf.begin() + cy + 1, rest);
    cy++;
    cx = 0;
}

void Editor::join_with_next_line() {
    if (cy + 1 < buf.size()) {
        buf[cy] += buf[cy + 1];
        buf.erase(buf.begin() + cy + 1);
    }
}

void Editor::ensure_cursor_in_bounds() {
    if (buf.empty()) {
        buf.push_back(string());
    }
    // Ensure row is in bounds
    if (cy >= buf.size()) cy = buf.size() - 1;
    // Ensure column is in bounds (can be up to size() which is one position past the last char)
    if (cx > buf[cy].size()) cx = buf[cy].size(); 
    
    if (buf.size() > MAX_LINES) {
        buf.resize(MAX_LINES);
        set_status("Truncated buffer to MAX_LINES");
    }
}

void Editor::set_status(const string& msg) {
    status_msg = msg;
}

void Editor::snapshot_undo() {
    undo_buf = buf;
    undo_cx = cx;
    undo_cy = cy;
}

// utils
bool Editor::is_buf_empty() {
    return buf.empty() || (buf.size() == 1 && buf[0].empty());
}

void Editor::center_view_on_cursor() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int avail = rows - 1; // Available lines for buffer display
    
    // Scroll up if cursor is above the visible window
    if (cy < top_line) {
        top_line = cy;
    } 
    // Scroll down if cursor is below the visible window
    else if (cy >= top_line + (size_t)avail) {
        top_line = cy - avail + 1;
    }
    // Optimization: keep cursor somewhat centered vertically
    // If the top_line is not 0, try to move the view up to center the cursor
    if (cy > (size_t)(avail / 2) && cy < buf.size() - (size_t)(avail / 2)) {
        top_line = cy - avail / 2;
    } else if (cy <= (size_t)(avail / 2)) {
        top_line = 0;
    } else if (cy >= buf.size() - (size_t)(avail / 2) && buf.size() > (size_t)avail) {
        top_line = buf.size() - avail;
    }
}

ssize_t Editor::find_next(const std::string& pattern, size_t start_line, size_t start_col) {
    if (pattern.empty()) return -1;
    
    // Search from current position to end of file
    for (size_t i = start_line; i < buf.size(); ++i) {
        size_t pos = 0;
        if (i == start_line) {
            // Start search from current column + 1 on the current line
            pos = buf[i].find(pattern, start_col); 
        } else {
            pos = buf[i].find(pattern);
        }
        if (pos != string::npos) {
            // Update cx to the start of the match
            cx = pos; 
            return (ssize_t)i;
        }
    }
    
    // Wrap-around search from BOF to starting line
    for (size_t i = 0; i < start_line; ++i) {
        size_t pos = buf[i].find(pattern);
        if (pos != string::npos) {
            // Update cx to the start of the match
            cx = pos;
            return (ssize_t)i;
        }
    }
    return -1;
}

string Editor::prompt_command(const string& prompt) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    echo(); // Enable echoing characters to screen
    curs_set(1); // Ensure cursor is visible
    move(rows - 1, 0); // Move to status line
    clrtoeol(); // Clear status line
    addstr(prompt.c_str());
    
    char input[1024];
    // Read user input
    getnstr(input, sizeof(input) - 1); 
    
    noecho(); // Disable echoing
    curs_set(1); // Ensure cursor is visible
    
    return string(input);
}

string Editor::prompt_input(const string& prompt) {
    return prompt_command(prompt);
}