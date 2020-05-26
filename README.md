# PCA9685_RT

Arduino library for I2C PCA9685 16 channel PWM extender

# Description

This library is to control the I2C PCA9685 PWM extender. 
The 16 channels are independently configurable is steps of 1/4096.
this allows for better than 0.1% finetuning of the duty-cycle
of the PWM signal. Furthermore the PWM's of the different signals
have individual start and stop moments. This can be used to distribute the
power more evenly over multiple servo's or give special effects when  
used in an RGB LED.



### interface

**begin()** initializes the library after startup. Mandatory.

**begin(sda, scl)** idem, ESP32 ESP8266 only. Library does not support 
multiple Wire instances (yet).

**reset()** resets the library to start up conditions.

**writeMode(reg, mode)** configuration of one of the two configuration registers.
check datasheet for details.

**readMode(reg)** reads back the configured mode, useful to add or remove a 
single flag (bit masking)

**setPWM(channel, ontime, offtime)** The chip has 16 channels to do PWM.
The signal is divided in 4096 steps, 0..4095. 
The pulse can begin =**ontime** on any step and it can stop on any step =**offtime**.
This allows e.g. to distribute the power over the 16 channels, e.g. the
channels do not need to start at the same moment with HIGH.

**setPWM(channel, offtime)** simple PWM that always start on **ontime = 0**

**getPWM(channel, ontime, offtime)** read back the configuration of the channel.

**setFrequency(freq)** set the update speed of the channels. 
This is for all channels at once.

**digitalWrite(channel, mode)** mode = HIGH or LOW, just use the PCA9685 as 
a digitalpin. 
This single function replaces the setON() and setOFF() that will become
obsolete in the future.

**lastERror()** returns **PCA9685_OK = 0** if all is OK, and 

| Error code | Value | Description |
|:----|:----:|:----|
| PCA9685_OK          | 0x00 | Everything went well
| PCA9685_ERROR       | 0xFF | generic error
| PCA9685_ERR_CHANNEL | 0xFE | Channel out of range
| PCA9685_ERR_MODE    | 0xFD | Invalid mode register chosen | 
| PCA9685_ERR_I2C     | 0xFF | PCA9685 I2C communication error



# Operation

See examples
