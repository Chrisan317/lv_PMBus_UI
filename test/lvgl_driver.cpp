#include "lvgl_driver.h"
#include "pmbus_driver.h" // 需要 PMBus 驅動的頭文件來呼叫 pmbus_send_clear_fault_command()

// --- LVGL 靜態變數和對象 ---
TFT_eSPI tft = TFT_eSPI(); // 顯示器驅動實例
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10]; // 緩衝區大小
static lv_disp_drv_t disp_drv;

// --- LVGL 物件實例化 (全局可見) ---
lv_obj_t *g_status_label;
lv_obj_t *g_detail_label;
lv_obj_t *g_status_led;
lv_obj_t *g_clear_fault_btn;
lv_obj_t *g_clear_fault_label;

lv_obj_t *g_vout_label;
lv_obj_t *g_iout_label;
lv_obj_t *g_pout_label;
lv_obj_t *g_vin_label;
lv_obj_t *g_iin_label;
lv_obj_t *g_temp_label;

// --- 顯示器刷新回調函數 ---
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// --- 清除故障按鈕的事件回調函數 ---
static void clear_fault_btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    Serial.println("LVGL: 清除故障按鈕被按下！");
    // 呼叫 PMBus 驅動中的函數來發送命令
    pmbus_send_clear_fault_command();
    lv_label_set_text(g_status_label, "清除故障命令已發送...");
  }
}

