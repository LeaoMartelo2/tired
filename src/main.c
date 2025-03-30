/* Copyright 2025 Henryk Szenkowicz Holtman

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”,
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "config.h"
#include <ctype.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* window and buffer sizes */
#define LAST_ACTION_SIZE 2048
#define PROMPT_WIN_HEIGHT 10
#define PROMPT_WIN_WIDTH 80
#define MSG_WIN_HEIGHT 5
#define MSG_WIN_WIDTH 60

#define MAX_LINE 2048
#define INFO_BAR_PADDING 20
#define ENTRIES_PER_PAGE 20

typedef enum {
    file_reg,
    file_dir,
    file_exec,
    file_link,
} file_type;

typedef struct ls_entry {
    char *full_line;
    char *prefix;
    char *fname;
    file_type type;
} ls_entry;

void trim_newline(char *s);
int parse_ls_line(char *line, ls_entry *entry);
int load_ls_entries(const char *path, ls_entry ***entries_out);
void free_ls_entry(ls_entry *entry);
void free_ls_entries(ls_entry **entries, int count);
const char *file_type_str(int type);
void show_help(void);
int confirm_box(const char *msg);
int prompt_input(const char *prompt, char *buffer, int buf_size);
void show_message(const char *msg);
void run_executable(const char *file_path);

static char last_action[LAST_ACTION_SIZE] = "";

void trim_newline(char *s) {
    char *p = strchr(s, '\n');
    if (p) {
        *p = '\0';
    }
}

int parse_ls_line(char *line, ls_entry *entry) {

    entry->full_line = strdup(line);

    char perm[32], links[32], owner[64], group[64], size[64], month[32], day[32], time_year[32];
    int offset = 0;
    int n = sscanf(line, "%31s %31s %63s %63s %63s %31s %31s %31s %n",
                   perm, links, owner, group, size, month, day, time_year, &offset);
    if (n < 8) {
        return -1;
    }

    while (line[offset] && isspace((unsigned char)line[offset])) {
        offset++;
    }

    entry->fname = strdup(line + offset);

    int prefix_len = offset;

    entry->prefix = malloc(prefix_len + 1);

    strncpy(entry->prefix, line, prefix_len);

    entry->prefix[prefix_len] = '\0';

    if (perm[0] == 'd') {

        entry->type = file_dir;

    } else if (perm[0] == 'l') {

        entry->type = file_link;

    } else if (perm[0] == '-' && strchr(perm, 'x') != NULL) {

        entry->type = file_exec;

    } else {

        entry->type = file_reg;
    }

    return 0;
}

int load_ls_entries(const char *path, ls_entry ***entries_out) {

    char command[256];

    int ret = snprintf(command, sizeof(command), LS_COMMAND " %s", path);

    if (ret < 0 || (size_t)ret >= sizeof(command)) {

        fprintf(stderr, "Command buffer too small.\n");
        return -1;
    }

    FILE *fp = popen(command, "r");
    if (!fp) {
        return -1;
    }

    ls_entry **entries = NULL;
    int capacity = 20, count = 0;

    entries = malloc(capacity * sizeof(ls_entry *));
    char line[MAX_LINE];

    /* skip the "total" line. */
    if (fgets(line, sizeof(line), fp) != NULL) {

        if (strncmp(line, "total", 5) != 0) {

            trim_newline(line);
            ls_entry *entry = malloc(sizeof(ls_entry));
            if (parse_ls_line(line, entry) == 0) {

                entries[count++] = entry;

            } else {

                free(entry);
            }
        }
    }

    while (fgets(line, sizeof(line), fp) != NULL) {

        trim_newline(line);
        if (strlen(line) == 0) {

            continue;
        }
        if (count >= capacity) {

            capacity *= 2;
            entries = realloc(entries, capacity * sizeof(ls_entry *));
        }

        ls_entry *entry = malloc(sizeof(ls_entry));

        if (parse_ls_line(line, entry) == 0) {

            entries[count++] = entry;

        } else {

            free(entry);
        }
    }

    pclose(fp);
    *entries_out = entries;
    return count;
}

void free_ls_entry(ls_entry *entry) {

    if (entry) {

        free(entry->full_line);
        free(entry->prefix);
        free(entry->fname);
        free(entry);
    }
}

void free_ls_entries(ls_entry **entries, int count) {

    for (int i = 0; i < count; i++) {

        free_ls_entry(entries[i]);
    }

    free(entries);
}

