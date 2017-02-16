#ifndef NCHMOD_CHECKBOX_H
#define NCHMOD_CHECKBOX_H
#include <curses.h>

/*
 * Type representing a checkbox for setting permission modes. Each checkbox has
 * the responsibility of drawing itself, but also for responding to key-presses
 * by the user (via the menu datatype, which holds pointers to all checkboxes).
 */
typedef struct checkbox checkbox_t;

typedef enum {
    UNCHECKED = 0,
    CHECKED = 1
} state_t;

/*
 * Create a new checkbox. Returns a pointer to the object or NULL if it
 * couldn't be created.
 *    win - Curses window to draw the checkbox in
 *      y - Vertical placement in win, 0 is top row of terminal
 *      x - Horizontal placement, 0 is left-most column
 *  state - Zero for unchecked, otherwise checked
 */
checkbox_t *checkbox_create(WINDOW *win, int y, int x, state_t state);

/*
 * Free all memory associated with the object.
 */
void checkbox_destroy(checkbox_t *cbox);

void checkbox_set_cmd(checkbox_t *cbox, int (*cmd)(int ch));

int checkbox_receive_input(checkbox_t *cbox, int ch);

/*
 * Display the checkbox on the logical curses screen, in associated window.
 * Note the word "logical"; a call to this function must be followed by e.g.
 * a call to curses' wrefresh() to draw also to the physical screen.
 *    current - nonzero if the checkbox is currently active (and hence should
 *              be painted as such).
 */
void checkbox_draw(checkbox_t *cbox, int current);

state_t checkbox_toggle(checkbox_t *cbox);

#if 0
state_t checkbox_get_state(checkbox_t *cbox);

void checkbox_set_state(checkbox_t *cbox, state_t s);
#endif

#endif
