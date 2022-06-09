//
//    FILE: PCA9685.cpp
//  AUTHOR: Rob Tillaart
//    DATE: 24-apr-2016
// VERSION: 0.4.0
// PURPOSE: Arduino library for I2C PCA9685 16 channel PWM 
//     URL: https://github.com/RobTillaart/PCA9685_RT
//
//  HISTORY:
//  0.1.0  2016-04-24  initial BETA version
//  0.1.1  2019-01-30  testing && fixing

//  0.2.0  2020-05-25  refactor; ESP32 begin(sda,scl)
//  0.2.1  2020-06-19  fix library.json
//  0.2.2  2020-09-21  fix #1 + add getFrequency()
//  0.2.3  2020-11-21  fix digitalWrite (internal version only)

//  0.3.0  2020-11-22  fix setting frequency
//  0.3.1  2021-01-05  Arduino-CI + unit test
//  0.3.2  2021-01-14  WireN support
//  0.3.3  2021-12-23  update library.json, license, readme, minor edits
//  0.3.4  2022-01-03  add channelCount()

//  0.4.0  2022-06-09  breaking changes (sync with pca9634)
//                      rename reset() to configure()
//                      add mode1 and mode2 parameter to configure.
//                      add SUB CALL and ALL CALL functions.
//                      update documentation.
//                      renamed PCA9685_MODE2_STOP to PCA9685_MODE2_ACK
//                      add mode parameters to begin()


#include "PCA9685.h"



//////////////////////////////////////////////////////////////
//
// Constructor
//
PCA9685::PCA9685(const uint8_t deviceAddress, TwoWire *wire)
{
  _address      = deviceAddress;
  _wire         = wire;
  _channelCount = 16;
  _error        = PCA9685_OK;
}


#if defined (ESP8266) || defined(ESP32)
bool PCA9685::begin(uint8_t sda, uint8_t scl, uint8_t mode1_mask, uint8_t mode2_mask)
{
  _wire = &Wire;
  if ((sda < 255) && (scl < 255))
  {
    _wire->begin(sda, scl);
  } else {
    _wire->begin();
  }
  if (! isConnected()) return false;
  configure(mode1_mask, mode2_mask);
  return true;
}
#endif


bool PCA9685::begin(uint8_t mode1_mask, uint8_t mode2_mask)
{
  _wire->begin();
  if (! isConnected()) return false;
  configure(mode1_mask, mode2_mask);
  return true;
}


bool PCA9685::isConnected()
{
  _wire->beginTransmission(_address);
  _error = _wire->endTransmission();
  return (_error == 0);
}


void PCA9685::configure(uint8_t mode1_mask, uint8_t mode2_mask)
{
  _error = PCA9685_OK;

  setMode1(mode1_mask);
  setMode2(mode2_mask);
}


uint8_t PCA9685::writeMode(uint8_t reg, uint8_t value)
{
  if ((reg == PCA9685_MODE1) || (reg == PCA9685_MODE2))
  {
    writeReg(reg, value);
    return PCA9685_OK;
  }
  _error = PCA9685_ERR_MODE;
  return PCA9685_ERROR;
}


uint8_t PCA9685::readMode(uint8_t reg)
{
  if ((reg == PCA9685_MODE1) || (reg == PCA9685_MODE2))
  {
    _error = PCA9685_OK;
    uint8_t value = readReg(reg);
    return value;
  }
  _error = PCA9685_ERR_MODE;
  return PCA9685_ERROR;
}


// write value to single PWM channel
void PCA9685::setPWM(uint8_t channel, uint16_t onTime, uint16_t offTime)
{
  _error = PCA9685_OK;
  if (channel >= _channelCount)
  {
    _error = PCA9685_ERR_CHANNEL;
    return;
  }
  offTime &= 0x0FFFF;   // non-doc feature - to easy set figure 8 P.17
  uint8_t reg = PCA9685_CHANNEL_0 + (channel << 2);
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
  _error = PCA9685_OK;
  if (channel >= _channelCount)
  {
    _error = PCA9685_ERR_CHANNEL;
    return;
  }
  uint8_t reg = PCA9685_CHANNEL_0 + (channel << 2);
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _error = _wire->endTransmission();
  if (_wire->requestFrom(_address, (uint8_t)4) != 4)
  {
    _error = PCA9685_ERR_I2C;
    return;
  }
  uint16_t _data = _wire->read();
  *onTime = (_wire->read() * 256) + _data;
  _data = _wire->read();
  *offTime = (_wire->read() * 256) + _data;
}


// set update frequency for all channels
void PCA9685::setFrequency(uint16_t freq, int offset)
{
  _error = PCA9685_OK;
  _freq = freq;
  //  limits frequency see page 25 datasheet
  if (_freq < PCA9685_MIN_FREQ) _freq = PCA9685_MIN_FREQ;
  if (_freq > PCA9685_MAX_FREQ) _freq = PCA9685_MAX_FREQ;
  // removed float operation for speed
  // faster but equal accurate
  // uint8_t scaler = round(25e6 / (_freq * 4096)) - 1;
  uint8_t scaler = 48828 / (_freq * 8) - 1;

  uint8_t mode1 = readMode(PCA9685_MODE1);
  writeMode(PCA9685_MODE1, mode1 | PCA9685_MODE1_SLEEP);
  scaler += offset;
  writeReg(PCA9685_PRE_SCALER, scaler);
  writeMode(PCA9685_MODE1, mode1);
}


