#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include "pmbus.h"

// UI 常數定義
#define TFT_HOR_RES   240
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_90
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

// 按鈕相關常數
#define BUTTON_PIN 14

// 全局UI元素宣告
extern lv_obj_t * tabview;
extern lv_obj_t * tab1;
extern lv_obj_t * tab2;
extern lv_obj_t * tab3;
extern lv_obj_t *voltage_label, *voltage_value;
extern lv_obj_t *current_label, *current_value;
extern lv_obj_t *power_label, *power_value;
extern lv_obj_t *voltage_bar, *current_bar, *power_arc;

// 按鈕狀態變數宣告
extern bool buttonPressed;
extern bool lastButtonState;
extern unsigned long lastDebounceTime;
extern unsigned long debounceDelay;
extern uint8_t activeTabIndex;

// 繪圖緩衝區宣告
extern uint32_t draw_buf[];

// 函數宣告
void slider_event_cb(lv_event_t *e);
void btn_event_cb(lv_event_t* e);
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t * px_map);
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data );
uint32_t my_tick(void);
void setupUI(void);
void updateUI(PMBusData_t* pmbus_data);
void handleButton(void);
lv_display_t* initDisplay(void);

#endif // UI_H