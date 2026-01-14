# 🔋 Sistema de Economia de Energia - ESP32 Estufa Inteligente

## 📋 Visão Geral

O ESP32 agora possui um sistema inteligente de economia de energia que:
- ✅ **Reduz consumo** durante períodos de inatividade
- ✅ **Mantém WiFi ativo** para receber comandos MQTT
- ✅ **Acorda automaticamente** quando chega o tempo de leitura
- ✅ **Acorda por comandos** MQTT em tempo real
- ✅ **Modo automático** que decide baseado no período de leitura

---

## 🎯 Modos de Operação

### 1. **NORMAL** (Sem economia)
- CPU sempre em 240 MHz
- Sem sleep entre leituras
- **Uso:** Debug, testes rápidos

### 2. **LIGHT_SLEEP** (Sempre economiza)
- Entra em sleep entre TODAS as leituras
- WiFi em modo economia (mantém conexão)
- **Uso:** Máxima economia de bateria

### 3. **AUTO** (Inteligente - PADRÃO)
- Decide automaticamente baseado no período
- Sleep apenas se período ≥ 60 segundos (1 minuto)
- **Uso:** Balanceamento automático

---

## ⚙️ Como Funciona

### Light Sleep do ESP32

O Light Sleep permite:
- 🔋 **CPU desligada** mas mantém RAM
- 📡 **WiFi em modo economia** (mantém conexão)
- ⏰ **Wake-up por timer** (tempo de leitura)
- 📨 **Wake-up por MQTT** (comando recebido)
- ⚡ **Retoma instantaneamente** ao acordar

### Consumo de Energia

| Modo | Consumo Aproximado |
|------|-------------------|
| Normal (240 MHz) | ~240 mA |
| Light Sleep | ~0.8 mA |
| **Economia** | **~99.7%** |

### Exemplo Prático

**Configuração:** Período de leitura = 5 minutos

```
┌─────────────────────────────────────┐
│ Ciclo de 5 minutos                  │
├─────────────────────────────────────┤
│ Leitura DHT11:      ~2s  (240mA)   │
│ Publicação MQTT:    ~1s  (240mA)   │
│ Light Sleep:      297s  (0.8mA)    │  ← 99% do tempo!
└─────────────────────────────────────┘

Economia: De ~72Wh/dia para ~1.9Wh/dia (97% menos!)
```

---

## 📡 Comandos MQTT

### 1. Habilitar Economia de Energia
```json
{
  "command": "power_save_on"
}
```

### 2. Desabilitar Economia de Energia
```json
{
  "command": "power_save_off"
}
```

### 3. Alterar Modo de Economia

**Modo Automático (padrão):**
```json
{
  "command": "set_power_mode",
  "mode": "auto"
}
```

**Modo Light Sleep (sempre economiza):**
```json
{
  "command": "set_power_mode",
  "mode": "light_sleep"
}
```

**Modo Normal (sem economia):**
```json
{
  "command": "set_power_mode",
  "mode": "normal"
}
```

### 4. Ver Estatísticas de Energia
```json
{
  "command": "power_stats"
}
```

**Resposta nos logs:**
```
════════════════════════════════════════
     ESTATÍSTICAS DE ECONOMIA DE ENERGIA
════════════════════════════════════════
Total de sleeps: 120
Tempo total dormindo: 578400 ms (9.64 min)
Wake-ups por timer: 115
Wake-ups por evento: 5
Média de sleep: 4820 ms
Modo atual: AUTO
Estado: HABILITADO
════════════════════════════════════════
```

### 5. Ver Status (inclui info de energia)
```json
{
  "command": "get_status"
}
```

**Resposta em `esp32/status`:**
```json
{
  "status": "online",
  "read_period_minutes": 5,
  "solenoid_state": false,
  "solenoid_enabled": true,
  "power_save_enabled": true,
  "power_save_mode": "auto",
  "uptime_seconds": 3600,
  "timestamp": 1736899200000,
  "datetime": "2025-01-14 15:30:00"
}
```

---

## 🔄 Fluxo de Operação

```
┌──────────────────────────────────────────┐
│ Task DHT11/UV/Soil                       │
└─────────────────┬────────────────────────┘
                  │
                  ▼
    ┌─────────────────────────────┐
    │ Lê sensor e publica MQTT    │
    └─────────────┬───────────────┘
                  │
                  ▼
    ┌─────────────────────────────┐
    │ Obtém período de leitura    │
    │ (ex: 5 minutos = 300.000ms) │
    └─────────────┬───────────────┘
                  │
                  ▼
    ┌──────────────────────────────────────┐
    │ power_manager_should_sleep(300000)?  │
    └─────────┬────────────────────┬───────┘
              │ SIM                │ NÃO
              ▼                    ▼
    ┌──────────────────┐   ┌──────────────┐
    │ Light Sleep      │   │ vTaskDelay   │
    │ (0.8mA)          │   │ (240mA)      │
    └────────┬─────────┘   └──────┬───────┘
             │                    │
             │ Timer OU MQTT      │
             ▼                    ▼
    ┌──────────────────────────────┐
    │ Acorda e volta ao início     │
    └──────────────────────────────┘
```