int PCA9685::getFrequency(bool cache)
{
  _error = PCA9685_OK;
  if (cache) return _freq;
  uint8_t scaler = readReg(PCA9685_PRE_SCALER);
  scaler++;
  _freq = 48828 / scaler;
  _freq /= 8;
  return _freq;
}


// datasheet P.18 - fig. 9:  
// Note: bit[11-0] ON should NOT equal timer OFF in ON mode
// in OFF mode it doesn't matter.
void PCA9685::digitalWrite(uint8_t channel, uint8_t mode)
{
  _error = PCA9685_OK;
  if (channel >= _channelCount)
  {
    _error = PCA9685_ERR_CHANNEL;
    return;
  }
  uint8_t reg = PCA9685_CHANNEL_0 + (channel << 2);
  if (mode != LOW) writeReg2(reg, 0x1000, 0x0000);
  else writeReg2(reg, 0x0000, 0x0000);
}


void PCA9685::allOFF()
{
  _error = PCA9685_OK;
  writeReg(PCA9685_ALL_OFF_H, 0x10);
}


int PCA9685::lastError()
{
  int e = _error;
  _error = PCA9685_OK;
  return e;
}


/////////////////////////////////////////////////////
//
// SUB CALL  -   ALL CALL
//
bool PCA9685::enableSubCall(uint8_t nr)
{
  if ((nr == 0) || (nr > 3)) return false;
  uint8_t prev = getMode1();
  uint8_t reg = prev;
  if (nr == 1)      reg |= PCA9685_MODE1_SUB1;
  else if (nr == 2) reg |= PCA9685_MODE1_SUB2;
  else              reg |= PCA9685_MODE1_SUB3;
  //  only update if changed.
  if (reg != prev) setMode1(reg);
  return true;
}


bool PCA9685::disableSubCall(uint8_t nr)
{
  if ((nr == 0) || (nr > 3)) return false;
  uint8_t prev = getMode1();
  uint8_t reg = prev;
  if (nr == 1)      reg &= ~PCA9685_MODE1_SUB1;
  else if (nr == 2) reg &= ~PCA9685_MODE1_SUB2;
  else              reg &= ~PCA9685_MODE1_SUB3;
  //  only update if changed.
  if (reg != prev) setMode1(reg);
  return true;
}


bool PCA9685::isEnabledSubCall(uint8_t nr)
{
  if ((nr == 0) || (nr > 3)) return false;
  uint8_t reg = getMode1();
  if (nr == 1) return (reg & PCA9685_MODE1_SUB1) > 0;
  if (nr == 2) return (reg & PCA9685_MODE1_SUB2) > 0;
  return (reg & PCA9685_MODE1_SUB3) > 0;
}


bool PCA9685::setSubCallAddress(uint8_t nr, uint8_t address)
{
  if ((nr == 0) || (nr > 3)) return false;
  writeReg(PCA9685_SUBADR(nr), address);
  return true;
}


uint8_t PCA9685::getSubCallAddress(uint8_t nr)
{
  if ((nr == 0) || (nr > 3)) return 0;
  uint8_t address = readReg(PCA9685_SUBADR(nr));
  return address;
}


bool PCA9685::enableAllCall()
{
  uint8_t prev = getMode1();
  uint8_t reg = prev | PCA9685_MODE1_ALLCALL;
  //  only update if changed.
  if (reg != prev) setMode1(reg);
  return true;
}


bool PCA9685::disableAllCall()
{
  uint8_t prev = getMode1();
  uint8_t reg = prev & ~PCA9685_MODE1_ALLCALL;
  //  only update if changed.
  if (reg != prev) setMode1(reg);
  return true;
}


bool PCA9685::isEnabledAllCall()
{
  uint8_t reg = getMode1();
  return reg & PCA9685_MODE1_ALLCALL;
}


bool PCA9685::setAllCallAddress(uint8_t address)
{
  writeReg(PCA9685_ALLCALLADR, address);
  return true;
}


uint8_t PCA9685::getAllCallAddress()
{
  uint8_t address = readReg(PCA9685_ALLCALLADR);
  return address;
}




//////////////////////////////////////////////////////////////
//
// PRIVATE
//
void PCA9685::writeReg(uint8_t reg, uint8_t value)
{
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  _error = _wire->endTransmission();
}


void PCA9685::writeReg2(uint8_t reg, uint16_t a, uint16_t b)
{
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(a & 0xFF);
  _wire->write((a >> 8) & 0x1F);
  _wire->write(b & 0xFF);
  _wire->write((b >> 8) & 0x1F);
  _error = _wire->endTransmission();
}


uint8_t PCA9685::readReg(uint8_t reg)
{
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _error = _wire->endTransmission();
  if (_wire->requestFrom(_address, (uint8_t)1) != 1)
  {
    _error = PCA9685_ERR_I2C;
    return 0;
  }
  uint8_t _data = _wire->read();
  return _data;
}

// -- END OF FILE --
