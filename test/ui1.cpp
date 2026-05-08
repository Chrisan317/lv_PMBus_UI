/*
*背景替換成漸層色，10分鐘後當機
*/
#include "ui.h"
#include <Arduino.h>

#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif

// Constants
const unsigned long DEBOUNCE_DELAY_MS = 50;
const float MAX_POWER_WATTS = 1000.0;

// Display and Drawing Buffers
uint32_t draw_buf[DRAW_BUF_SIZE / 4]; // Drawing buffer for LVGL

// Button State Variables
bool buttonPressed = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
uint8_t activeTabIndex = 0;

// LVGL UI Elements
lv_obj_t *tabview;
lv_obj_t *tab1;
lv_obj_t *tab2;
lv_obj_t *tab3;

// Tab1 I/O Readings
lv_obj_t *vin_label, *vin_value;
lv_obj_t *iin_label, *iin_value;
lv_obj_t *pin_label, *pin_value, *pin_arc;
lv_obj_t *temp1_label, *temp1_value;
lv_obj_t *temp2_label, *temp2_value;
lv_obj_t *voltage_label, *voltage_value;
lv_obj_t *current_label, *current_value;
lv_obj_t *fanspeed1_label, *fanspeed1_value;
lv_obj_t *pout_percentage, *pout_value, *pout_arc;

// Helper struct for display properties (e.g., label text, value, position)
struct DisplayProperty {
    lv_obj_t **label_obj;
    lv_obj_t **value_obj;
    const char *label_text;
    int y_offset;
};

/*
 * @brief LVGL display flush callback.
 * Called by LVGL when a rendered image needs to be copied to the display.
 * @param disp Pointer to the display object.
 * @param area Pointer to the area to be flushed.
 * @param px_map Pointer to the pixel map data.
 */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    lv_display_flush_ready(disp);
}

/*
 * @brief LVGL tick source callback.
 * Uses Arduino's millis() as the tick source for LVGL.
 * @return Current tick count in milliseconds.
 */
uint32_t my_tick(void) {
    return millis();
}

/*
 * @brief Creates a gradient ARC style for LVGL arcs.
 * @param style Pointer to the lv_style_t object to initialize.
 * @param color_start Starting color of the gradient.
 * @param color_end Ending color of the gradient.
 */
void create_gradient_arc_style(lv_style_t *style, lv_color_t color_start, lv_color_t color_end) {
    lv_style_init(style);

    // Set ARC basic properties
    lv_style_set_arc_width(style, 10);
    lv_style_set_arc_rounded(style, true);

    // Create gradient descriptor
    static lv_grad_dsc_t grad;
    grad.dir = LV_GRAD_DIR_VER; // Vertical gradient for better appearance
    grad.stops_count = 2;
    grad.stops[0].color = color_start;
    grad.stops[0].frac = 0;
    grad.stops[1].color = color_end;
    grad.stops[1].frac = 255;

    lv_style_set_arc_color(style, color_start); // Fallback color
    lv_style_set_bg_grad(style, &grad);
    lv_style_set_bg_main_stop(style, 0);     // Gradient start position (fraction)
    lv_style_set_bg_grad_stop(style, 255);  // Gradient end position (fraction)
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER); // Set gradient direction
}

/*
 * @brief Creates a gradient background style for an LVGL object.
 * @param parent Pointer to the parent LVGL object.
 */
void create_gradient_bg(lv_obj_t *parent) {
    static lv_style_t style_bg;
    lv_style_init(&style_bg);

    // Set gradient background properties
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bg, lv_color_hex(0x2196F3));     // Start color (Blue)
    lv_style_set_bg_grad_color(&style_bg, lv_color_hex(0x9C27B0)); // End color (Purple)
    lv_style_set_bg_grad_dir(&style_bg, LV_GRAD_DIR_VER);         // Vertical gradient

    lv_obj_add_style(parent, &style_bg, 0);
}

/*
 * @brief Helper function to create a label and its value display.
 * @param parent The parent LVGL object.
 * @param style The LVGL style to apply to the labels.
 * @param label_obj Pointer to the pointer for the label object.
 * @param value_obj Pointer to the pointer for the value object.
 * @param label_text The static text for the label.
 * @param y_offset The Y-offset for alignment.
 */
void create_labeled_value_pair(lv_obj_t *parent, lv_style_t *style,
                               lv_obj_t **label_obj, lv_obj_t **value_obj,
                               const char *label_text, int y_offset) {
    *label_obj = lv_label_create(parent);
    lv_obj_add_style(*label_obj, style, 0);
    lv_label_set_text(*label_obj, label_text);
    lv_obj_align(*label_obj, LV_ALIGN_TOP_LEFT, 110, y_offset);

    *value_obj = lv_label_create(parent);
    lv_obj_add_style(*value_obj, style, 0);
    lv_label_set_text(*value_obj, "0.00"); // Default initial value
    lv_obj_align(*value_obj, LV_ALIGN_TOP_LEFT, 190, y_offset);
}

/*
 * @brief Initializes and sets up the LVGL user interface.
 */
