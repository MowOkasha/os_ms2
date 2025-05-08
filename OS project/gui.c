#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "gui.h"
#include "main.h"

void update_process_list(GuiData* gui_data) {
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->process_list))));
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->process_list)));
    GtkTreeIter iter;

    for (int i = 0; i < gui_data->engine->process_count; i++) {
        Process* proc = gui_data->engine->processes[i];
        const char* state_str = proc->pcb.state == READY ? "Ready" :
                                proc->pcb.state == RUNNING ? "Running" :
                                proc->pcb.state == BLOCKED ? "Blocked" : "Terminated";
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, proc->pcb.process_id,
                           1, state_str,
                           2, proc->pcb.priority,
                           3, proc->pcb.program_counter,
                           4, proc->pcb.memory_lower,
                           5, proc->pcb.memory_upper,
                           -1);
    }
}

void update_queue_lists(GuiData* gui_data) {
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->ready_queue_list))));
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->blocked_queue_list))));
    GtkListStore* ready_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->ready_queue_list)));
    GtkListStore* blocked_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->blocked_queue_list)));
    GtkTreeIter iter;

    for (int i = 0; i < (gui_data->engine->algorithm == 2 ? 4 : 1); i++) {
        QueueNode* node = gui_data->engine->ready_queues[i].front;
        while (node) {
            gtk_list_store_append(ready_store, &iter);
            gtk_list_store_set(ready_store, &iter,
                               0, node->process->pcb.process_id,
                               1, i,
                               -1);
            node = node->next;
        }
    }

    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (gui_data->engine->mutex_ctrl->locked[i]) {
            QueueNode* node = gui_data->engine->mutex_ctrl->blocked_queues[i].front;
            while (node) {
                gtk_list_store_append(blocked_store, &iter);
                gtk_list_store_set(blocked_store, &iter,
                                   0, node->process->pcb.process_id,
                                   1, i,
                                   -1);
                node = node->next;
            }
        }
    }
    QueueNode* gnode = gui_data->engine->mutex_ctrl->general_blocked.front;
    while (gnode) {
        gtk_list_store_append(blocked_store, &iter);
        gtk_list_store_set(blocked_store, &iter,
                           0, gnode->process->pcb.process_id,
                           1, -1,
                           -1);
        gnode = gnode->next;
    }
}

void update_memory_view(GuiData* gui_data) {
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->memory_view))));
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->memory_view)));
    GtkTreeIter iter;

    for (int i = 0; i < gui_data->engine->memory->next_free; i++) {
        if (gui_data->engine->memory->words[i].name[0]) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, i,
                               1, gui_data->engine->memory->words[i].name,
                               2, gui_data->engine->memory->words[i].value,
                               -1);
        }
    }
}

void update_mutex_status(GuiData* gui_data) {
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->mutex_status))));
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->mutex_status)));
    GtkTreeIter iter;

    const char* resources[] = {"userInput", "userOutput", "file"};
    for (int i = 0; i < 3; i++) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, resources[i],
                           1, gui_data->engine->mutex_ctrl->locked[i] ? "Locked" : "Unlocked",
                           -1);
    }
}

void append_log(GuiData* gui_data, const char* message) {
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui_data->log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
}

void update_dashboard(GuiData* gui_data) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Clock Cycle: %d", gui_data->engine->clock_cycle);
    gtk_label_set_text(GTK_LABEL(gui_data->clock_label), buffer);
    snprintf(buffer, sizeof(buffer), "Total Processes: %d", gui_data->engine->process_count);
    gtk_label_set_text(GTK_LABEL(gui_data->process_count_label), buffer);
    const char* algo_str = gui_data->engine->algorithm == 0 ? "FCFS" :
                           gui_data->engine->algorithm == 1 ? "Round Robin" : "MLFQ";
    snprintf(buffer, sizeof(buffer), "Algorithm: %s", algo_str);
    gtk_label_set_text(GTK_LABEL(gui_data->algorithm_label), buffer);
}

