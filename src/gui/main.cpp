#include <adwaita.h>
#include <gtk/gtk.h>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>

#include "../core/operations.hpp"
#include "../core/error.hpp"

using namespace photoscrypto;
namespace fs = std::filesystem;

struct AppData {
    GtkWindow *window;
    GtkWidget *status_label;
};

struct EncodeContext {
    AppData* app_data;
    std::string photo_path;
    std::string password;
    bool self_destruct;
    int maxAttempts;
    GtkWidget* pass_dialog;
};

struct DecodeContext {
    AppData* app_data;
    std::string text_path;
    std::string password;
    DecodedData decoded;
    GtkWidget* pass_dialog;
};

struct ShieldState {
    int score = 0; // 0 to 5
};

struct PasswordWatchContext {
    GtkWidget* min_label;
    GtkWidget* shield_area;
    ShieldState shield_state;
};

static void draw_shield_func(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data) {
    (void)area;
    ShieldState* state = static_cast<ShieldState*>(user_data);
    int score = state ? state->score : 0;
    if (score < 0) score = 0;
    if (score > 5) score = 5;

    double w = width;
    (void)height;
    double cx = w / 2.0;

    auto create_shield_path = [&]() {
        cairo_new_path(cr);
        cairo_move_to(cr, cx - 13, 6);
        cairo_curve_to(cr, cx - 6, 4, cx + 6, 4, cx + 13, 6);
        cairo_curve_to(cr, cx + 14, 18, cx + 10, 26, cx, 31);
        cairo_curve_to(cr, cx - 10, 26, cx - 14, 18, cx - 13, 6);
        cairo_close_path(cr);
    };

    double r = 0.45, g = 0.45, b = 0.50;
    switch (score) {
        case 1: r = 0.90; g = 0.15; b = 0.15; break; // Red
        case 2: r = 1.00; g = 0.50; b = 0.00; break; // Orange
        case 3: r = 0.95; g = 0.80; b = 0.10; break; // Yellow / Amber
        case 4: r = 0.45; g = 0.85; b = 0.35; break; // Light Green
        case 5: r = 0.15; g = 0.78; b = 0.45; break; // Emerald Green
        default: r = 0.45; g = 0.45; b = 0.50; break;
    }

    // 1. Draw base shield background
    create_shield_path();
    cairo_set_source_rgba(cr, 0.12, 0.15, 0.22, 0.85);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.35, 0.45, 0.6);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // 2. Fill the shield progressively from bottom to top in 5 steps
    if (score > 0) {
        cairo_save(cr);
        create_shield_path();
        cairo_clip(cr);

        // Fill fraction: 1/5, 2/5, 3/5, 4/5, 5/5
        double fill_fraction = static_cast<double>(score) / 5.0;
        double fill_y = 31.0 - (26.0 * fill_fraction);

        cairo_pattern_t* grad = cairo_pattern_create_linear(cx, 31, cx, fill_y);
        cairo_pattern_add_color_stop_rgba(grad, 0.0, r, g, b, 0.95);
        cairo_pattern_add_color_stop_rgba(grad, 1.0, r, g, b, 0.65);
        cairo_set_source(cr, grad);
        cairo_rectangle(cr, 0, fill_y, w, 32 - fill_y);
        cairo_fill(cr);
        cairo_pattern_destroy(grad);

        cairo_restore(cr);

        // Outer stroke in step color
        create_shield_path();
        cairo_set_source_rgba(cr, r, g, b, 0.95);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke(cr);
    }

    // 3. Center icon inside shield
    if (score == 5) {
        cairo_new_path(cr);
        cairo_move_to(cr, cx - 5.5, 17.5);
        cairo_line_to(cr, cx - 1.5, 21.5);
        cairo_line_to(cr, cx + 5.5, 13.5);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_set_line_width(cr, 2.2);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_stroke(cr);
    } else {
        cairo_new_path(cr);
        cairo_arc(cr, cx, 15, 2.4, 0, 2 * M_PI);
        cairo_move_to(cr, cx - 1.3, 16.5);
        cairo_line_to(cr, cx + 1.3, 16.5);
        cairo_line_to(cr, cx + 0.9, 21.0);
        cairo_line_to(cr, cx - 0.9, 21.0);
        cairo_close_path(cr);
        if (score == 0) {
            cairo_set_source_rgba(cr, 0.6, 0.6, 0.6, 0.5);
        } else {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        }
        cairo_fill(cr);
    }
}

