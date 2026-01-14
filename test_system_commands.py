#!/usr/bin/env python3
"""
Script de teste para comandos MQTT do ESP32 Estufa Inteligente
Publica comandos no tópico esp32/commands e monitora respostas
"""

import json
import time
import sys
from awscrt import mqtt
from awsiot import mqtt_connection_builder

# ==================== CONFIGURAÇÃO ====================
# Substitua com seus arquivos de certificados
ENDPOINT = "a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com"
CLIENT_ID = "ESP32_Test_Client"
CERT_FILEPATH = "certs/376f19f7d489fd831039a918bc7a9ec29a363566a92e0c10b4fc5b0f69aa345f-certificate.pem.crt"
KEY_FILEPATH = "certs/376f19f7d489fd831039a918bc7a9ec29a363566a92e0c10b4fc5b0f69aa345f-private.pem.key"
CA_FILEPATH = "certs/AmazonRootCA1.pem"

# Tópicos
TOPIC_COMMANDS = "esp32/commands"
TOPIC_WILDCARD = "esp32/#"

# ==================== COMANDOS ====================
COMMANDS = {
    "1": {
        "name": "Ligar Solenoide",
        "payload": {"command": "solenoid_on"}
    },
    "2": {
        "name": "Desligar Solenoide",
        "payload": {"command": "solenoid_off"}
    },
    "3": {
        "name": "Publicar Todos os Dados",
        "payload": {"command": "publish_all"}
    },
    "4": {
        "name": "Alterar Período de Leitura (1 min)",
        "payload": {"command": "set_read_period", "minutes": 1}
    },
    "5": {
        "name": "Alterar Período de Leitura (5 min)",
        "payload": {"command": "set_read_period", "minutes": 5}
    },
    "6": {
        "name": "Alterar Período de Leitura (10 min)",
        "payload": {"command": "set_read_period", "minutes": 10}
    },
    "7": {
        "name": "Solicitar Status do Sistema",
        "payload": {"command": "get_status"}
    },
    "8": {
        "name": "Reiniciar ESP32 (CUIDADO!)",
        "payload": {"command": "restart"}
    },
}

# ==================== CALLBACKS ====================
def on_connection_interrupted(connection, error, **kwargs):
    print(f"❌ Conexão interrompida: {error}")

def on_connection_resumed(connection, return_code, session_present, **kwargs):
    print(f"✅ Conexão restaurada: {return_code}")

def on_message_received(topic, payload, dup, qos, retain, **kwargs):
    """Callback chamado quando uma mensagem é recebida"""
    try:
        message = json.loads(payload.decode())
        print(f"\n📨 Mensagem recebida em '{topic}':")
        print(json.dumps(message, indent=2, ensure_ascii=False))
    except:
        print(f"\n📨 Mensagem em '{topic}': {payload.decode()}")

# ==================== CONEXÃO ====================
def connect_to_aws_iot():
    """Estabelece conexão com AWS IoT Core"""
    print("🔌 Conectando ao AWS IoT Core...")
    print(f"   Endpoint: {ENDPOINT}")
    print(f"   Client ID: {CLIENT_ID}")
    
    mqtt_connection = mqtt_connection_builder.mtls_from_path(
        endpoint=ENDPOINT,
        cert_filepath=CERT_FILEPATH,
        pri_key_filepath=KEY_FILEPATH,
        ca_filepath=CA_FILEPATH,
        client_id=CLIENT_ID,
        clean_session=False,
        keep_alive_secs=30,
        on_connection_interrupted=on_connection_interrupted,
        on_connection_resumed=on_connection_resumed
    )
    
    connect_future = mqtt_connection.connect()
    connect_future.result()
    print("✅ Conectado com sucesso!")
    
    # Subscribe ao wildcard para receber todas as mensagens
    print(f"📡 Subscribing em '{TOPIC_WILDCARD}'...")
    subscribe_future, packet_id = mqtt_connection.subscribe(
        topic=TOPIC_WILDCARD,
        qos=mqtt.QoS.AT_LEAST_ONCE,
        callback=on_message_received
    )
    subscribe_result = subscribe_future.result()
    print(f"✅ Subscribed: {subscribe_result}")
    
    return mqtt_connection

# ==================== MENU ====================
def show_menu():
    """Mostra o menu de comandos disponíveis"""
    print("\n" + "="*60)
    print("  COMANDOS DISPONÍVEIS")
    print("="*60)
    for key, cmd in COMMANDS.items():
        print(f"  {key}. {cmd['name']}")
    print("  0. Sair")
    print("="*60)

def send_command(connection, command_payload):
    """Envia um comando para o ESP32"""
    payload_str = json.dumps(command_payload)
    print(f"\n📤 Enviando comando para '{TOPIC_COMMANDS}':")
    print(f"   Payload: {payload_str}")
    
    publish_future, packet_id = connection.publish(
        topic=TOPIC_COMMANDS,
        payload=payload_str,
        qos=mqtt.QoS.AT_LEAST_ONCE
    )
    publish_future.result()
    print(f"✅ Comando enviado (msg_id={packet_id})")

# ==================== MAIN ====================
def main():
    """Função principal"""
    print("="*60)
    print("  ESP32 Estufa Inteligente - Cliente de Teste MQTT")
    print("="*60)
    
    try:
        # Conecta ao AWS IoT
        mqtt_connection = connect_to_aws_iot()
        
        # Loop de comandos
        while True:
            show_menu()
            choice = input("\nEscolha um comando (0-8): ").strip()
            
            if choice == "0":
                print("\n👋 Encerrando...")
                break
            
            if choice in COMMANDS:
                cmd = COMMANDS[choice]
                print(f"\n🎯 Executando: {cmd['name']}")
                send_command(mqtt_connection, cmd["payload"])
                
                # Aguarda um pouco para receber resposta
                print("⏳ Aguardando resposta (3s)...")
                time.sleep(3)
            else:
                print("❌ Opção inválida!")
        
        # Desconecta
        print("\n🔌 Desconectando...")
        disconnect_future = mqtt_connection.disconnect()
        disconnect_future.result()
        print("✅ Desconectado!")
        
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrompido pelo usuário")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Erro: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