// --- LVGL 初始化函數 ---
void lvgl_driver_init() {
  lv_init();
  tft.begin();
  tft.setRotation(3); // 根據您的顯示器調整方向

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LV_HOR_RES_MAX;
  disp_drv.ver_res = LV_VER_RES_MAX;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_obj_t *screen = lv_obj_create(NULL);
  lv_scr_load(screen);

  // Status LED
  g_status_led = lv_obj_create(screen);
  lv_obj_set_size(g_status_led, 20, 20);
  lv_obj_align(g_status_led, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_set_style_radius(g_status_led, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_status_led, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(g_status_led, lv_color_make(100, 100, 100), LV_PART_MAIN);

  // Status Label (主狀態訊息)
  g_status_label = lv_label_create(screen);
  lv_obj_set_width(g_status_label, LV_HOR_RES_MAX - 40);
  lv_obj_set_align(g_status_label, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(g_status_label, 10, 10);
  lv_label_set_long_mode(g_status_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(g_status_label, "系統啟動中...");

  // Detail Label (詳細故障訊息)
  g_detail_label = lv_label_create(screen);
  lv_obj_set_width(g_detail_label, LV_HOR_RES_MAX - 20);
  lv_obj_set_align(g_detail_label, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(g_detail_label, 10, 100);
  lv_label_set_long_mode(g_detail_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(g_detail_label, "");

  // PMBus 讀值標籤
  g_vout_label = lv_label_create(screen);
  lv_obj_set_pos(g_vout_label, 10, 150);
  lv_label_set_text(g_vout_label, "Vout: -- V");

  g_iout_label = lv_label_create(screen);
  lv_obj_set_pos(g_iout_label, 10, 180);
  lv_label_set_text(g_iout_label, "Iout: -- A");

  g_pout_label = lv_label_create(screen);
  lv_obj_set_pos(g_pout_label, 10, 210);
  lv_label_set_text(g_pout_label, "Pout: -- W");

  g_vin_label = lv_label_create(screen);
  lv_obj_set_pos(g_vin_label, 160, 150);
  lv_label_set_text(g_vin_label, "Vin: -- V");

  g_iin_label = lv_label_create(screen);
  lv_obj_set_pos(g_iin_label, 160, 180);
  lv_label_set_text(g_iin_label, "Iin: -- A");

  g_temp_label = lv_label_create(screen);
  lv_obj_set_pos(g_temp_label, 160, 210);
  lv_label_set_text(g_temp_label, "Temp: -- °C");

  // 清除故障按鈕
  g_clear_fault_btn = lv_btn_create(screen);
  lv_obj_set_size(g_clear_fault_btn, 120, 40);
  lv_obj_align(g_clear_fault_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);

  g_clear_fault_label = lv_label_create(g_clear_fault_btn);
  lv_label_set_text(g_clear_fault_label, "清除故障");
  lv_obj_center(g_clear_fault_label);

  lv_obj_add_event_cb(g_clear_fault_btn, clear_fault_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// --- 更新 LVGL 介面顯示 PMBus 數據的函數 ---
void lvgl_update_pmbus_data(uint16_t status_word, bool read_error,
                            float vout, float iout, float pout,
                            float vin, float iin, float temp) {
  String statusText = "";
  String detailText = "";
  lv_color_t led_color = lv_color_make(0, 0, 0); // 預設為黑色

  if (read_error) {
    statusText = "PMBus 讀取失敗!\n";
    lv_obj_set_style_text_color(g_status_label, lv_color_make(255, 0, 0), LV_PART_MAIN);
    led_color = lv_color_make(255, 0, 0); // 錯誤時 LED 顯示紅色
  } else {
    lv_obj_set_style_text_color(g_status_label, lv_color_make(255, 255, 255), LV_PART_MAIN);
    statusText += "STATUS_WORD: 0x" + String(status_word, HEX) + "\n";

    bool isFault = (status_word & (1 << 1)); // Bit 1: Any FAULT
    bool isWarning = (status_word & (1 << 2)); // Bit 2: Any WARNING

    if (isFault) {
      led_color = lv_color_make(255, 0, 0); // 紅色 (Fault)
      statusText += "偵測到故障 (FAULT)！\n";
    } else if (isWarning) {
      led_color = lv_color_make(255, 255, 0); // 黃色 (Warning)
      statusText += "偵測到警告 (WARNING)！\n";
    } else if (status_word == 0x0000) {
      led_color = lv_color_make(0, 255, 0); // 綠色 (OK)
      statusText += "無活動故障或警告。\n";
    } else {
      led_color = lv_color_make(0, 255, 0); // 預設為綠色
    }

    // --- 解讀各 STATUS 位元，這裡僅顯示文字，實際讀取詳細狀態在 PMBus 任務中完成 ---
    if (status_word & (1 << 15)) statusText += "VOUT_OV_FAULT (Bit 15) - 過電壓故障\n";
    if (status_word & (1 << 14)) statusText += "VOUT_UV_FAULT (Bit 14) - 欠電壓故障\n";
    if (status_word & (1 << 13)) statusText += "IOUT_OC_FAULT (Bit 13) - 過電流故障\n";
    if (status_word & (1 << 12)) statusText += "IOUT_OC_WARNING (Bit 12) - 過電流警告\n";
    if (status_word & (1 << 11)) statusText += "OT_FAULT (Bit 11) - 過溫故障\n";
    if (status_word & (1 << 10)) statusText += "OT_WARNING (Bit 10) - 過溫警告\n";
    if (status_word & (1 << 9))  statusText += "VIN_UV_FAULT (Bit 9) - 輸入欠電壓故障\n";
    if (status_word & (1 << 8))  statusText += "MFR_SPECIFIC_FAULT (Bit 8) - 製造商特定故障\n";
    if (status_word & (1 << 7))  statusText += "POWER_GOOD_N (Bit 7) - 電源正常狀態\n";
    if (status_word & (1 << 6))  statusText += "FAN_FAULT (Bit 6) - 風扇故障\n";
    if (status_word & (1 << 5))  statusText += "OTHER_FAULT (Bit 5) - 其他故障\n";
    if (status_word & (1 << 4))  statusText += "UNKNOWN_FAULT (Bit 4) - 未知故障\n";
    if (status_word & (1 << 3))  statusText += "CML (Bit 3) - 通訊、記憶體、邏輯故障\n";

    // 可以在這裡加入讀取詳細 STATUS_X 寄存器的邏輯，並更新 detailText
    // 為了模組化，這裡我們假設詳細狀態在 PMBus 任務中處理後，直接更新 g_detail_label。
    // 但是，由於 STATUS_VOUT 等是透過 readPMBusByte 即時讀取的，所以仍建議在這裡執行。
    // 如果您希望詳細狀態的讀取也在 pmbusReadTask 中完成並通過全域變數傳遞，則 pmbusReadTask 會更複雜。
    // 為了簡潔，我們將詳細狀態讀取放在這，因為它只在有特定錯誤時觸發。

    if (status_word & (1 << 15) || status_word & (1 << 14)) { // VOUT 相關
        uint16_t status_vout = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_VOUT);
        if (status_vout != 0xFFFF) {
          detailText += "  STATUS_VOUT: 0x" + String(status_vout, HEX) + "\n";
          if (status_vout & (1 << 7)) detailText += "    VOUT_OV_FAULT (Bit 7) - 輸出過電壓故障\n";
          if (status_vout & (1 << 6)) detailText += "    VOUT_OV_WARNING (Bit 6) - 輸出過電壓警告\n";
          if (status_vout & (1 << 5)) detailText += "    VOUT_UV_WARNING (Bit 5) - 輸出欠電壓警告\n";
          if (status_vout & (1 << 4)) detailText += "    VOUT_UV_FAULT (Bit 4) - 輸出欠電壓故障\n";
        } else {
          detailText += "  讀取 STATUS_VOUT 失敗\n";
        }
    }
    if (status_word & (1 << 13) || status_word & (1 << 12)) { // IOUT 相關
        uint16_t status_iout = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_IOUT);
        if (status_iout != 0xFFFF) {
          detailText += "  STATUS_IOUT: 0x" + String(status_iout, HEX) + "\n";
          if (status_iout & (1 << 7)) detailText += "    IOUT_OC_FAULT (Bit 7) - 輸出過電流故障\n";
          if (status_iout & (1 << 6)) detailText += "    IOUT_OC_WARNING (Bit 6) - 輸出過電流警告\n";
        } else {
          detailText += "  讀取 STATUS_IOUT 失敗\n";
        }
    }
    if (status_word & (1 << 11) || status_word & (1 << 10)) { // TEMPERATURE 相關
        uint16_t status_temp = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_TEMPERATURE);
        if (status_temp != 0xFFFF) {
          detailText += "  STATUS_TEMPERATURE: 0x" + String(status_temp, HEX) + "\n";
          if (status_temp & (1 << 7)) detailText += "    OT_FAULT (Bit 7) - 過溫故障\n";
          if (status_temp & (1 << 6)) detailText += "    OT_WARNING (Bit 6) - 過溫警告\n";
        } else {
          detailText += "  讀取 STATUS_TEMPERATURE 失敗\n";
        }
    }
    if (status_word & (1 << 9)) { // INPUT 相關
        uint16_t status_input = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_INPUT);
        if (status_input != 0xFFFF) {
          detailText += "  STATUS_INPUT: 0x" + String(status_input, HEX) + "\n";
          if (status_input & (1 << 7)) detailText += "    VIN_OV_FAULT (Bit 7) - 輸入過電壓故障\n";
          if (status_input & (1 << 6)) detailText += "    VIN_UV_WARNING (Bit 6) - 輸入欠電壓警告\n";
        } else {
          detailText += "  讀取 STATUS_INPUT 失敗\n";
        }
    }
    if (status_word & (1 << 8)) { // MFR_SPECIFIC 相關
        uint16_t status_mfr_specific = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_MFR_SPECIFIC);
        if (status_mfr_specific != 0xFFFF) {
          detailText += "  STATUS_MFR_SPECIFIC: 0x" + String(status_mfr_specific, HEX) + "\n";
        } else {
          detailText += "  讀取 STATUS_MFR_SPECIFIC 失敗\n";
        }
    }
    if (status_word & (1 << 5)) { // OTHER 相關
        uint16_t status_other = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_OTHER);
        if (status_other != 0xFFFF) {
          detailText += "  STATUS_OTHER: 0x" + String(status_other, HEX) + "\n";
        } else {
          detailText += "  讀取 STATUS_OTHER 失敗\n";
        }
    }
    if (status_word & (1 << 3)) { // CML 相關
        uint16_t status_cml = readPMBusByte(PMBUS_SLAVE_ADDRESS, PMBUS_CMD_STATUS_CML);
        if (status_cml != 0xFFFF) {
          detailText += "  STATUS_CML: 0x" + String(status_cml, HEX) + "\n";
        } else {
          detailText += "  讀取 STATUS_CML 失敗\n";
        }
    }
  }

  // 更新主狀態文字和詳細狀態文字
  lv_label_set_text(g_status_label, statusText.c_str());
  lv_label_set_text(g_detail_label, detailText.c_str());

  // 更新 LED 顏色
  lv_obj_set_style_bg_color(g_status_led, led_color, LV_PART_MAIN);

  char buf[50]; // 用於格式化字串

  // 顯示 PMBus 讀值
  if (!isnan(vout)) {
    snprintf(buf, sizeof(buf), "Vout: %.2f V", vout);
    lv_label_set_text(g_vout_label, buf);
  } else {
    lv_label_set_text(g_vout_label, "Vout: -- V");
  }

  if (!isnan(iout)) {
    snprintf(buf, sizeof(buf), "Iout: %.2f A", iout);
    lv_label_set_text(g_iout_label, buf);
    snprintf(buf, sizeof(buf), "Pout: %.2f W", pout); // 使用傳入的 pout
    lv_label_set_text(g_pout_label, buf);
  } else {
    lv_label_set_text(g_iout_label, "Iout: -- A");
    lv_label_set_text(g_pout_label, "Pout: -- W");
  }

  if (!isnan(temp)) {
    snprintf(buf, sizeof(buf), "Temp: %.1f °C", temp);
    lv_label_set_text(g_temp_label, buf);
  } else {
    lv_label_set_text(g_temp_label, "Temp: -- °C");
  }

  if (!isnan(vin)) {
    snprintf(buf, sizeof(buf), "Vin: %.2f V", vin);
    lv_label_set_text(g_vin_label, buf);
  } else {
    lv_label_set_text(g_vin_label, "Vin: -- V");
  }

  if (!isnan(iin)) {
    snprintf(buf, sizeof(buf), "Iin: %.2f A", iin);
    lv_label_set_text(g_iin_label, buf);
  } else {
    lv_label_set_text(g_iin_label, "Iin: -- A");
  }
}