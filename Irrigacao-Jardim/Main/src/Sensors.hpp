#ifndef SENSORS_HPP
#define SENSORS_HPP

#include <DHT.h>  // Inclui a biblioteca DHT
#include "SetupGPIOs.hpp"

// Definições das variáveis globais
float temperatura;
float umidade;

float soilMoistureValue = 0;  // Armazena o valor lido do sensor
float soilmoisture;

DHT dht(DHTPIN, DHTTYPE);

// Funções para inicializar e ler o sensor DHT
void setupSensors() {
  dht.begin();  // Inicializa o sensor DHT com o pino e o tipo definidos
}

void readDHT() {
  umidade = dht.readHumidity();         // Lê a umidade
  temperatura = dht.readTemperature();  // Lê a temperatura em Celsius

  // Verifica se as leituras são válidas
  if (isnan(umidade) || isnan(temperatura)) {
    Serial.println("Falha ao ler do sensor DHT!");
  }

  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
}

// Função para ler o sensor de umidade do solo
void readSoilMoisture() {
  soilMoistureValue = analogRead(SOIL_SENSOR_PIN);         // Lê o valor analógico do sensor
  soilmoisture = map(soilMoistureValue, 1023, 0, 0, 100);  // Mapeia o valor para uma porcentagem de 0% a 100%

  Serial.print("Umidade do solo: ");
  Serial.print(soilmoisture);
  Serial.println("%");
}

#endif  // SENSORS_HPP
