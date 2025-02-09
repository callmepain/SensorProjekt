#include "SensorConfig.h"

INA219Manager ina1(0x40);
INA219Manager ina2(0x41);
INA219Manager ina3(0x44);
GY302Manager lightSensor(0x23);
Adafruit_BME280 bme;
