/**
 * PMBus函數實現
 * 
 * 實現PMBus通訊協議相關功能：
 * - 設備掃描與初始化
 * - 讀取電壓、電流、功率等數據
 * - 數據格式轉換
 */

#include "pmbus.h"

// 全局變數
static uint8_t pmbus_address = 0; // PMBus設備地址

/**
 * 掃描並初始化PMBus設備
 * 掃描範圍：0xB0~0xB8
 * 
 * @return 是否找到PMBus設備
 */
bool initPMBus() {
    bool found = false;
    
    Serial.println("Scan PMBus device...");
    for (uint8_t addr = PMBUS_SCAN_START; addr <= PMBUS_SCAN_END; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("Address 0x");
            Serial.print(addr, HEX);
            Serial.println(" found");
            
            // 找到第一個設備後就設置為PMBus地址
            pmbus_address = addr;
            found = true;
            break;
        }
    }
    
    // 如果沒有找到設備，使用默認地址
    if (!found) {
        pmbus_address = 0;
        Serial.print("device not found");
        //Serial.println(pmbus_address, HEX);
    }
    
    return found;
}

/**
 * 獲取當前PMBus設備地址
 * 
 * @return PMBus設備地址
 */
uint8_t getPMBusAddress() {
    return pmbus_address;
}

/**
 * 讀取輸出電壓
 * 
 * @return 電壓值(V)
 * 
 */

float readL16(uint8_t address, uint8_t command){
    Wire.beginTransmission(pmbus_address);
    Wire.write(VOUT_MODE);
    Wire.endTransmission(false);
    
    Wire.requestFrom(pmbus_address,(uint8_t) 1);
    uint8_t vout_mode = 0;
    if (Wire.available()) {
        vout_mode = Wire.read();
    }
    
    // 讀取L16 raw
    Wire.beginTransmission(pmbus_address);
    Wire.write(command);
    Wire.endTransmission(false);
    
    Wire.requestFrom(pmbus_address,(uint8_t) 2);
    uint16_t data_raw = 0;
    if (Wire.available() >= 2) {
        data_raw = Wire.read();
        data_raw |= (uint16_t)Wire.read() << 8;
    }
    
    // 轉換為實際電壓值 (Linear模式)
    int exponent = (vout_mode & 0x1F);
    if (exponent > 0x0F) exponent |= 0xE0; // 負數處理
    
    return data_raw * pow(2, exponent);
}

float readL11(uint8_t command){
    Wire.beginTransmission(pmbus_address);
    Wire.write(command);
    Wire.endTransmission(false);
    Wire.requestFrom(pmbus_address,(uint8_t) 2);
    uint16_t data_raw = 0;
    if (Wire.available() >= 2) {
        data_raw = Wire.read();
        data_raw |= (uint16_t)Wire.read() << 8;
    }
    Wire.endTransmission(true);
    // PMBus Linear11格式轉換
    int8_t exponent = (data_raw >> 11);
    int16_t mantissa = (data_raw & 0x07FF);

    if(exponent > 0x0F)
        exponent |= 0xE0;
    
    if(mantissa > 0x03FF)
        mantissa |= 0xF800;

    return mantissa * pow(2, exponent);

}

float readVoltage() {
    // 讀取VOUT_MODE來確定格式
    Wire.beginTransmission(pmbus_address);
    Wire.write(VOUT_MODE);
    Wire.endTransmission(false);
    
    Wire.requestFrom(pmbus_address,(uint8_t)1);
    uint8_t vout_mode = 0;
    if (Wire.available()) {
        vout_mode = Wire.read();
    }
    Wire.endTransmission();
    // 讀取VOUT
    Wire.beginTransmission(pmbus_address);
    Wire.write(READ_VOUT);
    Wire.endTransmission(false);
    
    Wire.requestFrom(pmbus_address,(uint8_t)2);
    uint16_t vout_raw = 0;
    if (Wire.available() >= 2) {
        vout_raw = Wire.read();
        vout_raw |= (uint16_t)Wire.read() << 8;
    }
    Wire.endTransmission();
    // 轉換為實際電壓值 (Linear模式)
    int8_t exponent = (vout_mode & 0x1F);
    if (exponent > 0x0F) exponent |= 0xE0; // 負數處理
    
    return vout_raw * pow(2, exponent);
}

