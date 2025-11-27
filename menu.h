#ifndef MENU_H
#define MENU_H
#include <cstddef>
#define MAX_LINE_LENGTH 100
#define MAX_LINES 50
extern char textBuffer[MAX_LINES][MAX_LINE_LENGTH + 1]; 
extern size_t bufferSizeLimit; 
extern size_t cursor_x;
extern size_t cursor_y;
extern size_t current_lines;
void init_ncurses();
void end_ncurses();
void display_menu();
void handle_new_task();
void handle_display_task();
void run_editor();
bool save_buffer_to_file(const char* filename, bool appendMode);
#endif