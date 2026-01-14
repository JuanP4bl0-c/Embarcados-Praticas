# Sistema de Comandos MQTT - ESP32 Estufa Inteligente

## Tópico de Comandos
```
esp32/commands
```

## Comandos Disponíveis

### 1. Ligar Solenoide
Liga manualmente a válvula solenoide para irrigação.

**Comando:**
```json
{
  "command": "solenoid_on"
}
```

**Resposta:** Publica status atualizado em `esp32/status`

---

### 2. Desligar Solenoide
Desliga manualmente a válvula solenoide.

**Comando:**
```json
{
  "command": "solenoid_off"
}
```

**Resposta:** Publica status atualizado em `esp32/status`

---

### 3. Publicar Todos os Dados
Força a publicação imediata de todos os sensores (DHT11, UV, Umidade do Solo, Config da Planta).

**Comando:**
```json
{
  "command": "publish_all"
}
```

**Resposta:** 
- `esp32/sensor/dht11` - Temperatura e umidade
- `esp32/sensor/uv` - Sensor UV
- `esp32/soil_moisture` - Umidade do solo
- `esp32/plant/config` - Configuração da planta

---

### 4. Alterar Período de Leitura
Define o intervalo em minutos entre leituras dos sensores (DHT11, UV, Umidade do Solo).

**Comando:**
```json
{
  "command": "set_read_period",
  "minutes": 10
}
```

**Parâmetros:**
- `minutes`: Período em minutos (mínimo: 1, máximo: 1440)

**Efeito:**
- ✅ Altera **imediatamente** o intervalo de todas as tasks de sensores
- ✅ DHT11, UV e Umidade do Solo passam a usar o novo período
- ✅ Não requer reinicialização

**Resposta:** Publica status atualizado em `esp32/status`

**Exemplo - Leitura rápida (1 minuto):**
```json
{
  "command": "set_read_period",
  "minutes": 1
}
```

**Exemplo - Leitura lenta (30 minutos):**
```json
{
  "command": "set_read_period",
  "minutes": 30
}
```

---

### 5. Solicitar Status do Sistema
Retorna o status atual do sistema com todas as configurações.

**Comando:**
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
  "uptime_seconds": 12345,
  "timestamp": 1736899200000,
  "datetime": "2025-01-14 15:30:00"
}
```

---

### 6. Reiniciar ESP32
Reinicia o microcontrolador ESP32 (útil para aplicar mudanças ou resolver problemas).

**Comando:**
```json
{
  "command": "restart"
}
```

**Resposta:** ESP32 publica mensagem de confirmação e reinicia em 3 segundos.

---

## Como Testar no AWS IoT Console

### 1. Acessar AWS IoT Test Client
1. Acesse AWS IoT Core Console
2. Região: **us-east-1** (Norte da Virgínia)
3. Menu lateral: **Test** > **MQTT test client**

### 2. Publicar Comandos

**Passo 1:** Na aba "Publish to a topic"
- **Topic name:** `esp32/commands`
- **Message payload:** Cole um dos comandos JSON acima

**Passo 2:** Clique em **Publish**

### 3. Monitorar Respostas

**Passo 1:** Na aba "Subscribe to a topic"
- **Topic filter:** `esp32/#` (wildcard para todos os tópicos)

**Passo 2:** Clique em **Subscribe**

Você verá todas as publicações do ESP32 em tempo real.

---

## Exemplos de Uso

### Exemplo 1: Teste Completo do Sistema
```json
{"command": "get_status"}
```

### Exemplo 2: Irrigação Manual
```json
{"command": "solenoid_on"}
```
*(Aguardar 30 segundos)*
```json
{"command": "solenoid_off"}
```

### Exemplo 3: Verificar Todos os Sensores
```json
{"command": "publish_all"}
```

### Exemplo 4: Mudar para Leitura Rápida (1 minuto)
```json
{
  "command": "set_read_period",
  "minutes": 1
}
```

### Exemplo 5: Mudar para Leitura Lenta (30 minutos)
```json
{
  "command": "set_read_period",
  "minutes": 30
}
```

---

## Tópicos de Publicação

| Tópico | Descrição | Frequência |
|--------|-----------|------------|
| `esp32/status` | Status do sistema | Sob demanda |
| `esp32/sensor/dht11` | Temperatura e umidade | Configurável |
| `esp32/sensor/uv` | Sensor UV | 15 segundos (dia) |
| `esp32/soil_moisture` | Umidade do solo | 20 segundos |
| `esp32/plant/config` | Configuração da planta | Sob demanda |
| `esp32/alerts` | Alertas (irrigação automática) | Eventos |

---

## Tópicos de Subscrição

| Tópico | Descrição |
|--------|-----------|
| `esp32/commands` | Comandos do sistema |
| `esp32/actuator/solenoid` | Controle do solenoide (legado) |
| `esp32/plant/config/set` | Configuração da planta |

---

## Notas Importantes

1. **Período de Leitura:** 
   - ✅ **Mudança em tempo real**: O comando `set_read_period` altera **imediatamente** o comportamento das tasks
   - ✅ Aplica-se a: DHT11, UV e Umidade do Solo
   - ✅ Não requer reinicialização do ESP32
   - ⚠️ A mudança só afeta o próximo ciclo de cada sensor

2. **Comando de Restart:** Use com cuidado - o ESP32 reiniciará imediatamente após 3 segundos.

3. **Estado do Solenoide:** O comando `solenoid_on/off` controla manualmente. A irrigação automática baseada em umidade do solo continua ativa.

4. **Timestamps:** Todos os dados incluem timestamp Unix (ms) e datetime formatado.

5. **LED Indicators:** GPIO2 e GPIO4 piscam a cada log - use como indicador visual de atividade.

---

## Troubleshooting

### Comando não funciona?
1. Verifique se o ESP32 está conectado ao MQTT
2. Monitore os logs serial com `idf.py monitor`
3. Verifique se o tópico está correto: `esp32/commands`
4. Confirme que o JSON está válido

### Status não atualiza?
1. Publique `{"command": "get_status"}` manualmente
2. Verifique conexão WiFi do ESP32
3. Confirme que está subscrito em `esp32/status`

### Período de leitura não muda?
- ✅ **Agora muda em tempo real!** Não precisa reiniciar
- Monitore os logs: verá "Próxima leitura em X ms"
- Cada sensor aplicará o novo período no próximo ciclo
- Use `{"command": "get_status"}` para confirmar
