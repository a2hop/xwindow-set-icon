#include "../x-window-icon-set.hpp"
#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>
#include <iostream>
#include <cstring>

// Main function for command-line usage
int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <icon.png> <window_id>" << std::endl;
        return 1;
    }
    
    const char* icon_path = argv[1];
    unsigned long window_id;
    
    // Parse window ID (hex or decimal)
    if (strncmp(argv[2], "0x", 2) == 0 || strncmp(argv[2], "0X", 2) == 0) {
        sscanf(argv[2] + 2, "%lx", &window_id);
    } else {
        sscanf(argv[2], "%lu", &window_id);
    }
    
    Window window = (Window)window_id;
    
    // Get the client window if needed
    Display *display = XOpenDisplay(NULL);
    if (display) {
        Window root;
        int dummyi;
        unsigned int dummy;
        
        if (XGetGeometry(display, window, &root, &dummyi, &dummyi,
                        &dummy, &dummy, &dummy, &dummy)
            && window != root)
            window = XmuClientWindow(display, window);
        
        XCloseDisplay(display);
    }
    
    if (xwindow_icon_set(icon_path, window)) {
        return 0;
    } else {
        return 1;
    }
}
