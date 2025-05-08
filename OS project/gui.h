#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "main.h"

typedef struct {
    SimulationEngine* engine;
    GtkWidget*        window;
    GtkWidget*        process_list;
    GtkWidget*        ready_queue_list;
    GtkWidget*        blocked_queue_list;
    GtkWidget*        memory_view;
    GtkWidget*        mutex_status;
    GtkWidget*        log_view;
    GtkWidget*        clock_label;
    GtkWidget*        process_count_label;
    GtkWidget*        algorithm_label;
    GtkWidget*        algorithm_combo;
    GtkWidget*        quantum_spin;
    GtkWidget*        start_button;
    GtkWidget*        stop_button;
    GtkWidget*        reset_button;
    GtkWidget*        step_button;
    GtkWidget*        auto_button;
    GtkWidget*        add_process_button;
    guint             timeout_id;
} GuiData;

/* GUI entrypoint */
void create_gtk_gui(SimulationEngine* engine);

/* GUI update callbacks */
gboolean update_gui_timeout(gpointer user_data);
void     update_process_list(GuiData* gui_data);
void     update_queue_lists(GuiData* gui_data);
void     update_memory_view(GuiData* gui_data);
void     update_mutex_status(GuiData* gui_data);
void     update_dashboard(GuiData* gui_data);
void     append_log(GuiData* gui_data, const char* message);

/* signal handlers */
void on_start_clicked      (GtkButton* button, gpointer user_data);
void on_stop_clicked       (GtkButton* button, gpointer user_data);
void on_reset_clicked      (GtkButton* button, gpointer user_data);
void on_step_clicked       (GtkButton* button, gpointer user_data);
void on_auto_clicked       (GtkButton* button, gpointer user_data);
void on_add_process_clicked(GtkButton* button, gpointer user_data);
void on_algorithm_changed  (GtkComboBox* combo, gpointer user_data);

#endif // GUI_H