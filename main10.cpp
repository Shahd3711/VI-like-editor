#include "editor.h"
#include <iostream>

int main(int argc, char** argv) 
{
    Editor ed;
    if (argc > 1) {
        ed.run(argv[1]);
    } else {
        ed.run();
    }
    return 0;
}

/*

NORMAL  h, l, j, k Movement Move cursor Left, Right, Down, Up.

NORMAL 0 or ^ Movement Move to the Beginning of the Line (BOL).

NORMAL $ Movement Move to the End of the Line (EOL).

NORMAL gg Movement Move to the Beginning of the File (BOF).

NORMAL G Movement Move to the End of the File (EOF).

NORMAL i, a, A, o, O Insert Enter INSERT Mode at various positions.

NORMAL x Editing Delete the character under the cursor.

NORMAL dd Editing Delete the current line (Cut).

NORMAL yy Editing Yank (Copy) the current line.

NORMAL p Editing Paste the yanked line(s) below the current line.

NORMAL u Utility Single-level Undo (revert last change).

NORMAL : Mode Switch Enter COMMAND Mode.

NORMAL / Mode Switch Enter SEARCH Mode.

INSERT ESC Mode Switch Exit INSERT Mode to NORMAL Mode.

INSERT Enter (\n) Editing Insert a new line.

INSERT Backspace Editing Delete preceding character / Join lines.

COMMAND :w [filename] File Ops Save the file (Use filename for 'Save As').

COMMAND :q File Ops Quit the editor.

COMMAND :wq or :x File Ops Save and Quit.

SEARCH / <pattern> Search Search for the specified pattern (wraps around).

/*