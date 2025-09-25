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

Por fim, o dispositivo móvel completa a topologia da solução. O aplicativo contido nele se comunica com o mestre via BLE e é responsável por enviar ordens de ativação de slaves.

## Pasta app
Contém arquivos referentes ao aplicativo android desenvolvido. O arquivo de extensão .apk é usado para instalar o app no dispositivo móvel. O arquivo de extensão .aia é usado para realizar edições na plataforma MIT app inventor.

## Pasta BLE
Diretório legado. Possui códigos desenvolvidos quando se pensava em implementar bluetooth low energy (BLE) como único protocolo de comunicação da solução.

## Pasta ESP-NOW
Diretório que contém códigos referentes ao atual desenvolvimento da solução. A solução envolve o uso de BLE e ESP-NOW.

### master.ino
Módulo legado. Código referente ao dispositivo central da solução. Possui interface bluetooth low energy (BLE) para comunicação com o celular e interface ESP-NOW para comunicação com dispostivos slave. O dispositivo mestre age como ponte entre o usuário e os dispositivos slave.

### master_BLE.ino
Módulo referente à interface de comunicação BLE do dispositivo mestre.

### master_ESP_NOW.ino
Módulo referente à interface de comunicação ESP-NOW do dispositivo mestre.

### slave.ino
Código referente aos dispositivos secundários da solução. Possui interface ESP-NOW para comunicação com o dispositivo mestre.

### utils.h
Arquivo header que contém macros e estruturas de dados usadas pelo master e pelos slaves.

### Demais pastas
Códigos auxiliares e exemplos de implementações do protocolo ESP-NOW.

## backlog.txt
Arquivo de texto contendo tarefas/implementações que devem ser discutidas no futuro.

## perguntas.txt
Arquivo de texto contendo ideias de implementações a serem discutidas com a equipe.
