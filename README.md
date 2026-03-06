# Sistema de Control de Acceso Biométrico con Servidor Flask

Este proyecto es una solución completa para el control de acceso y la gestión de asistencia, utilizando un dispositivo biométrico ESP8266 y una aplicación web Flask como servidor central.

## Características

- **Autenticación de usuarios:** Roles separados para administradores y docentes.
- **Panel de administración:** Permite la gestión completa de los usuarios docentes (crear, editar, eliminar).
- **Gestión de Permisos de Docentes:** Los administradores pueden registrar, editar y eliminar permisos o ausencias justificadas para los docentes.
- **Sincronización de Hora del Dispositivo:** Herramienta en el panel de admin para ajustar manualmente la fecha y hora del hardware biométrico.
- **Panel de docente:** Permite a los docentes ver sus registros de acceso y abrir la puerta de forma remota.
- **Monitorización en tiempo real:** Muestra los últimos eventos de acceso en el panel de administración.
- **Informes en Excel:** Genera informes detallados en formato Excel para la asistencia (formato matricial) y los permisos registrados, con filtros por rango de fechas y docente.
- **Apertura remota de la puerta:** Permite a los administradores y docentes autorizados abrir la puerta desde la interfaz web.
- **Integración de hardware:** Se integra con un dispositivo ESP8266 para el escaneo biométrico y el control de la puerta.
- **Capacidad sin conexión:** El ESP8266 almacena los registros de acceso si el servidor no está disponible y los envía más tarde.

## Pila Tecnológica

- **Back-end:** Python, Flask, SQLAlchemy
- **Front-end:** HTML, CSS, JavaScript, Bootstrap 5
- **Base de datos:** MySQL (configurable)
- **Hardware:** ESP8266 (NodeMCU), Lector biométrico Hikvision (o similar con API ISAPI), Relé
- **Otros:** OpenPyXL para la generación de informes.

## Arquitectura

- **Servidor Flask:** Actúa como el cerebro de la operación. Gestiona la autenticación de usuarios, la lógica de negocio, y sirve la interfaz de usuario web. También expone una API REST para la comunicación con el hardware.
- **ESP8266:**
  - Se conecta a la red WiFi local mediante WiFiManager para una fácil configuración.
  - Sincroniza una "lista blanca" de IDs de usuario autorizados desde el servidor Flask.
  - Cuando se escanea una huella digital, comprueba el ID con la lista blanca.
  - Si el ID está autorizado, activa un relé para abrir la puerta.
  - Envía los registros de acceso al servidor en tiempo real.
  - Comprueba periódicamente si hay comandos remotos pendientes en el servidor (como abrir la puerta o sincronizar la hora).

## Configuración e Instalación

1.  **Clonar el repositorio:**
    ```bash
    git clone <url-del-repositorio>
    cd <directorio-del-repositorio>
    ```

2.  **Crear un entorno virtual:**
    ```bash
    python -m venv venv
    source venv/bin/activate  # En Windows: venv\Scripts\activate
    ```

3.  **Instalar las dependencias:**
    ```bash
    pip install -r requirements.txt
    ```

4.  **Configurar la base de datos:**
    - Crea una base de datos MySQL.
    - Copia el archivo `config.py.example` a `config.py`.
    - Edita `config.py` con tus credenciales de la base de datos y una clave secreta (`SECRET_KEY`).

5.  **Ejecutar la aplicación:**
    ```bash
    flask run
    ```
    La aplicación estará disponible en `http://127.0.0.1:5000`.

6.  **Configurar el ESP8266:**
    - Abre el archivo `nodered4.ino` con el IDE de Arduino.
    - Asegúrate de tener las librerías necesarias instaladas (ESP8266WiFi, ESP8266HTTPClient, WiFiManager, etc.).
    - Actualiza las variables `HOST_URL`, `TOKEN_NODE` y los detalles del biométrico (`ip_bio`, `user_bio`, `pass_bio`).
    - Carga el código en tu ESP8266.

## Credenciales por Defecto

- **Usuario:** `admin`
- **Contraseña:** `istae123A*`

## Estructura de la Base de Datos

- **User:** Almacena la información de los usuarios (administradores y docentes).
  - `id`: Clave primaria.
  - `biometric_id`: El ID del dispositivo biométrico (por ejemplo, "101").
  - `nombre`: Nombre completo.
  - `username`: Nombre de usuario para el login.
  - `password`: Hash de la contraseña.
  - `rol`: `'admin'` o `'docente'`. 
  - `acceso_puerta`: `1` si el docente puede abrir la puerta, `0` si no.

- **Log:** Almacena los registros de acceso.
  - `id`: Clave primaria.
  - `fecha`: Marca de tiempo del evento.
  - `usuario_id`: El ID biométrico del usuario.
  - `tipo_evento`: Por ejemplo, "Asistencia + puerta", "Apertura Remota".
  - `origen`: Por ejemplo, "Huella", "Panel Control".

- **Comando:** Almacena los comandos que debe ejecutar el ESP8266.
  - `id`: Clave primaria.
  - `instruccion`: El comando (ej. `ABRIR` o `SET_TIME|2023-10-27T10:30:00-05:00`).
  - `estado`: `'PENDIENTE'` o `'ENVIADO'`.
  
- **Permiso:** Almacena los permisos o ausencias justificadas de los docentes.
  - `id`: Clave primaria.
  - `user_id`: Clave foránea al ID del docente.
  - `fecha_permiso`: La fecha para la cual se concede el permiso.
  - `observacion`: Una descripción o motivo del permiso.

## Configuración (`config.py`)

- `SECRET_KEY`: Clave secreta para la gestión de sesiones.
- `SQLALCHEMY_DATABASE_URI`: Cadena de conexión a la base de datos.
- `TOKEN_NODE`: Token secreto para autenticar las peticiones del ESP8266. Debe ser el mismo que en el fichero `nodered4.ino`.
