#ifndef NCHMOD_MENU_H
#define NCHMOD_MENU_H
#include <sys/types.h>
#include <sys/stat.h>
#include "checkbox.h"

typedef struct menu menu_t;

/*
 * Allocate a menu structure, including a curses window and checkbox widgets.
 * Draws the menu to screen and returns the menu object, or simply returns NULL
 * on failure (to allocate memory or setup curses window).
 */
menu_t *menu_create(struct stat *filestat);

int menu_destroy(menu_t *menu);

void menu_receive_input(menu_t *menu, int ch);

#endif