void setupUI(void) {
    // Basic style for labels
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_radius(&label_style, 5);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_12);
    lv_style_set_text_color(&label_style, lv_color_white());

    // Create Tabview
    tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_size(tabview, 25);
    lv_obj_set_style_bg_opa(tabview, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(tabview, lv_color_white(), LV_PART_ITEMS);

    // Add Tabs
    tab1 = lv_tabview_add_tab(tabview, "I/O");
    tab2 = lv_tabview_add_tab(tabview, "Fan speed");
    tab3 = lv_tabview_add_tab(tabview, "Status");

    // Set gradient background for each tab
    create_gradient_bg(tab1);
    create_gradient_bg(tab2);
    create_gradient_bg(tab3);

    // --- Tab1 Content: I/O Readings ---
    DisplayProperty tab1_properties[] = {
        {&vin_label, &vin_value, "Vin:", 10},
        {&iin_label, &iin_value, "Iin:", 25},
        {&temp1_label, &temp1_value, "Temp1:", 40},
        {&temp2_label, &temp2_value, "Temp2:", 55},
        {&fanspeed1_label, &fanspeed1_value, "Fanspeed1:", 70},
        {&voltage_label, &voltage_value, "Vout:", 110},
        {&current_label, &current_value, "Iout:", 125},
    };

    for (size_t i = 0; i < sizeof(tab1_properties) / sizeof(tab1_properties[0]); ++i) {
        create_labeled_value_pair(tab1, &label_style,
                                  tab1_properties[i].label_obj,
                                  tab1_properties[i].value_obj,
                                  tab1_properties[i].label_text,
                                  tab1_properties[i].y_offset);
    }

    // Pin Arc and Labels
    pin_label = lv_label_create(tab1);
    lv_obj_add_style(pin_label, &label_style, 0);
    lv_label_set_text(pin_label, "0.0%");
    lv_obj_align(pin_label, LV_ALIGN_TOP_LEFT, 30, 35);

    pin_value = lv_label_create(tab1);
    lv_obj_add_style(pin_value, &label_style, 0);
    lv_label_set_text(pin_value, "0.00W");
    lv_obj_align(pin_value, LV_ALIGN_TOP_LEFT, 30, 50);

    pin_arc = lv_arc_create(tab1);
    lv_obj_set_size(pin_arc, 100, 100);
    lv_obj_align(pin_arc, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_bar_set_range(pin_arc, 0, 100);
    lv_arc_set_value(pin_arc, 0);
    lv_obj_remove_style(pin_arc, NULL, LV_PART_KNOB); // Remove knob
    lv_arc_set_bg_angles(pin_arc, 135, 45);             // 270-degree range

    static lv_style_t style_arc_pin;
    create_gradient_arc_style(&style_arc_pin, lv_color_hex(0x00FF00), lv_color_hex(0xFF0000));
    lv_obj_add_style(pin_arc, &style_arc_pin, LV_PART_INDICATOR);

    // Set background ARC style
    static lv_style_t style_arc_bg;
    lv_style_init(&style_arc_bg);
    lv_style_set_arc_width(&style_arc_bg, 10);
    lv_style_set_arc_color(&style_arc_bg, lv_color_hex(0x333333));
    lv_obj_add_style(pin_arc, &style_arc_bg, LV_PART_MAIN);

    // Pout Arc and Labels
    pout_percentage = lv_label_create(tab1);
    lv_obj_add_style(pout_percentage, &label_style, 0);
    lv_label_set_text(pout_percentage, "0.0%");
    lv_obj_align(pout_percentage, LV_ALIGN_TOP_LEFT, 30, 135);

    pout_value = lv_label_create(tab1);
    lv_obj_add_style(pout_value, &label_style, 0);
    lv_label_set_text(pout_value, "0.00W");
    lv_obj_align(pout_value, LV_ALIGN_TOP_LEFT, 30, 150);

    pout_arc = lv_arc_create(tab1);
    lv_obj_set_size(pout_arc, 100, 100);
    lv_obj_align(pout_arc, LV_ALIGN_TOP_LEFT, 0, 100);
    lv_bar_set_range(pout_arc, 0, 100);
    lv_arc_set_value(pout_arc, 0);
    lv_obj_remove_style(pout_arc, NULL, LV_PART_KNOB); // Remove knob
    lv_arc_set_bg_angles(pout_arc, 135, 45);             // 270-degree range

    static lv_style_t style_arc_pout;
    create_gradient_arc_style(&style_arc_pout, lv_color_hex(0x00FF00), lv_color_hex(0xFF0000));
    lv_obj_add_style(pout_arc, &style_arc_pout, LV_PART_INDICATOR);

    // Set background style for Pout ARC
    static lv_style_t style_arc_pout_bg;
    lv_style_init(&style_arc_pout_bg);
    lv_style_set_arc_width(&style_arc_pout_bg, 10);
    lv_style_set_arc_color(&style_arc_pout_bg, lv_color_hex(0x333333));
    lv_obj_add_style(pout_arc, &style_arc_pout_bg, LV_PART_MAIN);

    // --- Tab2 Content ---
    lv_obj_t *label_tab2 = lv_label_create(tab2);
    lv_label_set_text(label_tab2, "Second tab");
    lv_obj_align(label_tab2, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *btn = lv_btn_create(tab2);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -40);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "button");
    lv_obj_center(btn_label);

    // --- Tab3 Content ---
    lv_obj_t *label_tab3 = lv_label_create(tab3);
    lv_label_set_text(label_tab3, "Third tab");
    lv_obj_align(label_tab3, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *slider = lv_slider_create(tab3);
    lv_obj_set_width(slider, 180);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 30);
}

