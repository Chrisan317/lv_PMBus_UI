/*
* 生成漸層會導致記憶體不足當機，改用漸層背景圖檔;觸控功能OK.
* 待增加第二組風扇控制，顯示狀態
*
*/

#include <Arduino.h>
#include "ui.h"
#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif

TFT_eSPI tft = TFT_eSPI();

// 繪圖緩衝區定義
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

//使用背景圖檔
extern const lv_img_dsc_t backgroundr;
LV_IMAGE_DECLARE(backgroundr);

// 按鈕狀態變數定義
bool buttonPressed = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
uint8_t activeTabIndex = 0;
float max_power = 1000.0;

// 全局UI元素定義
lv_obj_t * tabview;
lv_obj_t * tab1;
lv_obj_t * tab2;
lv_obj_t * tab3;
lv_obj_t *vin_label, *vin_value;
lv_obj_t *iin_label, *iin_value;
lv_obj_t *pin_label, *pin_value, *pin_arc;
lv_obj_t *temp1_label, *temp1_value;
lv_obj_t *temp2_label, *temp2_value;
lv_obj_t *voltage_label, *voltage_value;
lv_obj_t *current_label, *current_value;
lv_obj_t *fanspeed1_label, *fanspeed1_value;
lv_obj_t *pout_percentage, *pout_value, *pout_arc;
lv_obj_t *label_fanspd;

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
   /* uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();*/

    lv_display_flush_ready(disp);
}

/*use Arduinos millis() as tick source*/
uint32_t my_tick(void)
{
    return millis();
}

void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    //For example  ("my_..." functions needs to be implemented by you)
    uint16_t x, y, temp;
    
    bool touched = tft.getTouch( &x, &y, 600 );

    if(!touched) {
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_PRESSED;

        // 使用 setRotation(1)，需要轉換座標
        // ILI9341: 320x240，橫向顯示
        data->point.x = y;
        data->point.y = 320 - x;
    }
     
}

// 建立漸層ARC樣式
void create_gradient_arc_style(lv_style_t *style, lv_color_t color_start, lv_color_t color_end)
{
    lv_style_init(style);
    
    // 設定ARC的基本屬性
    lv_style_set_arc_width(style, 10);
    lv_style_set_arc_rounded(style, true);
    
    // 建立漸層描述符
    static lv_grad_dsc_t grad;
    grad.dir = LV_GRAD_DIR_HOR;
    grad.stops_count = 2;
    grad.stops[0].color = color_start;
    //grad.stops[0].opa = LV_OPA_COVER;
    grad.stops[0].frac = 0;
    grad.stops[1].color = color_end;
    //grad.stops[1].opa = LV_OPA_COVER;
    grad.stops[0].frac = 255;
    
    lv_style_set_arc_color(style, color_start);
    lv_style_set_bg_grad(style, &grad);
    //gemini
    lv_style_set_bg_main_stop(style, 0);    // 漸層開始的位置 (分數)
    lv_style_set_bg_grad_stop(style, 255); // 漸層結束的位置 (分數)
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER); // 或 HOR，嘗試以獲得最佳外觀

}

void create_gradient_bg(lv_obj_t *parent) {
    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    
    // 設定漸層背景
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
    lv_style_set_bg_img_src(&style_bg, &backgroundr);
    
    lv_obj_add_style(parent, &style_bg, 0);
}

void slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target_obj(e);
    int value = lv_slider_get_value(slider);
    lv_label_set_text_fmt(label_fanspd, "Speed: %d%%", value);
    fanCommand(0x3B, (uint8_t)value);

}

void btn_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        clearFault();  
    }
}

