#include "x-window-icon-set.hpp"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gd.h>
#include <glib.h>
#include <iostream>

typedef unsigned long int CARD32;

// Function to load icon using libgd (exactly like xseticon)
static void load_icon(const char* filename, int* ndata, CARD32** data) {
    FILE* iconfile = fopen(filename, "r");
    if (!iconfile) {
        std::cerr << "Failed to open icon file: " << filename << std::endl;
        exit(1);
    }
    
    gdImagePtr icon = gdImageCreateFromPng(iconfile);
    fclose(iconfile);
    
    if (!icon) {
        std::cerr << "Failed to load PNG" << std::endl;
        exit(1);
    }
    
    int width = gdImageSX(icon);
    int height = gdImageSY(icon);
    
    (*ndata) = (width * height) + 2;
    (*data) = g_new0(CARD32, (*ndata));
    
    int i = 0;
    (*data)[i++] = width;
    (*data)[i++] = height;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Data is stored as BGRA in memory
            guint8* cols = (guint8*)&((*data)[i++]);
            
            int pixcolour = gdImageGetPixel(icon, x, y);
            
            cols[0] = gdImageBlue(icon, pixcolour);
            cols[1] = gdImageGreen(icon, pixcolour);
            cols[2] = gdImageRed(icon, pixcolour);
            
            // Alpha handling
            int alpha = 127 - gdImageAlpha(icon, pixcolour); // 0 to 127
            
            // Scale it up to 0 to 255; remembering that 2*127 should be max
            if (alpha == 127)
                alpha = 255;
            else
                alpha *= 2;
            
            cols[3] = alpha;
        }
    }
    
    gdImageDestroy(icon);
}

// Function to set icon on a window
bool xwindow_icon_set(const char* icon_path, Window window) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Cannot open display" << std::endl;
        return false;
    }
    
    XSynchronize(display, TRUE);
    
    Atom property = XInternAtom(display, "_NET_WM_ICON", 0);
    if (!property) {
        std::cerr << "Failed to get _NET_WM_ICON atom" << std::endl;
        XCloseDisplay(display);
        return false;
    }
    
    int nelements;
    CARD32* data;
    
    load_icon(icon_path, &nelements, &data);
    
    int result = XChangeProperty(display, window, property, XA_CARDINAL, 32, 
                                 PropModeReplace, (unsigned char*)data, nelements);
    
    if (!result) {
        std::cerr << "XChangeProperty failed" << std::endl;
        g_free(data);
        XCloseDisplay(display);
        return false;
    }
    
    result = XFlush(display);
    
    if (!result) {
        std::cerr << "XFlush failed" << std::endl;
    }
    
    g_free(data);
    XCloseDisplay(display);
    
    return true;
}
