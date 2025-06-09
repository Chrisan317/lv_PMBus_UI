
#include <Arduino.h>
#include "pmbus.h"
/*
const byte addr_num = 16;
const byte ADDR[addr_num]={0x3C,0x3D,0x3E,0x3F,0x44,0x45,0x46,0x47,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F}; 
//const char Err_code[][7]={"NONE","CML","TMP","UVP","OCP","OVP","OFF","BUSY","UNKNOW","OTHER","FAN","PG","MFR","INPUT","IOUT","VOUT"};
uint8_t scan() {
  int nDevices = 0;
  byte res;
  
  for (byte i = 0; i < addr_num; ++i) {
        Wire.beginTransmission(ADDR[i]);
        byte error = Wire.endTransmission();

        if (error == 0) {
        nDevices++;
        res=ADDR[i];
        break;      
        }  
  }
  if (nDevices == 0) {   
    return res=0;
  } 
  else {
    return res;
  }
}
*/
static uint8_t pmbus_address = 0X58;
void setup()
{
    Serial.begin( 115200 );
    Wire.begin();
    //Serial.println(scan(), HEX);
    
    
     if (initPMBus()) {
        Serial.print("找到PMBus設備，地址: 0x");
        Serial.println(getPMBusAddress(), HEX);
    } else {
        Serial.println("未找到PMBus設備");
    }
}

void loop()
{
  Serial.println(readL11(READ_VIN));
  Serial.println(readL11(READ_IIN));
  Serial.println(readL11(READ_PIN));
  Serial.println(readL11(READ_IOUT));
  Serial.println(readL11(READ_POUT));
  Serial.println(readVoltage());
  Serial.println(readStatusWord(),HEX);
  /*
  Wire.beginTransmission(pmbus_address);
  Wire.write(STATUS_WORD);
  Wire.endTransmission(false);
  
  Wire.requestFrom(pmbus_address, (uint8_t)2);
  uint16_t status_raw = 0;
  if (Wire.available() >= 2) {
      status_raw = Wire.read();
      status_raw |= (uint16_t)Wire.read() << 8;
  }
  Wire.endTransmission();
  Serial.println(status_raw,HEX);*/
  /*
   Wire.beginTransmission(pmbus_address);
    Wire.write(PMBUS_CMD_VOUT_MODE);
    Wire.endTransmission(false);
    
    Wire.requestFrom(pmbus_address,(uint8_t)1);
    uint8_t vout_mode = 0;
    if (Wire.available()) {
        vout_mode = Wire.read();
    }
    Wire.endTransmission();
    Serial.println(vout_mode);
    // 讀取VOUT
    Wire.beginTransmission(pmbus_address);
    Wire.write(PMBUS_CMD_READ_VOUT);
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
    
    //return vout_raw * pow(2, exponent);
    Serial.println(vout_raw);
    Serial.println(vout_raw * pow(2, exponent));
    */
  delay(1005);
}