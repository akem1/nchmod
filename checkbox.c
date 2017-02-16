#include <stdlib.h>
#include "checkbox.h"

#define  CHECKMARK  'x'

struct checkbox {
    WINDOW *win;
    int y;
    int x;
    state_t state;
    int (*cmd)(int ch);  /* Callback for handling key presses */
};

checkbox_t *checkbox_create(WINDOW *win, int y, int x, state_t state)
{
    checkbox_t *cbox;
    cbox = malloc(sizeof *cbox);
    if (!cbox)
        return NULL;

    cbox->win = win;
    cbox->y = y;
    cbox->x = x;
    cbox->state = (state == 0) ? UNCHECKED : CHECKED;
    cbox->cmd = NULL;
    return cbox;
}

void checkbox_destroy(checkbox_t *cbox)
{
    free(cbox);
}

void checkbox_set_cmd(checkbox_t *cbox, int (*cmd)(int ch))
{
    cbox->cmd = cmd;
}

int checkbox_receive_input(checkbox_t *cbox, int ch)
{
    return cbox->cmd(ch);
}

void checkbox_draw(checkbox_t *cbox, int current)
{
    char c = (cbox->state == UNCHECKED) ? ' ' : CHECKMARK;
    char boxstr[] = "   ";
    if (current != 0) {
        boxstr[0] = '[';
        boxstr[2] = ']';
    }
    boxstr[1] = c;
    mvwprintw(cbox->win, cbox->y, cbox->x, "%s", boxstr);
}

state_t checkbox_toggle(checkbox_t *cbox)
{
    cbox->state = (cbox->state == UNCHECKED) ? CHECKED : UNCHECKED;
    return cbox->state;
}