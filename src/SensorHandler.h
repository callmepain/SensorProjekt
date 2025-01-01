#ifndef SENSORHANDLER_H
#define SENSORHANDLER_H

#include <Adafruit_BME280.h>

class SensorHandler {
public:
    SensorHandler(Adafruit_BME280& bme);

    void init();
    void updateSensors();
    void checkSensors();
    float getTemperature() const;
    float getHumidity() const;
    float getPressure() const;

    bool isBMEInitialized() const;
    bool isMagInitialized() const;

private:
    Adafruit_BME280& bme;

    float temperature;
    float humidity;
    float pressure;

    bool bmeInitialized;

    bool testBME(); // Interne Methode zum Testen des BME280
};

#endif // SENSORHANDLER_H
