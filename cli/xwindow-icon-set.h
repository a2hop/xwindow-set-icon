#ifndef XWINDOW_ICON_SET_H
#define XWINDOW_ICON_SET_H

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set an icon on an X11 window from a PNG file
 * 
 * @param icon_path Path to the PNG icon file
 * @param window X11 Window ID
 * @return true on success, false on failure
 */
bool xwindow_icon_set(const char* icon_path, Window window);

#ifdef __cplusplus
}
#endif

#endif // XWINDOW_ICON_SET_H