/*
 * @brief Updates the UI elements with new PMBus data.
 * @param pmbus_data Pointer to the PMBusData_t structure containing sensor readings.
 */
void updateUI(PMBusData_t *pmbus_data) {
    char buffer[20]; // Buffer for formatting strings

    // Update Power ARC and Labels (Pin)
    sprintf(buffer, "%.1fW", pmbus_data->pin);
    lv_label_set_text(pin_value, buffer);

    float pin_percentage = (pmbus_data->pin / pmbus_data->pomax) * 100.0;
    pin_percentage = constrain(pin_percentage, 0.0, 100.0); // Clamp value
    lv_arc_set_value(pin_arc, (int16_t)pin_percentage);
    sprintf(buffer, "%.1f%%", pin_percentage);
    lv_label_set_text(pin_label, buffer);

    // Update Power ARC and Labels (Pout)
    sprintf(buffer, "%.1fW", pmbus_data->power); // Pout
    lv_label_set_text(pout_value, buffer);

    float pout_percentage_val = (pmbus_data->power / pmbus_data->pomax) * 100.0;
    pout_percentage_val = constrain(pout_percentage_val, 0.0, 100.0); // Clamp value
    lv_arc_set_value(pout_arc, (int16_t)pout_percentage_val);
    sprintf(buffer, "%.1f%%", pout_percentage_val);
    lv_label_set_text(pout_percentage, buffer);

    // Dynamically change ARC color based on Pin percentage
    static lv_style_t style_dynamic_arc;
    lv_color_t color_start, color_end;

    if (pin_percentage < 33) {
        // Low power: Green
        color_start = lv_color_hex(0x00FF00);
        color_end = lv_color_hex(0x80FF00);
    } else if (pin_percentage < 66) {
        // Medium power: Yellow
        color_start = lv_color_hex(0x80FF00);
        color_end = lv_color_hex(0xFFFF00);
    } else {
        // High power: Red
        color_start = lv_color_hex(0xFFFF00);
        color_end = lv_color_hex(0xFF0000);
    }

    create_gradient_arc_style(&style_dynamic_arc, color_start, color_end);
    lv_obj_add_style(pin_arc, &style_dynamic_arc, LV_PART_INDICATOR);

    // Update Vin
    sprintf(buffer, "%.2fV", pmbus_data->vin);
    lv_label_set_text(vin_value, buffer);

    // Update Iin
    sprintf(buffer, "%.2fA", pmbus_data->iin);
    lv_label_set_text(iin_value, buffer);

    // Update Vout
    sprintf(buffer, "%.2fV", pmbus_data->voltage);
    lv_label_set_text(voltage_value, buffer);

    // Update Iout
    sprintf(buffer, "%.2fA", pmbus_data->current);
    lv_label_set_text(current_value, buffer);

    // Update Temp1
    sprintf(buffer, "%.1fC", pmbus_data->temp1);
    lv_label_set_text(temp1_value, buffer);

    // Update Temp2
    sprintf(buffer, "%.1fC", pmbus_data->temp2);
    lv_label_set_text(temp2_value, buffer);

    // Update Fanspeed1
    sprintf(buffer, "%drpm", pmbus_data->fanSpeed);
    lv_label_set_text(fanspeed1_value, buffer);
}

/*
 * @brief Handles button presses for tab switching.
 * Implements debouncing to prevent multiple triggers from a single press.
 */
void handleButton() {
    bool currentButtonState = digitalRead(BUTTON_PIN);

    // Check if enough time has passed since the last state change to debounce
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        // If the button state has changed (from HIGH to LOW, i.e., pressed)
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            activeTabIndex = (activeTabIndex + 1) % 3; // Cycle through tabs (0, 1, 2)
            Serial.println("Button pressed - switching tab");
            lv_tabview_set_act(tabview, activeTabIndex, LV_ANIM_ON);
            lastDebounceTime = millis(); // Reset debounce timer
        }
    }
    lastButtonState = currentButtonState; // Save the current button state for next iteration
}

/*
 * @brief Initializes the LVGL display.
 * @return Pointer to the initialized lv_display_t object.
 */
lv_display_t *initDisplay(void) {
    lv_display_t *disp;

#if LV_USE_TFT_ESPI
    /* TFT_eSPI can be enabled in lv_conf.h to initialize the display in a simple way */
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);
#else
    /* Else create a display yourself */
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    return disp;
}