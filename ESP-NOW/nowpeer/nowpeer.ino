#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int seg = 0; // Variável global, fuck it
int indentificador = 1;
int time_to_start = INT_MAX;
String message = "i";

void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
  
  // Verifica se temos dados suficientes para um inteiro (até 3 dígitos + null terminator)
  if (data_len >= 1 && data_len <= 4) {
    // Converte os dados recebidos para string e depois para inteiro, porque eu prrefiro padronizar tudo no str, mais easy 
    char receivedStr[4] = {0}; // Buffer para 3 dígitos + null terminator
    memcpy(receivedStr, data, min(data_len, 3));
    
    int receivedNumber = atoi(receivedStr);
    
    Serial.print("Número recebido: ");
    Serial.println(receivedNumber);
    
    if (receivedNumber < 999) {
      time_to_start = receivedNumber;
      //esp_now_unregister_recv_cb(); //desativa o  do callback
      //Serial.println("Mudando para modo transmissão");
    }
  } else {
    Serial.println("Dados recebidos não são um número válido");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Falha ao iniciar ESP-NOW");
    return;
  }

  // Configura o peer nem precisa, mas tinha no codigo default
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {

  if(time_to_start == 0)  {
    seg++;
    if(indentificador < 10) {
      message = message + "0" + String(indentificador) ;
    }  
    else{
      message = message + String(indentificador) ;
    }
    if(seg < 10){
      message = message + "00" + String(seg) + "f";
    }
    else if(seg < 100){
      message = message + "0" + String(seg) + "f" ;
    }
    else{
      message = message + String(seg) + "f";
    }
    esp_err_t result = ESP_FAIL;
    if(seg < 720)
      result = esp_now_send(broadcastAddress, 
                                    (uint8_t *)message.c_str(), 
                                    message.length());
      
    if (result == ESP_OK) {
      Serial.print("Enviado: ");
      Serial.println(message);
    } else {
      Serial.println("Erro no envio");
    }
    message = "i";
  }
  else if(time_to_start > 0 && seg < 720){
    time_to_start--;
    if(time_to_start > 10000)
      Serial.println("Esperando receber numero");
    //Receber tempo para inicio
    //Time to start == tempo para inicio
    Serial.println(time_to_start);
  }
  // else{

  // }
  

  delay(1000); // Espera bixo
}