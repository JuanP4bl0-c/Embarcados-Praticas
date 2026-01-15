import os
import json
import datetime
import logging
import psycopg2
from psycopg2.extras import Json
import paho.mqtt.client as mqtt

# CONFIG
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "estufa/#"

DB_HOST = "localhost"
DB_NAME = "iotdb"
DB_USER = "iot_user"
DB_PASS = "iot_pass"

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s: %(message)s")

def get_db_conn():
    return psycopg2.connect(
        host=DB_HOST,
        dbname=DB_NAME,
        user=DB_USER,
        password=DB_PASS
    )

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logging.info("MQTT conectado com sucesso")
        client.subscribe(MQTT_TOPIC)
        logging.info(f"Inscrito no tópico: {MQTT_TOPIC}")
    else:
        logging.error("Erro ao conectar no MQTT, rc=%s", rc)

def on_message(client, userdata, msg):
    topic = msg.topic
    payload_raw = msg.payload.decode().strip()

    logging.info(f"Mensagem recebida [{topic}]: {payload_raw}")

    device_id = "estufa-01"  # pode fixar ou extrair se quiser
    ts = datetime.datetime.utcnow()

    # tenta converter para número
    try:
        value = float(payload_raw)
        is_numeric = True
    except ValueError:
        value = None
        is_numeric = False

    try:
        conn = get_db_conn()
        cur = conn.cursor()

        if is_numeric:
            cur.execute("""
                INSERT INTO readings (device_id, sensor, value, ts, topic, raw)
                VALUES (%s, %s, %s, %s, %s, %s)
            """, (
                device_id,
                topic.split("/")[-1],  # ex: temperatura, umidade
                value,
                ts,
                topic,
                Json({"value": value})
            ))
            logging.info("Leitura numérica gravada")

        else:
            cur.execute("""
                INSERT INTO events (device_id, event_type, payload, topic, ts)
                VALUES (%s, %s, %s, %s, %s)
            """, (
                device_id,
                topic.split("/")[-1],  # comando, status
                Json({"value": payload_raw}),
                topic,
                ts
            ))
            logging.info("Evento gravado")

        conn.commit()
        cur.close()
        conn.close()

    except Exception as e:
        logging.error(f"Erro ao gravar no banco: {e}")

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    logging.info("Conectando ao broker MQTT...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()
