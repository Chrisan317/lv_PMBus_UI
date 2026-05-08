/* 
 *
 */

#include <lvgl.h>
#include "pmbus.h"
#include "ui.h"
/*
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // For mutex/semaphore if needed for shared resources, though not strictly necessary for simple read-only updates.
*/

PMBusData_t pmbus_data;

unsigned long lastUpdateTime = 0;
//unsigned long lastChartUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000;    // 1秒更新一次狀態
/*
void pmbusReadTask(void *pvParameters) {
    (void) pvParameters; // 避免編譯器警告
    //PMBusData_t pmbus_data;
    for (;;) {
        readAllPMBusData(&pmbus_data);

        // 延遲一段時間再進行下一次讀取
        vTaskDelay(pdMS_TO_TICKS(500)); // 每 500 毫秒讀取一次
    }
}
*/
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
/*
    xTaskCreatePinnedToCore(
        pmbusReadTask,   // 任務函數
        "PMBusReadTask", // 任務名稱
        4096,            // 任務堆疊大小 (位元組)
        NULL,            // 任務參數 (NULL表示沒有)
        1,               // 任務優先級 (0為最低，configMAX_PRIORITIES - 1 為最高)
        NULL,            // 任務Handle (NULL表示不需要Handle)
        1                // 運行在 Core 1 (Core 0 通常用於 WiFi/BT 和 Arduino loop)
    );
*/
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