const char *file_type_str(int type) {

    switch (type) {

    case file_dir:
        return "DIRECTORY";
        break;

    case file_exec:
        return "EXECUTABLE";
        break;

    case file_link:
        return "SYMLINK";
        break;

    default:
        return "REGULAR";
    }
}

void show_help(void) {

    clear();

    mvprintw(1, 2, "Key Bindings:");

    mvprintw(3, 4, "%c        : Jump to a line", KEY_JUMP);
    mvprintw(4, 4, "%c        : Next page", KEY_NEXT_PAGE);
    mvprintw(5, 4, "%c        : Previous page", KEY_PREV_PAGE);
    mvprintw(6, 4, "%c        : Rename file", KEY_RENAME_2);
    mvprintw(7, 4, "%c        : Delete file", KEY_DELETE_2);
    mvprintw(8, 4, "%c        : Search file", KEY_SEARCH_1);

    mvprintw(10, 4, "%c        : Run command", KEY_RUN_CMD);
    mvprintw(11, 4, "%c        : mkdir", KEY_MKDIR);
    mvprintw(12, 4, "%c        : create file", KEY_TOUCH);
    mvprintw(13, 4, "%c        : Show help", KEY_SHOW_HELP);
    mvprintw(14, 4, "%c        : Quit", KEY_QUIT);

    mvprintw(LINES - 2, 2, "Press any key to return.");

    refresh();
    getch();
}

int confirm_box(const char *msg) {

    int height = 10, width = 60;
    int starty = (LINES - height) / 2, startx = (COLS - width) / 2;

    WINDOW *win = newwin(height, width, starty, startx);
    box(win, 0, 0);

    mvwprintw(win, 2, 2, "%s (y/n)", msg);

    wrefresh(win);
    int ch, confirmed = 0;

    while ((ch = wgetch(win))) {

        if (ch == 'y' || ch == 'Y') {

            confirmed = 1;
            break;

        } else if (ch == 'n' || ch == 'N') {

            break;
        }
    }

    delwin(win);
    return confirmed;
}

void show_message(const char *msg) {

    int height = MSG_WIN_HEIGHT, width = MSG_WIN_WIDTH;
    int starty = (LINES - height) / 2, startx = (COLS - width) / 2;

    WINDOW *win = newwin(height, width, starty, startx);
    box(win, 0, 0);

    mvwprintw(win, 2, 2, "%s", msg);

    wrefresh(win);
    wgetch(win);
    delwin(win);
}

int prompt_input(const char *prompt, char *buffer, int buf_size) {

    int height = PROMPT_WIN_HEIGHT, width = PROMPT_WIN_WIDTH;
    int starty = (LINES - height) / 2, startx = (COLS - width) / 2;

    WINDOW *win = newwin(height, width, starty, startx);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "%s", prompt);

    if (strcmp(prompt, "Run command: ") == 0) {

        mvwprintw(win, 3, 2, "Command: ");

    } else {

        mvwprintw(win, 3, 2, "New name: ");
    }

    wrefresh(win);
    curs_set(1);

    int ch, pos = 0;

    memset(buffer, 0, buf_size);

    while (1) {

        ch = wgetch(win);
        if (ch == 27) { /* ESC cancels input */

            buffer[0] = '\0';
            break;

        } else if (ch == '\n') {

            break;

        } else if (ch == KEY_BACKSPACE || ch == 127) {

            if (pos > 0) {

                pos--;
                buffer[pos] = '\0';
                mvwprintw(win, 3, 12, "%-*s", buf_size - 12, " ");
                mvwprintw(win, 3, 12, "%s", buffer);
                box(win, 0, 0);
                wrefresh(win);
            }

        } else if (pos < buf_size - 1 && isprint(ch)) {

            buffer[pos++] = ch;
            buffer[pos] = '\0';
            mvwprintw(win, 3, 12, "%s", buffer);
            wrefresh(win);
        }
    }
    curs_set(0);
    delwin(win);
    return 0;
}

void run_executable(const char *file_path) {

    if (confirm_box("Open this file?")) {

        clear();
        refresh();
        endwin(); /* exit ncurses mode temporarily */

        printf("Running: %s\n", file_path);
        int status = system(file_path);
        printf("Process exited with status %d\n", status);
        printf("Press Enter to return...\n");

        getchar();
        initscr(); /* restore ncurses mode */
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
    }
}

