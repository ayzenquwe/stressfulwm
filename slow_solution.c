#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>

typedef struct {
    int x, y;
    Window window;
} WindowVelocity;

Display *display;
Window root;
unsigned int root_width, root_height;
int (*default_error_handler) (Display *, XErrorEvent *);

unsigned int windows_count;
WindowVelocity *velocities;

// Skips BadWindow/BadDrawable errors when an app is closed
static int error_handler(Display *display, XErrorEvent *error) {
    if (error->error_code == BadDrawable || error->error_code == BadWindow) return 0;
    else return default_error_handler(display, error);
}

void init_root_window() {
    root = DefaultRootWindow(display);
    Window out_root;
    int out_x, out_y;
    unsigned int out_border_width, out_depth;

    XGetGeometry(display, root, &out_root, &out_x, &out_y, &root_width, &root_height, &out_border_width, &out_depth);
}

void set_window_velocity(Window window, WindowVelocity *updated_velocity, int index) {
    updated_velocity->window = window;
    for (int i = 0; i < windows_count; i++) {
        if (velocities[i].window == window) {
            updated_velocity->x = velocities[i].x;
            updated_velocity->y = velocities[i].y;
            return;
        }
    }

    // Initialize velocity vector for a new window
    updated_velocity->x = rand() % 10 - 5;
    updated_velocity->y = rand() % 10 - 5;
}

void move_window(WindowVelocity *velocity) {
    Window out_root;
    int out_x, out_y;
    unsigned int out_width, out_height, out_border_width, out_depth;

    XGetGeometry(display, velocity->window, &out_root, &out_x, &out_y, &out_width, &out_height, &out_border_width, &out_depth);

    int x = out_x + velocity->x;
    int y = out_y + velocity->y;
    if ((x + out_width > root_width) || (x < 0)) {
        velocity->x *= -1;
    }
    if ((y + out_height > root_height) || (y < 0)) {
        velocity->y *= -1;
    }

    XMoveWindow(display, velocity->window, x, y);
}

void move_all_windows() {
    Window root_return, parent_return, *children_return;
    unsigned int nchildren_return;

    XQueryTree(display, root, &root_return, &parent_return, &children_return, &nchildren_return);
    WindowVelocity *updated_velocities = malloc(nchildren_return * sizeof(WindowVelocity));

    for (int i = 0; i < nchildren_return; i++) {
        set_window_velocity(children_return[i], &updated_velocities[i], i);
        move_window(&updated_velocities[i]);
    }
    XFree(children_return);

    free(velocities);
    velocities = updated_velocities;
    windows_count = nchildren_return;
}

int main(void) {
    srand(time(NULL));

    display = XOpenDisplay(NULL);
    if (!display) return 1;

    init_root_window();
    default_error_handler = XSetErrorHandler(error_handler);

    while(1) {
        move_all_windows();
        usleep(30000);
    }
}
