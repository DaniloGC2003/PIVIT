#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int seg = 0; // Variável global (não é reinicializada)
int indentificador = 5;
int timetoStart = 100;


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Falha ao iniciar ESP-NOW");
    return;
  }

  // Configura o peer (opcional para broadcast)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  seg++;
  esp_err_t result = ESP_FAIL;
  String mensagem = String(timetoStart);
  if(timetoStart > 0)
     result = esp_now_send(broadcastAddress, 
                         (uint8_t*)mensagem.c_str(),  // <-- Use .c_str() aqui
                         mensagem.length() + 1); 
    
  if (result == ESP_OK) {
    Serial.print("Enviado: ");
    Serial.println(timetoStart);
  } else {
    Serial.println("Erro no envio");
  }
  timetoStart--;
  delay(1000); // Espera 1 segundos
}