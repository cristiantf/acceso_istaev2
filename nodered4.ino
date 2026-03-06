/**
 * PROYECTO: CONTROL DE ACCESO BIOMÉTRICO ISTAE
 * VERSIÓN: Final Estabilizada + Sincronización de Hora Remota (Híbrido JSON/XML)
 * HARDWARE: NodeMCU ESP8266 + Hikvision DS-K1T8003EF
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <MD5Builder.h>

// --- CONFIGURACIÓN DE NUBE ---
const char* HOST_URL = "https://acceso.istae.edu.ec";
const char* TOKEN_NODE = "istae1805A";

// --- DATOS DEL BIOMÉTRICO ---
const char* ip_bio = "192.168.1.22";
const int port_bio = 80;
const char* user_bio = "admin";
const char* pass_bio = "istae1804A"; 

// --- PINES ---
const int RELAY_PIN = 5; // D1 en NodeMCU

// --- VARIABLES DE CONTROL ---
WiFiClient bioClient; 
String listaBlanca = "";
String bufferJSON = "";
int llaves = 0;
bool capturando = false;
bool puertaAbierta = false;

// Variables Digest para persistencia del stream
String realm = "", nonce = "";
unsigned long msRelay = 0, msCheckCmd = 0, msSync = 0;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  if(!LittleFS.begin()) Serial.println(F("Falla LittleFS"));

  // --- CONFIGURACIÓN RESILIENTE DE WIFI ---
  WiFiManager wm;
  wm.setConfigPortalTimeout(180); 
  
  if (!wm.autoConnect("NODE_PUERTA_ISTAE")) {
    Serial.println(F("❌ Tiempo agotado. Reiniciando..."));
    delay(3000);
    ESP.restart();
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial.println(F("\n✅ Conexión ISTAE Establecida."));
  sincronizarListaBlanca();
}

void loop() {
  unsigned long now = millis();

  // 1. Mantener flujo de eventos abierto (Digest GET)
  if (WiFi.status() == WL_CONNECTED) {
    if (!bioClient.connected()) {
      conectarConDigest();
      delay(2000);
    }
  }

  // 2. Procesar el Stream (Lógica JSON para extracción de ID y Hora)
  while (bioClient.available()) {
    char c = bioClient.read();
    if (c == '{') {
      if (!capturando) { capturando = true; bufferJSON = ""; }
      llaves++;
    }
    if (capturando) {
      bufferJSON += c;
      if (c == '}') {
        llaves--;
        if (llaves == 0) {
          procesarEventoAcceso(bufferJSON);
          capturando = false;
        }
      }
    }
    if (bufferJSON.length() > 2500) { capturando = false; bufferJSON = ""; llaves = 0; }
  }

  // 3. Temporizador de cierre de puerta
  if (puertaAbierta && (now - msRelay >= 3000)) {
    digitalWrite(RELAY_PIN, LOW);
    puertaAbierta = false;
    Serial.println(F("🔒 Cerradura bloqueada."));
  }

  // 4. Tareas de Nube
  if (WiFi.status() == WL_CONNECTED) {
    if (now - msCheckCmd >= 3000) { revisarComandosNube(); msCheckCmd = now; }
    if (now - msSync >= 600000) { sincronizarListaBlanca(); msSync = now; }
  }
}

/**
 * Conexión de Escucha (GET)
 * Extrae eventos en formato JSON del hardware
 */
void conectarConDigest() {
  Serial.println(F("\n📡 Abriendo canal de escucha ISAPI..."));
  if (!bioClient.connect(ip_bio, port_bio)) return;

  bioClient.println("GET /ISAPI/Event/notification/alertStream HTTP/1.1");
  bioClient.print("Host: "); bioClient.println(ip_bio);
  bioClient.println();

  unsigned long timeout = millis();
  while (bioClient.connected() && millis() - timeout < 2000) {
    String header = bioClient.readStringUntil('\n');
    if (header.indexOf("WWW-Authenticate: Digest") != -1) {
      realm = extraerParam(header, "realm");
      nonce = extraerParam(header, "nonce");
      break;
    }
    if (header == "\r") break;
  }
  bioClient.stop();

  if (realm == "" || nonce == "") return;

  String ha1 = getMD5(String(user_bio) + ":" + realm + ":" + String(pass_bio));
  String ha2 = getMD5("GET:/ISAPI/Event/notification/alertStream");
  String response = getMD5(ha1 + ":" + nonce + ":" + ha2);

  if (bioClient.connect(ip_bio, port_bio)) {
    bioClient.println("GET /ISAPI/Event/notification/alertStream HTTP/1.1");
    bioClient.print("Host: "); bioClient.println(ip_bio);
    bioClient.print("Authorization: Digest username=\"admin\", realm=\"");
    bioClient.print(realm); bioClient.print("\", nonce=\"");
    bioClient.print(nonce); bioClient.print("\", uri=\"/ISAPI/Event/notification/alertStream\", response=\"");
    bioClient.print(response); bioClient.println("\"");
    bioClient.println("Connection: keep-alive");
    bioClient.println();
    Serial.println(F("✅ ESCUCHA ACTIVA (Formato JSON)."));
  }
}

