# LV PMBus UI

A PMBus user interface project based on LVGL, running on ESP32.

基於 LVGL 的 PMBus 用戶介面專案，運行在 ESP32 上。

## Features / 功能

- PMBus communication and data reading / PMBus 通訊與數據讀取
- LVGL-based graphical user interface / 基於 LVGL 的圖形用戶介面
- Real-time data display and updates / 實時數據顯示與更新
- Button input handling / 按鈕輸入處理

## Installation / 安裝

1. Clone the repository / 複製倉庫:
   ```bash
   git clone https://github.com/Chrisan317/lv_PMBus_UI.git
   cd lv_PMBus_UI
   ```

2. Install PlatformIO / 安裝 PlatformIO:
   - Download and install PlatformIO from https://platformio.org/
   - 下載並安裝 PlatformIO：https://platformio.org/

3. Open the project in PlatformIO / 在 PlatformIO 中打開專案:
   - Open PlatformIO IDE
   - 打開 PlatformIO IDE
   - Select "Open Project" and choose the project directory
   - 選擇「打開專案」並選擇專案目錄

4. Build and upload / 編譯並上傳:
   - Connect your ESP32 board / 連接您的 ESP32 開發板
   - Click "Build" then "Upload" in PlatformIO / 在 PlatformIO 中點擊「編譯」然後「上傳」

## Usage / 使用方法

1. Power on the ESP32 device / 為 ESP32 設備供電
2. The device will initialize PMBus communication / 設備將初始化 PMBus 通訊
3. The LVGL UI will display PMBus data / LVGL UI 將顯示 PMBus 數據
4. Use the button to interact with the interface / 使用按鈕與介面互動

## Dependencies / 依賴項

- LVGL v9.2.2 / LVGL v9.2.2
- TFT_eSPI v2.5.43 / TFT_eSPI v2.5.43
- Arduino framework / Arduino 框架
- ESP32 platform / ESP32 平台

## Project Structure / 專案結構

```
lv_PMBus_UI/
├── include/          # Header files / 頭文件
├── lib/              # Private libraries / 私有庫
├── src/              # Source files / 源文件
│   ├── main.cpp      # Main application / 主應用程式
│   ├── pmbus.cpp     # PMBus functions / PMBus 函數
│   ├── pmbus.h       # PMBus definitions / PMBus 定義
│   ├── ui.cpp        # UI functions / UI 函數
│   ├── ui.h          # UI definitions / UI 定義
│   └── backgroundr.c # Background renderer / 背景渲染器
├── test/             # Test files / 測試文件
└── platformio.ini    # PlatformIO configuration / PlatformIO 配置
```

## Contributing / 貢獻

1. Fork the repository / Fork 此倉庫
2. Create a feature branch / 創建功能分支
3. Commit your changes / 提交您的變更
4. Push to the branch / 推送到分支
5. Create a Pull Request / 創建 Pull Request

## License / 許可證

This project is licensed under the MIT License / 此專案採用 MIT 許可證。

## Contact / 聯絡方式

For questions or support, please open an issue on GitHub / 如有問題或需要支援，請在 GitHub 上開啟 issue。