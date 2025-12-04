#ifndef EDITOR_H
#define EDITOR_H

#include <string>
#include <vector>
#include <iostream> // Needed for size_t

enum Mode { MODE_NORMAL, MODE_INSERT, MODE_COMMAND, MODE_SEARCH };

class Editor {
public:
    Editor();
    ~Editor();

    // main entry
    void run(const std::string& filename = "");

private:
    // buffer
    std::vector<std::string> buf;
    std::string filename;
    // cursor (row, col)
    size_t cy;
    size_t cx;

    // view offset (top line shown)
    size_t top_line;

    // status/message
    std::string status_msg;

    // modes
    Mode mode;

    // yank/cut buffer (lines)
    std::vector<std::string> yank_buffer;

    // single-level undo snapshot
    std::vector<std::string> undo_buf;
    size_t undo_cx, undo_cy;

    // helper limits
    const size_t MAX_LINE_LEN = 1024;
    const size_t MAX_LINES = 10000;

    // core
    void init_ncurses();
    void end_ncurses();
    void open_file(const std::string& fname);
    bool save_file(const std::string& fname);
    void draw();
    void draw_status();
    void draw_buffer();

    // input handlers
    void handle_normal(int ch);
    void handle_insert(int ch);
    void handle_command();
    void handle_search();

    // commands
    void cmd_i(); // insert before cursor
    void cmd_a(); // append at cursor
    void cmd_A(); // append end of line
    void cmd_o(); // open new line below
    void cmd_O(); // open new line above
    void cmd_x(); // delete char under cursor
    void cmd_dd(); // delete current line
    void cmd_yy(); // yank current line
    void cmd_p(); // paste after cursor/line
    void cmd_u(); // undo (single)
    
    // Movement commands
    void cmd_move_left();
    void cmd_move_right();
    void cmd_move_up();
    void cmd_move_down();
    void cmd_move_to_bol(); // 0 or ^
    void cmd_move_to_eol(); // $
    void cmd_move_to_bof(); // gg
    void cmd_move_to_eof(); // G

    // editing primitives
    void insert_char(char c);
    void delete_char();
    void split_line_at_cursor();
    void join_with_next_line();
    void ensure_cursor_in_bounds();

    // utils
    void set_status(const std::string& msg);
    void snapshot_undo();
    bool is_buf_empty();
    void center_view_on_cursor();
    ssize_t find_next(const std::string& pattern, size_t start_line, size_t start_col);

    // command-line helpers
    std::string prompt_command(const std::string& prompt);
    std::string prompt_input(const std::string& prompt);
};

#endif // EDITOR_H