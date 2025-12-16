#!/usr/bin/env python3
"""
Script para verificar o status do certificado na AWS IoT Core
"""

import boto3
import sys

def check_certificate(cert_id):
    """Verifica o status de um certificado na AWS IoT"""
    
    try:
        client = boto3.client('iot', region_name='us-east-1')
        
        print(f"🔍 Verificando certificado: {cert_id}")
        print("=" * 70)
        
        # Descreve o certificado
        cert_arn = f"arn:aws:iot:us-east-1:*:cert/{cert_id}"
        response = client.describe_certificate(certificateId=cert_id)
        
        cert_desc = response['certificateDescription']
        status = cert_desc['status']
        creation_date = cert_desc['creationDate']
        
        print(f"\n📋 STATUS DO CERTIFICADO:")
        print(f"   ID: {cert_id}")
        print(f"   Status: {status}")
        print(f"   Criado em: {creation_date}")
        
        if status == 'ACTIVE':
            print("   ✅ Certificado está ATIVO")
        else:
            print(f"   ❌ Certificado está {status} (precisa estar ACTIVE)")
            print("\n💡 Para ativar:")
            print(f"   aws iot update-certificate --certificate-id {cert_id} --new-status ACTIVE")
        
        # Lista políticas anexadas
        print(f"\n📜 POLÍTICAS ANEXADAS:")
        policies = client.list_principal_policies(principal=cert_desc['certificateArn'])
        
        if policies['policies']:
            for policy in policies['policies']:
                print(f"   ✅ {policy['policyName']}")
        else:
            print("   ❌ NENHUMA POLÍTICA ANEXADA!")
            print("\n💡 Para anexar uma política:")
            print(f"   aws iot attach-policy --policy-name <NOME_DA_POLITICA> --target {cert_desc['certificateArn']}")
        
        # Lista Things anexadas
        print(f"\n🔧 THINGS ANEXADAS:")
        things = client.list_principal_things(principal=cert_desc['certificateArn'])
        
        if things['things']:
            for thing in things['things']:
                print(f"   ✅ {thing}")
        else:
            print("   ℹ️  Nenhuma Thing anexada (opcional)")
        
        print("\n" + "=" * 70)
        
        # Verifica se está tudo OK
        if status == 'ACTIVE' and policies['policies']:
            print("✅ CERTIFICADO CONFIGURADO CORRETAMENTE!")
            return True
        else:
            print("❌ CERTIFICADO PRECISA DE CONFIGURAÇÃO")
            return False
            
    except client.exceptions.ResourceNotFoundException:
        print(f"❌ Certificado {cert_id} não encontrado na AWS IoT!")
        return False
    except Exception as e:
        print(f"❌ Erro ao verificar certificado: {e}")
        return False

if __name__ == "__main__":
    cert_id = "345efea71d0f781360dce6208ca2cbef0d37f5c500aaed78931ae9982a7eb75a"
    
    if len(sys.argv) > 1:
        cert_id = sys.argv[1]
    
    success = check_certificate(cert_id)
    sys.exit(0 if success else 1)
