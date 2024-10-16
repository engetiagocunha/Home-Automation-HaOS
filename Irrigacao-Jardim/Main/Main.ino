#include <WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <DHT.h>
#include <Ticker.h>
#include "src/SetupGPIOs.hpp"
#include "src/SetupWiFi.hpp"
#include "src/Sensors.hpp"
#include "src/PageHTML.hpp"

// Definições de variáveis
Ticker sensorTicker;
WebServer server(80);
WebSocketsServer webSocket(81);
Preferences preferences;
bool loggedIn = false;
const char* defaultUsername = "admin";
const char* defaultPassword = "admin";

// Variáveis de configurações do timer
int hora, minuto, segundo, diaDaSemana;
int configHoraInicio, configMinutoInicio, configHoraFim, configMinutoFim;
bool diasDaSemana[7] = { false, false, false, false, false, false, false };  // Domingo a Sábado

// Configuração do NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);  // UTC-3 para Fortaleza (Nordeste)
//NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000); // UTC-3 para Brasília e São Paulo
//NTPClient timeClient(ntpUDP, "pool.ntp.org", -14400, 60000); // UTC-4 para Manaus e Amazonas

// Função de configuração
void setup() {
  Serial.begin(115200);
  setupGPIOs();    // Configura GPIOs
  setupSensors();  // Inicializa o sensor DHT
  setupWiFi();     // Configura a conexão Wi-Fi
  setupMDNS();     // Configura a conexão mDNS

  // Inicializa Preferences e carrega as configurações
  preferences.begin("myApp", false);

  // Carrega configurações com valores padrão
  configHoraInicio = preferences.getInt("horaInicio", 0);
  configMinutoInicio = preferences.getInt("minutoInicio", 0);
  configHoraFim = preferences.getInt("horaFim", 23);
  configMinutoFim = preferences.getInt("minutoFim", 59);

  for (int i = 0; i < 7; i++) {
    diasDaSemana[i] = preferences.getBool(("dia" + String(i)).c_str(), false);
  }

  // Recupera estados salvos dos dispositivos
  for (int i = 0; i < numDevices; i++) {
    deviceStates[i] = preferences.getBool(("device" + String(i)).c_str(), false);
    digitalWrite(devicePins[i], deviceStates[i] ? LOW : HIGH);  // Aplica o estado salvo
  }

  setupServer();  // Configura o servidor web e WebSocket
  timeClient.begin();

  calculateTouchMedians();                   // Calcula as medianas dos toques
  sensorTicker.attach(1, updateSensorData);  // Atualiza dados dos sensores a cada segundo
}

// Função principal de loop
void loop() {
  checkButtonReset();
  server.handleClient();
  webSocket.loop();
  handleTouchButtons();  // Lida com os botões touch capacitivos
  checkAlarmAndControlRelay();
}

// Função para atualizar os dados dos sensores
void updateSensorData() {
  readDHT();
  readSoilMoisture();
  if (loggedIn) {
    notifyClients();
  }
}

// Configuração do servidor e WebSocket
void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleConfigPost);
  server.on("/login", HTTP_GET, handleLogin);
  server.on("/login", HTTP_POST, handleLoginPost);
  server.on("/logout", HTTP_GET, handleLogout);
  server.onNotFound(handleNotFound);

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void handleRoot() {
  if (!loggedIn) {
    server.sendHeader("Location", "/login");
    server.send(303);
    return;
  }

  String html = String(HOME_PAGE);
  updateHTMLPlaceholders(html);
  server.send(200, "text/html", html);
}

