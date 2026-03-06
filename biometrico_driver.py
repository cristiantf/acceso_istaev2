"""
Módulo de utilidades para interactuar con la base de datos a través de los modelos
SQLAlchemy de la aplicación Flask principal.
"""

from datetime import datetime
import pytz
import urllib.request
import urllib.error
import json
# Importamos app para el contexto, pero db y modelos dentro de las funciones 
# para evitar importación circular si app.py también importa este driver.
from app import app

def guardar_log_remoto(uid, evento, origen="Huella", fecha_dispositivo=None):
    """
    Guarda un log permitiendo definir una fecha específica (útil para LittleFS).
    """
    from app import db, Log # Importación local para evitar ciclos
    
    with app.app_context():
        try:
            tz_ecuador = pytz.timezone('America/Guayaquil')
            
            # Si no viene fecha del dispositivo, usamos la actual del servidor
            if not fecha_dispositivo:
                fecha_final = datetime.now(tz_ecuador).strftime("%Y-%m-%d %H:%M:%S")
            else:
                # Normalizamos el formato ISO de Hikvision (T) a espacio para fidelidad en BD
                fecha_final = fecha_dispositivo.replace('T', ' ')

            nuevo_log = Log(
                fecha=fecha_final,
                usuario_id=uid,
                tipo_evento=evento,
                origen=origen
            )
            db.session.add(nuevo_log)
            db.session.commit()
            print(f"INFO: Log guardado para usuario {uid} - Fecha: {fecha_final}")
            return True
        except Exception as e:
            db.session.rollback()
            print(f"ERROR: No se pudo guardar el log remoto. {e}")
            return False

def obtener_lista_blanca():
    """Retorna IDs autorizados (acceso_puerta=1) para sincronización del NodeMCU."""
    from app import User
    with app.app_context():
        try:
            usuarios_autorizados = User.query.filter_by(acceso_puerta=1).all()
            lista_ids = ",".join([str(u.biometric_id) for u in usuarios_autorizados])
            return lista_ids
        except Exception as e:
            print(f"ERROR: No se pudo obtener la lista blanca. {e}")
            return ""

def verificar_usuario(biometric_id):
    """Verifica existencia de usuario en la base de datos MySQL."""
    from app import User
    with app.app_context():
        try:
            usuario = User.query.filter_by(biometric_id=biometric_id).first()
            return usuario is not None
        except Exception as e:
            print(f"ERROR: Falla al verificar usuario {biometric_id}. {e}")
            return False

# --- MÉTODOS DE PRUEBA HTTP (SIMULACIÓN NODEMCU) ---

def test_http_enviar_log(uid, base_url="https://acceso.istae.edu.ec"):
    """Simula el POST que hace el NodeMCU con el token de seguridad y fecha personalizada."""
    print(f"   [HTTP] Probando POST {base_url}/api/recibir_log para ID {uid}...")
    
    payload = {
        "id": str(uid),
        "estado": "EXITO",
        "fecha_dispositivo": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "token": "istae1805A" # Debe coincidir con config.py
    }
    
    try:
        req = urllib.request.Request(
            f"{base_url}/api/recibir_log",
            data=json.dumps(payload).encode('utf-8'),
            headers={'Content-Type': 'application/json'}
        )
        # Nota: Se usa contexto SSL para el dominio acceso.istae.edu.ec
        with urllib.request.urlopen(req) as response:
            print(f"   -> Código: {response.getcode()}")
            print(f"   -> Respuesta: {response.read().decode('utf-8')}")
            return True
    except Exception as e:
        print(f"   -> ❌ Error: {e}")
        return False

if __name__ == '__main__':
    print("--- UTILIDAD DE PRUEBAS BIOMÉTRICO ISTAE ---")
    print("1. Probar Base de Datos Directa (Lista Blanca)")
    print("2. Simular envío HTTPS desde NodeMCU (Producción)")
    
    opcion = input("Seleccione: ")
    if opcion == "1":
        print(f"Lista Blanca: {obtener_lista_blanca()}")
    elif opcion == "2":
        id_prueba = input("Ingrese ID de usuario para prueba: ")
        test_http_enviar_log(id_prueba)
    else:
        print("Opción no válida.")