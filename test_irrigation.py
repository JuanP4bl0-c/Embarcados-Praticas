#!/usr/bin/env python3
"""
Script de teste para sistema de irrigação inteligente
Permite testar facilmente atualizações de configuração via MQTT

Uso:
    python3 test_irrigation.py --config tomate
    python3 test_irrigation.py --threshold 30
    python3 test_irrigation.py --disable-auto
    python3 test_irrigation.py --custom config.json
"""

import json
import sys
import argparse

# Configurações predefinidas por tipo de planta
PLANT_CONFIGS = {
    "tomate": {
        "temperature_min": 18,
        "temperature_max": 28,
        "humidity_min": 60,
        "humidity_max": 80,
        "soil_moisture_min": 60,
        "soil_moisture_max": 80,
        "uv_min": 30,
        "uv_max": 70,
        "irrigation_threshold": 25,
        "auto_irrigation": True
    },
    "alface": {
        "temperature_min": 15,
        "temperature_max": 22,
        "humidity_min": 70,
        "humidity_max": 85,
        "soil_moisture_min": 70,
        "soil_moisture_max": 85,
        "uv_min": 20,
        "uv_max": 50,
        "irrigation_threshold": 20,
        "auto_irrigation": True
    },
    "pimentao": {
        "temperature_min": 20,
        "temperature_max": 30,
        "humidity_min": 60,
        "humidity_max": 75,
        "soil_moisture_min": 65,
        "soil_moisture_max": 80,
        "uv_min": 35,
        "uv_max": 75,
        "irrigation_threshold": 25,
        "auto_irrigation": True
    },
    "manjericao": {
        "temperature_min": 18,
        "temperature_max": 25,
        "humidity_min": 50,
        "humidity_max": 70,
        "soil_moisture_min": 60,
        "soil_moisture_max": 75,
        "uv_min": 40,
        "uv_max": 80,
        "irrigation_threshold": 20,
        "auto_irrigation": True
    }
}

def print_config(config, title="Configuração"):
    """Imprime configuração de forma formatada"""
    print(f"\n{'='*60}")
    print(f"🌱 {title}")
    print('='*60)
    print(f"🌡️  Temperatura:    {config.get('temperature_min', '?')}°C - {config.get('temperature_max', '?')}°C")
    print(f"💨 Umidade Ar:     {config.get('humidity_min', '?')}% - {config.get('humidity_max', '?')}%")
    print(f"💧 Umidade Solo:   {config.get('soil_moisture_min', '?')}% - {config.get('soil_moisture_max', '?')}%")
    print(f"☀️  Exposição UV:   {config.get('uv_min', '?')}% - {config.get('uv_max', '?')}%")
    print(f"🚰 Limiar Irrigação: -{config.get('irrigation_threshold', '?')}%")
    print(f"⚙️  Auto-Irrigação:  {'ATIVADA' if config.get('auto_irrigation', False) else 'DESATIVADA'}")
    print('='*60)

def generate_mqtt_message(config):
    """Gera mensagem JSON para publicar no MQTT"""
    return json.dumps(config, indent=2)

def main():
    parser = argparse.ArgumentParser(
        description='Gerador de configurações para sistema de irrigação inteligente',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Exemplos de uso:
  
  # Configurar para cultivo de tomate
  %(prog)s --config tomate
  
  # Configurar para alface
  %(prog)s --config alface
  
  # Apenas mudar o limiar de irrigação
  %(prog)s --threshold 30
  
  # Desativar irrigação automática
  %(prog)s --disable-auto
  
  # Ativar irrigação automática
  %(prog)s --enable-auto
  
  # Configuração customizada
  %(prog)s --custom minha_config.json
  
  # Listar plantas disponíveis
  %(prog)s --list
        '''
    )
    
    parser.add_argument('--config', '-c', 
                       choices=list(PLANT_CONFIGS.keys()),
                       help='Tipo de planta predefinido')
    
    parser.add_argument('--threshold', '-t', 
                       type=int,
                       help='Limiar de irrigação (porcentagem abaixo do ideal)')
    
    parser.add_argument('--enable-auto', 
                       action='store_true',
                       help='Ativar irrigação automática')
    
    parser.add_argument('--disable-auto', 
                       action='store_true',
                       help='Desativar irrigação automática')
    
    parser.add_argument('--custom', 
                       type=str,
                       help='Arquivo JSON com configuração customizada')
    
    parser.add_argument('--list', '-l',
                       action='store_true',
                       help='Listar plantas disponíveis')
    
    parser.add_argument('--output', '-o',
                       type=str,
                       help='Salvar JSON em arquivo ao invés de imprimir')
    
    args = parser.parse_args()
    
    # Lista plantas disponíveis
    if args.list:
        print("\n🌱 Plantas Disponíveis:\n")
        for plant_name, config in PLANT_CONFIGS.items():
            print_config(config, f"{plant_name.capitalize()}")
        return
    
    # Se nenhum argumento, mostra ajuda
    if len(sys.argv) == 1:
        parser.print_help()
        return
    
    # Constrói configuração baseada nos argumentos
    config = {}
    
    if args.config:
        config = PLANT_CONFIGS[args.config].copy()
        print(f"\n✅ Usando configuração predefinida: {args.config.upper()}")
    
    if args.custom:
        try:
            with open(args.custom, 'r') as f:
                custom_config = json.load(f)
                config.update(custom_config)
                print(f"\n✅ Configuração customizada carregada de: {args.custom}")
        except Exception as e:
            print(f"\n❌ Erro ao ler arquivo {args.custom}: {e}")
            return
    
    if args.threshold is not None:
        config['irrigation_threshold'] = args.threshold
        print(f"\n✅ Limiar de irrigação definido: {args.threshold}%")
    
    if args.enable_auto:
        config['auto_irrigation'] = True
        print("\n✅ Irrigação automática ATIVADA")
    
    if args.disable_auto:
        config['auto_irrigation'] = False
        print("\n⚠️  Irrigação automática DESATIVADA")
    
    if not config:
        print("\n⚠️  Nenhuma configuração especificada. Use --help para ver opções.")
        return
    
    # Mostra configuração
    print_config(config, "Configuração Gerada")
    
    # Gera JSON
    json_message = generate_mqtt_message(config)
    
    print("\n📋 JSON para publicar no MQTT:")
    print("-" * 60)
    print(json_message)
    print("-" * 60)
    
    print("\n📡 Como usar no AWS IoT Core:")
    print("   1. Acesse: AWS IoT Console → Test → MQTT test client")
    print("   2. Tópico: esp32/config")
    print("   3. Cole o JSON acima")
    print("   4. Clique em 'Publish'")
    
    # Salva em arquivo se solicitado
    if args.output:
        try:
            with open(args.output, 'w') as f:
                f.write(json_message)
            print(f"\n💾 Configuração salva em: {args.output}")
        except Exception as e:
            print(f"\n❌ Erro ao salvar arquivo: {e}")
    
    print("\n✨ Para copiar facilmente, use:")
    print(f"   echo '{json_message.replace(chr(10), '')}' | xclip -selection clipboard")
    print()

if __name__ == "__main__":
    main()
