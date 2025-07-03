# PIVIT
Este repositório faz parte de um PIVIT, ofertado pela Universidade Tecnológica Federal do Paraná.

## Requisitos
A fim de poder rodar os programas aqui desenvolvidos, será necessário:
- Instalar a IDE do Arduino;
- Instalar dependências das placas da Espressif. Na IDE, acessar File -> Preferences. Em seguida, colar a seguinte URL em "Additional boards manager URLs":
> https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Após esse processo, as dependências necessárias para a ESP-32 usada no projeto (incluindo a biblioteca de BLE) serão instaladas.
- Instalar o boards manager da ESP32. Em Tools -> Boards -> Boards Manager, pesquisar por "esp32". EM seguida, instalar o boards manager "esp32" da Espressif Systems.

## Pasta BLE
Diretório legado. Possui códigos desenvolvidos quando se pensava em implementar bluetooth low energy (BLE) como único protocolo de comunicação da solução.

## Pasta ESP-NOW
Diretório que contém códigos referentes ao atual desenvolvimento da solução.

### master.ino
Código referente ao dispositivo central da solução. Possui interface bluetooth low energy (BLE) para comunicação com o celular e interface ESP-NOW para comunicação com dispostivos slave. O dispositivo mestre age como ponte entre o usuário e os dispositivos slave.

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
