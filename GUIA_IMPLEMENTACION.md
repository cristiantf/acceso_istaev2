# Guía de Implementación y Mantenimiento

Este documento proporciona todos los pasos necesarios para desplegar el Sistema de Control de Acceso desde cero y para actualizar una implementación ya existente con nuevas versiones del software y cambios en la base de datos.

---

## Parte 1: Implementación Inicial (Desde Cero)

Siga estos pasos si está instalando el sistema por primera vez en un servidor.

### Paso 1: Configuración del Entorno del Servidor

1.  **Acceda al servidor** donde se alojará la aplicación (por ejemplo, mediante SSH).
2.  **Clone el código fuente** desde el repositorio de Git.
    ```bash
    git clone <URL_DEL_REPOSITORIO>
    cd <NOMBRE_DEL_DIRECTORIO>
    ```
3.  **Cree y active un entorno virtual** para Python. Esto aísla las dependencias del proyecto.
    ```bash
    python3 -m venv .venv
    source .venv/bin/activate
    ```
4.  **Instale las librerías** necesarias que se especifican en el archivo `requirements.txt`.
    ```bash
    pip install -r requirements.txt
    ```

### Paso 2: Configuración de la Base de Datos MySQL en cPanel

1.  **Inicie sesión en su cPanel**.
2.  Vaya a **Bases de datos > Bases de datos MySQL®**.
3.  **Cree una nueva base de datos**: Asígnele un nombre, por ejemplo, `istae_asistencia`.
4.  **Cree un nuevo usuario de base de datos**: Genere un nombre de usuario y una contraseña segura.
5.  **Añada el usuario a la base de datos**: En la sección "Añadir usuario a la base de datos", seleccione el usuario y la base de datos que acaba de crear y haga clic en "Añadir".
6.  **Asigne todos los privilegios** al usuario y guarde los cambios.

### Paso 3: Permitir Conexión Remota (Whitelist IP)

1.  En cPanel, vaya a **Bases de datos > MySQL® Remoto**.
2.  En el campo "Host", añada la dirección IP del servidor donde se está ejecutando la aplicación Flask:
    ```
    34.31.209.167
    ```
3.  Haga clic en "Añadir Host". Esto es crucial para permitir que la aplicación se conecte a la base de datos de cPanel.

### Paso 4: Configuración de la Aplicación Flask

1.  En el servidor, cree una copia del archivo de configuración de ejemplo.
    ```bash
    cp config.py.example config.py
    ```
2.  **Edite el archivo `config.py`** y rellene los siguientes campos:
    *   `SECRET_KEY`: Genere una cadena aleatoria y segura.
    *   `SQLALCHEMY_DATABASE_URI`: Use el formato `mysql+pymysql://<usuario>:<contraseña>@<host_de_cpanel>/<base_de_datos>`.
    *   `TOKEN_NODE`: Genere otro token aleatorio para la comunicación segura con el hardware.

### Paso 5: Inicialización y Ejecución

1.  **Ejecute la aplicación Flask por primera vez**. SQLAlchemy detectará que las tablas no existen y las creará automáticamente.
    ```bash
    flask run --host=0.0.0.0 --port=5000
    ```
2.  **Verifique la creación**: Acceda a la aplicación en su navegador (`http://<IP_DEL_SERVIDOR>:5000`). Inicie sesión con las credenciales por defecto (`admin` / `istae123A*`). Si no hay errores, la instalación fue exitosa.

---

## Parte 2: Actualización de una Implementación Existente

Siga estos pasos cuando haya una nueva versión del software y necesite actualizar la aplicación que ya está en producción sin perder los datos existentes.

### Paso 1: Actualizar el Código Fuente

1.  Conéctese al servidor donde se está ejecutando la aplicación.
2.  Navegue al directorio del proyecto y **descargue los últimos cambios** desde el repositorio de Git.
    ```bash
    cd <NOMBRE_DEL_DIRECTORIO>
    git pull
    ```
3.  **Active el entorno virtual**.
    ```bash
    source .venv/bin/activate
    ```
4.  **Actualice las dependencias** por si se ha añadido alguna nueva librería.
    ```bash
    pip install -r requirements.txt
    ```

### Paso 2: Actualización Manual de la Base de Datos (Migración)

Esta es la parte más crítica. Cuando el código nuevo requiere campos adicionales en la base de datos, `db.create_all()` no los añadirá. Debe hacerlo manualmente con comandos SQL.

1.  **Inicie sesión en cPanel** y abra **phpMyAdmin**.
2.  **Seleccione la base de datos** de su aplicación en el menú de la izquierda. Según su captura de pantalla, el nombre de su base de datos es `user1_biom`.
3.  Haga clic en la pestaña **"SQL"** para abrir una ventana donde puede ejecutar comandos.
4.  **Copie y ejecute el siguiente comando SQL**. Las imágenes que adjuntó muestran que a su tabla `logs` le faltan las columnas para la asistencia remota. Este comando las añadirá sin borrar ningún dato existente.

    ```sql
    ALTER TABLE `logs`
    ADD COLUMN `latitud` FLOAT NULL DEFAULT NULL AFTER `origen`,
    ADD COLUMN `longitud` FLOAT NULL DEFAULT NULL AFTER `latitud`,
    ADD COLUMN `descripcion` TEXT NULL DEFAULT NULL AFTER `longitud`,
    ADD COLUMN `foto_path` VARCHAR(255) NULL DEFAULT NULL AFTER `descripcion`;
    ```

5.  **Verificación**: Después de ejecutar el comando, vaya a la estructura de la tabla `logs` y verifique que las nuevas columnas (`latitud`, `longitud`, `descripcion`, `foto_path`) ahora existen.

### Paso 3: Reiniciar la Aplicación

Para que todos los cambios surtan efecto (tanto el nuevo código como la nueva estructura de la base de datos), debe reiniciar el proceso de la aplicación Flask. La forma de hacerlo dependerá de cómo esté ejecutando la aplicación en producción (por ejemplo, `systemd`, `gunicorn`, `supervisor`, etc.).

-   Si usa `systemd`:
    ```bash
    sudo systemctl restart mi_aplicacion_flask.service
    ```
-   Si la está ejecutando directamente, simplemente deténgala (`Ctrl+C`) y vuelva a iniciarla:
    ```bash
    flask run --host=0.0.0.0 --port=5000
    ```

Una vez reiniciada, la aplicación funcionará con las nuevas características y seguirá utilizando todos los datos históricos que ya tenía almacenados.
