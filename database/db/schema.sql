-- schema.sql
-- criar usuário e banco (opcional se você já tiver)
-- CREATE USER iot_user WITH PASSWORD 'iot_pass';
-- CREATE DATABASE iotdb OWNER iot_user;

\connect iotdb

-- tabela de dispositivos (opcional)
CREATE TABLE IF NOT EXISTS devices (
  id SERIAL PRIMARY KEY,
  device_id TEXT UNIQUE NOT NULL,
  description TEXT,
  created_at TIMESTAMPTZ DEFAULT now()
);

-- sensores (opcional)
CREATE TABLE IF NOT EXISTS sensors (
  id SERIAL PRIMARY KEY,
  device_id TEXT NOT NULL,
  sensor_name TEXT,
  sensor_type TEXT,
  unit TEXT,
  created_at TIMESTAMPTZ DEFAULT now(),
  FOREIGN KEY (device_id) REFERENCES devices(device_id) ON DELETE SET NULL
);

-- leituras (dados de telemetria)
CREATE TABLE IF NOT EXISTS readings (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT,
  sensor TEXT,
  value NUMERIC,
  ts TIMESTAMPTZ DEFAULT now(),
  topic TEXT,
  raw JSONB,
  created_at TIMESTAMPTZ DEFAULT now()
);

-- eventos (ex: solenoide ligado/desligado)
CREATE TABLE IF NOT EXISTS events (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT,
  event_type TEXT,
  payload JSONB,
  ts TIMESTAMPTZ DEFAULT now(),
  topic TEXT,
  created_at TIMESTAMPTZ DEFAULT now()
);

-- índices úteis
CREATE INDEX IF NOT EXISTS idx_readings_device_ts ON readings(device_id, ts);
CREATE INDEX IF NOT EXISTS idx_events_device_ts ON events(device_id, ts);
