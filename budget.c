#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>

typedef enum { INCOME, EXPENSE } TransactionType;

typedef struct {
    TransactionType type;
    char description[50];
    float amount;
    char date[11]; // 格式: YYYY-MM-DD
} Transaction;

Transaction *transactions = NULL;
int transactionCount = 0;
int selectedTransactionIndex = -1;

GtkWidget *entry_desc, *entry_amount, *entry_date, *combo_type, *text_view;
GtkWidget *delete_button, *edit_button, *update_button, *cancel_button;
GtkWidget *treeview;
GtkListStore *list_store;
GtkWidget *chart_area = NULL;
GtkTextBuffer *buffer;
GtkWidget *main_window; // 儲存主視窗以便全局訪問

// 函式宣告
void addTransaction(GtkWidget *widget, gpointer data);
void viewTransactions();
void saveTransactions();
void loadTransactions();
void freeTransactions();
void deleteTransaction(GtkWidget *widget, gpointer data);
void prepareEditTransaction(GtkWidget *widget, gpointer data);
void updateTransaction(GtkWidget *widget, gpointer data);
void cancelEdit(GtkWidget *widget, gpointer data);
void refreshTreeView();
void updateTotalBalance();
gboolean createChart(GtkWidget *widget, cairo_t *cr, gpointer data);
void populateDataForChart(float *income_data, float *expense_data, int *num_months);
void showChart(GtkWidget *widget, gpointer data);
gboolean on_treeview_selection_changed(GtkTreeSelection *selection, gpointer data);
void setButtonStates(gboolean editing);

void addTransaction(GtkWidget *widget, gpointer data) {
    transactionCount++;
    transactions = realloc(transactions, transactionCount * sizeof(Transaction));

    transactions[transactionCount - 1].type = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_type));
    const char *desc = gtk_entry_get_text(GTK_ENTRY(entry_desc));
    strncpy(transactions[transactionCount - 1].description, desc, 50);
    transactions[transactionCount - 1].description[49] = '\0';

    transactions[transactionCount - 1].amount = atof(gtk_entry_get_text(GTK_ENTRY(entry_amount)));
    
    const char *date = gtk_entry_get_text(GTK_ENTRY(entry_date));
    if (strlen(date) > 0) {
        strncpy(transactions[transactionCount - 1].date, date, 10);
        transactions[transactionCount - 1].date[10] = '\0';
    } else {
        // 如果沒有輸入日期，使用當前日期
        GDateTime *now = g_date_time_new_now_local();
        char *date_str = g_date_time_format(now, "%Y-%m-%d");
        strncpy(transactions[transactionCount - 1].date, date_str, 10);
        transactions[transactionCount - 1].date[10] = '\0';
        g_free(date_str);
        g_date_time_unref(now);
    }

    // 存檔
    saveTransactions();

    // 刷新界面
    refreshTreeView();
    viewTransactions();
    updateTotalBalance();

    // 如果圖表已存在，重新繪製
    if (chart_area != NULL && GTK_IS_WIDGET(chart_area)) {
        gtk_widget_queue_draw(chart_area);
    }

    // 清空輸入框
    gtk_entry_set_text(GTK_ENTRY(entry_desc), "");
    gtk_entry_set_text(GTK_ENTRY(entry_amount), "");
    gtk_entry_set_text(GTK_ENTRY(entry_date), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_type), 0);
}

