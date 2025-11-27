#include "menu.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <ncurses.h>
using namespace std;
char textBuffer[MAX_LINES][MAX_LINE_LENGTH + 1] = {{'\0'}}; 
size_t bufferSizeLimit = 0; 
size_t cursor_x = 0;        
size_t cursor_y = 0;        
size_t current_lines = 1;
size_t get_total_size() {
    size_t total = 0;
    for (size_t i = 0; i < current_lines; ++i) {
        total += strlen(textBuffer[i]);
    }
    if (current_lines > 0) {
        total += current_lines - 1;
    }
    return total;
}
void init_ncurses() {
    initscr();              
    cbreak();               
    noecho();               
    keypad(stdscr, TRUE);   
    curs_set(0); 
}
void end_ncurses() {
    curs_set(1); 
    endwin(); 
}
void display_menu() {
    clear();
    printw("--- VI-Like Text Editor Menu (C-Style) ---\n");
    printw("1. new\n");
    printw("2. display\n");
    printw("3. exit\n");
    printw("Enter your choice (1, 2, or 3): ");
    refresh();
}
void draw_editor_screen() {
    clear();
    printw("--- Editor Mode (Limit: %zu) ---\n", bufferSizeLimit);
    printw("-----------------------------------\n");
    int current_line_num = 2; 
    for (size_t i = 0; i < current_lines; ++i) {
        move(current_line_num, 0);
        printw("%s", textBuffer[i]);
        current_line_num++;
    }
    move(cursor_y + 2, cursor_x); 
    curs_set(1); 
    refresh();
}
void run_editor() 
{
    int ch;
    textBuffer[0][0] = '\0';
    cursor_y = 0;
    cursor_x = 0;
    current_lines = 1;
    draw_editor_screen();
    while ((ch = getch()) != 27) { 
        char* currentLine = textBuffer[cursor_y];
        size_t lineLength = strlen(currentLine);
        size_t totalSize = get_total_size();
        if (ch >= 32 && ch <= 126 && totalSize < bufferSizeLimit && lineLength < MAX_LINE_LENGTH) 
        {
            memmove(currentLine + cursor_x + 1, currentLine + cursor_x, lineLength - cursor_x + 1);
            currentLine[cursor_x] = (char)ch;
            cursor_x++;
        }
        else if ((ch == '\n' || ch == KEY_ENTER) && current_lines < MAX_LINES)
        {
            if (totalSize + 1 > bufferSizeLimit)
                flash();
            else 
            {
                if (current_lines < MAX_LINES) 
                {
                    memmove(&textBuffer[cursor_y + 2], &textBuffer[cursor_y + 1], 
                            (current_lines - 1 - cursor_y) * (MAX_LINE_LENGTH + 1));
                    char restOfLine[MAX_LINE_LENGTH + 1];
                    strcpy(restOfLine, currentLine + cursor_x);
                    currentLine[cursor_x] = '\0';
                    strcpy(textBuffer[cursor_y + 1], restOfLine);
                    current_lines++;
                    cursor_y++;
                    cursor_x = 0;
                }
            }
        }
        else if (ch == KEY_UP) {
            if (cursor_y > 0) {
                cursor_y--;
                cursor_x = min(cursor_x, strlen(textBuffer[cursor_y]));
            }
        }
        else if (ch == KEY_DOWN) {
            if (cursor_y < current_lines - 1) {
                cursor_y++;
                cursor_x = min(cursor_x, strlen(textBuffer[cursor_y]));
            }
        }
        else if (ch == KEY_LEFT) {
            if (cursor_x > 0) {
                cursor_x--;
            } else if (cursor_y > 0) {
                cursor_y--;
                cursor_x = strlen(textBuffer[cursor_y]);
            }
        }
        else if (ch == KEY_RIGHT) {
            if (cursor_x < lineLength) {
                cursor_x++;
            } else if (cursor_y < current_lines - 1) {
                cursor_y++;
                cursor_x = 0;
            }
        }
        else if (ch == KEY_BACKSPACE || ch == 127) { 
            if (cursor_x > 0) {
                memmove(currentLine + cursor_x - 1, currentLine + cursor_x, lineLength - cursor_x + 1);
                cursor_x--;
            } else if (cursor_y > 0) {
                char* previousLine = textBuffer[cursor_y - 1];
                size_t prevLen = strlen(previousLine);
                if (prevLen + lineLength <= MAX_LINE_LENGTH) {
                    strcat(previousLine, currentLine);
                    cursor_x = prevLen;
                    memmove(&textBuffer[cursor_y], &textBuffer[cursor_y + 1], 
                            (current_lines - cursor_y) * (MAX_LINE_LENGTH + 1));
                    current_lines--;
                    cursor_y--;
                }
            }
        }
        else if (ch == KEY_DC) { 
            if (cursor_x < lineLength) {
                memmove(currentLine + cursor_x, currentLine + cursor_x + 1, lineLength - cursor_x);
            } else if (cursor_y < current_lines - 1) {
                char* nextLine = textBuffer[cursor_y + 1];
                size_t nextLen = strlen(nextLine);
                if (lineLength + nextLen <= MAX_LINE_LENGTH) {
                    strcat(currentLine, nextLine);
                    memmove(&textBuffer[cursor_y + 1], &textBuffer[cursor_y + 2], 
                            (current_lines - cursor_y - 1) * (MAX_LINE_LENGTH + 1));
                    current_lines--;
                }
            }
        }
        draw_editor_screen(); 
    }
    curs_set(0); 
    clear();
    printw("ESC Pressed! Save buffer (s), Discard (d)? ");
    refresh();
    char save_choice = getch();
    while (save_choice != 's' && save_choice != 'd' && save_choice != 'S' && save_choice != 'D') {
        save_choice = getch();
    }

    if (save_choice == 's' || save_choice == 'S') {
        clear();
        printw("Enter filename: ");
        refresh();
        echo(); 
        char filename_buffer[256];
        getstr(filename_buffer);
        noecho(); 
        clear();
        printw("Save mode: (a)ppend or (o)verwrite? ");
        refresh();
        char mode_choice = getch();
        while (mode_choice != 'a' && mode_choice != 'o') {
            mode_choice = getch();
        }

        bool saved = save_buffer_to_file(filename_buffer, mode_choice == 'a');

        clear();
        if (saved) {
            printw("Buffer successfully saved to %s.\n", filename_buffer);
        } else {
            printw("Error saving buffer to file.\n");
        }
        printw("Press any key to return to menu...");
        refresh();
        getch();

    } else { 
        textBuffer[0][0] = '\0';
        current_lines = 1;
        bufferSizeLimit = 0;
        cursor_x = 0;
        cursor_y = 0;
        clear();
        printw("Buffer discarded. Press any key to return to menu...");
        refresh();
        getch();
    }
}
void handle_new_task() {
    clear();
    printw("--- NEW TASK ---\n");
    printw("Enter maximum size of text (e.g., 500): ");
    refresh();

    echo(); 
    char size_str[10];
    getstr(size_str);
    noecho(); 

    try {
        bufferSizeLimit = stoul(size_str);
        if (bufferSizeLimit == 0) {
             throw runtime_error("Size must be positive.");
        }
        
        run_editor(); 

    } catch (...) {
        clear();
        printw("Invalid size. Press any key to return to menu...");
        refresh();
        getch();
    }
}

