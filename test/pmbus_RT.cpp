#include <Arduino.h>
#include <Wire.h> // For I2C (PMBus) communication

// LVGL Includes
#include <lvgl.h>
#include <TFT_eSPI.h> // Or your specific display driver

// FreeRTOS Includes
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // For mutex/semaphore if needed for shared resources, though not strictly necessary for simple read-only updates.

// --- LVGL Setup (Standard boilerplate) ---
#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10]; // 10 lines buffer, adjust based on RAM
static lv_disp_drv_t disp_drv;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// --- PMBus Communication (unchanged) ---
const uint8_t PMBUS_SLAVE_ADDRESS = 0x58; // **Replace with your device's actual PMBus address**
const uint8_t PMBUS_CMD_STATUS_WORD = 0x79;
const uint8_t PMBUS_CMD_STATUS_VOUT = 0x7A;
const uint8_t PMBUS_CMD_STATUS_IOUT = 0x7B;
const uint8_t PMBUS_CMD_STATUS_INPUT = 0x7C;
const uint8_t PMBUS_CMD_STATUS_TEMPERATURE = 0x7D;
const uint8_t PMBUS_CMD_STATUS_CML = 0x7E;
const uint8_t PMBUS_CMD_STATUS_OTHER = 0x7F;
const uint8_t PMBUS_CMD_STATUS_MFR_SPECIFIC = 0x80;

uint16_t readPMBusByte(uint8_t slaveAddress, uint8_t command) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(command);
  if (Wire.endTransmission(false) != 0) {
    return 0xFFFF;
  }
  if (Wire.requestFrom(slaveAddress, 1) == 1) {
    return Wire.read();
  }
  return 0xFFFF;
}

uint16_t readPMBusWord(uint8_t slaveAddress, uint8_t command) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(command);
  if (Wire.endTransmission(false) != 0) {
    return 0xFFFF;
  }
  if (Wire.requestFrom(slaveAddress, 2) == 2) {
    uint8_t lowByte = Wire.read();
    uint8_t highByte = Wire.read();
    return (highByte << 8) | lowByte;
  }
  return 0xFFFF;
}

// --- Global Variable for PMBus Status ---
// 使用 volatile 關鍵字確保編譯器不會優化掉對此變數的存取
volatile uint16_t currentStatusWord = 0;
volatile bool pmbusReadError = false;

// --- LVGL Objects ---
lv_obj_t *screen;
lv_obj_t *status_label;
lv_obj_t *detail_label;
lv_obj_t *status_led; // 新增的 LVGL LED 物件

/**
 * @brief FreeRTOS 任務，用於定期讀取 PMBus 狀態。
 * @param pvParameters 任務參數 (未使用)。
 */
void pmbusReadTask(void *pvParameters) {
  (void) pvParameters; // 避免編譯器警告

  for (;;) {
    uint16_t statusWord = readPMBusWord(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_WORD);

    if (statusWord != 0xFFFF) {
      currentStatusWord = statusWord;
      pmbusReadError = false;
    } else {
      pmbusReadError = true;
      currentStatusWord = 0; // 可以設定為一個特殊值表示錯誤
    }

    // 延遲一段時間再進行下一次讀取
    vTaskDelay(pdMS_TO_TICKS(500)); // 每 500 毫秒讀取一次
  }
}

/**
 * @brief 解讀 STATUS_WORD 並更新 LVGL 介面。
 * @param statusWord 要解讀的 16 位 STATUS_WORD 值。
 */