void viewTransactions() {
    gtk_text_buffer_set_text(buffer, "", -1);
    
    float totalIncome = 0, totalExpense = 0;
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);
    
    for (int i = 0; i < transactionCount; i++) {
        gtk_text_buffer_insert(buffer, &iter, transactions[i].type == INCOME ? "[收入] " : "[支出] ", -1);
        
        char dateAndDesc[100];
        snprintf(dateAndDesc, sizeof(dateAndDesc), "%s | %s", 
                transactions[i].date, transactions[i].description);
        gtk_text_buffer_insert(buffer, &iter, dateAndDesc, -1);
        
        char amountStr[20];
        snprintf(amountStr, sizeof(amountStr), " | %.2f\n", transactions[i].amount);
        gtk_text_buffer_insert(buffer, &iter, amountStr, -1);
        
        if (transactions[i].type == INCOME)
            totalIncome += transactions[i].amount;
        else
            totalExpense += transactions[i].amount;
    }

    char summary[100];
    snprintf(summary, sizeof(summary), "\n💰 總收入: %.2f\n💸 總支出: %.2f\n📊 結餘: %.2f\n", 
             totalIncome, totalExpense, totalIncome - totalExpense);
    gtk_text_buffer_insert(buffer, &iter, summary, -1);
}

void refreshTreeView() {
    gtk_list_store_clear(list_store);
    
    for (int i = 0; i < transactionCount; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        
        gtk_list_store_set(list_store, &iter,
                          0, i + 1, // ID
                          1, transactions[i].date,
                          2, transactions[i].type == INCOME ? "收入" : "支出",
                          3, transactions[i].description,
                          4, transactions[i].amount,
                          -1);
    }
}

void updateTotalBalance() {
    float totalIncome = 0, totalExpense = 0;
    for (int i = 0; i < transactionCount; i++) {
        if (transactions[i].type == INCOME)
            totalIncome += transactions[i].amount;
        else
            totalExpense += transactions[i].amount;
    }
    
    float balance = totalIncome - totalExpense;
    
    char balance_text[100];
    snprintf(balance_text, sizeof(balance_text), 
             "<span font_desc='16' weight='bold' foreground='%s'>目前總金額: %.2f</span>", 
             (balance >= 0) ? "green" : "red", balance);
    
    GtkWidget *balance_label = g_object_get_data(G_OBJECT(main_window), "balance_label");
    gtk_label_set_markup(GTK_LABEL(balance_label), balance_text);
}
void saveTransactions() {
    FILE *file = fopen("records.txt", "w");
    if (file == NULL) return;

    for (int i = 0; i < transactionCount; i++) {
        fprintf(file, "%d %s %.2f %s\n", 
                transactions[i].type, 
                transactions[i].description, 
                transactions[i].amount,
                transactions[i].date);
    }
    
    fclose(file);
}

void loadTransactions() {
    FILE *file = fopen("records.txt", "r");
    if (file == NULL) return;
    
    // 先清空現有的交易記錄
    transactionCount = 0;
    
    char line[200];
    while (fgets(line, sizeof(line), file)) {
        transactionCount++;
        transactions = realloc(transactions, transactionCount * sizeof(Transaction));
        
        int type;
        char desc[50];
        float amount;
        char date[11] = "2025-01-01"; // 預設日期
        
        // 嘗試解析包含日期的格式
        if (sscanf(line, "%d %s %f %10s", &type, desc, &amount, date) >= 3) {
            transactions[transactionCount - 1].type = (type == 0) ? INCOME : EXPENSE;
            strcpy(transactions[transactionCount - 1].description, desc);
            transactions[transactionCount - 1].amount = amount;
            strcpy(transactions[transactionCount - 1].date, date);
        }
    }

    fclose(file);
}

void freeTransactions() {
    free(transactions);
}

void deleteTransaction(GtkWidget *widget, gpointer data) {
    if (selectedTransactionIndex < 0 || selectedTransactionIndex >= transactionCount) 
        return;
    
    // 刪除選中的交易
    for (int i = selectedTransactionIndex; i < transactionCount - 1; i++) {
        transactions[i] = transactions[i + 1];
    }
    
    transactionCount--;
    transactions = realloc(transactions, transactionCount * sizeof(Transaction));
    
    // 更新UI
    saveTransactions();
    refreshTreeView();
    viewTransactions();
    updateTotalBalance();
    
    // 如果圖表已存在，重新繪製
    if (chart_area != NULL && GTK_IS_WIDGET(chart_area)) {
        gtk_widget_queue_draw(chart_area);
    }
    
    // 重置選擇
    selectedTransactionIndex = -1;
    gtk_widget_set_sensitive(delete_button, FALSE);
    gtk_widget_set_sensitive(edit_button, FALSE);
}

