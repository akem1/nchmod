#define _XOPEN_SOURCE 500  /* Bring in S_IFMT */

#include <curses.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include "menu.h"

/* TODO: Change name from "menu" to "panel" !!! <===== */

/* Local functions */
static void menu_draw(menu_t *menu);
static void menu_draw_extra(menu_t *menu);
static const char *file_type_str(const mode_t perm);
static char *file_perm_str(const mode_t perm);

/* Callback prototypes */
int cmd_own_r(int ch);
int cmd_own_w(int ch);
int cmd_own_e(int ch);
int cmd_grp_r(int ch);
int cmd_grp_w(int ch);
int cmd_grp_e(int ch);
int cmd_oth_r(int ch);
int cmd_oth_w(int ch);
int cmd_oth_e(int ch);


/* For tracking which checkbox the user is interacting with */
typedef enum {
    OWN_R, OWN_W, OWN_E,
    GRP_R, GRP_W, GRP_E,
    OTH_R, OTH_W, OTH_E,
    CBOX_COUNT  /* Nice way to store the number of values */
} cbox_position;

struct menu {
    checkbox_t  *cboxes[CBOX_COUNT];
    WINDOW      *cboxwin;   /* Curses window for the checkboxes           */
    WINDOW      *modewin;   /* Curses window for current mode             */
    struct stat *filestat;  /* Reference to file info to be output/edited */
};

/* Updated as user moves about with arrow keys */
static cbox_position curr_cbox;

menu_t *menu_create(struct stat *filestat)
{
    menu_t *menu = malloc(sizeof *menu);
    if (!menu)
        return NULL;

    menu->cboxwin = newwin(3, 27, 5, 13);   /* 3x27 size window at (5,12) */
    if (!menu->cboxwin)
        return NULL;
    menu->modewin = newwin(1, 17, 10, 22);  /* 1x17 size window at (10,22) */
    if (!menu->modewin)
        return NULL;

    menu->filestat = filestat;
    curr_cbox = OWN_R;

    /* Construct checkboxes with the permissions set */
    menu->cboxes[OWN_R] = checkbox_create(menu->cboxwin, 0, 4,
                                          (*filestat).st_mode & S_IRUSR);
    menu->cboxes[OWN_W] = checkbox_create(menu->cboxwin, 0, 12,
                                          (*filestat).st_mode & S_IWUSR);
    menu->cboxes[OWN_E] = checkbox_create(menu->cboxwin, 0, 21,
                                          (*filestat).st_mode & S_IXUSR);

    menu->cboxes[GRP_R] = checkbox_create(menu->cboxwin, 1, 4,
                                          (*filestat).st_mode & S_IRGRP);
    menu->cboxes[GRP_W] = checkbox_create(menu->cboxwin, 1, 12,
                                          (*filestat).st_mode & S_IWGRP);
    menu->cboxes[GRP_E] = checkbox_create(menu->cboxwin, 1, 21,
                                          (*filestat).st_mode & S_IXGRP);

    menu->cboxes[OTH_R] = checkbox_create(menu->cboxwin, 2, 4,
                                          (*filestat).st_mode & S_IROTH);
    menu->cboxes[OTH_W] = checkbox_create(menu->cboxwin, 2, 12,
                                          (*filestat).st_mode & S_IWOTH);
    menu->cboxes[OTH_E] = checkbox_create(menu->cboxwin, 2, 21,
                                          (*filestat).st_mode & S_IXOTH);

    checkbox_set_cmd(menu->cboxes[OWN_R], cmd_own_r);
    checkbox_set_cmd(menu->cboxes[OWN_W], cmd_own_w);
    checkbox_set_cmd(menu->cboxes[OWN_E], cmd_own_e);

    checkbox_set_cmd(menu->cboxes[GRP_R], cmd_grp_r);
    checkbox_set_cmd(menu->cboxes[GRP_W], cmd_grp_w);
    checkbox_set_cmd(menu->cboxes[GRP_E], cmd_grp_e);

    checkbox_set_cmd(menu->cboxes[OTH_R], cmd_oth_r);
    checkbox_set_cmd(menu->cboxes[OTH_W], cmd_oth_w);
    checkbox_set_cmd(menu->cboxes[OTH_E], cmd_oth_e);

    menu_draw(menu);
    menu_draw_extra(menu);
    return menu;
}

