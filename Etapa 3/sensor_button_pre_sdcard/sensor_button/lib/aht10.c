#include "aht10.h"

Sensor_CMD eSensorCalibrateCmd[3] = {0xE1, 0x08, 0x00};
Sensor_CMD eSensorNormalCmd[3]    = {0xA8, 0x00, 0x00};
Sensor_CMD eSensorMeasureCmd[3]   = {0xAC, 0x33, 0x00};
Sensor_CMD eSensorResetCmd        = 0xBA;
bool    GetRHumidityCmd        = true;
bool    GetTempCmd             = false;

unsigned long readSensor(bool GetDataCmd)
{
    unsigned long result;
    unsigned char temp[6];

    i2c_write_blocking(I2C_PORT, ADDR_AHT, eSensorMeasureCmd, 3, true);  // true to keep master control of bus
    i2c_read_blocking(I2C_PORT, ADDR_AHT, temp, 6, false);  // false - finished with bus

    if(GetDataCmd)
    {
        result = ((((unsigned long)temp[1] << 16)) | (((unsigned long)temp[2]) << 8) | ((unsigned long)temp[3])) >> 4;
    }
    else
    {
        result = ((((unsigned long)temp[3]) & 0x0F) << 16) | (((unsigned long)temp[4]) << 8) | ((unsigned long)temp[5]);
    }

    return result;
}

float GetTemperature(void)
{
    float value = readSensor(GetTempCmd);
    return ((200 * value) / 1048576) - 50;
}

float GetHumidity(void)
{
    float value = readSensor(GetRHumidityCmd);
    if (value == 0) {
        return 0;                       // Some unrealistic value
    }
    return value * 100 / 1048576;
}

float GetDewPoint(void)
{
  float humidity = GetHumidity();
  float temperature = GetTemperature();

  // Calculate the intermediate value 'gamma'
  float gamma = log(humidity / 100) + WATER_VAPOR * temperature / (BAROMETRIC_PRESSURE + temperature);
  // Calculate dew point in Celsius
  float dewPoint = BAROMETRIC_PRESSURE * gamma / (WATER_VAPOR - gamma);

  return dewPoint;
}