void prepareEditTransaction(GtkWidget *widget, gpointer data) {
    if (selectedTransactionIndex < 0 || selectedTransactionIndex >= transactionCount) 
        return;
    
    // 填充編輯框
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_type), transactions[selectedTransactionIndex].type);
    gtk_entry_set_text(GTK_ENTRY(entry_desc), transactions[selectedTransactionIndex].description);
    
    char amount_str[20];
    snprintf(amount_str, sizeof(amount_str), "%.2f", transactions[selectedTransactionIndex].amount);
    gtk_entry_set_text(GTK_ENTRY(entry_amount), amount_str);
    
    gtk_entry_set_text(GTK_ENTRY(entry_date), transactions[selectedTransactionIndex].date);
    
    // 更改按鈕狀態
    setButtonStates(TRUE);
}

void updateTransaction(GtkWidget *widget, gpointer data) {
    if (selectedTransactionIndex < 0 || selectedTransactionIndex >= transactionCount) 
        return;
    
    // 更新選中的交易
    transactions[selectedTransactionIndex].type = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_type));
    
    const char *desc = gtk_entry_get_text(GTK_ENTRY(entry_desc));
    strncpy(transactions[selectedTransactionIndex].description, desc, 50);
    transactions[selectedTransactionIndex].description[49] = '\0';
    
    transactions[selectedTransactionIndex].amount = atof(gtk_entry_get_text(GTK_ENTRY(entry_amount)));
    
    const char *date = gtk_entry_get_text(GTK_ENTRY(entry_date));
    strncpy(transactions[selectedTransactionIndex].date, date, 10);
    transactions[selectedTransactionIndex].date[10] = '\0';
    
    // 更新UI
    saveTransactions();
    refreshTreeView();
    viewTransactions();
    updateTotalBalance();
    
    // 如果圖表已存在，重新繪製
    if (chart_area != NULL && GTK_IS_WIDGET(chart_area)) {
        gtk_widget_queue_draw(chart_area);
    }
    
    // 清空輸入框並重置按鈕狀態
    gtk_entry_set_text(GTK_ENTRY(entry_desc), "");
    gtk_entry_set_text(GTK_ENTRY(entry_amount), "");
    gtk_entry_set_text(GTK_ENTRY(entry_date), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_type), 0);
    
    setButtonStates(FALSE);
}

void cancelEdit(GtkWidget *widget, gpointer data) {
    // 清空輸入框
    gtk_entry_set_text(GTK_ENTRY(entry_desc), "");
    gtk_entry_set_text(GTK_ENTRY(entry_amount), "");
    gtk_entry_set_text(GTK_ENTRY(entry_date), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_type), 0);
    
    // 重置按鈕狀態
    setButtonStates(FALSE);
}

void setButtonStates(gboolean editing) {
    gtk_widget_set_visible(update_button, editing);
    gtk_widget_set_visible(cancel_button, editing);
    gtk_widget_set_visible(delete_button, !editing);
    gtk_widget_set_visible(edit_button, !editing);
    
    GtkWidget *add_button = g_object_get_data(G_OBJECT(update_button), "add_button");
    gtk_widget_set_sensitive(add_button, !editing);
}