static void show_message(GtkWindow* parent, const char* title, const char* message, GtkMessageType type) {
    (void)type;
    GtkWidget *dialog = adw_message_dialog_new(parent, title, message);
    adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "ok", "OK");
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
    gtk_widget_set_visible(dialog, true);
}

// ------------------- ENCODE FLOW -------------------

static void on_encode_file_saved(GObject* source_object, GAsyncResult* res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    EncodeContext* ctx = static_cast<EncodeContext*>(user_data);
    
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(dialog, res, &error);
    
    if (file) {
        char *save_path = g_file_get_path(file);
        try {
            encodeFile(ctx->photo_path, save_path, ctx->password, true, ctx->self_destruct, ctx->maxAttempts);
            show_message(ctx->app_data->window, "Success", ("File successfully saved to:\n" + std::string(save_path)).c_str(), GTK_MESSAGE_INFO);
            gtk_label_set_text(GTK_LABEL(ctx->app_data->status_label), "Last action: Encoding successful.");
        } catch (const Error& e) {
            show_message(ctx->app_data->window, "Error", e.what(), GTK_MESSAGE_ERROR);
        }
        g_free(save_path);
        g_object_unref(file);
    } else if (error) {
        g_error_free(error);
    }
    
    if (ctx->pass_dialog) {
        gtk_window_destroy(GTK_WINDOW(ctx->pass_dialog));
    }
    delete ctx;
}

static void on_encode_password_next(GtkButton*, gpointer user_data) {
    GtkWidget** widgets = static_cast<GtkWidget**>(user_data);
    EncodeContext* ctx = static_cast<EncodeContext*>(g_object_get_data(G_OBJECT(widgets[0]), "ctx"));
    
    GtkCheckButton* enable_pass_check = GTK_CHECK_BUTTON(widgets[1]);
    GtkEntry* pass_entry = GTK_ENTRY(widgets[2]);
    GtkEntry* conf_entry = GTK_ENTRY(widgets[3]);
    GtkCheckButton* destruct_check = GTK_CHECK_BUTTON(widgets[4]);
    GtkSpinButton* attempts_spin = GTK_SPIN_BUTTON(widgets[5]);

    const bool protect = gtk_check_button_get_active(enable_pass_check);

    if (protect) {
        std::string pass = gtk_editable_get_text(GTK_EDITABLE(pass_entry));
        std::string conf = gtk_editable_get_text(GTK_EDITABLE(conf_entry));

        if (pass.length() < kMinPasswordLength) {
            show_message(GTK_WINDOW(ctx->pass_dialog), "Error", ("Password must be at least " + std::to_string(kMinPasswordLength) + " characters!").c_str(), GTK_MESSAGE_ERROR);
            return;
        }

        if (pass != conf) {
            show_message(GTK_WINDOW(ctx->pass_dialog), "Error", "Passwords do not match!", GTK_MESSAGE_ERROR);
            return;
        }

        ctx->password = pass;
        ctx->self_destruct = gtk_check_button_get_active(destruct_check);
        ctx->maxAttempts = gtk_spin_button_get_value_as_int(attempts_spin);
    } else {
        ctx->password = "";
        ctx->self_destruct = false;
        ctx->maxAttempts = kDefaultAttempts;
    }
    
    gtk_widget_set_visible(ctx->pass_dialog, false);
    
    GtkFileDialog* save_dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(save_dialog, "Save Encoded File As");
    std::string default_name = fs::path(ctx->photo_path).stem().string() + ".txt";
    gtk_file_dialog_set_initial_name(save_dialog, default_name.c_str());
    
    gtk_file_dialog_save(save_dialog, ctx->app_data->window, nullptr, on_encode_file_saved, ctx);
}

static void on_password_changed(GtkEditable* editable, gpointer user_data) {
    PasswordWatchContext* pw = static_cast<PasswordWatchContext*>(user_data);
    const std::string pass = gtk_editable_get_text(editable);

    for (const char* css : {"error", "warning", "success", "dim-label"}) {
        gtk_widget_remove_css_class(pw->min_label, css);
    }

    int score = passwordStrengthScore(pass);
    pw->shield_state.score = score;
    gtk_widget_queue_draw(pw->shield_area);

    if (pass.empty()) {
        gtk_label_set_text(GTK_LABEL(pw->min_label), "Minimum 6 characters");
        gtk_widget_add_css_class(pw->min_label, "dim-label");
    } else {
        // Clear text while typing to prevent visual clutter
        gtk_label_set_text(GTK_LABEL(pw->min_label), "");
    }
}

