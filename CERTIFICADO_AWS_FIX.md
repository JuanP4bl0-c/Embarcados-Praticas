# 🔐 GUIA: Ativar Certificado e Configurar AWS IoT Core

## ❌ Problema Atual

O ESP32 está falhando na autenticação TLS com erro:
```
E (8212) MQTT_MANAGER: MQTT_EVENT_ERROR
E (8219) MQTT_MANAGER:    ESP-ERR: 0x8008 (32776)
```

**Causa:** O certificado `345efea7...` não está ativo ou não tem política anexada.

---

## ✅ Solução: Configurar no AWS IoT Console

### 1️⃣ Ativar o Certificado

1. Acesse: https://console.aws.amazon.com/iot/
2. Vá em: **Security** → **Certificates**
3. Procure pelo certificado: `345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a`
4. Clique no certificado
5. No menu **Actions**, clique em **Activate**
6. Confirme a ativação

### 2️⃣ Criar uma Política (se não existir)

**Via Console:**
1. Vá em: **Security** → **Policies**
2. Clique em **Create policy**
3. Nome da política: `ESP32_IoT_Policy`
4. **Policy document** (copie e cole):

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Connect"
      ],
      "Resource": "arn:aws:iot:us-east-1:*:client/ESP32_Client"
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Receive"
      ],
      "Resource": [
        "arn:aws:iot:us-east-1:*:topic/esp32/*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Subscribe"
      ],
      "Resource": [
        "arn:aws:iot:us-east-1:*:topicfilter/esp32/*"
      ]
    }
  ]
}
```

5. Clique em **Create**

**Via AWS CLI:**
```bash
# Salve a política em um arquivo
cat > esp32_policy.json << 'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:*:client/ESP32_Client"
    },
    {
      "Effect": "Allow",
      "Action": ["iot:Publish", "iot:Receive"],
      "Resource": "arn:aws:iot:us-east-1:*:topic/esp32/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:us-east-1:*:topicfilter/esp32/*"
    }
  ]
}
EOF

# Crie a política
aws iot create-policy \
  --policy-name ESP32_IoT_Policy \
  --policy-document file://esp32_policy.json \
  --region us-east-1
```

### 3️⃣ Anexar a Política ao Certificado

**Via Console:**
1. Vá em: **Security** → **Certificates**
2. Clique no certificado `345efea7...`
3. Na aba **Policies**, clique em **Attach policies**
4. Selecione `ESP32_IoT_Policy`
5. Clique em **Attach**

**Via AWS CLI:**
```bash
# Substitua <ACCOUNT_ID> pelo seu ID da conta AWS
aws iot attach-policy \
  --policy-name ESP32_IoT_Policy \
  --target "arn:aws:iot:us-east-1:<ACCOUNT_ID>:cert/345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a" \
  --region us-east-1
```

### 4️⃣ Verificar Configuração

**Via AWS CLI:**
```bash
# Verificar status do certificado
aws iot describe-certificate \
  --certificate-id 345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a \
  --region us-east-1

# Listar políticas anexadas
aws iot list-principal-policies \
  --principal "arn:aws:iot:us-east-1:<ACCOUNT_ID>:cert/345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a" \
  --region us-east-1
```

---

## 🧪 Testar a Conexão

Após configurar, faça o flash novamente:

```bash
cd "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto"
idf.py flash monitor
```

**Você deve ver:**
```
I (xxxx) MQTT_MANAGER: ✅ MQTT_EVENT_CONNECTED - Conectado ao AWS IoT!
```

---

## 📊 Monitorar no AWS IoT Test

1. Vá em: **Test** → **MQTT test client**
2. **Subscribe to a topic:** `esp32/#`
3. Clique em **Subscribe**
4. Você deve ver as mensagens dos sensores:
   - `esp32/uv`
   - `esp32/soil_moisture`
   - `esp32/dht11`
   - `esp32/config`

---

## 🔍 Diagnóstico de Problemas

### Certificado não aparece no console:
- O certificado pode não ter sido criado corretamente
- Verifique se você baixou os arquivos da AWS IoT Core
- Recrie o certificado se necessário

### Ainda falha após ativar:
1. **Verifique o endpoint:**
   ```bash
   aws iot describe-endpoint --endpoint-type iot:Data-ATS --region us-east-1
   ```
   Deve retornar: `a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com`

2. **Verifique os certificados embeddados:**
   - Confirme que os arquivos em `main/certs/` estão corretos
   - Recompile com `idf.py fullclean && idf.py build`

3. **Teste com mosquitto_pub:**
   ```bash
   mosquitto_pub \
     --cafile main/certs/AmazonRootCA1.pem \
     --cert main/certs/345efea7...-certificate.pem.crt \
     --key main/certs/345efea7...-private.pem.key \
     -h a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com \
     -p 8883 \
     -q 1 \
     -t esp32/test \
     -i ESP32_Client \
     -m "teste" \
     -d
   ```

---

## 📝 Checklist de Configuração

- [ ] Certificado está **ACTIVE** no AWS IoT Console
- [ ] Política `ESP32_IoT_Policy` foi criada
- [ ] Política foi **anexada** ao certificado
- [ ] Endpoint correto: `a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com`
- [ ] Certificados corretos em `main/certs/`
- [ ] WiFi conectado: `brisa-2504280`
- [ ] Recompilado e flasheado após mudanças

---

## 🆘 Comandos Úteis

**Ativar certificado:**
```bash
aws iot update-certificate \
  --certificate-id 345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a \
  --new-status ACTIVE \
  --region us-east-1
```

**Desativar certificado antigo:**
```bash
aws iot update-certificate \
  --certificate-id 86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c \
  --new-status INACTIVE \
  --region us-east-1
```

**Listar todos os certificados:**
```bash
aws iot list-certificates --region us-east-1
```