---

## 💡 Decisão Automática (Modo AUTO)

O modo AUTO decide se deve usar sleep baseado no período:

| Período de Leitura | Comportamento |
|-------------------|---------------|
| < 1 minuto | vTaskDelay (sem sleep) |
| ≥ 1 minuto | Light Sleep (economia) |

**Lógica:**
```c
bool power_manager_should_sleep(uint32_t period_ms)
{
    if (!enabled) return false;
    if (mode == NORMAL) return false;
    if (mode == LIGHT_SLEEP) return true;
    
    // Modo AUTO:
    return period_ms >= 60000;  // ≥ 1 minuto
}
```

---

## 🧪 Cenários de Uso

### Cenário 1: Monitoramento Intensivo (Período 10s)
```json
{"command": "set_read_period", "minutes": 0.16}  // ~10s
{"command": "set_power_mode", "mode": "normal"}
```
- ❌ Sem sleep (leituras muito frequentes)
- ✅ Máxima responsividade

### Cenário 2: Monitoramento Normal (Período 5min)
```json
{"command": "set_read_period", "minutes": 5}
{"command": "set_power_mode", "mode": "auto"}
```
- ✅ Light sleep entre leituras
- ✅ Economia de ~97%
- ✅ Acorda por comando MQTT

### Cenário 3: Modo Bateria (Período 30min)
```json
{"command": "set_read_period", "minutes": 30}
{"command": "set_power_mode", "mode": "light_sleep"}
```
- ✅ Máxima economia
- ✅ Ideal para alimentação por bateria
- ✅ Ainda responde a comandos MQTT

---

## 📊 Comparação de Consumo

### Período de 5 minutos, 24 horas

| Modo | Consumo/dia | Duração Bateria 3000mAh |
|------|-------------|-------------------------|
| **NORMAL** | 240mA × 24h = 5.76Ah | ~12h |
| **LIGHT_SLEEP** | Avg 5mA × 24h = 0.12Ah | ~25 dias! |

**Cálculo detalhado (5 min):**
```
Normal: 
  240mA × 24h = 5760mAh/dia

Light Sleep:
  - Ativo: 3s × 240mA = 720mA·s
  - Sleep: 297s × 0.8mA = 237.6mA·s
  - Total por ciclo: 957.6mA·s
  - Ciclos/dia: 288 (24h ÷ 5min)
  - Total: 957.6 × 288 = 275.8Ah·s ÷ 3600 = 76.6mAh/dia
  
Economia: (5760 - 76.6) / 5760 = 98.7%! 🎉
```

---

## ⚠️ Considerações Importantes

### 1. WiFi em Light Sleep
- ✅ Mantém conexão
- ⚠️ Latência aumenta (~100ms)
- ✅ Recebe comandos MQTT normalmente

### 2. Wake-up por MQTT
- O ESP32 acorda automaticamente ao receber dados
- Comandos como `publish_all` funcionam imediatamente
- Não há necessidade de esperar o timer

### 3. Tasks e Core Pinning
- DHT11 no Core 1 (alta prioridade)
- Sleep afeta apenas tasks em espera
- Tasks ativas não são interrompidas

### 4. Período Mínimo para Sleep
- Sleeps < 1s usam vTaskDelay
- Overhead de wake-up: ~10ms
- Sleep efetivo: duration - 10ms

---

## 🐛 Troubleshooting

### Sleep não ativa?
```bash
# Verifique status
{"command": "get_status"}

# Verifique modo
power_save_enabled: true
power_save_mode: "auto"

# Verifique período
read_period_minutes: >= 1
```

### WiFi desconecta?
- Light sleep mantém conexão WiFi
- Se desconectar, use `WIFI_PS_NONE`:
```c
esp_wifi_set_ps(WIFI_PS_NONE);  // Desabilita power save WiFi
```

### Comandos MQTT atrasam?
- Normal em light sleep (~100ms latência)
- Para latência zero: modo NORMAL

---

## 📝 Resumo

| Recurso | Benefício |
|---------|-----------|
| ✅ Light Sleep Automático | Economia de ~98% |
| ✅ WiFi Ativo | Recebe comandos instantaneamente |
| ✅ 3 Modos | Normal, Auto, Light Sleep |
| ✅ Wake-up Inteligente | Timer + MQTT |
| ✅ Estatísticas | Monitora economia |
| ✅ Controle Remoto | Liga/desliga por MQTT |

---

## 🚀 Início Rápido

```bash
# 1. Build e Flash
idf.py build flash monitor

# 2. Configurar período de 5 minutos
{"command": "set_read_period", "minutes": 5}

# 3. Habilitar economia (já vem habilitado)
{"command": "power_save_on"}

# 4. Modo automático (padrão)
{"command": "set_power_mode", "mode": "auto"}

# 5. Ver estatísticas após 1 hora
{"command": "power_stats"}
```

**Pronto! Sistema economizando energia automaticamente! 🔋✨**
