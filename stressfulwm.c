#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

typedef struct {
    int x, y;
    unsigned int width, height;
} WindowRect;

typedef struct {
    Window window;
    WindowRect rect;
    int vel_x, vel_y;
} WindowMove;

Display *display;
Window root;
WindowRect root_rect;
int (*default_error_handler) (Display *, XErrorEvent *);

unsigned int windows_count = 0;
WindowMove *windows;

// Skips BadWindow/BadDrawable errors when an app is closed
static int error_handler(Display *display, XErrorEvent *error) {
    if (error->error_code == BadDrawable || error->error_code == BadWindow) return 0;
    else return default_error_handler(display, error);
}

void get_window_geometry(Window window, WindowRect *rect) {
    Window out_root;
    unsigned int out_border_width, out_depth;

    XGetGeometry(display, window, &out_root, &rect->x, &rect->y, &rect->width, &rect->height, &out_border_width, &out_depth);
}

void init_root_window() {
    root = DefaultRootWindow(display);
    get_window_geometry(root, &root_rect);
}

void add_new_window(Window window) {
    windows = realloc(windows, ++windows_count * sizeof(WindowMove));
    WindowMove *move = &windows[windows_count - 1];
    move->window = window;

    get_window_geometry(window, &move->rect);

    move->vel_x = rand() % 10 - 5;
    move->vel_y = rand() % 10 - 5;
}

void add_all_windows() {
    Window root_return, parent_return, *children_return;
    unsigned int nchildren_return;

    XQueryTree(display, root, &root_return, &parent_return, &children_return, &nchildren_return);
    for (int i = 0; i < nchildren_return; i++) {
        add_new_window(children_return[i]);
    }
    XFree(children_return);
}

void remove_window(Window window) {
    WindowMove *new_windows = malloc((windows_count - 1) * sizeof(WindowMove));

    for (int i = 0, j = 0; i < windows_count; i++) {
        if (windows[i].window != window) {
            new_windows[j++] = windows[i];
        }
    }

    windows_count--;
    windows = new_windows;
}

void move_window(WindowMove *move) {
    WindowRect *rect = &move->rect;
    rect->x = rect->x + move->vel_x;
    rect->y = rect->y + move->vel_y;
    if ((rect->x + rect->width > root_rect.width) || (rect->x < 0)) {
        move->vel_x *= -1;
    }
    if ((rect->y + rect->height > root_rect.height) || (rect->y < 0)) {
        move->vel_y *= -1;
    }

    XMoveWindow(display, move->window, rect->x, rect->y);
}

void move_all_windows() {
    for (int i = 0; i < windows_count; i++) {
        move_window(&windows[i]);
    }
}

void check_windows_management_events() {
    XEvent event;

    while (XPending(display) != 0) {
        XNextEvent(display, &event);

        if (event.type == MapNotify) {
            add_new_window(event.xmap.window);
        } else if (event.type == UnmapNotify) {
            remove_window(event.xunmap.window);
        }
    }
}

int main(void) {
    srand(time(NULL));

    display = XOpenDisplay(NULL);
    if (!display) return 1;

    init_root_window();
    default_error_handler = XSetErrorHandler(error_handler);

    add_all_windows();

    XSelectInput(display, root, SubstructureNotifyMask);

    while(1) {
        check_windows_management_events();
        move_all_windows();

        usleep(30000);
    }
}
