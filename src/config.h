/* Copyright 2025 Henryk Szenkowicz Holtman

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”,
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*

 _____ _              _
|_   _(_)_ __ ___  __| |
  | | | | '__/ _ \/ _` |
  | | | | | |  __/ (_| |
  |_| |_|_|  \___|\__,_|

Configuration file
*/

#include <ncurses.h>

#define LS_COMMAND "\\ls -F -l -h -a"

/* LS_COMMAND configuration:
 * You can change the custom flags for the 'ls' command output in the program
 * just keep in mind that it works best with the default flags on (or atleast -F and -l)
 * and requires a '\' (escaped) at the start to ignore any changes in your .bashrc or any shell you might have. */

#define IMAGE_VIEWER_COMMAND "gwenview %s"
#define VIDEO_PLAYER_COMMAND "mpv %s"

/* Custom commands configuration:
 * Change the command to open the file type */

#define ENTRIES_PER_PAGE 20

#define COLOR_DIRECTORY COLOR_BLUE
#define COLOR_EXECUTABLE COLOR_GREEN
#define COLOR_REGULAR COLOR_WHITE
#define COLOR_SYMLINK COLOR_CYAN
#define COLOR_BACKGROUND COLOR_BLACK

/* COLORS configuration:
 * Here you can change the colors of the different file types on the list
 * those use the ncurses colors, that can be found at the end of the file
 *
 * Default:
#define COLOR_DIRECTORY COLOR_BLUE
#define COLOR_EXECUTABLE COLOR_GREEN
#define COLOR_REGULAR COLOR_WHITE
#define COLOR_SYMLINK COLOR_CYAN
#define COLOR_BACKGROUND COLOR_BLACK

*/

#define KEY_QUIT 'q'
#define KEY_SHOW_HELP 'h'
#define KEY_RELOAD KEY_F(5)
#define KEY_RENAME_1 KEY_F(2)
#define KEY_RENAME_2 'r'
#define KEY_DELETE_1 KEY_DC
#define KEY_DELETE_2 'd'
#define KEY_JUMP 'g'
#define KEY_NEXT_PAGE 'n'
#define KEY_PREV_PAGE 'p'
#define KEY_GO_UP KEY_BACKSPACE
#define KEY_RUN_CMD 'x'
#define KEY_SEARCH_1 '/'
#define KEY_SEARCH_2 'f'
#define KEY_MKDIR 'm'
#define KEY_TOUCH 't'

/* Keybindings configuration:
 * Customize your keybinds for the program to adjust your usage experience
 */

/* Ncurses color list:

COLOR_BLACK
COLOR_RED
COLOR_GREEN
COLOR_YELLOW
COLOR_BLUE
COLOR_MAGENTA
COLOR_CYAN
COLOR_WHITE

*/