static void prompt_encode_password(const char* photo_path, AppData* app_data) {
    EncodeContext* ctx = new EncodeContext{app_data, photo_path, "", false, kDefaultAttempts, nullptr};
    
    GtkWidget *dialog = gtk_window_new();
    ctx->pass_dialog = dialog;
    gtk_window_set_title(GTK_WINDOW(dialog), "Password & Security Options");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), app_data->window);
    gtk_window_set_modal(GTK_WINDOW(dialog), true);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 440, 420);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget *enable_pass_check = gtk_check_button_new_with_label("Enable Password Protection (Optional)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(enable_pass_check), true);
    gtk_box_append(GTK_BOX(box), enable_pass_check);

    GtkWidget *pass_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(box), pass_box);

    GtkWidget *pass_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), false);
    gtk_widget_set_tooltip_text(pass_entry, "Password (Minimum 6 characters)");
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Enter password");
    gtk_box_append(GTK_BOX(pass_box), pass_entry);

    // Row below password entry: Left = Minimum 6 characters label, Right = 5-step Security Shield
    GtkWidget *indicator_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(indicator_row, 2);
    gtk_widget_set_margin_bottom(indicator_row, 4);

    GtkWidget *min_label = gtk_label_new("Minimum 6 characters");
    gtk_widget_add_css_class(min_label, "dim-label");
    gtk_label_set_xalign(GTK_LABEL(min_label), 0.0f);
    gtk_widget_set_hexpand(min_label, TRUE);
    gtk_box_append(GTK_BOX(indicator_row), min_label);

    GtkWidget *shield_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(shield_area), 36);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(shield_area), 36);

    PasswordWatchContext* pw = new PasswordWatchContext{min_label, shield_area, {0}};
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(shield_area), draw_shield_func, &pw->shield_state, nullptr);
    gtk_box_append(GTK_BOX(indicator_row), shield_area);

    gtk_box_append(GTK_BOX(pass_box), indicator_row);

    g_signal_connect(pass_entry, "changed", G_CALLBACK(on_password_changed), pw);
    g_object_set_data_full(G_OBJECT(dialog), "pw_watch", pw, (GDestroyNotify)+[](gpointer p){ delete static_cast<PasswordWatchContext*>(p); });

    GtkWidget *conf_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(conf_entry), false);
    gtk_widget_set_tooltip_text(conf_entry, "Confirm Password");
    gtk_entry_set_placeholder_text(GTK_ENTRY(conf_entry), "Confirm password");
    gtk_box_append(GTK_BOX(pass_box), conf_entry);

    GtkWidget *destruct_check = gtk_check_button_new_with_label("Self-destruct after failed attempts");
    gtk_box_append(GTK_BOX(pass_box), destruct_check);

    GtkWidget *attempts_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(attempts_row), gtk_label_new("Allowed failed attempts:"));
    GtkWidget *attempts_spin = gtk_spin_button_new_with_range(kMinAttempts, kMaxAttempts, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(attempts_spin), kDefaultAttempts);
    gtk_box_append(GTK_BOX(attempts_row), attempts_spin);
    gtk_widget_set_sensitive(attempts_row, false);
    gtk_box_append(GTK_BOX(pass_box), attempts_row);
    g_object_bind_property(destruct_check, "active", attempts_row, "sensitive", G_BINDING_DEFAULT);

    // Bind enable_pass_check to pass_box sensitivity
    g_object_bind_property(enable_pass_check, "active", pass_box, "sensitive", G_BINDING_DEFAULT);

    GtkWidget *btn = gtk_button_new_with_label("Continue");
    gtk_widget_add_css_class(btn, "suggested-action");
    gtk_box_append(GTK_BOX(box), btn);

    GtkWidget** widgets = new GtkWidget*[6]{dialog, enable_pass_check, pass_entry, conf_entry, destruct_check, attempts_spin};
    g_object_set_data(G_OBJECT(dialog), "ctx", ctx);
    
    g_signal_connect_data(btn, "clicked", G_CALLBACK(on_encode_password_next), widgets, 
        (GClosureNotify)[](gpointer data, GClosure*) { delete[] static_cast<GtkWidget**>(data); }, (GConnectFlags)0);
    
    g_signal_connect(dialog, "destroy", G_CALLBACK(+[](GtkWidget* w, gpointer) {
        EncodeContext* c = static_cast<EncodeContext*>(g_object_get_data(G_OBJECT(w), "ctx"));
        if (c && gtk_widget_is_visible(w)) {
            delete c;
        }
    }), nullptr);

    gtk_widget_set_visible(dialog, true);
}