gboolean on_treeview_selection_changed(GtkTreeSelection *selection, gpointer data) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gint id;
        gtk_tree_model_get(model, &iter, 0, &id, -1);
        
        selectedTransactionIndex = id - 1;
        
        // 如果沒有在編輯模式中，啟用刪除和編輯按鈕
        if (!gtk_widget_get_visible(update_button)) {
            gtk_widget_set_sensitive(delete_button, TRUE);
            gtk_widget_set_sensitive(edit_button, TRUE);
        }
        
        return TRUE;
    }
    
    selectedTransactionIndex = -1;
    gtk_widget_set_sensitive(delete_button, FALSE);
    gtk_widget_set_sensitive(edit_button, FALSE);
    return FALSE;
}

void populateDataForChart(float *income_data, float *expense_data, int *num_months) {
    // 初始化數據
    memset(income_data, 0, 12 * sizeof(float));
    memset(expense_data, 0, 12 * sizeof(float));
    *num_months = 0;
    
    // 計算每個月的收入和支出
    for (int i = 0; i < transactionCount; i++) {
        int year, month, day;
        if (sscanf(transactions[i].date, "%d-%d-%d", &year, &month, &day) == 3) {
            if (month >= 1 && month <= 12) {
                if (transactions[i].type == INCOME) {
                    income_data[month-1] += transactions[i].amount;
                } else {
                    expense_data[month-1] += transactions[i].amount;
                }
                
                if (income_data[month-1] > 0 || expense_data[month-1] > 0) {
                    if (month > *num_months) *num_months = month;
                }
            }
        }
    }
}

gboolean createChart(GtkWidget *widget, cairo_t *cr, gpointer data) {
    float income_data[12] = {0};
    float expense_data[12] = {0};
    int num_months = 0;
    
    populateDataForChart(income_data, expense_data, &num_months);
    
    if (num_months == 0) {
        // 沒有數據
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 15);
        cairo_move_to(cr, 50, 100);
        cairo_show_text(cr, "沒有足夠的數據來生成圖表");
        return FALSE;
    }
    
    // 圖表尺寸和邊距
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int margin = 50;
    int graph_width = width - 2 * margin;
    int graph_height = height - 2 * margin;
    
    // 找出最大值以縮放圖表
    float max_value = 0;
    for (int i = 0; i < num_months; i++) {
        if (income_data[i] > max_value) max_value = income_data[i];
        if (expense_data[i] > max_value) max_value = expense_data[i];
    }
    
    // 稍微增加最大值，避免圖表頂到最高處
    max_value *= 1.1;
    
    // 繪製座標軸
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    
    // Y軸
    cairo_move_to(cr, margin, margin);
    cairo_line_to(cr, margin, height - margin);
    cairo_stroke(cr);
    
    // X軸
    cairo_move_to(cr, margin, height - margin);
    cairo_line_to(cr, width - margin, height - margin);
    cairo_stroke(cr);
    
    // 設定字體，支援中文
    cairo_select_font_face(cr, "Noto Sans CJK TC", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    
    // 繪製月份標籤
    const char *month_names[] = {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"};
    
    float bar_width = graph_width / (num_months * 2 + 1);
    
    for (int i = 0; i < num_months; i++) {
        float x = margin + (i * 2 + 1) * bar_width;
        
        cairo_move_to(cr, x, height - margin + 15);
        cairo_show_text(cr, month_names[i]);
    }
    
    // 繪製Y軸刻度
    int y_divisions = 5;
    for (int i = 0; i <= y_divisions; i++) {
        float y_pos = height - margin - (i * graph_height / y_divisions);
        float value = i * max_value / y_divisions;
        
        cairo_move_to(cr, margin - 5, y_pos);
        cairo_line_to(cr, margin, y_pos);
        cairo_stroke(cr);
        
        char value_str[20];
        snprintf(value_str, sizeof(value_str), "%.0f", value);
        
        cairo_move_to(cr, margin - 35, y_pos + 5);
        cairo_show_text(cr, value_str);
    }
    
    // 繪製收入柱狀圖（藍色）
    cairo_set_source_rgb(cr, 0.3, 0.5, 0.8);
    
    for (int i = 0; i < num_months; i++) {
        float x = margin + i * 2 * bar_width;
        float bar_height = (income_data[i] / max_value) * graph_height;
        
        cairo_rectangle(cr, x, height - margin - bar_height, bar_width * 0.8, bar_height);
        cairo_fill(cr);
    }
    
    // 繪製支出柱狀圖（紅色）
    cairo_set_source_rgb(cr, 0.8, 0.3, 0.3);
    
    for (int i = 0; i < num_months; i++) {
        float x = margin + (i * 2 + 1) * bar_width;
        float bar_height = (expense_data[i] / max_value) * graph_height;
        
        cairo_rectangle(cr, x, height - margin - bar_height, bar_width * 0.8, bar_height);
        cairo_fill(cr);
    }
    
    // 繪製圖例
    cairo_set_source_rgb(cr, 0.3, 0.5, 0.8);
    cairo_rectangle(cr, width - margin - 80, margin, 15, 15);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, width - margin - 60, margin + 10);
    cairo_show_text(cr, "收入");
    
    cairo_set_source_rgb(cr, 0.8, 0.3, 0.3);
    cairo_rectangle(cr, width - margin - 80, margin + 20, 15, 15);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, width - margin - 60, margin + 30);
    cairo_show_text(cr, "支出");
    
    return FALSE;
}

