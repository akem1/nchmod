#define _XOPEN_SOURCE 500  /* Bring in realpath(), lstat() */

#include <curses.h>
#include <errno.h>
#include <limits.h>  /* For PATH_MAX */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "menu.h"

#ifndef PATH_MAX
# define PATH_MAX 2048
#endif

static int term_h;  /* Height of terminal window (number of lines) */
static int term_w;  /* Width of terminal window */

void handle_error(const char *errstr, int error)
{
    endwin();
    if ((errno = error) != 0)
        perror(errstr);
    exit(EXIT_FAILURE);
}

/*
 * Initialize and setup ncurses.
 */
static void init_display(void)
{
    initscr();  /* Exits on failure */
    cbreak();
    noecho();
    nonl();
    keypad(stdscr, TRUE);
    curs_set(FALSE);
    set_escdelay(0);
    setlocale(LC_ALL, "");
}

/*
 * Draw the top bar of the program, fitted to terminal width.
 *    fpstr - File path string to print in top bar
 */
static void draw_top_bar(const char *fpstr)
{
    char *topstr;  /* Top bar represented as a string */
    int verlen;    /* Length of string containing version of program */

    /* Create top bar as string, padding it with blanks to fit terminal width */
    topstr = malloc(term_w + 1);
    if (!topstr)
        handle_error("draw_top_bar", errno);
    verlen = strlen(NCHMOD_VERSION);
    sprintf(topstr,
            " nchmod : %-*s%s ", term_w - (30 + verlen), fpstr,
            "ESC/F10 to Cancel/Save");

    standout();
    mvprintw(0, 0, topstr);
    standend();
    refresh();

    free(topstr);
}

/*
 * Draw the main labels of the user interface. For simplicity we put this
 * directly on stdscr.
 */
static void draw_labels(void)
{
    mvprintw(3, 16, "Read    Write   Execute");
    mvhline(4, 5, ACS_HLINE, 35);
    mvprintw(5, 6, " Owner:");
    mvprintw(6, 6, " Group:");
    mvprintw(7, 6, "Others:");
    mvhline(8, 5, ACS_HLINE, 35);
    mvprintw(10, 8, "Current mode:");
    mvprintw(11, 8, "   File type:");
    mvprintw(12, 8, "    Owned by:");
    mvprintw(13, 8, "       Group:");
    refresh();
}

int main(int argc, char *argv[])
{
    menu_t *cbox_menu;       /* Menu of checkboxes for changing file mode bits */
    char pathbuf[PATH_MAX];  /* For storing resolved path of file */
    struct stat filestat;    /* File status info retrieved by lstat() */
    int ch;
    int error = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: nchmod FILE\n");
        exit(EXIT_FAILURE);
    }

    /* Attempt to get absolute path of user-supplied file */
    if (!realpath(argv[1], pathbuf)) {
        perror("nchmod");
        exit(EXIT_FAILURE);
    }

    if (lstat(argv[1], &filestat) == -1) {
        perror("nchmod (lstat)");
        exit(EXIT_FAILURE);
    }

    /*file_perm_str(filestat.st_mode, modestr);
    printf("%s  0%o\n", modestr, filestat.st_mode & 0777);*/

    init_display();
    getmaxyx(stdscr, term_h, term_w);

    draw_top_bar(pathbuf);
    draw_labels();

    cbox_menu = menu_create(&filestat);

    /* Main event loop */
    while((ch = getch()) != 27) {
        if (ch != KEY_F(10)) {
            menu_receive_input(cbox_menu, ch);
        } else {  /* F10 pressed, attempt to save */
            if (chmod(pathbuf, filestat.st_mode) == -1) {
                error = errno;
            }
            break;
        }
    }

    menu_destroy(cbox_menu);
    endwin();
    if (error == 0) {
        exit(EXIT_SUCCESS);
    } else {
        errno = error;
        perror("nchmod");
        exit(EXIT_FAILURE);
    }
}