// ------------------- DECODE FLOW -------------------

static void on_decode_file_saved(GObject* source_object, GAsyncResult* res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    DecodeContext* ctx = static_cast<DecodeContext*>(user_data);
    
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(dialog, res, &error);
    
    if (file) {
        char *save_path = g_file_get_path(file);
        try {
            FILE* f = std::fopen(save_path, "wb");
            if (!f) throw Error(ErrorCode::Io, "Cannot open file for writing");
            if (std::fwrite(ctx->decoded.photo.data(), 1, ctx->decoded.photo.size(), f) != ctx->decoded.photo.size()) {
                std::fclose(f);
                throw Error(ErrorCode::Io, "Error writing file");
            }
            std::fclose(f);

            show_message(ctx->app_data->window, "Success", ("File successfully decoded to:\n" + std::string(save_path)).c_str(), GTK_MESSAGE_INFO);
            gtk_label_set_text(GTK_LABEL(ctx->app_data->status_label), "Last action: Decoding successful.");
        } catch (const Error& e) {
            show_message(ctx->app_data->window, "Write Error", e.what(), GTK_MESSAGE_ERROR);
        }
        g_free(save_path);
        g_object_unref(file);
    } else if (error) {
        g_error_free(error);
    }
    
    if (ctx->pass_dialog) {
        gtk_window_destroy(GTK_WINDOW(ctx->pass_dialog));
    }
    delete ctx;
}

static void on_decode_password_next(GtkButton*, gpointer user_data) {
    GtkWidget** widgets = static_cast<GtkWidget**>(user_data);
    DecodeContext* ctx = static_cast<DecodeContext*>(g_object_get_data(G_OBJECT(widgets[0]), "ctx"));
    
    GtkEntry* pass_entry = GTK_ENTRY(widgets[1]);
    std::string pass = gtk_editable_get_text(GTK_EDITABLE(pass_entry));
    
    if (pass.length() < kMinPasswordLength) {
        show_message(GTK_WINDOW(ctx->pass_dialog), "Error", ("Password must be at least " + std::to_string(kMinPasswordLength) + " characters!").c_str(), GTK_MESSAGE_ERROR);
        return;
    }

    try {
        ctx->decoded = decodeToMemory(ctx->text_path, pass);
        
        gtk_widget_set_visible(ctx->pass_dialog, false);
        
        GtkFileDialog* save_dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(save_dialog, "Save Decoded File As");
        
        std::string default_name = fs::path(ctx->text_path).stem().string();
        if (!ctx->decoded.extension.empty()) {
            if (!fs::path(default_name).has_extension() || fs::path(default_name).extension() != ctx->decoded.extension) {
                 default_name += ctx->decoded.extension;
            }
        }
        gtk_file_dialog_set_initial_name(save_dialog, default_name.c_str());
        
        gtk_file_dialog_save(save_dialog, ctx->app_data->window, nullptr, on_decode_file_saved, ctx);

    } catch (const Error& e) {
        show_message(GTK_WINDOW(ctx->pass_dialog), "Decryption Error", e.what(), GTK_MESSAGE_ERROR);
    }
}

static void prompt_decode_password(const char* text_path, AppData* app_data) {
    DecodeContext* ctx = new DecodeContext{app_data, text_path, "", {}, nullptr};

    GtkWidget *dialog = gtk_window_new();
    ctx->pass_dialog = dialog;
    gtk_window_set_title(GTK_WINDOW(dialog), "Enter Password");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), app_data->window);
    gtk_window_set_modal(GTK_WINDOW(dialog), true);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 320, 160);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget *pass_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), false);
    gtk_widget_set_tooltip_text(pass_entry, "Password");
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Enter file password");
    gtk_box_append(GTK_BOX(box), pass_entry);

    GtkWidget *btn = gtk_button_new_with_label("Decode");
    gtk_widget_add_css_class(btn, "suggested-action");
    gtk_box_append(GTK_BOX(box), btn);

    GtkWidget** widgets = new GtkWidget*[2]{dialog, pass_entry};
    g_object_set_data(G_OBJECT(dialog), "ctx", ctx);
    
    g_signal_connect_data(btn, "clicked", G_CALLBACK(on_decode_password_next), widgets, 
        (GClosureNotify)[](gpointer data, GClosure*) { delete[] static_cast<GtkWidget**>(data); }, (GConnectFlags)0);

    g_signal_connect(dialog, "destroy", G_CALLBACK(+[](GtkWidget* w, gpointer) {
        DecodeContext* c = static_cast<DecodeContext*>(g_object_get_data(G_OBJECT(w), "ctx"));
        if (c && gtk_widget_is_visible(w)) {
            delete c;
        }
    }), nullptr);

    gtk_widget_set_visible(dialog, true);
}

