# PIVIT
Este repositório faz parte de um PIVIT, ofertado pela Universidade Tecnológica Federal do Paraná.

## Requisitos
A fim de poder rodar os programas aqui desenvolvidos, será necessário:
- Instalar a IDE do Arduino;
- Instalar dependências das placas da Espressif. Na IDE, acessar File -> Preferences. Em seguida, colar a seguinte URL em "Additional boards manager URLs":
> https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Após esse processo, as dependências necessárias para a ESP-32 usada no projeto (incluindo a biblioteca de BLE) serão instaladas.
- Instalar o boards manager da ESP32. Em Tools -> Boards -> Boards Manager, pesquisar por "esp32". Em seguida, selecionar o board ESP32S3 Dev Module.

## Topologia
A solução envolve uma rede de dispositivos que usam BLE, ESP-NOW e barramentos seriais para realizar a comunicação entre si.

Esse projeto tem como objetivo criar um teste de tempo de reação. Nele, diversos dispositivos contendo um botão e um LED serão dispostos ao alcance do usuário. Sempre que um LED é aceso, o objetivo do usuário é apertar o botão desse dispositivo o mais rápido o possível. O intervalo de tempo entre o acender do LED e a resposta do usuário é chamado de tempo de reação. Essas informações serão enviadas a um aplicativo celular.

O dispositivo mestre é a ponte de comunicação entre os dispositivos com botão e LED (chamados slaves). Ele é composto por duas ESPs: a primeira realiza transmissão de dados BLE, já a outra realiza transmissão via ESP-NOW. As ESPs do dispositivo mestre usam barramentos seriais para comunicação entre si.

Os dispositivos slaves são ESPs que se comunicam com o dispositivo mestre via ESP-NOW. Possuem a função de enviar e receber informações do dispositivo mestre e cronometrar o tempo de reação do usuário.

A interface de contato do usuário com o dispositivo master pode operar em dois modos: 
- Push button: variável operation_mode =  1. Nesse caso, a interface de contato com o usuário é apenas um push button. Mais especificamente, o dispositivo entende como interação do usuário o curto do GPIO0 com GND.
- Sensor de pressão: variável operation_mode = 0. Nesse caso, o sensor BMP280 é conectado à ESP32. O sensor medirá a pressão de uma bolsa de ar, que servirá de ponto de contato com o usuário.

Por fim, o dispositivo móvel completa a topologia da solução. O aplicativo contido nele se comunica com o mestre via BLE e é responsável por enviar ordens de ativação de slaves.

## Pasta app
Contém arquivos referentes ao aplicativo android desenvolvido. O arquivo de extensão .apk é usado para instalar o app no dispositivo móvel. O arquivo de extensão .aia é usado para realizar edições na plataforma MIT app inventor.

## Pasta ESP-NOW
Diretório que contém códigos referentes ao atual desenvolvimento da solução. A solução envolve o uso de BLE e ESP-NOW.

### master.ino
Módulo legado. Código referente ao dispositivo central da solução. Possui interface bluetooth low energy (BLE) para comunicação com o celular e interface ESP-NOW para comunicação com dispostivos slave. O dispositivo mestre age como ponte entre o usuário e os dispositivos slave.

### master_BLE_Sensor.ino
Módulo referente à interface de comunicação BLE do dispositivo mestre. Comunica-se via BLE com o celular e via UART com o mestre ESP_NOW.

### master_ESPNOW.ino
Módulo referente à interface de comunicação ESP-NOW do dispositivo mestre. Comunica-se via ESP_NOW com os dispositivos slave e via UART com o mestre BLE.

### slave.ino
Código referente aos dispositivos secundários da solução. Possui interface ESP-NOW para comunicação com o dispositivo mestre.

### utils.h
Arquivo header que contém macros e estruturas de dados usadas pelo master e pelos slaves.

### Demais pastas
Códigos auxiliares e exemplos de implementações do protocolo ESP-NOW.

## backlog.txt
Arquivo de texto contendo tarefas/implementações que devem ser discutidas no futuro.

## Pasta kicad
Contém o projeto de PCIs para os dispositivos slave.