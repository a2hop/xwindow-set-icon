#ifndef X_WINDOW_ICON_SET_HPP
#define X_WINDOW_ICON_SET_HPP

#include <X11/Xlib.h>

/**
 * Set an icon on an X11 window from a PNG file
 * 
 * @param icon_path Path to the PNG icon file
 * @param window X11 Window ID
 * @return true on success, false on failure
 */
bool xwindow_icon_set(const char* icon_path, Window window);

#endif // X_WINDOW_ICON_SET_HPP