void interpretAndDisplayStatus(uint16_t statusWord, bool error) {
  String statusText = "";
  String detailText = "";
  lv_color_t led_color = lv_color_make(0, 0, 0); // 預設為黑色

  if (error) {
    statusText = "PMBus 讀取失敗!\n";
    lv_obj_set_style_text_color(status_label, lv_color_make(255, 0, 0), LV_PART_MAIN); // 錯誤時顯示紅色文字
    led_color = lv_color_make(255, 0, 0); // 錯誤時 LED 顯示紅色
  } else {
    lv_obj_set_style_text_color(status_label, lv_color_make(255, 255, 255), LV_PART_MAIN); // 正常時顯示白色文字
    statusText += "STATUS_WORD: 0x" + String(statusWord, HEX) + "\n";

    // 判斷 Fault 或 Warning
    bool isFault = (statusWord & (1 << 1)); // Bit 1: Any FAULT
    bool isWarning = (statusWord & (1 << 2)); // Bit 2: Any WARNING

    if (isFault) {
      led_color = lv_color_make(255, 0, 0); // 紅色 (Fault)
      statusText += "偵測到故障 (FAULT)！\n";
    } else if (isWarning) {
      led_color = lv_color_make(255, 255, 0); // 黃色 (Warning)
      statusText += "偵測到警告 (WARNING)！\n";
    } else if (statusWord == 0x0000) {
      led_color = lv_color_make(0, 255, 0); // 綠色 (OK)
      statusText += "無活動故障或警告。\n";
    } else { // 針對某些特定位元可能不是 Fault 或 Warning 的情況
      led_color = lv_color_make(0, 255, 0); // 預設為綠色
    }

    // --- 解讀各 STATUS 位元，並讀取詳細資訊 (與您之前的邏輯相同) ---
    // (將您之前 interpretStatusWord 函數中解析 STATUS_WORD 的邏輯直接複製到此處)
    if (statusWord & (1 << 15)) {
      statusText += "VOUT_OV_FAULT (Bit 15) - 過電壓故障\n";
      detailText += "讀取 STATUS_VOUT...\n";
      uint16_t status_vout = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_VOUT);
      if (status_vout != 0xFFFF) {
        detailText += "  STATUS_VOUT: 0x" + String(status_vout, HEX) + "\n";
        if (status_vout & (1 << 7)) detailText += "    VOUT_OV_FAULT (Bit 7) - 輸出過電壓故障\n";
        if (status_vout & (1 << 6)) detailText += "    VOUT_OV_WARNING (Bit 6) - 輸出過電壓警告\n";
      } else {
        detailText += "  讀取 STATUS_VOUT 失敗\n";
      }
    }
    if (statusWord & (1 << 14)) {
      statusText += "VOUT_UV_FAULT (Bit 14) - 欠電壓故障\n";
      detailText += "讀取 STATUS_VOUT...\n"; // 再次讀取以防萬一
      uint16_t status_vout = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_VOUT);
      if (status_vout != 0xFFFF) {
        detailText += "  STATUS_VOUT: 0x" + String(status_vout, HEX) + "\n";
        if (status_vout & (1 << 5)) detailText += "    VOUT_UV_WARNING (Bit 5) - 輸出欠電壓警告\n";
        if (status_vout & (1 << 4)) detailText += "    VOUT_UV_FAULT (Bit 4) - 輸出欠電壓故障\n";
      } else {
        detailText += "  讀取 STATUS_VOUT 失敗\n";
      }
    }
    if (statusWord & (1 << 13)) {
      statusText += "IOUT_OC_FAULT (Bit 13) - 過電流故障\n";
      detailText += "讀取 STATUS_IOUT...\n";
      uint16_t status_iout = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_IOUT);
      if (status_iout != 0xFFFF) {
        detailText += "  STATUS_IOUT: 0x" + String(status_iout, HEX) + "\n";
        if (status_iout & (1 << 7)) detailText += "    IOUT_OC_FAULT (Bit 7) - 輸出過電流故障\n";
        if (status_iout & (1 << 6)) detailText += "    IOUT_OC_WARNING (Bit 6) - 輸出過電流警告\n";
      } else {
        detailText += "  讀取 STATUS_IOUT 失敗\n";
      }
    }
    if (statusWord & (1 << 12)) {
      statusText += "IOUT_OC_WARNING (Bit 12) - 過電流警告\n";
    }
    if (statusWord & (1 << 11)) {
      statusText += "OT_FAULT (Bit 11) - 過溫故障\n";
      detailText += "讀取 STATUS_TEMPERATURE...\n";
      uint16_t status_temp = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_TEMPERATURE);
      if (status_temp != 0xFFFF) {
        detailText += "  STATUS_TEMPERATURE: 0x" + String(status_temp, HEX) + "\n";
        if (status_temp & (1 << 7)) detailText += "    OT_FAULT (Bit 7) - 過溫故障\n";
        if (status_temp & (1 << 6)) detailText += "    OT_WARNING (Bit 6) - 過溫警告\n";
      } else {
        detailText += "  讀取 STATUS_TEMPERATURE 失敗\n";
      }
    }
    if (statusWord & (1 << 10)) {
      statusText += "OT_WARNING (Bit 10) - 過溫警告\n";
    }
    if (statusWord & (1 << 9)) {
      statusText += "VIN_UV_FAULT (Bit 9) - 輸入欠電壓故障\n";
      detailText += "讀取 STATUS_INPUT...\n";
      uint16_t status_input = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_INPUT);
      if (status_input != 0xFFFF) {
        detailText += "  STATUS_INPUT: 0x" + String(status_input, HEX) + "\n";
        if (status_input & (1 << 7)) detailText += "    VIN_OV_FAULT (Bit 7) - 輸入過電壓故障\n";
        if (status_input & (1 << 6)) detailText += "    VIN_UV_WARNING (Bit 6) - 輸入欠電壓警告\n";
      } else {
        detailText += "  讀取 STATUS_INPUT 失敗\n";
      }
    }
    if (statusWord & (1 << 8)) {
      statusText += "MFR_SPECIFIC_FAULT (Bit 8) - 製造商特定故障\n";
      detailText += "讀取 STATUS_MFR_SPECIFIC...\n";
      uint16_t status_mfr_specific = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_MFR_SPECIFIC);
      if (status_mfr_specific != 0xFFFF) {
        detailText += "  STATUS_MFR_SPECIFIC: 0x" + String(status_mfr_specific, HEX) + "\n";
      } else {
        detailText += "  讀取 STATUS_MFR_SPECIFIC 失敗\n";
      }
    }
    if (statusWord & (1 << 7)) {
      statusText += "POWER_GOOD_N (Bit 7) - 電源正常狀態\n";
    }
    if (statusWord & (1 << 6)) {
      statusText += "FAN_FAULT (Bit 6) - 風扇故障\n";
    }
    if (statusWord & (1 << 5)) {
      statusText += "OTHER_FAULT (Bit 5) - 其他故障\n";
      detailText += "讀取 STATUS_OTHER...\n";
      uint16_t status_other = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_OTHER);
      if (status_other != 0xFFFF) {
        detailText += "  STATUS_OTHER: 0x" + String(status_other, HEX) + "\n";
      } else {
        detailText += "  讀取 STATUS_OTHER 失敗\n";
      }
    }
    if (statusWord & (1 << 4)) {
      statusText += "UNKNOWN_FAULT (Bit 4) - 未知故障\n";
    }
    if (statusWord & (1 << 3)) {
      statusText += "CML (Bit 3) - 通訊、記憶體、邏輯故障\n";
      detailText += "讀取 STATUS_CML...\n";
      uint16_t status_cml = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_CML);
      if (status_cml != 0xFFFF) {
        detailText += "  STATUS_CML: 0x" + String(status_cml, HEX) + "\n";
      } else {
        detailText += "  讀取 STATUS_CML 失敗\n";
      }
    }
    // Bit 2 和 Bit 1 已經用於判斷 isWarning 和 isFault，通常不需要單獨列出
  }

  // LVGL 介面更新必須在主執行緒 (或由 lv_timer_handler 呼叫的函數) 中進行
  // 透過 lv_async_call 或在主迴圈中判斷來更新
  // 這裡為了簡潔，假設此函數在 loop() 或由 lv_timer_handler 觸發
  lv_label_set_text(status_label, statusText.c_str());
  lv_label_set_text(detail_label, detailText.c_str());

  // 更新 LED 顏色
  lv_obj_set_style_bg_color(status_led, led_color, LV_PART_MAIN);
}

