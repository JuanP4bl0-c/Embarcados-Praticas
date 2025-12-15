# 🌱 Sistema de Estufa Inteligente com MQTT e AWS IoT Core

Sistema de monitoramento e irrigação automática para cultivo inteligente, desenvolvido com ESP32 WROVER e AWS IoT Core.

## 📋 Visão Geral

Este projeto visa o desenvolvimento de uma estufa inteligente com controle remoto via MQTT, substituindo comunicação local (WiFi/Bluetooth de curta distância) por protocolo MQTT através da AWS IoT Core. O sistema monitora condições ambientais e de solo em tempo real, tomando decisões automatizadas de irrigação baseadas em parâmetros configuráveis para cada tipo de cultivo.

## ✨ Funcionalidades

### Monitoramento em Tempo Real
- **Sensor UV** (GPIO 32): Exposição solar a cada 15s
- **Sensor de Umidade do Solo** (GPIO 33): Umidade do solo a cada 20s
- **Sensor DHT11** (GPIO 27): Temperatura e umidade do ar a cada 10s

### Irrigação Inteligente
- **Controle Manual**: Via MQTT topic `esp32/solenoid`
- **Controle Automático**: Baseado em umidade do solo e parâmetros de cultivo
- **Solenoide 12V** (GPIO 26): Válvula de irrigação acionada automaticamente

### Configuração Remota
- **Parâmetros por Planta**: Tomate, alface, pimentão, manjericão (pré-configurados)
- **Atualização via MQTT**: Topic `esp32/config` permite ajuste remoto
- **Alertas Inteligentes**: Notificações quando parâmetros saem do ideal

## 🚀 Status do Projeto

✅ **Concluído:**
1. Refatoração do código para ESP32 WiFi
2. Regulação da placa fotovoltaica para controle de exposição solar
3. Implementação da comunicação com protocolo MQTT
4. Configuração do ambiente AWS IoT Core
5. Válvula solenoide integrada e funcionando
6. **NOVO:** Sistema modular com arquivos separados por sensor
7. **NOVO:** Irrigação automática baseada em IA (parâmetros de cultivo)
8. **NOVO:** Configuração remota via MQTT

⏳ **Em andamento:**
- Gerenciamento de energia para baixo consumo
- Implementação do Banco de Dados
- Modelagem física e impressão da PCB (80% concluído)

## 🛠️ Arquitetura do Sistema

```
┌──────────────────────────────────────────────────┐
│          ESP32 WROVER (Estufa Inteligente)       │
└──────────────────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
   Sensores      Configuração     Atuadores
        │              │              │
  ┌─────┴─────┐  ┌────┴────┐   ┌─────┴─────┐
  │ UV (32)   │  │ Plant   │   │ Solenoide │
  │ DHT11(27) │  │ Config  │   │ (GPIO 26) │
  │ Solo (33) │  │ Manager │   └───────────┘
  └───────────┘  └─────────┘
        │              │              
        └──────┬───────┘              
               ▼                      
         ┌──────────┐                
         │   MQTT   │                
         │ AWS IoT  │                
         └────┬─────┘                
              │                      
      ┌───────┴────────┐             
      ▼                ▼             
 Publicação       Subscrição         
  (Sensores)    (Comandos/Config)    
```

## 📡 Tópicos MQTT

### Publicação (ESP32 → Cloud)
- `esp32/uv` - Dados do sensor UV
- `esp32/soil_moisture` - Umidade do solo
- `esp32/dht11` - Temperatura e umidade do ar
- `esp32/alerts` - Alertas e notificações
- `esp32/config` - Configuração atual (inicial)

### Subscrição (Cloud → ESP32)
- `esp32/solenoid` - Controle manual da irrigação
- `esp32/config` - Atualizar parâmetros de cultivo

## 🔧 Hardware

### ESP32 WROVER Freenove
- **WiFi**: 2.4GHz
- **PSRAM**: 8MB (GPIO 16/17 reservados)
- **Tensão**: 3.3V
- **ADC**: 12-bit, atenuação 12dB

### Sensores
- **UV**: Sensor analógico (ADC1_CH4 - GPIO 32)
- **Umidade do Solo**: Sensor resistivo (ADC1_CH5 - GPIO 33)
- **DHT11**: Temperatura e umidade digital (GPIO 27)

### Atuadores
- **Solenoide 12V**: Válvula de irrigação (GPIO 26 via relé)

### Pinout Otimizado
Todos os periféricos no lado direito da placa para facilitar conexão:
```
GPIO 32 → UV Sensor (Analógico)
GPIO 33 → Umidade do Solo (Analógico)
GPIO 27 → DHT11 (Digital)
GPIO 26 → Solenoide (Digital)
```