uint8_t CRC8_Table(const uint8_t *p, uint8_t counter)  
{  
    uint8_t crc8 = 0;  
  
    for( ; counter > 0; counter--){  
        crc8 = CRC8Table[crc8^*p];  
        p++;  
    }  
    return(crc8);  
  
}  
void clearFault() {

    uint8_t msg[] = { CLEAR_FAULTS, 0x00};  // 預留 PEC 位元
    uint8_t pec_input[] = { uint8_t(pmbus_address << 1), CLEAR_FAULTS};
    
    msg[1] = CRC8_Table(pec_input, sizeof(pec_input));  // 計算 PEC 並寫入 msg[1]

    Wire.beginTransmission(pmbus_address);
    Wire.write(msg, sizeof(msg));
    Wire.endTransmission();    
    
    /*
    const byte  clr_cmd[] = {slv_address, CLEAR_FAULTS};  
    byte crc8 = CRC8_Table(clr_cmd, 2);
    const byte  clr_cmd_pec[] = {0x03, crc8};       
    Wire.beginTransmission(pmbus_address); 
    Wire.write(clr_cmd_pec,2);  
    Wire.endTransmission(); */
}

void fanCommand(uint8_t command, uint8_t data){

    uint8_t msg[] = { command, data, 0x00 ,0x00};  // 預留 PEC 位元
    uint8_t pec_input[] = { uint8_t(pmbus_address << 1), command, data, 0x00 };
    
    msg[3] = CRC8_Table(pec_input, sizeof(pec_input));  // 計算 PEC 並寫入 msg[3]

    Wire.beginTransmission(pmbus_address);
    Wire.write(msg, sizeof(msg));
    Wire.endTransmission();    
 
}
/**
 * 讀取狀態字
 * 
 * @return 16位狀態字
 */
uint16_t readStatusWord() {
    Wire.beginTransmission(pmbus_address);
    Wire.write(STATUS_WORD);
    Wire.endTransmission(false);
    
    Wire.requestFrom(pmbus_address,(uint8_t) 2);
    uint16_t status_raw = 0;
    if (Wire.available() >= 2) {
        status_raw = Wire.read();
        status_raw |= (uint16_t)Wire.read() << 8;
    }
    Wire.endTransmission();
    return status_raw;
}

/**
 * 讀取所有PMBus數據並存入結構體
 * 
 * @param data 指向PMBusData_t結構體的指針
 */
void readAllPMBusData(PMBusData_t* data) {
    //data->voltage = readL16(pmbus_address,READ_VOUT);
    //data->current = readL11(pmbus_address,READ_IOUT);
   // data->power = readL11(pmbus_address,READ_POUT);
    
    data->voltage = readVoltage();
    data->current = readL11(READ_IOUT);
    data->pin = readL11(READ_PIN);
    data->power = readL11(READ_POUT);
    data->vin = readL11(READ_VIN);
    data->iin = readL11(READ_IIN);
    data->temp1 = readL11(READ_TEMP1);
    data->temp2 = readL11(READ_TEMP2);
    data->fanSpeed = readL11(READ_FAN_SPEED_1);
    data->statusWord = readStatusWord();
    data->pomax = readL11(MFR_POUT_MAX);
    
    // 打印到串行監視器以便調試
    Serial.print("V: "); Serial.print(data->voltage);
    Serial.print(" | I: "); Serial.print(data->current);
    Serial.print(" | P: "); Serial.print(data->power);
    Serial.print(" | T: "); Serial.print(data->temp1);
    Serial.print(" | Fan: "); Serial.print(data->fanSpeed);
    Serial.print(" | Pmax: "); Serial.print(data->pomax);
    Serial.print(" | Status: 0x"); Serial.println(data->statusWord, HEX);
}