void setup() {
  Serial.begin(115200);

  // Initialize I2C (PMBus)
  Wire.begin();
  Wire.setClock(100000); // Set I2C clock to 100kHz (PMBus standard)

  // Initialize LVGL
  lv_init();
  tft.begin();
  tft.setRotation(3); // Adjust rotation for your display

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LV_HOR_RES_MAX;
  disp_drv.ver_res = LV_VER_RES_MAX;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Create LVGL screen and widgets
  screen = lv_obj_create(NULL);
  lv_scr_load(screen);

  // Status LED
  status_led = lv_obj_create(screen);
  lv_obj_set_size(status_led, 20, 20); // 調整 LED 大小
  lv_obj_align(status_led, LV_ALIGN_TOP_RIGHT, -10, 10); // 調整 LED 位置
  lv_obj_set_style_radius(status_led, LV_RADIUS_CIRCLE, LV_PART_MAIN); // 設為圓形
  lv_obj_set_style_border_width(status_led, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(status_led, lv_color_make(100, 100, 100), LV_PART_MAIN);

  // Status Label
  status_label = lv_label_create(screen);
  lv_obj_set_width(status_label, LV_HOR_RES_MAX - 40); // 調整寬度，避開 LED
  lv_obj_set_align(status_label, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(status_label, 10, 10);
  lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(status_label, "初始化 PMBus 讀取...");

  // Detail Label
  detail_label = lv_label_create(screen);
  lv_obj_set_width(detail_label, LV_HOR_RES_MAX - 20);
  lv_obj_set_align(detail_label, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(detail_label, 10, 100); // 位置調整
  lv_label_set_long_mode(detail_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(detail_label, "");

  // 創建 FreeRTOS 任務
  xTaskCreatePinnedToCore(
    pmbusReadTask,   // 任務函數
    "PMBusReadTask", // 任務名稱
    4096,            // 任務堆疊大小 (位元組)
    NULL,            // 任務參數 (NULL表示沒有)
    1,               // 任務優先級 (0為最低，configMAX_PRIORITIES - 1 為最高)
    NULL,            // 任務句柄 (NULL表示不需要句柄)
    1                // 運行在 Core 1 (Core 0 通常用於 WiFi/BT 和 Arduino loop)
  );

  Serial.println("FreeRTOS PMBus 讀取任務已啟動。");
}

void loop() {
  // 在 LVGL 中安全地更新 UI
  // 因為 currentStatusWord 是由另一個 FreeRTOS 任務更新的，
  // 為了避免同時存取導致問題，可以將其傳遞給 UI 更新函數。
  // 注意：如果 UI 更新函數內部有寫入 PMBus 的操作，就需要用到 Semaphore/Mutex
  // 但目前只有讀取，所以直接傳遞值相對安全。
  interpretAndDisplayStatus(currentStatusWord, pmbusReadError);

  lv_timer_handler(); // 讓 LVGL 處理其任務
  delay(5); // 讓出 CPU 時間，避免忙等，也可以使用 vTaskDelay
}