void handle_display_task() {
    clear();
    printw("--- DISPLAY TASK ---\n");

    if (get_total_size() > 0) {
        printw("Buffer content:\n");
        printw("-----------------------------------\n");
        for (size_t i = 0; i < current_lines; ++i) {
            printw("%s\n", textBuffer[i]);
        }
        printw("-----------------------------------\n");
    } else {
        printw("Buffer is empty.\n");
    }

    printw("Do you want to display a file? (y/n): ");
    refresh();
    
    char file_choice = getch();
    if (file_choice == 'y' || file_choice == 'Y') {
        clear();
        printw("Enter filename to display: ");
        refresh();
        echo(); 
        char filename_buffer[256];
        getstr(filename_buffer);
        noecho(); 

        ifstream file(filename_buffer);
        if (file.is_open()) {
            string line;
            printw("\nContent of %s:\n", filename_buffer);
            printw("-----------------------------------\n");
            while (getline(file, line)) {
                printw("%s\n", line.c_str());
            }
            printw("-----------------------------------\n");
            file.close();
        } else {
            printw("\nError: Could not open file %s.\n", filename_buffer);
        }
    }
    
    printw("Press any key to return to menu...");
    refresh();
    getch();
}


bool save_buffer_to_file(const char* filename, bool appendMode) {
    ios_base::openmode mode = ios::out;
    if (appendMode) {
        mode |= ios::app; 
    } else {
        mode |= ios::trunc; 
    }
    ofstream file(filename, mode); 
    if (file.is_open()) {
        for (size_t i = 0; i < current_lines; ++i) {
            file << textBuffer[i];
            if (i < current_lines - 1) {
                file << '\n';
            }
        }
        file.close();
        textBuffer[0][0] = '\0';
        current_lines = 1;
        bufferSizeLimit = 0;
        cursor_x = 0;
        cursor_y = 0;

        return true;
    }
    return false;
}