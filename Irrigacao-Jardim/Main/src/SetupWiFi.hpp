#ifndef WIFI_SETUP_HPP
#define WIFI_SETUP_HPP

#include <ESPmDNS.h>      // Biblioteca para mDNS
#include <WiFiManager.h>  // Biblioteca para WiFiManager
#include "SetupGPIOs.hpp"

WiFiManager wm;  // Instância global do WiFiManager

// Função para configurar o WiFiManager
void setupWiFi() {
  WiFi.mode(WIFI_STA);  // Define o modo Wi-Fi para estação (STA)
  Serial.println("\nIniciando configuração do WiFi...");

  // Configuração WiFiManager
  bool wm_nonblocking = false;  // Define o modo não bloqueante
  if (wm_nonblocking) wm.setConfigPortalBlocking(false);

  // Menu customizado
  std::vector<const char*> menu = { "wifi", "info", "exit" };
  wm.setMenu(menu);
  wm.setClass("invert");          // Define o tema invertido
  wm.setConfigPortalTimeout(30);  // Tempo limite de 30 segundos

  // Conectar automaticamente
  bool res = wm.autoConnect("ESP3201-CONFIG", "12345678");
  if (!res) {
    Serial.println("Falha na conexão ou tempo limite esgotado");
  } else {
    Serial.println("Conectado com sucesso!");
  }
}

// Função para Configuração do mDNS
void setupMDNS() {
  if (!MDNS.begin("esphome")) {
    Serial.println("Erro ao configurar o mDNS!");
    while (1) {
      delay(1000);  // Loop infinito se falhar
    }
  }
  Serial.println("mDNS iniciado. Acesse esphome.local");
  MDNS.addService("http", "tcp", 80);  // Serviço HTTP via mDNS
}

// Função para verificar se o botão de reset foi pressionado
void checkButtonReset() {
  unsigned long buttonPressStartTime = 0;

  // Verifica se o botão foi pressionado
  if (digitalRead(RESET_BUTTON) == LOW) {
    Serial.println("Botão pressionado...");
    delay(50);  // Debounce
    if (digitalRead(RESET_BUTTON) == LOW) {
      buttonPressStartTime = millis();  // Salva o momento em que o botão foi pressionado

      // Mantém o loop enquanto o botão estiver pressionado
      while (digitalRead(RESET_BUTTON) == LOW) {
        unsigned long pressDuration = millis() - buttonPressStartTime;

        // Acessa o WiFi Manager após 3 segundos
        if (pressDuration >= 2000 && pressDuration < 4000) {
          Serial.println("Acessando as configurações do WiFi Manager...");
          wm.startConfigPortal();  // Abre o portal de configuração sem reiniciar
          return;
        }

        // Reseta o dispositivo após 4 segundos
        if (pressDuration >= 4000) {
          Serial.println("Apagando as configurações do WiFi e reiniciando...");
          wm.resetSettings();  // Limpa as configurações do WiFi
          ESP.restart();       // Reinicia o ESP32
        }
      }
    }
  }
}


#endif  // WIFI_SETUP_HPP