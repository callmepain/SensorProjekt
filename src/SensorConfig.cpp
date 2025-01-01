#include "SensorConfig.h"

INA219Manager ina1(0x40);
INA219Manager ina2(0x41);
GY302Manager lightSensor(0x23);
VL53L1XManager distanceSensor(0x29, Wire);
Adafruit_BME280 bme;