void showChart(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("收支分析圖表",
                                                  GTK_WINDOW(data),
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  "關閉", GTK_RESPONSE_CLOSE,
                                                  NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 550, 350);
    chart_area = drawing_area; // 儲存圖表區域引用以便更新
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(createChart), NULL);
    
    gtk_container_add(GTK_CONTAINER(content_area), drawing_area);
    gtk_widget_show_all(dialog);
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    chart_area = NULL; // 對話框關閉時重置圖表區域引用
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    gtk_init(&argc, &argv);

    loadTransactions();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    main_window = window; // 儲存主視窗引用
    gtk_window_set_title(GTK_WINDOW(window), "進階記帳軟體");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 1200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_box);
    
    // 總金額顯示區域
    GtkWidget *balance_frame = gtk_frame_new("資產狀況");
    gtk_box_pack_start(GTK_BOX(main_box), balance_frame, FALSE, FALSE, 5);

    GtkWidget *balance_label = gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(balance_frame), balance_label);
    gtk_label_set_markup(GTK_LABEL(balance_label), "<span font_desc='16' weight='bold'>目前總金額: 0.00</span>");
    g_object_set_data(G_OBJECT(window), "balance_label", balance_label);
    
    // 上方編輯區域
    GtkWidget *editor_frame = gtk_frame_new("交易資料");
    gtk_box_pack_start(GTK_BOX(main_box), editor_frame, FALSE, FALSE, 0);
    
    GtkWidget *editor_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(editor_frame), editor_box);
    
    GtkWidget *input_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(input_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(input_grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(input_grid), 10);
    gtk_box_pack_start(GTK_BOX(editor_box), input_grid, FALSE, FALSE, 0);
    
    GtkWidget *type_label = gtk_label_new("交易類型:");
    gtk_grid_attach(GTK_GRID(input_grid), type_label, 0, 0, 1, 1);
    combo_type = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_type), NULL, "收入");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_type), NULL, "支出");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_type), 0);
    gtk_grid_attach(GTK_GRID(input_grid), combo_type, 1, 0, 1, 1);
    
    GtkWidget *desc_label = gtk_label_new("交易描述:");
    gtk_grid_attach(GTK_GRID(input_grid), desc_label, 0, 1, 1, 1);
    entry_desc = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_desc), "輸入交易描述...");
    gtk_grid_attach(GTK_GRID(input_grid), entry_desc, 1, 1, 1, 1);
    
    GtkWidget *amount_label = gtk_label_new("金額:");
    gtk_grid_attach(GTK_GRID(input_grid), amount_label, 0, 2, 1, 1);
    entry_amount = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_amount), "輸入金額...");
    gtk_grid_attach(GTK_GRID(input_grid), entry_amount, 1, 2, 1, 1);
    
    GtkWidget *date_label = gtk_label_new("日期:");
    gtk_grid_attach(GTK_GRID(input_grid), date_label, 0, 3, 1, 1);
    entry_date = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_date), "YYYY-MM-DD");
    gtk_grid_attach(GTK_GRID(input_grid), entry_date, 1, 3, 1, 1);
    
    // 填入當前日期
    GDateTime *now = g_date_time_new_now_local();
    char *date_str = g_date_time_format(now, "%Y-%m-%d");
    gtk_entry_set_text(GTK_ENTRY(entry_date), date_str);
    g_free(date_str);
    g_date_time_unref(now);
    
    // 按鈕區域
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(button_box), 10);
    gtk_box_pack_start(GTK_BOX(editor_box), button_box, FALSE, FALSE, 0);
    
    GtkWidget *add_button = gtk_button_new_with_label("新增交易");
    g_signal_connect(add_button, "clicked", G_CALLBACK(addTransaction), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), add_button, TRUE, TRUE, 0);
    
    edit_button = gtk_button_new_with_label("修改交易");
    g_signal_connect(edit_button, "clicked", G_CALLBACK(prepareEditTransaction), NULL);
    gtk_widget_set_sensitive(edit_button, FALSE);
    gtk_box_pack_start(GTK_BOX(button_box), edit_button, TRUE, TRUE, 0);
    
    delete_button = gtk_button_new_with_label("刪除交易");
    g_signal_connect(delete_button, "clicked", G_CALLBACK(deleteTransaction), NULL);
    gtk_widget_set_sensitive(delete_button, FALSE);
    gtk_box_pack_start(GTK_BOX(button_box), delete_button, TRUE, TRUE, 0);
    
    update_button = gtk_button_new_with_label("更新交易");
    g_signal_connect(update_button, "clicked", G_CALLBACK(updateTransaction), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), update_button, TRUE, TRUE, 0);
    gtk_widget_set_visible(update_button, FALSE);
    
    cancel_button = gtk_button_new_with_label("取消編輯");
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(cancelEdit), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_button, TRUE, TRUE, 0);
    gtk_widget_set_visible(cancel_button, FALSE);
    
    // 儲存add_button的參考以供後續使用
    g_object_set_data(G_OBJECT(update_button), "add_button", add_button);
    
    GtkWidget *chart_button = gtk_button_new_with_label("收支分析圖表");
    g_signal_connect(chart_button, "clicked", G_CALLBACK(showChart), window);
    gtk_box_pack_start(GTK_BOX(button_box), chart_button, TRUE, TRUE, 0);
    
    // 下方表格和總覽區域
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);
    
    // 交易列表
    GtkWidget *treeview_frame = gtk_frame_new("交易記錄");
    gtk_paned_add1(GTK_PANED(paned), treeview_frame);

    // 建立表格模型
    list_store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    
    // 加入各列
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("日期", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("類型", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("描述", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("金額", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    // 設置選擇模式
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(on_treeview_selection_changed), NULL);
    
    // 添加滾動窗口
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(treeview_frame), scrolled_window);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    
    // 下方交易摘要
    GtkWidget *summary_frame = gtk_frame_new("交易摘要");
    gtk_paned_add2(GTK_PANED(paned), summary_frame);
    
    GtkWidget *summary_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(summary_frame), summary_scroll);
    
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(summary_scroll), text_view);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    // 初始化界面
    refreshTreeView();
    viewTransactions();
    updateTotalBalance();
    
    // 設定垂直分割條位置
    gtk_paned_set_position(GTK_PANED(paned), 300);

    gtk_widget_show_all(window);
    gtk_main();

    freeTransactions();
    return 0;
}