int main(void) {

    char current_path[1024] = ".";
    int ch, selected = 0, page = 0;
    int num_entries = 0;
    ls_entry **entries = NULL;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_DIRECTORY, COLOR_BLACK);
    init_pair(2, COLOR_EXECUTABLE, COLOR_BLACK);
    init_pair(3, COLOR_REGULAR, COLOR_BLACK);
    init_pair(4, COLOR_SYMLINK, COLOR_BLACK);

    num_entries = load_ls_entries(current_path, &entries);
    if (num_entries < 0) {

        endwin();
        fprintf(stderr, "Failed to load directory entries.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {

        clear();
        /* display last action message at the top */
        mvprintw(0, 0, "%s", last_action);

        int total_pages = (num_entries + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE;
        int start_index = page * ENTRIES_PER_PAGE;
        int end_index = start_index + ENTRIES_PER_PAGE;

        if (end_index > num_entries) {

            end_index = num_entries;
        }

        for (int i = start_index; i < end_index; i++) {

            if (i == selected) {

                attron(A_REVERSE);
            }

            mvprintw(i - start_index + 1, 0, "[%2d]", i);
            int col = 4 + snprintf(NULL, 0, "[%2d]", i);
            mvprintw(i - start_index + 1, col, "%s", entries[i]->prefix);
            col += strlen(entries[i]->prefix);

            switch (entries[i]->type) {

            case file_dir: {
                attron(COLOR_PAIR(1));
                break;
            }

            case file_exec: {
                attron(COLOR_PAIR(2));
                break;
            }

            case file_link: {
                attron(COLOR_PAIR(4));
                break;
            }

            default: {
                attron(COLOR_PAIR(3));
                break;
            }
            }

            mvprintw(i - start_index + 1, col, "%s", entries[i]->fname);
            attroff(COLOR_PAIR(1));
            attroff(COLOR_PAIR(2));
            attroff(COLOR_PAIR(3));
            attroff(COLOR_PAIR(4));

            if (i == selected) {

                attroff(A_REVERSE);
            }
        }

        char info_bar[100];
        int ret = snprintf(info_bar, sizeof(info_bar), "INFO: %-*s | Page (%d/%d)",
                           INFO_BAR_PADDING, file_type_str(entries[selected]->type),
                           page + 1, total_pages);
        if (ret < 0 || (size_t)ret >= sizeof(info_bar)) {
            info_bar[sizeof(info_bar) - 1] = '\0';
        }
        mvprintw(LINES - 2, 0, "%s", info_bar);
        mvprintw(LINES - 1, 0, "q: quit | h: help | r: rename | d: delete | n: next | p: prev | m: mkdir | t: touch | x: run command");
        refresh();

        ch = getch();

        if (ch == KEY_QUIT) {

            if (confirm_box("Are you sure you want to quit?")) {
                break;
            }

        } else if (ch == KEY_UP && selected > 0) {

            selected--;
            if (selected < start_index) {
                page--;
            }

        } else if (ch == KEY_DOWN && selected < num_entries - 1) {

            selected++;
            if (selected >= end_index) {
                page++;
            }

        } else if (ch == KEY_NEXT_PAGE) {

            if (page < total_pages - 1) {

                page++;
                selected = page * ENTRIES_PER_PAGE;
            }

        } else if (ch == KEY_PREV_PAGE) {

            if (page > 0) {
                page--;
                selected = page * ENTRIES_PER_PAGE;
            }

        } else if (ch == KEY_JUMP) {

            echo();
            char num_str[10];
            mvprintw(LINES - 3, 0, "Jump to line: ");

            getnstr(num_str, sizeof(num_str) - 1);
            noecho();

            int jump = atoi(num_str);

            if (jump >= 0 && jump < num_entries) {

                selected = jump;
                page = jump / ENTRIES_PER_PAGE;
            }

        } else if (ch == KEY_SEARCH_1 || ch == KEY_SEARCH_2) {

            char search_query[256] = {0};
            prompt_input("Search: ", search_query, sizeof(search_query));

            if (strlen(search_query) > 0) {

                int found = 0;
                for (int i = 0; i < num_entries; i++) {
                    if (strcasestr(entries[i]->fname, search_query) != NULL) {
                        selected = i;
                        page = i / ENTRIES_PER_PAGE;
                        found = 1;
                        break;
                    }
                }

                if (!found) {

                    show_message("No matching file found. Press any key.");
                }
            }

        } else if (ch == '\n') {

            if ((entries[selected]->type == file_dir) ||
                (strcmp(entries[selected]->fname, "../") == 0)) {
                char new_path[1024];
                int ret2 = snprintf(new_path, sizeof(new_path), "%s/%s", current_path, entries[selected]->fname);

                if (ret2 < 0 || (size_t)ret2 >= sizeof(new_path)) {

                    new_path[sizeof(new_path) - 1] = '\0';
                }

                if (chdir(new_path) == 0) {

                    if (realpath(".", current_path) == NULL) {

                        perror("realpath");
                        break;
                    }

                    free_ls_entries(entries, num_entries);
                    num_entries = load_ls_entries(current_path, &entries);

                    if (selected >= num_entries) {
                        selected = num_entries - 1;
                    }

                    page = selected / ENTRIES_PER_PAGE;
                }

            } else {

                char *ext = strrchr(entries[selected]->fname, '.');

                char command[1024];

                if ((ext && strcasecmp(ext, ".png") == 0) ||
                    (ext && strcasecmp(ext, ".jpeg") == 0) ||
                    (ext && strcasecmp(ext, ".jpg") == 0) ||
                    (ext && strcasecmp(ext, ".gif") == 0)) {

                    snprintf(command, sizeof(command), IMAGE_VIEWER_COMMAND, entries[selected]->fname);
                    run_executable(command);

                } else if ((ext && strcasecmp(ext, ".mp4") == 0) ||
                           (ext && strcasecmp(ext, ".mov")) == 0) {

                    snprintf(command, sizeof(command), VIDEO_PLAYER_COMMAND, entries[selected]->fname);
                    run_executable(command);

                } else if ((ext && strcasecmp(ext, ".mp3")) == 0 ||
                           (ext && strcasecmp(ext, ".ogg")) == 0 ||
                           (ext && strcasecmp(ext, ".wav")) == 0) {

                    snprintf(command, sizeof(command), AUDIO_PLAYER_COMMAND, entries[selected]->fname);
                    run_executable(command);

                } else if (entries[selected]->type == file_exec) {

                    char exec_path[2048];

                    snprintf(exec_path, sizeof(exec_path), "%s/%s", current_path, entries[selected]->fname);
                    run_executable(exec_path);

                } else {

                    /* fallback */
                    snprintf(command, sizeof(command), "xdg-open %s", entries[selected]->fname);
                    run_executable(command);
                }

                free_ls_entries(entries, num_entries);
                num_entries = load_ls_entries(current_path, &entries);

                if (selected >= num_entries)
                    selected = num_entries - 1;

                page = selected / ENTRIES_PER_PAGE;
            }

        } else if (ch == KEY_RENAME_1 || ch == KEY_RENAME_2) {

            char new_name[256] = {0};

            prompt_input("Rename file: ", new_name, sizeof(new_name));

            if (strlen(new_name) > 0) {

                char old_filename[512];
                strncpy(old_filename, entries[selected]->fname, sizeof(old_filename));
                old_filename[sizeof(old_filename) - 1] = '\0';

                if (entries[selected]->type == file_exec) {

                    size_t len = strlen(old_filename);

                    if (len > 0 && old_filename[len - 1] == '*') {

                        old_filename[len - 1] = '\0';
                    }
                }

                char confirm_msg[512];
                int ret = snprintf(confirm_msg, sizeof(confirm_msg),
                                   "Rename '%.50s' to '%.50s'?", old_filename, new_name);

                if (ret < 0 || (size_t)ret >= sizeof(confirm_msg)) {
                    confirm_msg[sizeof(confirm_msg) - 1] = '\0';
                }

                if (confirm_box(confirm_msg)) {

                    char old_path[1024], new_path[1024];
                    int ret4 = snprintf(old_path, sizeof(old_path), "%s/%s", current_path, old_filename);

                    if (ret4 < 0 || (size_t)ret4 >= sizeof(old_path)) {

                        old_path[sizeof(old_path) - 1] = '\0';
                    }

                    int ret5 = snprintf(new_path, sizeof(new_path), "%s/%s", current_path, new_name);

                    if (ret5 < 0 || (size_t)ret5 >= sizeof(new_path)) {

                        new_path[sizeof(new_path) - 1] = '\0';
                    }

                    if (rename(old_path, new_path) == 0) {

                        snprintf(last_action, LAST_ACTION_SIZE, "Renamed '%.50s' to '%.50s'", old_filename, new_name);
                        free_ls_entries(entries, num_entries);

                        num_entries = load_ls_entries(current_path, &entries);

                        if (selected >= num_entries)
                            selected = num_entries - 1;

                        page = selected / ENTRIES_PER_PAGE;
                    }
                }
            }

        } else if (ch == KEY_DELETE_1 || ch == KEY_DELETE_2) {

            if (confirm_box("Confirm delete?")) {

                char del_path[1024];
                int ret6 = snprintf(del_path, sizeof(del_path), "%s/%s", current_path, entries[selected]->fname);
                if (ret6 < 0 || (size_t)ret6 >= sizeof(del_path)) {
                    del_path[sizeof(del_path) - 1] = '\0';
                }

                if (remove(del_path) == 0) {
                    snprintf(last_action, LAST_ACTION_SIZE, "Deleted '%.50s'", entries[selected]->fname);
                    free_ls_entries(entries, num_entries);
                    num_entries = load_ls_entries(current_path, &entries);

                    if (selected >= num_entries)
                        selected = num_entries - 1;

                    page = selected / ENTRIES_PER_PAGE;
                }
            }

        } else if (ch == KEY_RUN_CMD) {

            char cmd[256] = {0};
            prompt_input("Run command: ", cmd, sizeof(cmd));

            if (strlen(cmd) > 0) {

                int status = system(cmd);
                snprintf(last_action, LAST_ACTION_SIZE, "Ran '%.50s' (status %d)", cmd, status);
                free_ls_entries(entries, num_entries);
                num_entries = load_ls_entries(current_path, &entries);

                if (selected >= num_entries)
                    selected = num_entries - 1;

                page = selected / ENTRIES_PER_PAGE;
            }

        } else if (ch == KEY_MKDIR) {

            char dir_name[256] = {0};
            prompt_input("Mkdir: ", dir_name, sizeof(dir_name));

            if (strlen(dir_name) > 0) {

                if (mkdir(dir_name, 0755) == 0) {

                    snprintf(last_action, LAST_ACTION_SIZE, "Created directory '%s'", dir_name);

                } else {

                    snprintf(last_action, LAST_ACTION_SIZE, "Mkdir failed for '%s'", dir_name);
                }

                free_ls_entries(entries, num_entries);
                num_entries = load_ls_entries(current_path, &entries);

                if (selected >= num_entries)
                    selected = num_entries - 1;

                page = selected / ENTRIES_PER_PAGE;
            }

        } else if (ch == KEY_TOUCH) {
            char file_name[256] = {0};
            prompt_input("Touch: ", file_name, sizeof(file_name));
            if (strlen(file_name) > 0) {

                int fd = open(file_name, O_CREAT | O_WRONLY, 0644);

                if (fd != -1) {

                    close(fd);
                    snprintf(last_action, LAST_ACTION_SIZE, "Created file '%s'", file_name);

                } else {

                    snprintf(last_action, LAST_ACTION_SIZE, "Touch failed for '%s'", file_name);
                }

                free_ls_entries(entries, num_entries);
                num_entries = load_ls_entries(current_path, &entries);

                if (selected >= num_entries)
                    selected = num_entries - 1;

                page = selected / ENTRIES_PER_PAGE;
            }
        } else if (ch == KEY_RELOAD) {

            free_ls_entries(entries, num_entries);
            num_entries = load_ls_entries(current_path, &entries);

            if (selected >= num_entries)
                selected = num_entries - 1;

            page = selected / ENTRIES_PER_PAGE;

        } else if (ch == KEY_GO_UP) {
            if (chdir("..") == 0) {

                if (realpath(".", current_path) == NULL) {

                    perror("realpath");
                    break;
                }

                free_ls_entries(entries, num_entries);
                num_entries = load_ls_entries(current_path, &entries);

                if (selected >= num_entries)
                    selected = num_entries - 1;

                page = selected / ENTRIES_PER_PAGE;
            }
        } else if (ch == KEY_SHOW_HELP) {

            show_help();
        }
    }

    free_ls_entries(entries, num_entries);
    endwin();
    return 0;
}