int menu_destroy(menu_t *menu)
{
    size_t i;
    int res;

    for (i = 0; i < CBOX_COUNT; i++)
        checkbox_destroy(menu->cboxes[i]);
    res = delwin(menu->cboxwin);
    free(menu);
    return (res == OK) ? 0 : -1;
}

/* Called each time the user presses a key */
void menu_receive_input(menu_t *menu, int ch)
{
    int redraw = 0;

    if (ch == ' ') {  /* User toggled a checkbox */
        checkbox_toggle(menu->cboxes[curr_cbox]);

        /* Flip bits in st_mode depending on current checkbox. E.g. if
           the owner execute permission changes, then we XOR with the
           bit mask 001000000 = 64 = 2^6 = (1 << 6). */
        (*menu->filestat).st_mode ^= (1 << (CBOX_COUNT - (curr_cbox + 1)));

        redraw = 1;
    } else {
        redraw = checkbox_receive_input(menu->cboxes[curr_cbox], ch);
    }

    if (redraw)
        menu_draw(menu);
}

static void menu_draw(menu_t *menu)
{
    size_t i;

    werase(menu->cboxwin);
    for (i = 0; i < CBOX_COUNT; i++) {
        if (i == curr_cbox)
            checkbox_draw(menu->cboxes[i], 1);
        else
            checkbox_draw(menu->cboxes[i], 0);
    }
    wrefresh(menu->cboxwin);

    werase(menu->modewin);
    wprintw(menu->modewin, "%s (0%o)",
            file_perm_str((*menu->filestat).st_mode),
            (*menu->filestat).st_mode & 0x1ff);  /* Only use nine LSBs */
    wrefresh(menu->modewin);
}

/*
 * Show some additional info about the file.
 */
static void menu_draw_extra(menu_t *menu)
{
    struct passwd *userinfo;  /* For retrieving user info tied to user ID */
    struct group  *grpinfo;   /* For retrieving group info tied to group ID */
    const char *filetype;
    const char *owner;
    const char *group;

    filetype = file_type_str((*menu->filestat).st_mode);

    userinfo = getpwuid((*menu->filestat).st_uid);
    owner = (userinfo != NULL) ? userinfo->pw_name : NULL;
    grpinfo = getgrgid((*menu->filestat).st_gid);
    group = (grpinfo != NULL) ? grpinfo->gr_name : NULL;

    /* Print directly on stdscr for simplicity (text never changes) */
    mvprintw(11, 22, "%s", filetype);
    mvprintw(12, 22, "%s", (owner != NULL) ? owner : "n/a");
    mvprintw(13, 22, "%s", (group != NULL) ? group : "n/a");
    refresh();
}

/*
 * Return a string describing the file type, or "n/a" if it couldn't
 * be determined (should never happen). The return is a pointer to
 * a string literal, hence statically allocated.
 */
static const char *file_type_str(const mode_t perm)
{
    const char *str;
    switch (perm & S_IFMT) {
        case S_IFREG:
            str = "regular file"; break;
        case S_IFDIR:
            str = "directory"; break;
        case S_IFCHR:
            str = "character device"; break;
        case S_IFBLK:
            str = "block device"; break;
        case S_IFIFO:
            str = "fifo or pipe"; break;
        case S_IFSOCK:
            str = "socket"; break;
        case S_IFLNK:
            str = "symbolic link"; break;
        default:
            str = "n/a"; break;
    }
    return str;
}

/*
 * Convert a file permissions mask, e.g. given by a call to lstat(),
 * into a string. Adopted from Kerrisk's book "The Linux Programming
 * Interface" (2010), p. 296. Returns a pointer to the statically
 * allocated string.
 *    perm - Mask to be converted
 */
