# Como Funciona o Período de Leitura Dinâmico

## 🎯 Problema Resolvido

**Antes:** As tasks usavam valores fixos (hardcoded):
```c
vTaskDelay(pdMS_TO_TICKS(10000)); // DHT11 - 10s fixo
vTaskDelay(pdMS_TO_TICKS(15000)); // UV - 15s fixo  
vTaskDelay(pdMS_TO_TICKS(20000)); // Soil - 20s fixo
```

**Agora:** As tasks consultam o período dinâmico:
```c
int delay_ms = system_commands_get_read_period_ms();
vTaskDelay(pdMS_TO_TICKS(delay_ms)); // Valor dinâmico!
```

---

## 🔄 Fluxo de Funcionamento

```
┌─────────────────────────────────────────────────────────┐
│  AWS IoT Console / App / Script Python                  │
│  Publica: {"command":"set_read_period","minutes":5}     │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  ESP32 - MQTT Handler                                   │
│  Recebe comando no tópico: esp32/commands               │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  system_commands_mqtt_handler()                         │
│  Parse do JSON e chama:                                 │
│  system_commands_set_read_period_minutes(5)             │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  system_config.read_period_minutes = 5                  │
│  (Variável global atualizada IMEDIATAMENTE)             │
└────────────────────┬────────────────────────────────────┘
                     │
                     ├──────────────┬──────────────┬───────┐
                     ▼              ▼              ▼       ▼
              ┌──────────┐   ┌──────────┐   ┌──────────┐
              │ DHT11    │   │ UV       │   │ Soil     │
              │ Task     │   │ Task     │   │ Task     │
              └─────┬────┘   └─────┬────┘   └─────┬────┘
                    │              │              │
                    │              │              │
         Próximo ciclo:  Próximo ciclo:  Próximo ciclo:
                    │              │              │
                    ▼              ▼              ▼
       system_commands_get_read_period_ms()
                    │              │              │
                    ▼              ▼              ▼
            Retorna: 5 min = 300.000 ms
                    │              │              │
                    ▼              ▼              ▼
       vTaskDelay(pdMS_TO_TICKS(300000))
                    │              │              │
                    └──────────────┴──────────────┘
                                   │
                    ✅ Todas aguardam 5 minutos!
```

---

## 📊 Exemplo Prático

### 1. Sistema Inicia (Padrão: 5 minutos)
```
DHT11  Task: Próxima leitura em 300000 ms (5 min)
UV     Task: Próxima leitura em 300000 ms (5 min)
Soil   Task: Próxima leitura em 300000 ms (5 min)
```

### 2. Comando Recebido: 1 minuto
```json
{"command": "set_read_period", "minutes": 1}
```

**Log do ESP32:**
```
[SYS_CMD] 🔧 Comando: ALTERAR PERÍODO DE LEITURA
[SYS_CMD] Novo período: 1 minutos
[SYS_CMD] Período de leitura atualizado: 1 minutos
```

### 3. Próximo Ciclo de Cada Sensor
```
DHT11  Task: Próxima leitura em 60000 ms (1 min)  ← Mudou!
UV     Task: Próxima leitura em 60000 ms (1 min)  ← Mudou!
Soil   Task: Próxima leitura em 60000 ms (1 min)  ← Mudou!
```

### 4. Comando Recebido: 30 minutos
```json
{"command": "set_read_period", "minutes": 30}
```

### 5. Próximo Ciclo
```
DHT11  Task: Próxima leitura em 1800000 ms (30 min)  ← Mudou!
UV     Task: Próxima leitura em 1800000 ms (30 min)  ← Mudou!
Soil   Task: Próxima leitura em 1800000 ms (30 min)  ← Mudou!
```

---

## 💡 Detalhes Técnicos

### Arquivo: `system_commands.c`
```c
static system_config_t system_config = {
    .read_period_minutes = 5,  // Padrão: 5 minutos
    .solenoid_enabled = true
};

int system_commands_get_read_period_ms(void)
{
    return system_config.read_period_minutes * 60 * 1000;
}

void system_commands_set_read_period_minutes(int minutes)
{
    if (minutes < 1) minutes = 1;
    if (minutes > 1440) minutes = 1440;
    
    system_config.read_period_minutes = minutes;
    ESP_LOGI(TAG, "Período de leitura atualizado: %d minutos", minutes);
}
```

### Arquivo: `dht11_sensor.c` (Exemplo)
```c
void dht11_sensor_task(void *pvParameters)
{
    while (1) {
        // ... leitura do sensor ...
        
        // Usa período dinâmico
        int delay_ms = system_commands_get_read_period_ms();
        ESP_LOGI(TAG, "Próxima leitura em %d ms (%d min)", 
                 delay_ms, delay_ms/60000);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
```

---

## ⚙️ Características

✅ **Mudança em tempo real** - Não precisa reiniciar ESP32
✅ **Sem race conditions** - Leitura atômica da variável global
✅ **Validação automática** - Limites: 1 min (mínimo) até 1440 min/24h (máximo)
✅ **Feedback visual** - Log mostra próximo período em cada ciclo
✅ **Sincronização** - Todas as tasks usam o mesmo valor

---

## 🧪 Como Testar

### Teste 1: Mudança Rápida
```bash
# 1. Inicie o monitor serial
idf.py monitor

# 2. Em outro terminal/AWS Console, publique:
{"command": "set_read_period", "minutes": 1}

# 3. Observe os logs:
[DHT11] Próxima leitura em 60000 ms (1 min)
[UV_SENSOR] Próxima leitura em 60000 ms (1 min)
[SOIL_MOISTURE] Próxima leitura em 60000 ms (1 min)
```

### Teste 2: Validação de Limites
```bash
# Teste com valor muito baixo
{"command": "set_read_period", "minutes": 0}
# Resultado: Ajustado para 1 minuto

# Teste com valor muito alto
{"command": "set_read_period", "minutes": 9999}
# Resultado: Ajustado para 1440 minutos (24h)
```

### Teste 3: Verificar Status
```bash
{"command": "get_status"}

# Resposta em esp32/status:
{
  "status": "online",
  "read_period_minutes": 1,  ← Valor atual
  "solenoid_state": false,
  ...
}
```

---

## 🎓 Por Que Funciona?

1. **Variável Global Compartilhada**: `system_config.read_period_minutes`
2. **Tasks Independentes**: Cada task chama `get_read_period_ms()` no seu próprio loop
3. **Leitura Atômica**: Em ESP32, leitura de `int` é thread-safe
4. **Sem Mutex Necessário**: Apenas leitura (não escrita concorrente)
5. **Efeito Imediato**: Próximo ciclo de cada task já usa o novo valor

---

## 📝 Resumo

| Item | Antes | Depois |
|------|-------|--------|
| **Período DHT11** | 10s fixo | Configurável (1-1440 min) |
| **Período UV** | 15s fixo | Configurável (1-1440 min) |
| **Período Soil** | 20s fixo | Configurável (1-1440 min) |
| **Mudança** | Requer recompilação | Comando MQTT em tempo real |
| **Reinicialização** | Necessária | Não necessária |
| **Feedback** | Nenhum | Log + Status MQTT |