void setupUI(void)
{   
    /*Create a Tab view object*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 5);
    lv_style_set_text_font(&style, &lv_font_montserrat_12);
    lv_style_set_text_color(&style, lv_color_white());//lv_color_hex(0x110042 lv_color_white()

    tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_size(tabview, 25);
    
    /* 禁止左右滑動*/
    lv_obj_t *tabview_content = lv_tabview_get_content(tabview);
    lv_obj_clear_flag(tabview_content, LV_OBJ_FLAG_SCROLLABLE);
    
    tab1 = lv_tabview_add_tab(tabview, "I/O");
    tab2 = lv_tabview_add_tab(tabview, "Fan speed");
    tab3 = lv_tabview_add_tab(tabview, "Status");
    // 為每個tab設定漸層背景
    create_gradient_bg(tab1);
    create_gradient_bg(tab2);
    create_gradient_bg(tab3);
   
    
    // Vin
    vin_label = lv_label_create(tab1);
    lv_obj_add_style(vin_label, &style, 0);
    lv_label_set_text(vin_label, "Vin:");
    lv_obj_align(vin_label, LV_ALIGN_TOP_LEFT, 110, 10);
    
    vin_value = lv_label_create(tab1);
    lv_obj_add_style(vin_value, &style, 0);
    lv_label_set_text(vin_value, "0.00");
    lv_obj_align(vin_value, LV_ALIGN_TOP_LEFT, 190, 10);

    // Iin
    iin_label = lv_label_create(tab1);
    lv_obj_add_style(iin_label, &style, 0);
    lv_label_set_text(iin_label, "Iin:");
    lv_obj_align(iin_label, LV_ALIGN_TOP_LEFT, 110, 25);
    
    iin_value = lv_label_create(tab1);
    lv_obj_add_style(iin_value, &style, 0);
    lv_label_set_text(iin_value, "0.00");
    lv_obj_align(iin_value, LV_ALIGN_TOP_LEFT, 190, 25);   

    // temp1
    temp1_label = lv_label_create(tab1);
    lv_obj_add_style(temp1_label, &style, 0);
    lv_label_set_text(temp1_label, "Temp1:");
    lv_obj_align(temp1_label, LV_ALIGN_TOP_LEFT, 110, 40);
    
    temp1_value = lv_label_create(tab1);
    lv_obj_add_style(temp1_value, &style, 0);
    lv_label_set_text(temp1_value, "0.0");
    lv_obj_align(temp1_value, LV_ALIGN_TOP_LEFT, 190, 40);

    // temp2
    temp2_label = lv_label_create(tab1);
    lv_obj_add_style(temp2_label, &style, 0);
    lv_label_set_text(temp2_label, "Temp2:");
    lv_obj_align(temp2_label, LV_ALIGN_TOP_LEFT, 110, 55);
    
    temp2_value = lv_label_create(tab1);
    lv_obj_add_style(temp2_value, &style, 0);
    lv_label_set_text(temp2_value, "0.0");
    lv_obj_align(temp2_value, LV_ALIGN_TOP_LEFT, 190, 55);


    // fanspeed1
    fanspeed1_label = lv_label_create(tab1);
    lv_obj_add_style(fanspeed1_label, &style, 0);
    lv_label_set_text(fanspeed1_label, "Fanspeed1:");
    lv_obj_align(fanspeed1_label, LV_ALIGN_TOP_LEFT, 110, 70);
    
    fanspeed1_value = lv_label_create(tab1);
    lv_obj_add_style(fanspeed1_value, &style, 0);
    lv_label_set_text(fanspeed1_value, "0.00");
    lv_obj_align(fanspeed1_value, LV_ALIGN_TOP_LEFT, 190, 70);

    // vout
    voltage_label = lv_label_create(tab1);
    lv_obj_add_style(voltage_label, &style, 0);
    lv_label_set_text(voltage_label, "Vout:");
    lv_obj_align(voltage_label, LV_ALIGN_TOP_LEFT, 110, 110);
    
    voltage_value = lv_label_create(tab1);
    lv_obj_add_style(voltage_value, &style, 0);
    lv_label_set_text(voltage_value, "0.00");
    lv_obj_align(voltage_value, LV_ALIGN_TOP_LEFT, 190, 110);

    // Iout
    current_label = lv_label_create(tab1);
    lv_obj_add_style(current_label, &style, 0);
    lv_label_set_text(current_label, "Iout:");
    lv_obj_align(current_label, LV_ALIGN_TOP_LEFT, 110, 125);
    
    current_value = lv_label_create(tab1);
    lv_obj_add_style(current_value, &style, 0);
    lv_label_set_text(current_value, "0.00");
    lv_obj_align(current_value, LV_ALIGN_TOP_LEFT, 190, 125);

    
    // Pin
    pin_label = lv_label_create(tab1);
    lv_obj_add_style(pin_label, &style, 0);
    lv_label_set_text(pin_label, "%");    
    lv_obj_align(pin_label, LV_ALIGN_TOP_LEFT, 30, 35);
    
    pin_value = lv_label_create(tab1);
    lv_obj_add_style(pin_value, &style, 0);
    lv_label_set_text(pin_value, "0.00");
    lv_obj_align(pin_value, LV_ALIGN_TOP_LEFT, 30, 50);
    
    pin_arc = lv_arc_create(tab1);
    lv_obj_set_size(pin_arc, 100, 100);
    lv_obj_align(pin_arc, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_bar_set_range(pin_arc, 0, 100); 
    lv_arc_set_value(pin_arc, 0);
    lv_obj_remove_style(pin_arc, NULL, LV_PART_KNOB);//移除knob
    lv_arc_set_bg_angles(pin_arc, 135, 45);  // 從135度到45度 (270度範圍)
    lv_obj_clear_flag(pin_arc, LV_OBJ_FLAG_CLICKABLE);//禁止互動
    
    static lv_style_t style_arc;
    create_gradient_arc_style(&style_arc, lv_color_hex(0x00FF00), lv_color_hex(0xFF0000));
    lv_obj_add_style(pin_arc, &style_arc, LV_PART_INDICATOR);
    
    
    // 設定背景ARC樣式
    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    lv_style_set_arc_width(&style_bg, 10);
    lv_style_set_arc_color(&style_bg, lv_color_hex(0x333333));
    lv_obj_add_style(pin_arc, &style_bg, LV_PART_MAIN);

    // Pout
    pout_percentage = lv_label_create(tab1);
    lv_obj_add_style(pout_percentage, &style, 0);
    lv_label_set_text(pout_percentage, "%");    
    lv_obj_align(pout_percentage, LV_ALIGN_TOP_LEFT, 30, 135);
    
    pout_value = lv_label_create(tab1);
    lv_obj_add_style(pout_value, &style, 0);
    lv_label_set_text(pout_value, "0.00");
    lv_obj_align(pout_value, LV_ALIGN_TOP_LEFT, 30, 150);
    
    pout_arc = lv_arc_create(tab1);
    lv_obj_set_size(pout_arc, 100, 100);
    lv_obj_align(pout_arc, LV_ALIGN_TOP_LEFT, 0, 100);
    lv_bar_set_range(pout_arc, 0, 100); 
    lv_arc_set_value(pout_arc, 0);
    lv_obj_remove_style(pout_arc, NULL, LV_PART_KNOB);//移除knob
    lv_arc_set_bg_angles(pout_arc, 135, 45);  // 從135度到45度 (270度範圍)
    lv_obj_clear_flag(pout_arc, LV_OBJ_FLAG_CLICKABLE);//禁止互動
    
    static lv_style_t style_arc2;
    create_gradient_arc_style(&style_arc2, lv_color_hex(0x00FF00), lv_color_hex(0xFF0000));
    lv_obj_add_style(pout_arc, &style_arc2, LV_PART_INDICATOR);
    
    
    // set background style
    static lv_style_t style_bg1;
    lv_style_init(&style_bg1);
    lv_style_set_arc_width(&style_bg1, 10);
    lv_style_set_arc_color(&style_bg1, lv_color_hex(0x333333));
    lv_obj_add_style(pout_arc, &style_bg1, LV_PART_MAIN);
    

    // Tab2 內容
    label_fanspd = lv_label_create(tab2);
    lv_label_set_text(label_fanspd, "fan control");
    lv_obj_add_style(label_fanspd, &style, 0);
    lv_obj_align(label_fanspd, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *slider_fan = lv_slider_create(tab2);
    lv_obj_set_width(slider_fan, 180);
    lv_slider_set_range(slider_fan, 0, 100);
    lv_obj_align(slider_fan, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(slider_fan, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Tab3 內容
    lv_obj_t *label = lv_label_create(tab3);
    lv_label_set_text(label, "status tab");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *btn = lv_btn_create(tab3);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -40);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Clear");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);


}

void updateUI(PMBusData_t* pmbus_data) {
    char buffer[20];
    
    // 更新功率 ARC
    sprintf(buffer, "%.1fW", pmbus_data->pin);
    lv_label_set_text(pin_value, buffer);

    sprintf(buffer, "%.1fW", pmbus_data->power);//Pout
    lv_label_set_text(pout_value, buffer);

    // 計算百分比
    float percentage = ((pmbus_data->pin )/ pmbus_data->pomax) * 100.0;
    if (percentage > 100.0) percentage = 100.0;
    if (percentage < 0.0) percentage = 0.0;
    
    // 更新ARC映射值
    lv_arc_set_value(pin_arc, (int16_t)percentage);
    sprintf(buffer, "%.1f%%", percentage);
    lv_label_set_text(pin_label, buffer);

    // Calculate Pout percentage
    float percentage1 = ((pmbus_data->power )/ pmbus_data->pomax) * 100.0;
    if (percentage1 > 100.0) percentage1 = 100.0;
    if (percentage1 < 0.0) percentage1 = 0.0;
    
    // update Pout ARC mapping
    lv_arc_set_value(pout_arc, (int16_t)percentage1);
    sprintf(buffer, "%.1f%%", percentage1);
    lv_label_set_text(pout_percentage, buffer);
    
    
    // 根據功率等級改變顏色
    static lv_style_t style_dynamic;
    lv_color_t color_start, color_end;
    
    if (percentage < 33) {
        // 低功率：綠色系
        color_start = lv_color_hex(0x00FF00);
        color_end = lv_color_hex(0x80FF00);//(0x80FF00);
    } else if (percentage < 66) {
        // 中功率：黃色系
        color_start = lv_color_hex(0x80FF00);
        color_end = lv_color_hex(0xFFFF00);
    } else {
        // 高功率：紅色系
        color_start = lv_color_hex(0xFFFF00);
        color_end = lv_color_hex(0xFF0000);
    }
    
    create_gradient_arc_style(&style_dynamic, color_start, color_end);
    lv_obj_add_style(pin_arc, &style_dynamic, LV_PART_INDICATOR);
    
    //update Vin
    sprintf(buffer, "%.2fV", pmbus_data->vin);
    lv_label_set_text(vin_value, buffer);
  
    // update Iin
    sprintf(buffer, "%.2fA", pmbus_data->iin);
    lv_label_set_text(iin_value, buffer);

    // update Vout
    sprintf(buffer, "%.2fV", pmbus_data->voltage);
    lv_label_set_text(voltage_value, buffer);
  
    
    // update Iout
    sprintf(buffer, "%.2fA", pmbus_data->current);
    lv_label_set_text(current_value, buffer);
    
    // Update temp1
    sprintf(buffer, "%.1fC", pmbus_data->temp1);
    lv_label_set_text(temp1_value, buffer);

    // Update temp1
    sprintf(buffer, "%.1fC", pmbus_data->temp2);
    lv_label_set_text(temp2_value, buffer);

    //update fanspeed1
    sprintf(buffer, "%drpm", pmbus_data->fanSpeed);
    lv_label_set_text(fanspeed1_value, buffer);

    
}

// button-switch tab
void handleButton() {
    // read button and debounce
    bool currentButtonState = digitalRead(BUTTON_PIN);

    if ((millis() - lastDebounceTime) > debounceDelay) {
        // if the button state has changed
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            // button pressed
            activeTabIndex = (activeTabIndex + 1) % 3;
            Serial.println("Button pressed - switching tab");
            lv_tabview_set_act(tabview, activeTabIndex, LV_ANIM_ON);
            lastDebounceTime = millis();
        }
    }
    
    lastButtonState = currentButtonState;
}

lv_display_t* initDisplay(void) {
    lv_display_t * disp;

    tft.begin();
    tft.setRotation(1);
    uint16_t calData[5] = { 514, 3190, 319, 3419, 4 };
    tft.setTouch( calData );
    
#if LV_USE_TFT_ESPI
    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);
#else
    /*Else create a display yourself*/
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); //Touchpad should have POINTER type
    lv_indev_set_read_cb(indev, my_touchpad_read);


    return disp;
}