gboolean update_gui_timeout(gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    if (gui_data->engine->completed_processes >= gui_data->engine->process_count) {
        append_log(gui_data, "Simulation complete: All processes terminated.");
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Auto Run");
        gui_data->timeout_id = 0;
        return FALSE;
    }

    Process* current_proc = select_next_process(gui_data->engine);
    if (!current_proc && gui_data->engine->completed_processes < gui_data->engine->process_count) {
        append_log(gui_data, "No ready processes (possible deadlock).");
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Auto Run");
        gui_data->timeout_id = 0;
        return FALSE;
    }

    int instr_addr = current_proc && current_proc->pcb.program_counter < current_proc->instruction_count ?
                     current_proc->instruction_start + current_proc->pcb.program_counter : -1;
    char instruction[MAX_VALUE] = "";
    if (instr_addr >= 0) {
        strncpy(instruction, gui_data->engine->memory->words[instr_addr].value, MAX_VALUE - 1);
        instruction[MAX_VALUE - 1] = '\0';
    }

    int result = execute_cycle(gui_data->engine, current_proc);
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "Cycle %d: PID %d executed %s (Result: %s)",
             gui_data->engine->clock_cycle,
             current_proc ? current_proc->pcb.process_id : -1,
             instruction,
             result == 1 ? "Success" : result == 0 ? "Completed" : "Blocked/Error");
    append_log(gui_data, log_msg);

    update_process_list(gui_data);
    update_queue_lists(gui_data);
    update_memory_view(gui_data);
    update_mutex_status(gui_data);
    update_dashboard(gui_data);

    return result && gui_data->timeout_id != 0;
}

void on_start_clicked(GtkButton* button, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    if (gui_data->timeout_id == 0) {
        append_log(gui_data, "Starting simulation...");
        gui_data->timeout_id = g_timeout_add(1000, update_gui_timeout, gui_data);
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Stop Auto");
    }
}

void on_stop_clicked(GtkButton* button, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    if (gui_data->timeout_id != 0) {
        g_source_remove(gui_data->timeout_id);
        gui_data->timeout_id = 0;
        append_log(gui_data, "Simulation stopped.");
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Auto Run");
    }
}

void on_reset_clicked(GtkButton* button, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    if (gui_data->timeout_id != 0) {
        g_source_remove(gui_data->timeout_id);
        gui_data->timeout_id = 0;
    }

    init_memory(gui_data->engine->memory);
    init_mutex(gui_data->engine->mutex_ctrl);
    gui_data->engine->completed_processes = 0;
    gui_data->engine->clock_cycle = 0;

    for (int i = 0; i < gui_data->engine->process_count; i++) {
        free_process(gui_data->engine->memory, gui_data->engine->processes[i]);
    }
    gui_data->engine->process_count = 0;

    Process* p1 = build_process(gui_data->engine->memory, 1, "./Program_1.txt");
    Process* p2 = build_process(gui_data->engine->memory, 2, "./Program_2.txt");
    Process* p3 = build_process(gui_data->engine->memory, 3, "./Program_3.txt");
    if (p1) gui_data->engine->processes[gui_data->engine->process_count++] = p1;
    if (p2) gui_data->engine->processes[gui_data->engine->process_count++] = p2;
    if (p3) gui_data->engine->processes[gui_data->engine->process_count++] = p3;

    init_simulation(gui_data->engine, gui_data->engine->memory, gui_data->engine->mutex_ctrl,
                    gui_data->engine->processes, gui_data->engine->process_count,
                    gtk_combo_box_get_active(GTK_COMBO_BOX(gui_data->algorithm_combo)),
                    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui_data->quantum_spin)));

    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->process_list))));
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->ready_queue_list))));
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->blocked_queue_list))));
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->memory_view))));
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gui_data->mutex_status))));
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui_data->log_view)), "", -1);

    append_log(gui_data, "Simulation reset.");
    update_process_list(gui_data);
    update_queue_lists(gui_data);
    update_memory_view(gui_data);
    update_mutex_status(gui_data);
    update_dashboard(gui_data);
    gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Auto Run");
}

void on_step_clicked(GtkButton* button, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    if (gui_data->timeout_id != 0) {
        g_source_remove(gui_data->timeout_id);
        gui_data->timeout_id = 0;
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Auto Run");
    }

    Process* current_proc = select_next_process(gui_data->engine);
    if (!current_proc && gui_data->engine->completed_processes < gui_data->engine->process_count) {
        append_log(gui_data, "No ready processes (possible deadlock).");
        return;
    }

    int instr_addr = current_proc && current_proc->pcb.program_counter < current_proc->instruction_count ?
                     current_proc->instruction_start + current_proc->pcb.program_counter : -1;
    char instruction[MAX_VALUE] = "";
    if (instr_addr >= 0) {
        strncpy(instruction, gui_data->engine->memory->words[instr_addr].value, MAX_VALUE - 1);
        instruction[MAX_VALUE - 1] = '\0';
    }

    int result = execute_cycle(gui_data->engine, current_proc);
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "Cycle %d: PID %d executed %s (Result: %s)",
             gui_data->engine->clock_cycle,
             current_proc ? current_proc->pcb.process_id : -1,
             instruction,
             result == 1 ? "Success" : result == 0 ? "Completed" : "Blocked/Error");
    append_log(gui_data, log_msg);

    update_process_list(gui_data);
    update_queue_lists(gui_data);
    update_memory_view(gui_data);
    update_mutex_status(gui_data);
    update_dashboard(gui_data);
}

