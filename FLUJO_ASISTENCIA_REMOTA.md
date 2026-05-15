# Documentación del Flujo de Asistencia Remota

Este documento detalla el proceso técnico completo para el registro de una asistencia remota por parte de un docente, desde la interacción en la interfaz de usuario (frontend) hasta el procesamiento y almacenamiento en el servidor (backend).

## Resumen del Flujo

El proceso se puede dividir en dos fases principales:

1.  **Frontend (Lado del Cliente):** El navegador del docente captura la ubicación GPS y la foto, y empaqueta esta información en un formulario.
2.  **Backend (Lado del Servidor):** El servidor Flask recibe los datos, valida la información, guarda el archivo de la foto y registra todos los detalles en la base de datos.

---

## 1. Frontend: Interfaz del Docente (`templates/docente.html`)

La interacción del usuario ocurre dentro de una ventana modal que se activa al presionar el botón "Marcar Asistencia Remota".

### Paso 1.1: Activación y Captura de GPS

1.  **Disparador:** El docente hace clic en el botón que abre el modal de asistencia.
2.  **Geolocalización:** Inmediatamente, un script de JavaScript utiliza la API de geolocalización del navegador:
    ```javascript
    navigator.geolocation.getCurrentPosition(successCallback, errorCallback);
    ```
3.  **Permisos del Usuario:** El navegador solicita al docente permiso para acceder a su ubicación.
4.  **Poblar el Formulario:**
    *   Si el usuario acepta, la función `successCallback` recibe un objeto `position` que contiene las coordenadas.
    *   El script inserta la latitud y la longitud en dos campos ocultos (`<input type="hidden">`) dentro del formulario del modal. Esto asegura que las coordenadas se envíen junto con el resto de los datos sin que el usuario las vea o modifique directamente.

### Paso 1.2: Entrada de Datos del Usuario

El formulario (`<form>`) contiene tres campos clave para el usuario:

1.  **Descripción (`<textarea name="descripcion">`):** Un campo de texto opcional para que el docente añada cualquier observación relevante.

2.  **Foto (`<input type="file" name="foto" accept="image/*" capture="camera">`):** Este es el campo más importante para la evidencia.
    *   `accept="image/*"`: Limita la selección de archivos a solo imágenes.
    *   `capture="camera"`: En dispositivos móviles, este atributo le indica al navegador que abra directamente la aplicación de la cámara, forzando a que la foto sea tomada en el momento.

3.  **Campos Ocultos de GPS:**
    ```html
    <input type="hidden" name="latitud" id="latitud-input">
    <input type="hidden" name="longitud" id="longitud-input">
    ```

### Paso 1.3: Envío del Formulario

1.  **Acción de Envío:** El docente presiona el botón "Registrar Asistencia".
2.  **Método y Destino:** El formulario se envía a través de una petición `POST` a la ruta `/docente/marcar_web` del servidor Flask.
3.  **Tipo de Codificación:** Es crucial que el formulario incluya el atributo `enctype="multipart/form-data"`. Esto es necesario para poder enviar archivos (la foto) junto con los datos de texto.

    ```html
    <form action="{{ url_for('docente_marcar') }}" method="POST" enctype="multipart/form-data">
        <!-- ... campos del formulario ... -->
    </form>
    ```

---

## 2. Backend: Procesamiento en el Servidor (`app.py`)

La lógica para manejar la petición se encuentra en la función `docente_marcar`.

### Paso 2.1: Recepción de la Petición

1.  **Ruta y Método:** La función está decorada para aceptar peticiones `POST` en la ruta `/docente/marcar_web`.
    ```python
    @app.route('/docente/marcar_web', methods=['POST'])
    @login_required
    def docente_marcar():
        # ...
    ```
2.  **Autenticación:** El decorador `@login_required` asegura que solo un usuario autenticado (en este caso, un docente) pueda realizar esta acción. El objeto `current_user` está disponible con la información del docente que realiza la petición.

### Paso 2.2: Extracción y Validación de Datos

1.  **Datos de Texto:** Se extraen la latitud, longitud y descripción del objeto `request.form`.
    ```python
    lat = request.form.get('latitud')
    lon = request.form.get('longitud')
    descripcion = request.form.get('descripcion')
    ```
2.  **Archivo de Foto:** La foto se extrae del objeto `request.files`.
    ```python
    foto = request.files.get('foto')
    ```
3.  **Validación de GPS:** El código verifica que las coordenadas GPS se hayan recibido. Si no, devuelve un mensaje de error al usuario.

### Paso 2.3: Manejo y Almacenamiento de la Foto

1.  **Verificación del Archivo:** Se comprueba si el objeto `foto` existe y si tiene un nombre de archivo (es decir, si realmente se subió un archivo).
2.  **Seguridad del Nombre:** Se utiliza la función `secure_filename()` de Werkzeug para limpiar el nombre del archivo y evitar ataques de travesía de directorios.
3.  **Creación de un Nombre Único:** Para prevenir que los archivos se sobrescriban, se genera un nuevo nombre de archivo único, combinando el ID del usuario, la fecha/hora actual y el nombre original del archivo.
    ```python
    filename = secure_filename(f"{current_user.id}_{datetime.now().strftime('%Y%m%d%H%M%S')}_{foto.filename}")
    ```
4.  **Guardado Físico:** El archivo de la foto se guarda en el servidor, dentro de la carpeta `uploads`, usando el nombre único generado.
    ```python
    foto.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
    ```

### Paso 2.4: Registro en la Base de Datos

1.  **Creación del Objeto `Log`:** Se instancia un nuevo modelo `Log` de SQLAlchemy.
2.  **Poblar el Objeto:** Se rellenan todos los campos del objeto con la información recopilada:
    *   `fecha`: Se usa la hora del servidor (`datetime.now()`) para garantizar la consistencia.
    *   `usuario_id`: Se obtiene de `current_user.biometric_id`.
    *   `tipo_evento`: Se establece como "Asistencia".
    *   `origen`: Se establece como "Asistencia remota".
    *   `latitud`, `longitud`, `descripcion`: Los datos extraídos del formulario.
    *   `foto_path`: **Este es el campo clave.** Se guarda el `filename` (el nombre único del archivo), que actúa como el enlace entre el registro de la base de datos y el archivo físico de la foto en el servidor.
3.  **Commit a la Base de Datos:** Se añade el nuevo objeto `Log` a la sesión de la base de datos y se confirman los cambios.
    ```python
    db.session.add(nuevo_log)
    db.session.commit()
    ```

### Paso 2.5: Confirmación al Usuario (Feedback)

1.  **Mensaje Flash:** Se genera un mensaje de éxito (`flash('Asistencia remota registrada con éxito.')`) que se mostrará al usuario en la siguiente página.
2.  **Redirección:** Se redirige al docente de vuelta a su dashboard, donde verá el mensaje de confirmación.

Este ciclo completo asegura que la asistencia remota se registre de forma segura, con toda la evidencia necesaria y con confirmación para el usuario.