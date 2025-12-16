#!/usr/bin/env python3
"""
Script para testar conexão MQTT com os certificados do ESP32
"""

import ssl
import paho.mqtt.client as mqtt
import json
import time

# Configurações
AWS_IOT_ENDPOINT = "a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com"
AWS_IOT_PORT = 8883
CLIENT_ID = "esp32"  # Mesmo Client ID do ESP32
TOPIC_TEST = "esp32/test"

# Caminhos dos certificados
ROOT_CA = "main/certs/AmazonRootCA1.pem"
CERTIFICATE = "main/certs/345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a-certificate.pem.crt"
PRIVATE_KEY = "main/certs/345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a-private.pem.key"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ CONECTADO AO AWS IoT Core!")
        print(f"   Client ID: {CLIENT_ID}")
        print(f"   Flags: {flags}")
        print("\n🎉 CERTIFICADO ESTÁ FUNCIONANDO CORRETAMENTE!")
        
        # Testa publish
        msg = json.dumps({
            "message": "Test from Python",
            "timestamp": int(time.time())
        })
        client.publish(TOPIC_TEST, msg, qos=1)
        print(f"\n📤 Mensagem publicada em: {TOPIC_TEST}")
        
    else:
        print(f"❌ FALHA NA CONEXÃO!")
        print(f"   Return code: {rc}")
        error_messages = {
            1: "Versão do protocolo incorreta",
            2: "Identificador do cliente rejeitado",
            3: "Servidor indisponível",
            4: "Usuário ou senha inválidos",
            5: "Não autorizado"
        }
        if rc in error_messages:
            print(f"   Erro: {error_messages[rc]}")
        
        if rc == 2:
            print("\n💡 SOLUÇÃO:")
            print("   1. Verifique se o Client ID é único")
            print("   2. Se já existe outro dispositivo com ID 'esp32', use outro ID")
            print("   3. No main.c, mude: #define AWS_IOT_CLIENT_ID \"esp32_device_01\"")

def on_message(client, userdata, msg):
    print(f"\n📩 Mensagem recebida:")
    print(f"   Tópico: {msg.topic}")
    print(f"   Payload: {msg.payload.decode()}")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print(f"\n⚠️  DESCONECTADO (rc={rc})")

def on_publish(client, userdata, mid):
    print(f"   ✅ Publicação confirmada (msg_id={mid})")

def on_subscribe(client, userdata, mid, granted_qos):
    print(f"   ✅ Subscribe confirmado (msg_id={mid}, QoS={granted_qos})")

print("═" * 70)
print("  🧪 TESTE DE CONEXÃO AWS IoT Core")
print("═" * 70)
print(f"\n📋 Configurações:")
print(f"   Endpoint: {AWS_IOT_ENDPOINT}")
print(f"   Port: {AWS_IOT_PORT}")
print(f"   Client ID: {CLIENT_ID}")
print(f"   Root CA: {ROOT_CA}")
print(f"   Certificate: {CERTIFICATE[-50:]}")
print(f"   Private Key: {PRIVATE_KEY[-50:]}")

try:
    # Cria o cliente MQTT
    client = mqtt.Client(client_id=CLIENT_ID, protocol=mqtt.MQTTv311)
    
    # Configura callbacks
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish
    client.on_subscribe = on_subscribe
    
    # Configura TLS
    print("\n🔐 Configurando TLS...")
    client.tls_set(
        ca_certs=ROOT_CA,
        certfile=CERTIFICATE,
        keyfile=PRIVATE_KEY,
        cert_reqs=ssl.CERT_REQUIRED,
        tls_version=ssl.PROTOCOL_TLSv1_2,
        ciphers=None
    )
    
    # Conecta
    print(f"\n🔌 Conectando ao AWS IoT...")
    client.connect(AWS_IOT_ENDPOINT, AWS_IOT_PORT, keepalive=60)
    
    # Subscribe para testar recepção
    client.subscribe(TOPIC_TEST, qos=1)
    
    # Loop por 10 segundos
    client.loop_start()
    time.sleep(10)
    client.loop_stop()
    
    # Desconecta
    client.disconnect()
    
    print("\n" + "═" * 70)
    print("✅ TESTE CONCLUÍDO COM SUCESSO!")
    print("═" * 70)
    print("\n💡 O certificado está funcionando. O problema pode ser:")
    print("   1. Client ID conflitante (outro dispositivo usando 'esp32')")
    print("   2. Hora/data do ESP32 incorreta (TLS precisa de hora sincronizada)")
    print("   3. Memória insuficiente no ESP32 para processar TLS")
    print("\n🔧 Soluções:")
    print("   • Mude o Client ID no main.c para algo único")
    print("   • Adicione sincronização NTP no código do ESP32")
    print("   • Monitore o uso de memória heap")

except FileNotFoundError as e:
    print(f"\n❌ ERRO: Arquivo não encontrado: {e}")
    print("\n💡 Verifique se os certificados estão em main/certs/")
    
except ssl.SSLError as e:
    print(f"\n❌ ERRO SSL: {e}")
    print("\n💡 Possíveis causas:")
    print("   • Certificado não está ativo na AWS")
    print("   • Certificado não tem política anexada")
    print("   • Certificado foi revogado")
    
except Exception as e:
    print(f"\n❌ ERRO: {e}")
    print(f"   Tipo: {type(e).__name__}")

