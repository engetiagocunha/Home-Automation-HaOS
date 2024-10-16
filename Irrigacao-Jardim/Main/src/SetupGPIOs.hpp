#ifndef SETUPGPIOS_HPP
#define SETUPGPIOS_HPP

#include <Arduino.h>

// Definição dos pinos de entrada e saída
#define RELAY_PIN1 18
#define RELAY_PIN2 19
#define RELAY_PIN3 23
#define RELAY_PIN4 5

#define TOUCH_PIN1 13
#define TOUCH_PIN2 12
#define TOUCH_PIN3 14

#define RESET_BUTTON 26  // Botão de reset rede WiFi

#define DHTPIN 27
#define SOIL_SENSOR_PIN 36

// Descomente qualquer tipo que você esteja usando!
#define DHTTYPE DHT11  // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Número de dispositivos e botões táteis
const int numDevices = 4;
const int numTouchButtons = 3;

// Arrays de pinos utilizando nos #define
const int devicePins[numDevices] = { RELAY_PIN1, RELAY_PIN2, RELAY_PIN3, RELAY_PIN4 };
const int touchButtonPins[numTouchButtons] = { TOUCH_PIN1, TOUCH_PIN2, TOUCH_PIN3 };

bool deviceStates[numDevices] = { HIGH, HIGH, HIGH, HIGH };
bool lastTouchStates[numTouchButtons] = { LOW, LOW, LOW };

const int capacitanceThreshold = 30;
int touchMedians[numTouchButtons] = { 0, 0, 0 };

// Função para configurar os pinos como INPUT e OUTPUT
void setupGPIOs() {
  // Configura os dispositivos (relés) como saída
  for (int i = 0; i < numDevices; i++) {
    pinMode(devicePins[i], OUTPUT);
    digitalWrite(devicePins[i], HIGH);  // Inicializa os relés desligados
  }

  // Configura os botões tácteis como entrada
  for (int i = 0; i < numTouchButtons; i++) {
    pinMode(touchButtonPins[i], INPUT);  // Configura os botões como entrada (se aplicável)
  }

  // Configura o botão de reset como entrada com resistor interno PULLUP
  pinMode(RESET_BUTTON, INPUT_PULLUP);

  // Configura o pino do sensor de umidade de solo
  pinMode(SOIL_SENSOR_PIN, INPUT);
}



#endif  // SETUPGPIOS_HPP