static char *file_perm_str(const mode_t perm)
{
    static char str[9];
    sprintf(str, "%c%c%c%c%c%c%c%c%c",
            (perm & S_IRUSR) ? 'r' : '-',
            (perm & S_IWUSR) ? 'w' : '-',
            (perm & S_IXUSR) ? 'x' : '-',
            (perm & S_IRGRP) ? 'r' : '-',
            (perm & S_IWGRP) ? 'w' : '-',
            (perm & S_IXGRP) ? 'x' : '-',
            (perm & S_IROTH) ? 'r' : '-',
            (perm & S_IWOTH) ? 'w' : '-',
            (perm & S_IXOTH) ? 'x' : '-');
    return str;
}


/*
 * Callback definitions below. These functions are called indirectly via
 * menu_receive_input through function pointers in each checkbox object.
 * Each function returns either 0 or 1, where 0 means no action was taken that
 * necessitates a redraw of the menu, and 1 otherwise (e.g. the user moved the
 * cursor to the right).
 */

/* Owner read */
int cmd_own_r(int ch)
{
    switch (ch) {
        case KEY_RIGHT:
            curr_cbox = OWN_W;
            return 1;
        case KEY_DOWN:
            curr_cbox = GRP_R;
            return 1;
        default:
            return 0;
    }
}

/* Owner write */
int cmd_own_w(int ch)
{
    switch (ch) {
        case KEY_LEFT:
            curr_cbox = OWN_R;
            return 1;
        case KEY_RIGHT:
            curr_cbox = OWN_E;
            return 1;
        case KEY_DOWN:
            curr_cbox = GRP_W;
            return 1;
        default:
            return 0;
    }
}

/* Owner execute */
int cmd_own_e(int ch)
{
    switch (ch) {
        case KEY_LEFT:
            curr_cbox = OWN_W;
            return 1;
        case KEY_RIGHT:
            curr_cbox = GRP_R;
            return 1;
        case KEY_DOWN:
            curr_cbox = GRP_E;
            return 1;
        default:
            return 0;
    }
}

/* Group read */
int cmd_grp_r(int ch)
{
    switch (ch) {
        case KEY_UP:
            curr_cbox = OWN_R;
            return 1;
        case KEY_LEFT:
            curr_cbox = OWN_E;
            return 1;
        case KEY_RIGHT:
            curr_cbox = GRP_W;
            return 1;
        case KEY_DOWN:
            curr_cbox = OTH_R;
            return 1;
        default:
            return 0;
    }
}

/* Group write */
int cmd_grp_w(int ch)
{
    switch (ch) {
        case KEY_UP:
            curr_cbox = OWN_W;
            return 1;
        case KEY_LEFT:
            curr_cbox = GRP_R;
            return 1;
        case KEY_RIGHT:
            curr_cbox = GRP_E;
            return 1;
        case KEY_DOWN:
            curr_cbox = OTH_W;
            return 1;
        default:
            return 0;
    }
}

/* Group execute */
int cmd_grp_e(int ch)
{
    switch (ch) {
        case KEY_UP:
            curr_cbox = OWN_E;
            return 1;
        case KEY_LEFT:
            curr_cbox = GRP_W;
            return 1;
        case KEY_RIGHT:
            curr_cbox = OTH_R;
            return 1;
        case KEY_DOWN:
            curr_cbox = OTH_E;
            return 1;
        default:
            return 0;
    }
}

/* Others read */
int cmd_oth_r(int ch)
{
    switch (ch) {
        case KEY_LEFT:
            curr_cbox = GRP_E;
            return 1;
        case KEY_UP:
            curr_cbox = GRP_R;
            return 1;
        case KEY_RIGHT:
            curr_cbox = OTH_W;
            return 1;
        default:
            return 0;
    }
}

/* Others write */
int cmd_oth_w(int ch)
{
    switch (ch) {
        case KEY_UP:
            curr_cbox = GRP_W;
            return 1;
        case KEY_LEFT:
            curr_cbox = OTH_R;
            return 1;
        case KEY_RIGHT:
            curr_cbox = OTH_E;
            return 1;
        default:
            return 0;
    }
}

/* Others execute */
int cmd_oth_e(int ch)
{
    switch (ch) {
        case KEY_UP:
            curr_cbox = GRP_E;
            return 1;
        case KEY_LEFT:
            curr_cbox = OTH_W;
            return 1;
        default:
            return 0;
    }
}