void on_auto_clicked(GtkButton* button, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    if (gui_data->timeout_id == 0) {
        append_log(gui_data, "Starting auto-run...");
        gui_data->timeout_id = g_timeout_add(1000, update_gui_timeout, gui_data);
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Stop Auto");
    } else {
        g_source_remove(gui_data->timeout_id);
        gui_data->timeout_id = 0;
        append_log(gui_data, "Auto-run stopped.");
        gtk_button_set_label(GTK_BUTTON(gui_data->auto_button), "Auto Run");
    }
}

void on_add_process_clicked(GtkButton* button, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Select Process File",
                                                    GTK_WINDOW(gui_data->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        Process* proc = build_process(gui_data->engine->memory, gui_data->engine->process_count + 1, filename);
        if (proc) {
            gui_data->engine->processes[gui_data->engine->process_count++] = proc;
            enqueue(&gui_data->engine->ready_queues[gui_data->engine->algorithm == 2 ? proc->pcb.priority - 1 : 0], proc);
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "Added process PID %d from %s", proc->pcb.process_id, filename);
            append_log(gui_data, log_msg);
            update_process_list(gui_data);
            update_queue_lists(gui_data);
            update_memory_view(gui_data);
            update_dashboard(gui_data);
        } else {
            append_log(gui_data, "Failed to load process.");
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_algorithm_changed(GtkComboBox* combo, gpointer user_data) {
    GuiData* gui_data = (GuiData*)user_data;
    gui_data->engine->algorithm = gtk_combo_box_get_active(combo);
    gui_data->engine->quantum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui_data->quantum_spin));
    init_simulation(gui_data->engine, gui_data->engine->memory, gui_data->engine->mutex_ctrl,
                    gui_data->engine->processes, gui_data->engine->process_count,
                    gui_data->engine->algorithm, gui_data->engine->quantum);
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Algorithm changed to %s",
             gui_data->engine->algorithm == 0 ? "FCFS" :
             gui_data->engine->algorithm == 1 ? "Round Robin" : "MLFQ");
    append_log(gui_data, log_msg);
    update_queue_lists(gui_data);
    update_dashboard(gui_data);
}