void handleConfigPost() {
  if (!loggedIn) {
    server.send(403, "text/plain", "Acesso negado");
    return;
  }

  if (server.hasArg("horaInicio") && server.arg("horaInicio") != "") {
    int novaHoraInicio = server.arg("horaInicio").toInt();
    if (novaHoraInicio >= 0 && novaHoraInicio < 24) {
      configHoraInicio = novaHoraInicio;
      preferences.putInt("horaInicio", configHoraInicio);
    }
  }

  if (server.hasArg("minutoInicio") && server.arg("minutoInicio") != "") {
    int novoMinutoInicio = server.arg("minutoInicio").toInt();
    if (novoMinutoInicio >= 0 && novoMinutoInicio < 60) {
      configMinutoInicio = novoMinutoInicio;
      preferences.putInt("minutoInicio", configMinutoInicio);
    }
  }

  if (server.hasArg("horaFim") && server.arg("horaFim") != "") {
    int novaHoraFim = server.arg("horaFim").toInt();
    if (novaHoraFim >= 0 && novaHoraFim < 24) {
      configHoraFim = novaHoraFim;
      preferences.putInt("horaFim", configHoraFim);
    }
  }

  if (server.hasArg("minutoFim") && server.arg("minutoFim") != "") {
    int novoMinutoFim = server.arg("minutoFim").toInt();
    if (novoMinutoFim >= 0 && novoMinutoFim < 60) {
      configMinutoFim = novoMinutoFim;
      preferences.putInt("minutoFim", configMinutoFim);
    }
  }

  for (int i = 0; i < 7; i++) {
    String diaKey = "dia" + String(i);
    diasDaSemana[i] = server.hasArg(diaKey);
    preferences.putBool(diaKey.c_str(), diasDaSemana[i]);
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLogin() {
  server.send(200, "text/html", LOGIN_PAGE);
}

void handleLoginPost() {
  if (server.hasArg("username") && server.hasArg("password")) {
    if (server.arg("username") == defaultUsername && server.arg("password") == defaultPassword) {
      loggedIn = true;
      server.sendHeader("Set-Cookie", "session=1");
      server.sendHeader("Location", "/");
      server.send(303);
      return;
    }
  }
  server.send(401, "text/plain", "Login falhou");
}

void handleLogout() {
  loggedIn = false;
  server.sendHeader("Set-Cookie", "session=; Max-Age=0");
  server.sendHeader("Location", "/login");
  server.send(303);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Não encontrado");
}

void updateHTMLPlaceholders(String& html) {
  html.replace("%TEMPERATURE%", String(temperatura, 1));
  html.replace("%HUMIDITY%", String(umidade, 1));
  html.replace("%SOIL_MOISTURE%", String(soilmoisture, 1));
  html.replace("%NUM_DEVICES%", String(numDevices));
  html.replace("%HORA_INICIO%", String(configHoraInicio));
  html.replace("%MINUTO_INICIO%", String(configMinutoInicio));
  html.replace("%HORA_FIM%", String(configHoraFim));
  html.replace("%MINUTO_FIM%", String(configMinutoFim));
  String diasSemanaHTML = "";
  for (int i = 0; i < 7; i++) {
    diasSemanaHTML += "<input type='checkbox' name='dia" + String(i) + "' " + (diasDaSemana[i] ? "checked" : "") + "> "
                      + (i == 0 ? "D" : i == 1 ? "S"
                                      : i == 2 ? "T"
                                      : i == 3 ? "Q"
                                      : i == 4 ? "Q"
                                      : i == 5 ? "S"
                                               : "S")  // Último caso, sem necessidade de verificação condicional
                      + "<br>";
  }
  html.replace("%DIAS_DA_SEMANA%", diasSemanaHTML);
}

void notifyClients() {
  String message = "{\"deviceStates\":[";
  for (int i = 0; i < numDevices; i++) {
    message += deviceStates[i] ? "true" : "false";
    if (i < numDevices - 1) message += ",";
  }
  message += "],";
  message += "\"temperature\":" + String(temperatura) + ",";
  message += "\"humidity\":" + String(umidade) + "}";
  webSocket.broadcastTXT(message);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    int index = atoi((char*)payload);
    if (index >= 0 && index < numDevices) {
      deviceStates[index] = !deviceStates[index];
      digitalWrite(devicePins[index], deviceStates[index] ? LOW : HIGH);
      preferences.putBool(("device" + String(index)).c_str(), deviceStates[index]);
      notifyClients();
    }
  }
}

void checkAlarmAndControlRelay() {
  timeClient.update();
  hora = timeClient.getHours();
  minuto = timeClient.getMinutes();
  segundo = timeClient.getSeconds();
  diaDaSemana = timeClient.getDay();

  Serial.print("Hora atual: ");
  Serial.print(hora);
  Serial.print(":");
  Serial.print(minuto);
  Serial.print(":");
  Serial.println(segundo);

  bool dentroDoIntervalo = (hora > configHoraInicio || (hora == configHoraInicio && minuto >= configMinutoInicio)) && (hora < configHoraFim || (hora == configHoraFim && minuto <= configMinutoFim));

  // Controle apenas do relé 1 com a condição de intervalo de tempo e dia da semana
  if (deviceStates[0]) {
    digitalWrite(devicePins[0], LOW);  // Relé 1 ligado manualmente
  } else if (dentroDoIntervalo && diasDaSemana[diaDaSemana]) {
    digitalWrite(devicePins[0], LOW);  // Relé 1 ligado automaticamente
  } else {
    digitalWrite(devicePins[0], HIGH);  // Relé 1 desligado
  }
  preferences.putBool("device0", deviceStates[0]);  // Salva o estado do relé 1
}

void handleTouchButtons() {
  for (int i = 0; i < numTouchButtons; i++) {
    int touchValue = touchRead(touchButtonPins[i]);
    if (touchValue < touchMedians[i] - capacitanceThreshold && lastTouchStates[i] == HIGH) {
      if (i == 0) {
        deviceStates[0] = !deviceStates[0];  // Controle manual do relé 1
      } else {
        deviceStates[i] = !deviceStates[i];
      }
      digitalWrite(devicePins[i], deviceStates[i] ? LOW : HIGH);
      preferences.putBool(("device" + String(i)).c_str(), deviceStates[i]);
      notifyClients();
    }
    lastTouchStates[i] = (touchValue < touchMedians[i] - capacitanceThreshold) ? LOW : HIGH;
  }
}

void calculateTouchMedians() {
  for (int i = 0; i < numTouchButtons; i++) {
    int sum = 0;
    for (int j = 0; j < 100; j++) {
      sum += touchRead(touchButtonPins[i]);
    }
    touchMedians[i] = sum / 100;
  }
}
