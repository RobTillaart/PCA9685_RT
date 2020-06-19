//
//    FILE: PCA9685.cpp
//  AUTHOR: Rob Tillaart
//    DATE: 24-apr-2016
// VERSION: 0.2.1
// PURPOSE: Arduino library for I2C PCA9685 16 channel PWM 
//     URL: https://github.com/RobTillaart/PCA9685_RT
//
// HISTORY:
// 0.1.0  2016-04-24  initial BETA version
// 0.1.1  2019-01-30  testing && fixing
// 0.2.0  2020-05-25  refactor; ESP32 begin(sda,scl)
// 0.2.1  2020-06-19  fix library.json

#include <Wire.h>
#include "PCA9685.h"


// check datasheet for details
#define PCA9685_MODE1       0x00
#define PCA9685_MODE2       0x01


// MODE1 REGISTERS
#define PCA9685_RESTART     0x80
#define PCA9685_EXTCLK      0x40
#define PCA9685_AUTOINCR    0x20
#define PCA9685_SLEEP       0x10
#define PCA9685_SUB1        0x08
#define PCA9685_SUB2        0x04
#define PCA9685_SUB3        0x02
#define PCA9685_ALLCALL     0x01


// MODE2 REGISTERS (see datasheet)
#define PCA9685_INVERT      0x10
#define PCA9685_OCH         0x08
#define PCA9685_OUTDRV      0x04
#define PCA9685_OUTNE       0x03


// SPECIAL REGISTER - FREQUENCY
#define PCA9685_PRE_SCALE   0xFE


// NOT IMPLEMENTED YET
#define PCA9685_SUBADR(x)   (0x01+(x))  // x = 1..3
#define PCA9685_ALLCALLADR  0x05
#define PCA9685_TESTMODE    0xFF


//////////////////////////////////////////////////////////////
//
// Constructor
//
PCA9685::PCA9685(const uint8_t deviceAddress)
{
  _address = deviceAddress;
  _error = 0;
}


#if defined (ESP8266) || defined(ESP32)
void PCA9685::begin(uint8_t sda, uint8_t scl)
{
  Wire.begin(sda, scl);
  reset();
}
#endif

void PCA9685::begin()
{
  Wire.begin();
  reset();
}

void PCA9685::reset()
{
  _error = 0;
  writeMode(PCA9685_MODE1, PCA9685_AUTOINCR | PCA9685_ALLCALL);
  writeMode(PCA9685_MODE2, PCA9685_OUTDRV);
}


void PCA9685::writeMode(uint8_t reg, uint8_t value)
{
  if ((reg != PCA9685_MODE1) && (reg != PCA9685_MODE2))
  {
    _error = PCA9685_ERR_MODE;
    return;
  }
  writeReg(reg, value);
}


uint8_t PCA9685::readMode(uint8_t reg)
{
  if ((reg != PCA9685_MODE1) && (reg != PCA9685_MODE2))
  {
    _error = PCA9685_ERR_MODE;
    return 0;
  }
  uint8_t value = readReg(reg);
  return value;
}


// write value to single PWM channel
void PCA9685::setPWM(uint8_t channel, uint16_t onTime, uint16_t offTime)
{
  if (channel > 15)
  {
    _error = PCA9685_ERR_CHANNEL;
    return;
  }
  uint8_t reg = 0x06 + (channel << 2);
  writeReg2(reg, onTime, offTime);
}


// write value to single PWM channel
void PCA9685::setPWM(uint8_t channel, uint16_t offTime)
{
  setPWM(channel, 0, offTime);
}


// read value from single PWM channel
void PCA9685::getPWM(uint8_t channel, uint16_t* onTime, uint16_t* offTime)
{
  if (channel > 15)
  {
    _error = PCA9685_ERR_CHANNEL;
    return;
  }
  uint8_t reg = 0x06 + (channel << 2);
  Wire.beginTransmission(_address);
  Wire.write(reg);
  _error = Wire.endTransmission();
  if (Wire.requestFrom(_address, (uint8_t)4) != 4)
  {
    _error = PCA9685_ERR_I2C;
    return;
  }
  uint16_t _data = Wire.read();
  *onTime = (Wire.read() * 256) + _data;
  _data = Wire.read();
  *offTime = (Wire.read() * 256) + _data;
}


// set update frequency for all channels
void PCA9685::setFrequency(uint16_t freq)
{
  if (freq < 24) freq = 24;
  if (freq > 1526) freq = 1526;
  // removed float operation for speed
  // faster but equal accurate
  // uint8_t scaler = round(25e6 / (freq * 4096)) - 1;
  uint8_t scaler = 28828 / (freq * 8) - 1;
  writeReg(PCA9685_PRE_SCALE, scaler);
}


void PCA9685::digitalWrite(uint8_t channel, uint8_t mode)
{
  if (mode != LOW) setPWM(channel, 0x1000, 0x0000);
  else setPWM(channel, 0x0000, 0x1000);
}


int PCA9685::lastError()
{
  int e = _error;
  _error = 0;
  return e;
}

//////////////////////////////////////////////////////////////
//
// PRIVATE
//
void PCA9685::writeReg(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(_address);
  Wire.write(reg);
  Wire.write(value);
  _error = Wire.endTransmission();
}


void PCA9685::writeReg2(uint8_t reg, uint16_t a, uint16_t b)
{
  Wire.beginTransmission(_address);
  Wire.write(reg);
  Wire.write(a & 0xFF);
  Wire.write((a >> 8) & 0x0F);
  Wire.write(b & 0xFF);
  Wire.write((b >> 8) & 0x0F);
  _error = Wire.endTransmission();
}


uint8_t PCA9685::readReg(uint8_t reg)
{
  Wire.beginTransmission(_address);
  Wire.write(reg);
  _error = Wire.endTransmission();
  if (Wire.requestFrom(_address, (uint8_t)1) != 1)
  {
    _error = PCA9685_ERR_I2C;
    return 0;
  }
  uint8_t _data = Wire.read();
  return _data;
}

// -- END OF FILE --