void create_gtk_gui(SimulationEngine* engine) {
    gtk_init(NULL, NULL);
    GuiData* gui_data = g_malloc0(sizeof(GuiData));
    gui_data->engine = engine;
    gui_data->timeout_id = 0;

    gui_data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui_data->window), "OS Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(gui_data->window), 1200, 800);
    g_signal_connect(gui_data->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(gui_data->window), main_box);

    // Dashboard
    GtkWidget* dashboard = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gui_data->clock_label = gtk_label_new("Clock Cycle: 0");
    gui_data->process_count_label = gtk_label_new("Total Processes: 0");
    gui_data->algorithm_label = gtk_label_new("Algorithm: None");
    gtk_box_pack_start(GTK_BOX(dashboard), gui_data->clock_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dashboard), gui_data->process_count_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dashboard), gui_data->algorithm_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), dashboard, FALSE, FALSE, 5);

    // Control Panel
    GtkWidget* control_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gui_data->algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui_data->algorithm_combo), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui_data->algorithm_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui_data->algorithm_combo), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui_data->algorithm_combo), engine->algorithm);
    g_signal_connect(gui_data->algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), gui_data);

    gui_data->quantum_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui_data->quantum_spin), engine->quantum);
    gui_data->start_button = gtk_button_new_with_label("Start");
    gui_data->stop_button = gtk_button_new_with_label("Stop");
    gui_data->reset_button = gtk_button_new_with_label("Reset");
    gui_data->step_button = gtk_button_new_with_label("Step");
    gui_data->auto_button = gtk_button_new_with_label("Auto Run");
    gui_data->add_process_button = gtk_button_new_with_label("Add Process");

    g_signal_connect(gui_data->start_button, "clicked", G_CALLBACK(on_start_clicked), gui_data);
    g_signal_connect(gui_data->stop_button, "clicked", G_CALLBACK(on_stop_clicked), gui_data);
    g_signal_connect(gui_data->reset_button, "clicked", G_CALLBACK(on_reset_clicked), gui_data);
    g_signal_connect(gui_data->step_button, "clicked", G_CALLBACK(on_step_clicked), gui_data);
    g_signal_connect(gui_data->auto_button, "clicked", G_CALLBACK(on_auto_clicked), gui_data);
    g_signal_connect(gui_data->add_process_button, "clicked", G_CALLBACK(on_add_process_clicked), gui_data);

    gtk_box_pack_start(GTK_BOX(control_panel), gtk_label_new("Algorithm:"), FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->algorithm_combo, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gtk_label_new("Quantum:"), FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->quantum_spin, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->start_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->stop_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->reset_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->step_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->auto_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(control_panel), gui_data->add_process_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), control_panel, FALSE, FALSE, 5);

    // Process and Queue Lists
    GtkWidget* lists_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    gui_data->process_list = gtk_tree_view_new();
    GtkListStore* process_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gui_data->process_list), GTK_TREE_MODEL(process_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->process_list), gtk_tree_view_column_new_with_attributes("PID", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->process_list), gtk_tree_view_column_new_with_attributes("State", gtk_cell_renderer_text_new(), "text", 1, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->process_list), gtk_tree_view_column_new_with_attributes("Priority", gtk_cell_renderer_text_new(), "text", 2, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->process_list), gtk_tree_view_column_new_with_attributes("PC", gtk_cell_renderer_text_new(), "text", 3, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->process_list), gtk_tree_view_column_new_with_attributes("Mem Lower", gtk_cell_renderer_text_new(), "text", 4, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->process_list), gtk_tree_view_column_new_with_attributes("Mem Upper", gtk_cell_renderer_text_new(), "text", 5, NULL));
    GtkWidget* process_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(process_scroll), gui_data->process_list);
    gtk_box_pack_start(GTK_BOX(lists_box), process_scroll, TRUE, TRUE, 5);

    gui_data->ready_queue_list = gtk_tree_view_new();
    GtkListStore* ready_queue_store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gui_data->ready_queue_list), GTK_TREE_MODEL(ready_queue_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->ready_queue_list), gtk_tree_view_column_new_with_attributes("PID", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->ready_queue_list), gtk_tree_view_column_new_with_attributes("Queue", gtk_cell_renderer_text_new(), "text", 1, NULL));
    GtkWidget* ready_queue_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(ready_queue_scroll), gui_data->ready_queue_list);
    gtk_box_pack_start(GTK_BOX(lists_box), ready_queue_scroll, TRUE, TRUE, 5);

    gui_data->blocked_queue_list = gtk_tree_view_new();
    GtkListStore* blocked_queue_store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gui_data->blocked_queue_list), GTK_TREE_MODEL(blocked_queue_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->blocked_queue_list), gtk_tree_view_column_new_with_attributes("PID", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->blocked_queue_list), gtk_tree_view_column_new_with_attributes("Resource", gtk_cell_renderer_text_new(), "text", 1, NULL));
    GtkWidget* blocked_queue_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(blocked_queue_scroll), gui_data->blocked_queue_list);
    gtk_box_pack_start(GTK_BOX(lists_box), blocked_queue_scroll, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), lists_box, TRUE, TRUE, 5);

    // Memory and Mutex
    GtkWidget* mem_mutex_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    gui_data->memory_view = gtk_tree_view_new();
    GtkListStore* memory_store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gui_data->memory_view), GTK_TREE_MODEL(memory_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->memory_view), gtk_tree_view_column_new_with_attributes("Index", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->memory_view), gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 1, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->memory_view), gtk_tree_view_column_new_with_attributes("Value", gtk_cell_renderer_text_new(), "text", 2, NULL));
    GtkWidget* memory_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(memory_scroll), gui_data->memory_view);
    gtk_box_pack_start(GTK_BOX(mem_mutex_box), memory_scroll, TRUE, TRUE, 5);

    gui_data->mutex_status = gtk_tree_view_new();
    GtkListStore* mutex_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gui_data->mutex_status), GTK_TREE_MODEL(mutex_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->mutex_status), gtk_tree_view_column_new_with_attributes("Resource", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui_data->mutex_status), gtk_tree_view_column_new_with_attributes("Status", gtk_cell_renderer_text_new(), "text", 1, NULL));
    GtkWidget* mutex_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(mutex_scroll), gui_data->mutex_status);
    gtk_box_pack_start(GTK_BOX(mem_mutex_box), mutex_scroll, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), mem_mutex_box, TRUE, TRUE, 5);

    // Log View
    gui_data->log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gui_data->log_view), FALSE);
    GtkWidget* log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(log_scroll), gui_data->log_view);
    gtk_box_pack_start(GTK_BOX(main_box), log_scroll, TRUE, TRUE, 5);

    update_process_list(gui_data);
    update_queue_lists(gui_data);
    update_memory_view(gui_data);
    update_mutex_status(gui_data);
    update_dashboard(gui_data);

    gtk_widget_show_all(gui_data->window);
    gtk_main();
}

