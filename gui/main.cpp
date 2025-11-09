#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include "../x-window-icon-set.hpp"

namespace fs = std::filesystem;

// Function to select a window using mouse
Window select_window_mouse(Display *display) {
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    Cursor cursor = XCreateFontCursor(display, XC_crosshair);
    
    int status = XGrabPointer(display, root, False,
                             ButtonPressMask | ButtonReleaseMask, 
                             GrabModeSync, GrabModeAsync, 
                             root, cursor, CurrentTime);
    
    if (status != GrabSuccess) {
        std::cerr << "Can't grab the mouse" << std::endl;
        return 0;
    }
    
    XEvent event;
    Window target_win = None;
    int buttons = 0;
    
    while ((target_win == None) || (buttons != 0)) {
        XAllowEvents(display, SyncPointer, CurrentTime);
        XWindowEvent(display, root, ButtonPressMask | ButtonReleaseMask, &event);
        
        switch (event.type) {
        case ButtonPress:
            if (event.xbutton.button == Button3) {
                // Right button pressed - cancel selection
                XUngrabPointer(display, CurrentTime);
                XFreeCursor(display, cursor);
                XFlush(display);
                XSync(display, False);
                return None;
            }
            if (target_win == None) {
                target_win = event.xbutton.subwindow;
                if (target_win == None) target_win = root;
            }
            buttons++;
            break;
        case ButtonRelease:
            if (event.xbutton.button == Button3) {
                // Right button released - cancel selection
                XUngrabPointer(display, CurrentTime);
                XFreeCursor(display, cursor);
                XFlush(display);
                XSync(display, False);
                return None;
            }
            if (buttons > 0) buttons--;
            break;
        }
    }
    
    XUngrabPointer(display, CurrentTime);
    XFreeCursor(display, cursor);
    XFlush(display);
    XSync(display, False);
    
    return target_win;
}

// Function to get list of icon files
std::vector<std::string> get_icon_list(const std::string &icon_dir) {
    std::vector<std::string> icons;
    
    if (!fs::exists(icon_dir) || !fs::is_directory(icon_dir)) {
        std::cerr << "Icon directory not found: " << icon_dir << std::endl;
        return icons;
    }
    
    for (const auto &entry : fs::directory_iterator(icon_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            icons.push_back(entry.path().stem().string());
        }
    }
    
    std::sort(icons.begin(), icons.end());
    return icons;
}

// Global variables for the window and selection
static GtkWidget *main_window = NULL;
static GtkWidget *status_label = NULL;
static GtkWidget *tree_view_widget = NULL;
static std::string current_icon_dir;
static std::string selected_icon_name;

// Callback for OK button
void on_ok_clicked(GtkWidget *button, gpointer user_data) {
    if (selected_icon_name.empty()) {
        gtk_label_set_text(GTK_LABEL(status_label), "Please select an icon first!");
        return;
    }
    
    gtk_label_set_text(GTK_LABEL(status_label), "Click on a window to set its icon (right-click to cancel)...");
    
    // Process all pending GTK events to update the UI
    while (gtk_events_pending())
        gtk_main_iteration();
    
    // Small delay to ensure button release is processed
    usleep(100000);
    
    // Select window
    Display *display = XOpenDisplay(NULL);
    if (display) {
        Window window = select_window_mouse(display);
        
        if (window != None) {
            Window root;
            int dummyi;
            unsigned int dummy;
            
            if (XGetGeometry(display, window, &root, &dummyi, &dummyi,
                            &dummy, &dummy, &dummy, &dummy)
                && window != root)
                window = XmuClientWindow(display, window);
            
            XCloseDisplay(display);
            
            // Set the icon
            std::string icon_path = current_icon_dir + "/" + selected_icon_name + ".png";
            
            if (xwindow_icon_set(icon_path.c_str(), window)) {
                // Icon set successfully - just update status to ready state
                gtk_label_set_text(GTK_LABEL(status_label), 
                                 "Select an icon, then click OK to choose a window");
            } else {
                gtk_label_set_text(GTK_LABEL(status_label), "Failed to set icon. Try again.");
            }
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Selection canceled. Select an icon, then click OK.");
        }
    } else {
        gtk_label_set_text(GTK_LABEL(status_label), "Failed to open display. Try again.");
    }
}

// Callback for Cancel button
void on_cancel_clicked(GtkWidget *button, gpointer user_data) {
    gtk_main_quit();
}

// Callback when an icon row is selected (single-click)
void on_icon_selected(GtkTreeSelection *selection, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *icon_name = NULL;
        gtk_tree_model_get(model, &iter, 1, &icon_name, -1);
        
        if (icon_name) {
            selected_icon_name = icon_name;
            gtk_label_set_text(GTK_LABEL(status_label), 
                             ("Selected: " + std::string(icon_name) + ". Click OK to choose a window.").c_str());
            g_free(icon_name);
        }
    } else {
        selected_icon_name.clear();
    }
}

