# 🌡️ Troubleshooting DHT11 - Sensor de Temperatura e Umidade

## ❌ Erro Comum: "Timeout esperando resposta inicial"

```
W (114203) DHT11_SENSOR: Timeout esperando resposta inicial (1)
W (114204) DHT11_SENSOR: Falha ao ler DHT11
```

Este erro significa que o ESP32 **não está recebendo resposta** do DHT11.

---

## 🔧 Causas e Soluções

### 1️⃣ **Problema de Conexão** ⭐ MAIS COMUM

**Verifique:**
- ✅ DHT11 conectado ao **GPIO 27**
- ✅ Alimentação: **3.3V** (VCC) e **GND**
- ✅ Fios bem conectados (sem mau contato)

**Pinout do DHT11:**
```
DHT11 (visto de frente)
┌─────────┐
│  ┌───┐  │
│  │ ▓ │  │ <- Grade frontal
│  └───┘  │
│ 1 2 3 4 │
└─────────┘
  │ │ │ │
  │ │ │ └── 4: NC (não conectar)
  │ │ └──── 3: GND
  │ └────── 2: DATA -> GPIO 27
  └──────── 1: VCC -> 3.3V
```

**ESP32 Freenove WROVER:**
```
GPIO 27 (DATA) ────────┬──── DHT11 pin 2
                       │
3.3V ──────────────────┼──── DHT11 pin 1
                       │
GND ───────────────────┘──── DHT11 pin 3
```

---

### 2️⃣ **Falta de Resistor Pull-Up**

O DHT11 usa comunicação **1-Wire** e precisa de um resistor pull-up no pino de dados.

**Solução:**
- Adicione um resistor de **10kΩ** entre:
  - **VCC (3.3V)** e **DATA (GPIO 27)**

```
        10kΩ
3.3V ───/\/\/\───┬─── GPIO 27
                 │
                DHT11
                DATA
```

**Se não tiver resistor:**
- Alguns DHT11 em módulos já vêm com o resistor soldado
- Verifique se o seu módulo tem o resistor (geralmente um componente azul/preto perto do sensor)

---

### 3️⃣ **Timing Incorreto**

O DHT11 é **muito sensível ao timing**:
- ⏱️ Requer **pelo menos 2 segundos** entre leituras
- 🚫 Se ler muito rápido, o sensor não responde

**Código corrigido:**
```c
// Aguarda 250ms + vTaskDelay antes de cada leitura
vTaskDelay(pdMS_TO_TICKS(5000)); // 5 segundos entre leituras
```

---

### 4️⃣ **Sensor Defeituoso ou Falsificado**

Muitos DHT11 vendidos são **falsificações** com qualidade inferior.

**Sintomas:**
- ❌ Nunca responde
- ❌ Checksum sempre inválido
- ❌ Valores erráticos

**Teste:**
1. Conecte em outro microcontrolador (Arduino, por exemplo)
2. Use um código de teste simples
3. Se não funcionar em nenhum lugar, o sensor está defeituoso

---

### 5️⃣ **Interferência Elétrica**

Se outros sensores estão no mesmo barramento ou próximos:

**Solução:**
- Use fios mais curtos (< 20cm idealmente)
- Afaste o DHT11 de fontes de ruído (motores, relés, solenoides)
- Adicione um capacitor de **100nF** entre VCC e GND do DHT11

---

## ✅ Melhorias Implementadas no Código

### 1. **Delays Corretos**
```c
vTaskDelay(pdMS_TO_TICKS(250));  // 250ms entre leituras
vTaskDelay(pdMS_TO_TICKS(20));   // 20ms para sinal de início
```

### 2. **Timeouts Maiores**
```c
if (++timeout > 200) {  // 200us ao invés de 100us
    ESP_LOGW(TAG, "⚠️ DHT11 não respondeu");
    return ESP_FAIL;
}
```

### 3. **Retry com Backoff**
```c
if (retry_count >= 3) {
    ESP_LOGE(TAG, "⚠️ 3 falhas consecutivas! Aguardando 5 segundos...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    retry_count = 0;
}
```

### 4. **Reset do Pino**
```c
// Se falhar, coloca o pino em estado conhecido
gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
gpio_set_level(DHT11_GPIO, 1);
```

---

## 🧪 Teste de Diagnóstico

**Passo 1: Verificar Conexão**
```bash
# No monitor serial, procure por:
I (xxx) DHT11_SENSOR: Inicializando sensor DHT11 no GPIO 27
```

**Passo 2: Observar Erros**
```bash
# Se aparecer:
W DHT11_SENSOR: ⚠️ DHT11 não respondeu (pino ficou HIGH)
# → Sensor não está conectado ou defeituoso

# Se aparecer:
W DHT11_SENSOR: Checksum inválido
# → Sensor responde mas há ruído/interferência
```

**Passo 3: Medir com Multímetro**
- VCC do DHT11: deve ter **3.3V**
- DATA em repouso: deve estar entre **2.5V e 3.3V** (por causa do pull-up)
- Se DATA está em **0V**, há curto-circuito

---

## 📊 Especificações do DHT11

| Parâmetro | Valor |
|-----------|-------|
| Alimentação | 3.3V - 5.5V |
| Corrente | 0.5mA - 2.5mA |
| Faixa Umidade | 20% - 90% RH |
| Faixa Temperatura | 0°C - 50°C |
| Precisão Umidade | ±5% RH |
| Precisão Temperatura | ±2°C |
| **Taxa de amostragem** | **1 amostra a cada 2 segundos** ⚠️ |
| Tempo de resposta | < 5 segundos |

---

## 🔄 Alternativas ao DHT11

Se o DHT11 continua com problemas, considere:

### **DHT22 (AM2302)** - Recomendado
- ✅ Mais preciso (±2% RH, ±0.5°C)
- ✅ Faixa maior (-40°C a 80°C)
- ✅ Mais confiável
- ❌ Mais caro (~R$30)
- ✅ **Compatível com o código** (mesma biblioteca)

### **BME280** - Melhor opção
- ✅ Temperatura + Umidade + Pressão
- ✅ Interface I2C/SPI (mais confiável)
- ✅ Muito preciso (±3% RH, ±1°C)
- ❌ Requer mudança no código

---

## 📝 Checklist de Troubleshooting

- [ ] DHT11 conectado ao GPIO 27
- [ ] VCC do DHT11 em 3.3V
- [ ] GND do DHT11 conectado
- [ ] Resistor pull-up de 10kΩ instalado (ou módulo com resistor)
- [ ] Fios curtos (< 20cm) e bem conectados
- [ ] Código atualizado com delays corretos
- [ ] Aguardando pelo menos 2 segundos entre leituras
- [ ] Sem interferência de outros dispositivos
- [ ] Sensor testado e funcionando

---

## 💡 Dica Final

**Se mesmo após todas as correções o sensor não funcionar:**

1. **Teste com código mínimo:**
```c
void app_main() {
    dht11_sensor_init();
    while(1) {
        int16_t temp, hum;
        if (dht11_sensor_read(&hum, &temp) == ESP_OK) {
            printf("OK: T=%d°C H=%d%%\n", temp, hum);
        } else {
            printf("FALHA\n");
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
```

2. **Troque de sensor** - DHT11 falso é muito comum!

3. **Use um DHT22** - Mais caro mas muito mais confiável