/**
 * Función para comandos de configuración (PUT)
 * Envía parámetros en XML según el estándar ISAPI
 */
void enviarPutDigest(String uri, String xmlBody) {
  WiFiClient client;
  if (!client.connect(ip_bio, port_bio)) return;

  client.print("PUT " + uri + " HTTP/1.1\r\n");
  client.print("Host: " + String(ip_bio) + "\r\n\r\n");

  String l_realm = "", l_nonce = "";
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 2000) {
    String header = client.readStringUntil('\n');
    if (header.indexOf("WWW-Authenticate: Digest") != -1) {
      l_realm = extraerParam(header, "realm");
      l_nonce = extraerParam(header, "nonce");
      break;
    }
    if (header == "\r") break;
  }
  client.stop();

  if (l_nonce == "") return;

  String ha1 = getMD5(String(user_bio) + ":" + l_realm + ":" + String(pass_bio));
  String ha2 = getMD5("PUT:" + uri);
  String response = getMD5(ha1 + ":" + l_nonce + ":" + ha2);

  if (client.connect(ip_bio, port_bio)) {
    client.print("PUT " + uri + " HTTP/1.1\r\n");
    client.print("Host: " + String(ip_bio) + "\r\n");
    client.print("Authorization: Digest username=\"admin\", realm=\"" + l_realm + "\", nonce=\"" + l_nonce + "\", uri=\"" + uri + "\", response=\"" + response + "\"\r\n");
    client.print("Content-Type: application/xml\r\n");
    client.print("Content-Length: " + String(xmlBody.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(xmlBody);
    client.stop();
    Serial.println(F("⏰ Configuración de hora enviada al hardware."));
  }
}

void procesarEventoAcceso(String json) {
  // Búsqueda en el JSON (Prioridad de tesis: Extracción directa de ID y Hora)
  int posID = json.indexOf("\"employeeNoString\":");
  if (posID == -1) return;
  int startID = json.indexOf("\"", posID + 20) + 1;
  int endID = json.indexOf("\"", startID);
  String id = json.substring(startID, endID);

  int posDate = json.indexOf("\"dateTime\":");
  String fechaBio = "";
  if (posDate != -1) {
    int startDate = json.indexOf("\"", posDate + 12) + 1;
    int endDate = json.indexOf("\"", startDate);
    fechaBio = json.substring(startDate, endDate);
  }

  if (id != "" && id != "null") {
    Serial.printf("\n👤 ID: %s | HORA HARDWARE: %s\n", id.c_str(), fechaBio.c_str());
    if (listaBlanca.indexOf(id) != -1) {
      abrirPuerta();
      enviarLogNube(id, "EXITO", fechaBio);
    } else {
      enviarLogNube(id, "DENEGADO", fechaBio);
    }
  }
}

void abrirPuerta() {
  digitalWrite(RELAY_PIN, HIGH);
  puertaAbierta = true;
  msRelay = millis();
  Serial.println(F("🔓 PUERTA ABIERTA"));
}

void enviarLogNube(String id, String estado, String fecha) {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String payload = "{\"id\":\"" + id + "\",\"estado\":\"" + estado + 
                   "\",\"fecha_dispositivo\":\"" + fecha + 
                   "\",\"token\":\"" + String(TOKEN_NODE) + 
                   "\",\"origen\":\"Huella\"}";
                   
  http.begin(client, String(HOST_URL) + "/api/recibir_log");
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();
}

void sincronizarListaBlanca() {
  WiFiClientSecure client; client.setInsecure(); HTTPClient http;
  if(http.begin(client, String(HOST_URL) + "/api/sincronizar")){
    if(http.GET() == 200) {
      listaBlanca = http.getString();
      Serial.println(F("☁️ Lista blanca sincronizada."));
    }
    http.end();
  }
}

void revisarComandosNube() {
  WiFiClientSecure client; client.setInsecure(); HTTPClient http;
  if(http.begin(client, String(HOST_URL) + "/api/check_comando")){
    if(http.GET() == 200) {
      String response = http.getString();
      if (response == "ABRIR") {
        abrirPuerta();
      } else if (response.startsWith("SET_TIME|")) {
        String isoTime = response.substring(9);
        // XML requerido para configuración de sistema (ISAPI Standard)
        String xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Time xmlns=\"http://www.isapi.org/ver20/XMLSchema\" version=\"2.0\"><timeMode>manual</timeMode><localTime>" + isoTime + "</localTime></Time>";
        enviarPutDigest("/ISAPI/System/time", xml);
      }
    }
    http.end();
  }
}

String getMD5(String data) {
  MD5Builder md5;
  md5.begin();
  md5.add(data);
  md5.calculate();
  return md5.toString();
}

String extraerParam(String header, String param) {
  int pos = header.indexOf(param + "=\"");
  if (pos == -1) return "";
  int start = pos + param.length() + 2;
  int end = header.indexOf("\"", start);
  return header.substring(start, end);
}
