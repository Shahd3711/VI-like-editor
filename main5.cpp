#include "menu.h"
#include <iostream>
#include <ncurses.h>

using namespace std;

int main() {
    init_ncurses();

    bool running = true;
    
    while (running) {
        display_menu();
        curs_set(1); 
        char input_ch = getch();
        curs_set(0); 

        if (input_ch == '1') {
            handle_new_task();
        } else if (input_ch == '2') {
            handle_display_task();
        } else if (input_ch == '3') {
            running = false;
        } else {
            clear();
            printw("Invalid choice. Press any key to try again...");
            refresh();
            getch();
        }
    }

    end_ncurses();
    cout << "Editor Exited. Goodbye!" << endl;
    return 0;
}

//g++ -Wall -Wextra -std=c++17 main5.cpp menu.cpp -o main5 -lncurses
//./main5