static void start_decode_without_password(const char* text_path, AppData* app_data) {
    try {
        DecodeContext* ctx = new DecodeContext{app_data, text_path, "", {}, nullptr};
        ctx->decoded = decodeToMemory(text_path, "");
        
        GtkFileDialog* save_dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(save_dialog, "Save Decoded File As");
        
        std::string default_name = fs::path(ctx->text_path).stem().string();
        if (!ctx->decoded.extension.empty()) {
            if (!fs::path(default_name).has_extension() || fs::path(default_name).extension() != ctx->decoded.extension) {
                 default_name += ctx->decoded.extension;
            }
        }
        gtk_file_dialog_set_initial_name(save_dialog, default_name.c_str());
        gtk_file_dialog_save(save_dialog, app_data->window, nullptr, on_decode_file_saved, ctx);
    } catch (const Error& e) {
        show_message(app_data->window, "Decode Error", e.what(), GTK_MESSAGE_ERROR);
    }
}

// ------------------- FILE SELECTION -------------------

static void on_file_opened(GObject* source, GAsyncResult* res, gpointer user_data) {
    AppData* app_data = static_cast<AppData*>(user_data);
    GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
    
    GError* error = nullptr;
    GFile* file = gtk_file_dialog_open_finish(dialog, res, &error);
    
    if (file) {
        char *path = g_file_get_path(file);
        const char* title = gtk_file_dialog_get_title(dialog);
        if (std::string(title) == "Select Photo to Encode") {
            prompt_encode_password(path, app_data);
        } else {
            if (!isPasswordProtected(path)) {
                start_decode_without_password(path, app_data);
            } else {
                prompt_decode_password(path, app_data);
            }
        }
        g_free(path);
        g_object_unref(file);
    } else if (error) {
        g_error_free(error);
    }
}

static void on_btn_encode_clicked(GtkButton*, gpointer user_data) {
    AppData* app_data = static_cast<AppData*>(user_data);
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Photo to Encode");
    gtk_file_dialog_open(dialog, app_data->window, nullptr, on_file_opened, app_data);
}

static void on_btn_decode_clicked(GtkButton*, gpointer user_data) {
    AppData* app_data = static_cast<AppData*>(user_data);
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Text File to Decode");
    gtk_file_dialog_open(dialog, app_data->window, nullptr, on_file_opened, app_data);
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    AppData* app_data = static_cast<AppData*>(user_data);
    
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "PhotosCrypto");
    gtk_window_set_default_size(GTK_WINDOW(window), 440, 280);
    app_data->window = GTK_WINDOW(window);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 30);
    gtk_widget_set_margin_end(box, 30);
    gtk_widget_set_margin_top(box, 30);
    gtk_widget_set_margin_bottom(box, 30);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget *title = gtk_label_new("PhotosCrypto");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *desc = gtk_label_new("Securely encrypt and compress photos into text");
    gtk_widget_add_css_class(desc, "dim-label");
    gtk_box_append(GTK_BOX(box), desc);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), btn_box);

    GtkWidget *btn_encode = gtk_button_new_with_label("Encode (Hide)");
    gtk_widget_add_css_class(btn_encode, "suggested-action");
    gtk_widget_add_css_class(btn_encode, "pill");
    g_signal_connect(btn_encode, "clicked", G_CALLBACK(on_btn_encode_clicked), app_data);
    gtk_box_append(GTK_BOX(btn_box), btn_encode);

    GtkWidget *btn_decode = gtk_button_new_with_label("Decode (Restore)");
    gtk_widget_add_css_class(btn_decode, "pill");
    g_signal_connect(btn_decode, "clicked", G_CALLBACK(on_btn_decode_clicked), app_data);
    gtk_box_append(GTK_BOX(btn_box), btn_decode);

    app_data->status_label = gtk_label_new("Ready");
    gtk_widget_add_css_class(app_data->status_label, "dim-label");
    gtk_box_append(GTK_BOX(box), app_data->status_label);

    gtk_widget_set_visible(window, true);
}

int main(int argc, char **argv) {
    AdwApplication *app = adw_application_new("io.github.hunte_av.PhotosCrypto", G_APPLICATION_DEFAULT_FLAGS);
    AppData app_data{};
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), &app_data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
