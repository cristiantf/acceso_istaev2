# Sistema de Control de Acceso Biométrico con Servidor Flask

Este proyecto es una solución completa para el control de acceso y la gestión de asistencia, utilizando un dispositivo biométrico ESP8266 y una aplicación web Flask como servidor central.

## Características

- **Autenticación de usuarios:** Roles separados para administradores y docentes.
- **Dashboard de Administración:** Panel central con KPIs (Indicadores Clave de Rendimiento) y un monitor de eventos en tiempo real.
- **Gestión Modular:**
    - **Gestión de Asistencia:** Página dedicada para buscar (por fecha y docente), filtrar, editar y eliminar cualquier registro de asistencia.
    - **Gestión de Permisos:** Módulo especializado para buscar (por fecha y docente), registrar, editar y eliminar permisos de los docentes.
    - **Gestión de Docentes:** Funcionalidad para crear, editar, eliminar y buscar docentes, incluyendo el control de permisos para la apertura de puertas.
- **Asistencia Remota con Evidencia:** Los docentes pueden registrar su asistencia de forma remota. Esta función:
    - Captura la **ubicación GPS** del dispositivo.
    - Solicita una **evidencia fotográfica** tomada directamente desde la cámara del teléfono para mayor seguridad.
    - Permite añadir una **descripción opcional**.
- **Visor de Evidencias:** Un modal integrado muestra todos los detalles de la marcación remota: mapa de ubicación, foto y descripción.
- **Informes en Excel:**
    - **Reporte de Asistencia:** Genera un informe matricial con filtros por rango de fechas, docente, y rangos de hora personalizables para los turnos de mañana y tarde.
    - **Reporte de Permisos:** Genera un informe detallado de los permisos registrados con filtros avanzados.
- **Control de Hardware:**
    - **Apertura Remota:** Permite a los administradores y docentes autorizados abrir la puerta desde la interfaz web.
    - **Sincronización de Hora:** Herramienta para ajustar la fecha y hora del dispositivo biométrico.
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

5.  **Ejecutar la aplicación y crear tablas:**
    ```bash
    flask run
    ```
    La primera vez que se ejecuta, Flask-SQLAlchemy creará las tablas necesarias. Si las tablas ya existen y necesitas actualizarlas (por ejemplo, añadir las nuevas columnas de evidencia a la tabla `logs`), deberás hacerlo manualmente con SQL.

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

- **Log:** Almacena los registros de acceso.
  - `id`: Clave primaria.
  - `fecha`: Marca de tiempo del evento.
  - `usuario_id`: El ID biométrico del usuario.
  - `tipo_evento`: Por ejemplo, "Asistencia + puerta", "Apertura Remota".
  - `origen`: Por ejemplo, "Huella", "Asistencia remota".
  - `latitud`: Coordenada de latitud (para asistencia remota).
  - `longitud`: Coordenada de longitud (para asistencia remota).
  - `descripcion`: Observación opcional (para asistencia remota).
  - `foto_path`: Ruta del archivo de la foto de evidencia (para asistencia remota).

- **Comando:** Almacena los comandos que debe ejecutar el ESP8266.

- **Permiso:** Almacena los permisos o ausencias justificadas de los docentes.

## Configuración (`config.py`)

- `SECRET_KEY`: Clave secreta para la gestión de sesiones.
- `SQLALCHEMY_DATABASE_URI`: Cadena de conexión a la base de datos.
- `TOKEN_NODE`: Token secreto para autenticar las peticiones del ESP8266.