## 📦 Estrutura de Arquivos

```
main/
├── main.c                 # Aplicação principal
├── wifi_manager.c/h       # Gerenciamento WiFi
├── mqtt_manager.c/h       # Gerenciamento MQTT
├── uv_sensor.c/h          # Módulo sensor UV
├── soil_moisture.c/h      # Módulo umidade do solo
├── dht11_sensor.c/h       # Módulo DHT11
├── solenoid.c/h           # Módulo solenoide
├── plant_config.c/h       # Configuração de cultivo
└── certs/                 # Certificados AWS IoT

Documentação:
├── README.md              # Este arquivo
├── IRRIGACAO_INTELIGENTE.md  # Guia do sistema de irrigação
├── MQTT_TOPICS.md         # Guia completo de tópicos MQTT
├── ARQUITETURA.md         # Arquitetura do sistema
├── PINOUT.md              # Mapeamento de GPIOs
├── WROVER_PINOUT.md       # Especificações WROVER
└── COMPATIBILIDADE.md     # DevKit vs WROVER

Scripts:
├── test_irrigation.py     # Gerador de configs de cultivo
└── test_mqtt_python.py    # Testes MQTT
```

## 🚦 Como Usar

### 1. Compilar e Flashear

```bash
# Configurar ambiente ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Compilar
idf.py build

# Flashear
idf.py flash

# Monitorar
idf.py monitor
```

### 2. Configurar WiFi e AWS

Edite `main/main.c`:
```c
#define WIFI_SSID "seu_wifi"
#define WIFI_PASS "sua_senha"
#define AWS_IOT_ENDPOINT "seu-endpoint.iot.region.amazonaws.com"
```

### 3. Testar Irrigação Manual

No AWS IoT Test, publicar em `esp32/solenoid`:
```json
{"state": true}    // Liga
{"state": false}   // Desliga
```

### 4. Configurar Parâmetros de Cultivo

Usar script Python para gerar configuração:
```bash
# Para tomate (padrão)
python3 test_irrigation.py --config tomate

# Para alface
python3 test_irrigation.py --config alface

# Customizado
python3 test_irrigation.py --threshold 30 --enable-auto
```

Copiar JSON gerado e publicar em `esp32/config`

### 5. Monitorar Sistema

Subscrever em `esp32/#` para ver todos os dados e alertas

## 🌱 Parâmetros de Cultivo (Padrão: Tomate)

```
🌡️  Temperatura:    18°C - 28°C
💨 Umidade Ar:      60% - 80%
💧 Umidade Solo:    60% - 80%
☀️  Exposição UV:    30% - 70%
🚰 Limiar Irrigação: -25%
```

Quando umidade do solo < 35% (60% - 25%), sistema irriga automaticamente por 10 segundos.

## 📚 Documentação Detalhada

- **[IRRIGACAO_INTELIGENTE.md](./IRRIGACAO_INTELIGENTE.md)** - Guia completo do sistema de irrigação automática
- **[MQTT_TOPICS.md](./MQTT_TOPICS.md)** - Todos os tópicos MQTT com exemplos e testes
- **[ARQUITETURA.md](./ARQUITETURA.md)** - Arquitetura do sistema e fluxo de dados
- **[PINOUT.md](./PINOUT.md)** - Mapeamento de todos os GPIOs utilizados
- **[COMPATIBILIDADE.md](./COMPATIBILIDADE.md)** - Diferenças DevKit vs WROVER

## 🧪 Testes

### Teste de Integração Completo

1. Ligar ESP32 e verificar conexão WiFi/MQTT
2. Subscrever em `esp32/#` no AWS IoT Test
3. Verificar recebimento de dados dos sensores
4. Publicar `{"state":true}` em `esp32/solenoid`
5. Verificar GPIO 26 = HIGH no monitor serial
6. Publicar `{"irrigation_threshold": 80}` em `esp32/config`
7. Aguardar acionamento automático de irrigação
8. Verificar alerta publicado em `esp32/alerts`

## 🔐 Segurança

- **TLS 1.2**: Comunicação criptografada com AWS IoT
- **Certificados X.509**: Autenticação mútua
- **Políticas IoT**: Controle granular de permissões por tópico

## 🤝 Contribuindo

Este é um projeto acadêmico para a disciplina de Sistemas Embarcados (UFC - Campus Quixadá).

## 📄 Licença

Projeto desenvolvido para fins educacionais.

## 👥 Autores

Estudantes de Engenharia de Software - UFC Quixadá

---

**Versão:** 2.0 (Sistema Inteligente)  
**Última atualização:** 2025  
**ESP-IDF:** v6.0  
**Hardware:** ESP32 WROVER Freenove
