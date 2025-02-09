#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include "INA219Manager.h"
#include "GY302Manager.h"
#include <Adafruit_BME280.h>

extern INA219Manager ina1;
extern INA219Manager ina2;
extern INA219Manager ina3;
extern GY302Manager lightSensor;
extern Adafruit_BME280 bme;

#endif
