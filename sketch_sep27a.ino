
 /*
  *Title : Weather forecasting 
  *Author : Group ...
  *Description : Deploying a model that tries to forecast the sky overcast done with tensorflow converted to c++ .h file. Done with EloquentTinyML library. 
  *Date : 27.09.2021
  *Tested with : NANO 33 BLE
  */


#include <Arduino.h>
#include <EloquentTinyML.h>
#include <Array.h>           // Array Library
#include <ArduinoBLE.h>      // Bluetooth Library
#include <Arduino_HTS221.h>  // Library for the humidity and temperature sensor of the NANO 33 BLE Sense
#include <Arduino_LPS22HB.h> // Library for the pressure sensor of the NANO 33 BLE Sense

#include "model.h"           // ML model

#define INPUTS 3              // Number of inputs
#define OUTPUTS 3             // Number of outputs
#define Tensor_ARENA 8*1024   // Memory you will allocate to the model

Eloquent::TinyML::TfLite<INPUTS, OUTPUTS, Tensor_ARENA> weather_model; //Declare our model

// Initalizing global variables 
String t,h,p;
float temp_g, hum_g, pressure_g;
String classes[OUTPUTS] = {"regnerisch", "Unwetter", "kein Unwetter"};

// Return weather prediction as String
String returnPredict(float array[OUTPUTS])
{
  Array<float> out = Array<float>(array, OUTPUTS); // Create an array 
  int max_index = out.getMaxIndex();
  return classes[max_index];
}

// Read Values of BLE 33 Sensors and store them in t,h,p
void readValues()
{    
    float temp = HTS.readTemperature();        // Read temperature in °C
    float hum = HTS.readHumidity();            // Read humidity in %
    float pressure = BARO.readPressure() * 10; // Read pressure, *10 to convert it into hPa

    // Saving sensor values into a user presentable way with units
    t = String(temp) + " C";
    h = String(hum) + " %";
    p = String(pressure) + " hPa";

    temp_g = temp;
    hum_g = hum;
    pressure_g = pressure;
}

// BLE Service Name
BLEService customService("180C");

// BLE Characteristics
// Syntax: BLE<DATATYPE>Characteristic <NAME>(<UUID>, <PROPERTIES>, <DATA LENGTH>)
BLEStringCharacteristic ble_temperature("2A56", BLERead | BLENotify, 20);
BLEStringCharacteristic ble_humidity("2A57", BLERead | BLENotify, 20);
BLEStringCharacteristic ble_pressure("2A58", BLERead | BLENotify, 20);

int ledState = LOW;

void readValues();

void setup()
{
  Serial.begin(9600);         // Start the serial communication
  weather_model.begin(model); // Start the model

  if (!HTS.begin())           // Intanciate the temperature and humidity sensor
  {
    Serial.println("Feuchteigkeits-Temperatur-Sensor konnte nicht initialisiert werden!");
    while (1);
  }       
  if (!BARO.begin())          // Intanciate the pressure sensor
  {
    Serial.println("Luftdrucksensor konnte nicht initialisiert werden!");
    while (1);
  }
  if (!BLE.begin())           // Intanciate BLE 
  {
    Serial.println("BLE konnte nicht initialisiert werden!");
    while (1);
  }

    // Setting BLE Name
    BLE.setLocalName("Arduino Wetter Sensor");
    
    // Setting BLE Service Advertisment
    BLE.setAdvertisedService(customService);
    
    // Adding characteristics to BLE Service Advertisment
    customService.addCharacteristic(ble_temperature);
    customService.addCharacteristic(ble_humidity);
    customService.addCharacteristic(ble_pressure);

    // Adding the service to the BLE stack
    BLE.addService(customService);

    // Start advertising
    BLE.advertise();
    Serial.println("Das Bluetooth-Gerät ist jetzt aktiv und wartet auf Verbindungen...");

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  // Variable to check if cetral device is connected
    BLEDevice central = BLE.central();
    if (central)
    {
        Serial.print("Verbunden mit der Zentrale: ");
        Serial.println(central.address());
        while (central.connected())
        {
            delay(200);
            
            // Read values from sensors
            readValues();  

            // Writing sensor values to the characteristic
            ble_temperature.writeValue(t);
            ble_humidity.writeValue(h);
            ble_pressure.writeValue(p);
            
            // Displaying the sensor values on the Serial Monitor
            Serial.println("Sensoren ablesen");
            Serial.print("Temperatur beträgt ");
            Serial.println(t);
            Serial.print("Luftfeuchtigkeit beträgt ");
            Serial.println(h);
            Serial.print("Luftdruck beträgt ");
            Serial.println(p);
            
            float inputs[INPUTS] = {temp_g, hum_g, pressure_g}; // Create an array containing our parameters in the same order than in google colab
            float outputs[OUTPUTS] = {0, 0, 0};     // Create an empty array that will contain the 5 outputs of the Neural Network
            weather_model.predict(inputs, outputs);       // Predict the inputs and fill the outputs arguments with the neural network output

            // Print result
            Serial.print("Wettervorhersage: ");
            Serial.println(returnPredict(outputs));
            Serial.println("");

             
            // blink the LED every cycle
            // (heartbeat indicator)
            ledState = ledState ? LOW: HIGH;
            digitalWrite(LED_BUILTIN,  ledState);
            delay(10000); 
        }
    }
    Serial.print("Von der Zentrale getrennt: ");
    Serial.println(central.address());
}
  
