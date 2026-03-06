import os
from urllib.parse import quote_plus

# --- SEGURIDAD DE SESIÓN ---
# Se utiliza para cifrar las cookies de sesión y proteger los mensajes flash
SECRET_KEY = 'tesis_secreta_istae'

# --- CONFIGURACIÓN DE BASE DE DATOS ---
# Formato: mysql+pymysql://usuario:contraseña@servidor/nombre_base_datos
# Las credenciales corresponden a tu configuración en el servidor HestiaCP
password = quote_plus('avril18wen04@@A1')
SQLALCHEMY_DATABASE_URI = f'mysql+pymysql://auropelv_istae:{password}@45.79.40.132/auropelv_biom2'


# --- PROTOCOLO DE SEGURIDAD IOT ---
# Este token es obligatorio en cada petición POST del NodeMCU
# Evita que dispositivos externos registren asistencias falsas
TOKEN_NODE = 'istae1805A'
