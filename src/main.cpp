/* 
 *
 */

#include <lvgl.h>
#include "pmbus.h"
#include "ui.h"

PMBusData_t pmbus_data;

unsigned long lastUpdateTime = 0;
//unsigned long lastChartUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000;    // 1秒更新一次狀態

void setup()
{
    String LVGL_Arduino = "LVGL ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin(115200);
    Wire.begin();
    Serial.println(LVGL_Arduino);
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(my_tick);

    // 初始化顯示器
    lv_display_t* disp = initDisplay();
    
    // 設置UI
    setupUI();

    Serial.println("Setup done");
}

void loop()
{
    
    unsigned long currentTime = millis();
    // PMBus 通訊和數據讀取
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        if (initPMBus()) {
            
            Serial.println(getPMBusAddress(), HEX);
            readAllPMBusData(&pmbus_data);
            updateUI(&pmbus_data);
        } else {
            Serial.println("device not found");
        }
    } 

    // 處理按鈕輸入
    handleButton();
    
    // LVGL 任務處理
    lv_timer_handler(); /* let the GUI do its work */
    
    delay(100); /* let this time pass */
}