// Function to create icon selection window with thumbnails
GtkWidget* create_icon_window(const std::vector<std::string> &icons, const std::string &icon_dir) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *scrolled_window;
    GtkWidget *tree_view;
    GtkListStore *list_store;
    GtkTreeIter iter;
    
    current_icon_dir = icon_dir;
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Select Icon");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    // Add status label
    status_label = gtk_label_new("Select an icon, then click OK to choose a window");
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 5);
    
    // Create list store with pixbuf (thumbnail) and text (name)
    list_store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    
    // Load thumbnails for each icon
    for (const auto &icon : icons) {
        std::string icon_path = icon_dir + "/" + icon + ".png";
        GError *error = NULL;
        
        // Load the original image
        GdkPixbuf *original = gdk_pixbuf_new_from_file(icon_path.c_str(), &error);
        
        if (original) {
            // Scale to thumbnail size (32x32)
            int width = gdk_pixbuf_get_width(original);
            int height = gdk_pixbuf_get_height(original);
            int thumb_size = 32;
            
            // Calculate scaled dimensions maintaining aspect ratio
            int new_width, new_height;
            if (width > height) {
                new_width = thumb_size;
                new_height = (height * thumb_size) / width;
            } else {
                new_height = thumb_size;
                new_width = (width * thumb_size) / height;
            }
            
            GdkPixbuf *thumbnail = gdk_pixbuf_scale_simple(original, new_width, new_height, 
                                                          GDK_INTERP_BILINEAR);
            
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter, 
                             0, thumbnail,
                             1, icon.c_str(), 
                             -1);
            
            g_object_unref(thumbnail);
            g_object_unref(original);
        } else {
            // If failed to load, just add the name without thumbnail
            if (error) {
                std::cerr << "Failed to load thumbnail for " << icon << ": " << error->message << std::endl;
                g_error_free(error);
            }
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter, 
                             0, NULL,
                             1, icon.c_str(), 
                             -1);
        }
    }
    
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    tree_view_widget = tree_view;
    
    // Connect selection changed signal
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(selection, "changed", G_CALLBACK(on_icon_selected), NULL);
    
    // Add pixbuf column for thumbnail
    GtkCellRenderer *pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *pixbuf_column = gtk_tree_view_column_new_with_attributes(
        "", pixbuf_renderer, "pixbuf", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), pixbuf_column);
    
    // Add text column for icon name
    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *text_column = gtk_tree_view_column_new_with_attributes(
        "Icon Name", text_renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), text_column);
    
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), TRUE);
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Add button box at the bottom
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 5);
    
    // Add OK button
    GtkWidget *ok_button = gtk_button_new_with_label("OK");
    gtk_box_pack_end(GTK_BOX(button_box), ok_button, FALSE, FALSE, 0);
    g_signal_connect(ok_button, "clicked", G_CALLBACK(on_ok_clicked), NULL);
    
    // Add Cancel button
    GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
    gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_cancel_clicked), NULL);
    
    g_object_unref(list_store);
    
    return window;
}

void print_usage(const char *program_name) {
    std::cout << "Usage: " << program_name << " --icon-src=PATH\n"
              << "Options:\n"
              << "  --icon-src=PATH    Path to directory containing icon PNG files (required)\n"
              << "  -h, --help         Show this help message\n";
}

int main(int argc, char *argv[]) {
    std::string icon_dir;
    
    // Parse command line options before GTK init
    // Look for --icon-src= argument, ignore all other arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.find("--icon-src=") == 0) {
            icon_dir = arg.substr(11); // Length of "--icon-src="
            break;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        // Ignore any other arguments
    }
    
    // Check if icon-src was provided
    if (icon_dir.empty()) {
        std::cerr << "Error: --icon-src option is required\n\n";
        print_usage(argv[0]);
        return 1;
    }
    
    gtk_init(&argc, &argv);
    
    std::vector<std::string> icons = get_icon_list(icon_dir);
    
    if (icons.empty()) {
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
                                                         GTK_DIALOG_MODAL,
                                                         GTK_MESSAGE_ERROR,
                                                         GTK_BUTTONS_OK,
                                                         "No icons found in %s",
                                                         icon_dir.c_str());
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return 1;
    }
    
    // Create and show the main icon selection window
    main_window = create_icon_window(icons, icon_dir);
    gtk_widget_show_all(main_window);
    
    // Run the GTK main loop
    gtk_main();
    
    return 0;
}
