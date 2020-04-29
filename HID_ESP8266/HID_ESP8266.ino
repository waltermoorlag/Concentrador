// Include framework code and libraries
//#include <NTPClient.h>
#include <ezTime.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

// WIEGAND
#define MAX_BITS 100                 // max number of bits
#define WEIGAND_WAIT_TIME  3000      // time to wait for another weigand pulse. 

////////////////////////////////
////// VARIABLES GLOBALES //////
////////////////////////////////
unsigned long timeUpdateCheck = millis();
//int MinutesBeforeUpdate = 1;
char MinutesBeforeUpdate[50];
boolean CheckUpdateOne = false;
boolean CheckConfig = false;

String listaMensajes[50];
unsigned long syncMsj;


String urlServer = "http://34.209.112.168:3000/concentrador/version/soft/";
String urlService = "http://18.229.201.234:3000/mensajeria/mensaje"; 

String comportamientoBornera1 = "";
const int canalBornera1 = D7;
char bornera1[50];
char empresa[50];
char chkAwid[50];
char chkRfid[50];
String inicio = "AAFFAA";
String fin = "FFAAFF";

String ConnectDevice = "99";
unsigned long timeCheckConnect_ini;
unsigned long timeIniDevice = millis();


String infoFalcom = "05";
String valvulaDescarga = "04";
String idContacto = "03";
String infoWid = "02";
String tipoInfo = "01";
boolean AlertaIdentificado_bool = false;
unsigned long AlertaIdentificado_ini;
int AlertaIdentificado_limit = 60000;



boolean verificaUpdate = true;

char emulationMode[50];
String auxEmulation = "";
boolean emulation = false;
boolean printSerial = true;

boolean accesPointMode = false;

boolean boton_apretado = false;
unsigned long update_ini;
boolean update_forzado = false;


////////////////////////////////////////
////////////////////////////////////////

///////////////////////////////////////
// VARIABLES VALVULA DESCARGA        //
///////////////////////////////////////
boolean  valvulaEnviada = false;
///////////////////////////////////////
///////////////////////////////////////

///////////////////////////////////////
// VARIABLES IDENTIFICADO y CONTACTO //
///////////////////////////////////////
boolean identificado = false;
boolean prendido = false;
unsigned long buzzer_ini;

boolean contacto = false;
boolean enviarContacto = true;
///////////////////////////////////////
///////////////////////////////////////

// VARIABLES AWID //
int timeSecAwid;
unsigned long t_now;
unsigned long t_ultLectura;
unsigned long t_ini;
unsigned long t_fin;
String awidstr;
String awidSend;
String ultimoAwid;
String nroAntena;
int cantLecturas;
String OrigenAntena = "";
boolean envioDesacople = false;
////////////////////

/////////////////////////////////////
////        VARIABLES RFID       ////
/////////////////////////////////////
unsigned char databits[MAX_BITS];    // stores all of the data bits
unsigned char bitCount;              // number of bits currently captured
unsigned char flagDone;              // goes low when data is currently being captured
unsigned int weigand_counter;        // countdown until we assume there are no more bits
unsigned long facilityCode = 0;      // decoded facility code
unsigned long cardCode = 0;          // decoded card code

// interrupt that happens when INTO goes low (0 bit)
void ICACHE_RAM_ATTR isr0() {
  bitCount++;
  flagDone = 0;
  weigand_counter = WEIGAND_WAIT_TIME;
}

// interrupt that happens when INT1 goes low (1 bit)
void ICACHE_RAM_ATTR isr1() {
  databits[bitCount] = 1;
  bitCount++;
  flagDone = 0;
  weigand_counter = WEIGAND_WAIT_TIME;
}
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

//////////////////////////////////
// VARIABLE WIFI Y ACCESS POINT //
//////////////////////////////////
char ssid[50];
char password[50];
//IP o DNS domain
char serverName[] = "192.168.225.1";
// change to your server's port
int serverPort = 5544;
const char *ssidConf = "concentrador";
const char *passConf = "12345678";
String mensaje = "";
String pagina = obtenerHtml();
String paginafin =
  "<a href='resetdispo'><button style='background-color:red !important; width:100%; height:60px;font-size: 45px;font-weight: bold;position: absolute;bottom: -2px;'>RESET DISPO</button></a><br>"
  "</div>"
  "</body>"
  "</html>";
///////////////////////////////////
///////////////////////////////////
///////////////////////////////////


ESP8266WebServer server(80);
SoftwareSerial antenaAWID (D1, 11);

void setup()
{
  syncMsj = millis();
  Serial.begin(9600);
  Serial.println("");
  EEPROM.begin(512);
  //  antenaAWID.begin(9600);
  timeSecAwid = 15;
  t_ini =  millis();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, INPUT_PULLUP);  //D5
  pinMode(D7, INPUT);  //D7
  digitalWrite(D5, LOW);
  pinMode(D2, INPUT);     // DATA0 (INT0)
  pinMode(D3, INPUT);     // DATA1 (INT1)
  attachInterrupt(D2, isr0, FALLING);
  attachInterrupt(D3, isr1, FALLING);
  weigand_counter = WEIGAND_WAIT_TIME;
  sonarBuzzer(2);

  if (digitalRead(D6) == 0) {
    accesPointMode = true;
    Serial.println("Access Point Mode ON");
    delay(500);
    sonarBuzzer(3);
    modoconf();
  } else {
    WiFi.mode(WIFI_STA);
    Serial.println("Access Point Mode OFF");
    WiFi.softAPdisconnect(true);
    accesPointMode = false;

    initMemory();

    if (chkAwid[0] == '0') {
      antenaAWID.begin(115200);
      Serial.println("Velocidad de 115200");
    } else {
      antenaAWID.begin(9600);
    }

    String auxEmulation = charToString(emulationMode);
    if (auxEmulation.length() == 0) {
      grabar(300, "0");
      auxEmulation = "0";
    }
    if (auxEmulation == "1") {
      emulation = true;
    } else {
      emulation = false;
    }
    //    verificaActualizacionFirmware();
    if ( comportamientoBornera1 == "03") {
      if (digitalRead(canalBornera1) == HIGH) {
        contacto = true;
        identificado = false;
      } else {
        contacto = false;
        identificado = true;
      }
    }
  }
  //  waitForSync(3);
  AlertaIdentificado_ini = millis();
  Serial.println("Modulo listo.");
}
///////////////////////////////////////////////////////////////

void loop() {
//  checkConnect();
  //
  leerSerial();
  //

  if (!accesPointMode) {
    if (comportamientoBornera1 == "03") {
      // METODO QUE EVALUA SI SE PUSO EN CONCTACTO //
      evaluaContacto(canalBornera1);
      ///////////////////////////////////////////////
      if (contacto) {
        verificaUpdate = true;
      }
    }
    if (comportamientoBornera1 == "04") {
      // METODO QUE EVALUA VALVULA DE DESCARGA //
      evaluaValvula(canalBornera1);
      ///////////////////////////////////////////////

    }

    // FORZAR UPDATE CON PULSADOR //
    evaluaForzadorUpdate();
    ////////////////////////////////

    // METODO QUE HACER SONAR EL BUZZER SI NO SE IDENTIFICO CON HID //
    evaluaIdentificado();
    evaluaAlertaIdentificado(); // envia alerta al pasar tiempo sin identificar
    //////////////////////////////////////////////////////////////////

    // DETECCION DE DATOS DE RFID //
    detectHID();
    ////////////////////////////////

    // DETECCION DE DATOS DE AWID //
    detectAWID();
    enviarUltimoDesacople();
    ////////////////////////////////



    if (comportamientoBornera1 == "03") {
      informarContacto();
    }
    if ((millis() - syncMsj) > 45000) {
      syncMsj = millis();
      enviarInfoDispo(""); // cada 2 minutos mando a informar por si hay mensajes encolados.
    }

    if (!CheckConfig) {
      if ((millis() - timeUpdateCheck) > (1 * 60 * 1000)) {
        Serial.println("Check Config.");
        CheckConfig = true;
        verificaConfiguracion();
      }
    }
    if (!CheckUpdateOne) {
      if ((millis() - timeUpdateCheck) > (String(MinutesBeforeUpdate[0]).toInt() * 60 * 1000)) {
        Serial.println("Check update.");
        CheckUpdateOne = true;
        verificaActualizacionFirmware();
      }
    }
    if (comportamientoBornera1 == "03" && !contacto) {
      timeUpdateCheck = millis();
      if (verificaUpdate) {
        verificaConfiguracion();
        verificaActualizacionFirmware();
        verificaUpdate = false;
      }
    }
  }
}

unsigned long Redondeo(float num){
//    float num;
    int entero;
    int numero;    
    int ult_cifra;
  
    entero = int(num * 1000);
    numero = int(num * 100);
    
    ult_cifra = entero % 10;
    
    if ( ult_cifra >= 5 ){
        numero = numero  + 1;
    } else {
        numero = numero + 0;
    }
    return numero/100;
}

void checkConnect(){
  if((millis() - timeCheckConnect_ini) > 60000){
    timeCheckConnect_ini = millis();
    String msjAdicional = "";
    msjAdicional += "TIME(" + String(Redondeo((millis() - timeIniDevice) / 1000)) + ")|";
    if (!identificado) {
      msjAdicional += "NO IDENTIFICADO|";
    } else {
      msjAdicional += "IDENTIFICADO|";
    }
      
    if(!AlertaIdentificado_bool){
      msjAdicional += "ALERTA NO ENVIADA|";
    } else {
      msjAdicional += "ALERTA ENVIADA|";
    }
    enviarWebService(inicio + "000000000000" + ConnectDevice + msjAdicional +fin);
  }
}

String obtenerValorTag(String msj, String tag) {
  Serial.println("Mensaje2:" + msj);
  size_t foundtag = msj.indexOf(tag);
  size_t inde = 0;
  Serial.println("Buscando tag: "+tag);
  Serial.println(foundtag);
  if (foundtag >= 0 && foundtag <= 500) {
    inde = foundtag + tag.length();
    while (true) {
      if (msj.substring(inde, inde + 1) == ";") {
        break;
      } else {
        inde ++;
      }
    }
  }
  return inde == 0 ? "NOENCONTRADO" : msj.substring(foundtag + tag.length(), inde);
}


void guardarConfSerial(String mensaje, String tag, int pos) {
  String confValor = obtenerValorTag(mensaje, tag);
  Serial.println(confValor);
  if (confValor != "NOENCONTRADO") {
    Serial.println(tag + confValor);
    grabar(pos, confValor);
  }


}

void leerSerial() {
  if (Serial.available()) {
    int mcant = 0;
    String mensaje = "";
    while (Serial.available()) {
      char ch = Serial.read();
      mensaje.concat(ch);
      delay(2);
      mcant ++;
    }
    if (mcant > 0) {
      Serial.println("Mensaje:" + mensaje);
      guardarConfSerial(mensaje, "WIFI:", 0);
      guardarConfSerial(mensaje, "PASS:", 50);
      guardarConfSerial(mensaje, "EMPR:", 100);
      guardarConfSerial(mensaje, "AWID:", 150);
      guardarConfSerial(mensaje, "RFID:", 200);
      guardarConfSerial(mensaje, "BORN:", 250);
      guardarConfSerial(mensaje, "EMUL:", 300);
      guardarConfSerial(mensaje, "MUPD:", 350);
      leer(0).toCharArray(ssid, 50);
      leer(50).toCharArray(password, 50);
      leer(100).toCharArray(empresa, 50);
      leer(150).toCharArray(chkAwid, 50);
      leer(200).toCharArray(chkRfid, 50);
      leer(250).toCharArray(bornera1, 50);
      comportamientoBornera1 = charToString(bornera1);
      if (comportamientoBornera1.length() == 0) {
        grabar(250, "03");
        comportamientoBornera1 = "03";
      }
      leer(300).toCharArray(emulationMode, 50);
      auxEmulation = charToString(emulationMode);
      if (auxEmulation.length() == 0) {
        grabar(300, "0");
        auxEmulation = "0";
      }
      if (auxEmulation == "1") {
        emulation = true;
      } else {
        emulation = false;
      }
      leer(350).toCharArray(MinutesBeforeUpdate, 50);
      if (String(MinutesBeforeUpdate[0]).toInt() == 0) {
        grabar(350, "3");
        leer(350).toCharArray(MinutesBeforeUpdate, 50);
      }
      Serial.print("Wifi: ");
      Serial.println(ssid);
      Serial.print("Pass: ");
      Serial.println(password);
      Serial.print("empresa: ");
      Serial.println(empresa);
      Serial.print("chkAwid: ");
      Serial.println(chkAwid);
      Serial.print("chkRfid: ");
      Serial.println(chkRfid);
      Serial.print("Bornera1: ");
      Serial.println(bornera1);
      Serial.print("Emulation Mode: ");
      Serial.println(emulationMode);
      Serial.print("Minute update: ");
      Serial.println(MinutesBeforeUpdate);

    }
  }
}

void initMemory() {
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(password, 50);
  leer(100).toCharArray(empresa, 50);

  leer(150).toCharArray(chkAwid, 50);
  if (chkAwid[0] != '0' && chkAwid[0] != '1') {
    grabar(150, "0");
    leer(150).toCharArray(chkAwid, 50);
  }

  leer(200).toCharArray(chkRfid, 50);
  if (chkRfid[0] != '0' && chkRfid[0] != '1') {
    grabar(200, "0");
    leer(200).toCharArray(chkRfid, 50);
  }

  leer(250).toCharArray(bornera1, 50);
  comportamientoBornera1 = charToString(bornera1);
  if (comportamientoBornera1.length() == 0) {
    grabar(250, "03");
    comportamientoBornera1 = "03";
  }

  leer(300).toCharArray(emulationMode, 50);

  leer(350).toCharArray(MinutesBeforeUpdate, 50);
  Serial.println("udpate" + String(MinutesBeforeUpdate[0]));
  if (String(MinutesBeforeUpdate[0]).toInt() == 0) {
    grabar(350, "5");
    leer(350).toCharArray(MinutesBeforeUpdate, 50);
  }
}

void destelloLed(int cantidad, char cortoLargo) {
  int myDelay = 200;
  if (cortoLargo == 'L') {
    myDelay = 500;
  }
  for (int v = 0; v < cantidad; v++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(myDelay);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(myDelay);
  }
}

void sonarBuzzer(int cantidad) {
  for (int v = 0; v < cantidad; v++) {
    digitalWrite(D5, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(D5, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
}

void sonarBuzzerCorto(int cantidad) {
  for (int v = 0; v < cantidad; v++) {
    digitalWrite(D5, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(D5, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

void evaluaForzadorUpdate() {

  if (digitalRead(D6) == 0) {
    if (!boton_apretado) {
      boton_apretado = true;
      update_ini = millis();
    }
    if ((millis() - update_ini) > 3000) {
      if (!update_forzado) {
        update_forzado = true;
        sonarBuzzer(5);
        verificaActualizacionFirmware();
      }
    }

  } else {
    boton_apretado = false;
    update_forzado = false;
  }
}

void evaluaValvula(int bornera) {
  if (!digitalRead(bornera) == HIGH) {
    buzzer_ini = millis();
    valvulaEnviada = false;
  } else {
    if ((millis() - buzzer_ini) > 7000 && !valvulaEnviada) {
      enviarInfoDispo(valvulaDescarga + "1" + fin);
      valvulaEnviada = true;
    }
  }
}

void informarContacto() {
  if (enviarContacto) {
    enviarContacto = false;
    Serial.println("Envia contacto");
    if (contacto) {
      enviarInfoDispo(idContacto + "1" + fin);
    } else {
      enviarInfoDispo(idContacto + "0" + fin);
    }
  }
}

void evaluaContacto(int bornera) {
  if (digitalRead(bornera) == HIGH) {
    if (!contacto) {
      contacto = true;
      identificado = false;
      enviarContacto = true;
    }
  } else {
    if (contacto) {
      enviarContacto = true;
    }
    contacto = false;
    identificado = true;
  }
}

void evaluaAlertaIdentificado(){
  if (chkRfid[0] == '1') {
//    Serial.println("ok 1");
    if (!identificado) {
//      Serial.println("ok 2");
      if(!AlertaIdentificado_bool){
//        Serial.println("ok 3");
        if((millis() - AlertaIdentificado_ini) > AlertaIdentificado_limit){
//          Serial.println("ok 4");
          AlertaIdentificado_bool = true;
          enviarWebService("AAFFAA00000000000006FFAAFF");
        }
      }
    }
  }
}

void evaluaIdentificado() {
  if (chkRfid[0] == '1') {
    if (!identificado) {
      if (!prendido) {
        if (buzzer_ini == 0 || (millis() - buzzer_ini) > 100 ) {
          prendido = true;
          buzzer_ini = millis();
          if (digitalRead(D6) != 0) {
            digitalWrite(D5, HIGH);
            //                        Serial.println("Suena buzzer");
          }
        }
      } else {
        if ((millis() - buzzer_ini) > 100 ) {
          prendido = false;
          buzzer_ini = millis();
          digitalWrite(D5, LOW);
        }
      }
    } else {
      digitalWrite(D5, LOW);
    }
  } else {
    digitalWrite(D5, LOW);
    identificado = true;
  }
}

void detectHID() {
  if (chkRfid[0] == '0') {
    return;
  }
  if (!flagDone) {
    if (--weigand_counter == 0)
      flagDone = 1;
  }
  // if we have bits and we the weigand counter went out
  if (bitCount > 0 && flagDone) {
    unsigned char i;
    if (bitCount == 35) {
      // 35 bit HID Corporate 1000 format
      // facility code = bits 2 to 14
      for (i = 2; i < 14; i++) {
        facilityCode <<= 1;
        facilityCode |= databits[i];
      }
      // card code = bits 15 to 34
      for (i = 14; i < 34; i++) {
        cardCode <<= 1;
        cardCode |= databits[i];
      }
      printBits();
    }
    else if (bitCount == 26) {
      // standard 26 bit format
      for (i = 1; i < 9; i++) {
        facilityCode <<= 1;
        facilityCode |= databits[i];
      }
      // card code = bits 10 to 23
      for (i = 9; i < 25; i++) {
        cardCode <<= 1;
        cardCode |= databits[i];
      }
      printBits();
    }
    // cleanup and get ready for the next card
    bitCount = 0;
    facilityCode = 0;
    cardCode = 0;
    for (i = 0; i < MAX_BITS; i++)
    {
      databits[i] = 0;
    }
  }
}



void printBits() {
  // inicio imprimir en la nube
  identificado = true;
  if (printSerial) {
    Serial.println("Identificado");
  }
  String tempmsj = tipoInfo + cardCode + fin;
  if (printSerial) {
    Serial.println(tempmsj);
  }
  enviarInfoDispo(tempmsj);
}

void detectAWID() {
  bool detectInfo = false;
  if (chkAwid[0] == '0') {
    if (antenaAWID.available()) {
      int mcant = 0;
      String mensaje = "";
      while (antenaAWID.available()) {
        char ch = antenaAWID.read();
        mensaje.concat(ch);
        delay(2);
        mcant ++;
      }
      if ((mcant == 35 && mensaje.substring(0, 2) == "20") || mensaje.substring(0, 2) == "39") {
        String codigo = "";
        for (int i = 0; i < mensaje.length(); i++) {
          if (String(mensaje[i]) == ",") {
            break;
          } else {
            codigo += String(mensaje[i]);
          }
        }
        //        Serial.println("Codigo Antena:" + codigo);
        detectInfo = true;
        awidSend = codigo.substring(0,codigo.length() - 2); // saco los ultimos 2 por miedo a que tenga basura.
        OrigenAntena = infoFalcom;
      }
    }
  } else {
    if (antenaAWID.available()) {
      awidstr = "";
      int l = 0;
      while (l < 21) {
        int awid = antenaAWID.read();
        if (l == 0 && !awid) {
          awidstr = awidstr + "0";
          l++;
        }
        if (awid > 0) {
          awidstr = awidstr + String(awid);
        }
        l++;
        delay(2);
      }
      if (awidstr.length() > 25) {
        detectInfo = true;
        awidSend = awidstr;
        OrigenAntena = infoWid;
      }
      awidstr = "";
    }
  }
  // ENVIAR INFO AWID A DISPO
  if (detectInfo) {
    enviarInfoAWID(OrigenAntena);
  }
  // FIN ENVIAR INFO AWID A DISPO

}


void enviarInfoAWID(String Origen) {
  t_ultLectura =  millis();
  Serial.println(awidSend);

  if (nroAntena.length() == 0) {
    nroAntena = awidSend;
    cantLecturas = 0;
  }

  if (nroAntena != awidSend) {
    nroAntena = awidSend;
    cantLecturas = 1;
  } else {
    cantLecturas += 1;
  }
  if (cantLecturas < 5) {
    return;
  }

  if (ultimoAwid != awidSend) {
    envioDesacople = true;
    if (ultimoAwid.length() != 0) {
      String mensaje = Origen + "0" + ultimoAwid + fin;
      enviarInfoDispo(mensaje);
      //      envioDesacople = false;
    }
    String mensaje = Origen + "1" + awidSend + fin;
    enviarInfoDispo(mensaje);
  }
  ultimoAwid = awidSend;
  awidSend = "";
  //  t_ini = t_fin;
}

void enviarUltimoDesacople() {
  if (ultimoAwid.length() > 0) {
    double secs;
    t_now =  millis();
    secs = (double)(t_now - t_ultLectura) / 1000;
    if (secs > 10 && envioDesacople) {
      String mensaje = OrigenAntena + "0" + ultimoAwid + fin;
      enviarInfoDispo(mensaje);
      awidSend = "";
      ultimoAwid = "";
      nroAntena = "";
      cantLecturas = 0;
      envioDesacople = false;
    }
  }
}

void connectToWifi(int cantidadReintentos) {
  if (printSerial) {
    Serial.print("WiFi: ");
    Serial.println(ssid);
    Serial.print("Pass: ");
    Serial.println(password);
//    WiFi.begin(ssid, password);
  }
  if (cantidadReintentos == 0) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      if (printSerial) {
        Serial.println("reintentando...");
      }
      sonarBuzzer(1);
    }
  } else {
    int aux = 1;
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && aux <= cantidadReintentos) {

      if (printSerial) {
        Serial.println("reintentando... " + String(aux) + " de " + String(cantidadReintentos));
      }
      aux ++;
      destelloLed(1, 'L');
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    sonarBuzzerCorto(5);
  }
}

void enviarInfoDispo(String msj) {
  Serial.println("Enviando info a dispo msj: " + msj);
  if (msj.length() > 0) {
    sonarBuzzerCorto(2);
  }
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi(7);
  }
  if (WiFi.status() == WL_CONNECTED) {
    waitForSync(2);
  }
  Timezone Argentina;
  if (msj.length() > 0) {
    ListaMsjAdd(inicio + Argentina.dateTime("Hisdmy") + msj);
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (printSerial) {
      Serial.println("Error al conectar con WIFI");
    }
  } else {
    WiFiClient client;
    String MensjaesADispo = ListaMsjNext();
    while (MensjaesADispo.length() > 0) {
      destelloLed(1, 'C');
      if (emulation) {
        if (printSerial) {
          Serial.println("Emulation Enviando al dispo: " + MensjaesADispo);
        }
      } else {
        if (printSerial) {
          Serial.println("Enviando al dispo: " + MensjaesADispo);
        }
        if (client.connect(serverName, serverPort))
        {
          client.print(MensjaesADispo + '\n'); // enviando informacion al dispo
        } else {
          if (printSerial) {
            Serial.println("Error de Red");
          }
        }
        enviarWebService(MensjaesADispo);
      }
      MensjaesADispo = ListaMsjNext();
    }
    client.stop();
  }
}

void enviarWebService(String mensaje){
  HTTPClient http;
  if (printSerial) {
    Serial.println("Enviando a WS: " + mensaje);
  }  
  if (http.begin(urlService)) { // HTTP
    http.addHeader("Content-Type", "text/plain");
    int httpCode = http.POST("ID:" + String(ssid) + ";MENSAJE:" + mensaje + ";");
    if (printSerial) {
      Serial.println("Respuesta: " + String(httpCode));
    }

    http.end();
  }
}

//-------------------PAGINA DE CONFIGURACION--------------------
void paginaconf() {
  server.send(200, "text/html", pagina + mensaje + paginafin);
}
//--------------------MODO_CONFIGURACION------------------------
void modoconf() {
  WiFi.mode(WIFI_STA);
  IPAddress Ip(192, 168, 1, 1);
  IPAddress GateWay(192, 168, 1, 10);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, GateWay, NMask);
  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP();
  if (printSerial) {
    Serial.print("IP del acces point: ");
    Serial.println(myIP);
    Serial.println("WebServer iniciado...");
  }
  server.on("/", paginaconf); //esta es la pagina de configuracion
  server.on("/guardar_conf", guardar_conf); //Graba en la eeprom la configuracion
  server.on("/escanear", escanear); //Escanean las redes wifi disponibles
  server.on("/resetdispo", resetDispo); //Escanean las redes wifi disponibles
  server.begin();
  while (true) {
    server.handleClient();
  }
}


//---------------------GUARDAR CONFIGURACION-------------------------
void guardar_conf() {
  if (printSerial) {
    Serial.println(server.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  }
  grabar(0, server.arg("ssid"));
  if (printSerial) {
    Serial.println(server.arg("pass"));
  }
  grabar(50, server.arg("pass"));

  if (printSerial) {
    Serial.println(server.arg("empresa"));
  }
  grabar(100, server.arg("empresa"));

  mensaje = "<p>Configuracion Guardada</p>";
  sonarBuzzer(1);
  paginaconf();
}

//----------------Función para grabar en la EEPROM-------------------
void grabar(int addr, String a) {
  int tamano = a.length();
  char inchar[50];
  a.toCharArray(inchar, tamano + 1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr + i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr + i, 255);
  }
  EEPROM.commit();
}


//-----------------Función para leer la EEPROM------------------------
String leer(int addr) {
  byte lectura;
  String strlectura;
  for (int i = addr; i < addr + 50; i++) {
    lectura = EEPROM.read(i);
    if (lectura != 255) {
      strlectura += (char)lectura;
    }
  }
  return strlectura;
}
//
void resetDispo() {
  ESP.restart();
}
//---------------------------ESCANEAR----------------------------
void escanear() {
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas
  if (printSerial) {
    Serial.println("escaneo terminado");
  }
  if (n == 0) { //si no encuentra ninguna red
    if (printSerial) {
      Serial.println("no se encontraron redes");
    }
    mensaje = "no se encontraron redes";
  }
  else
  {
    if (printSerial) {
      Serial.print(n);
      Serial.println(" redes encontradas");
    }
    mensaje = "<ul style='margin: 25px;'>";
    char tempssid[50];
    for (int i = 0; i < n; ++i)
    {
      // agrega al STRING "mensaje" la información de las redes encontradas
      //mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i)+"</p>\r\n";
      WiFi.SSID(i).toCharArray(tempssid, 50);
      if (strcmp(ssid, tempssid) == 0)
      {
        mensaje = (mensaje) + "<li><button class='wifiActual' onclick='selec_wifi(this)'>" + WiFi.SSID(i) + "</button><br> </li>";
      }
      else
      {
        mensaje = (mensaje) + "<li><button class='myButtonWifi' onclick='selec_wifi(this)'>" + WiFi.SSID(i) + "</button><br> </li>";
      }

      //) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + "
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(10);
    }
    mensaje = (mensaje) + "</ul>";
    //Serial.println(mensaje);
    paginaconf();
  }
}

String obtenerHtml () {
  String pagina_html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "    <title>Concentrador dbTrust</title>"
    "    <meta charset='UTF-8'>"
    "</head>"
    "<style>"
    "    body { font: 50px sans-serif;padding: 5px; margin: 0; width: 100%; background: rgb(150, 196, 223);}"
    "    form { position: relative; width: 100%; margin: 0 auto; }"
    "    .myinput {width: 97%;font-size: 45px; margin-left: 8px; height: 45px; }"
    "    .myButton {"
    "  -moz-box-shadow: 0px 10px 14px -7px #276873;"
    "  -webkit-box-shadow: 0px 10px 14px -7px #276873;"
    "  box-shadow: 0px 10px 14px -7px #276873;"
    "  background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #599bb3), color-stop(1, #408c99));"
    "  background:-moz-linear-gradient(top, #599bb3 5%, #408c99 100%);"
    "  background:-webkit-linear-gradient(top, #599bb3 5%, #408c99 100%);"
    "  background:-o-linear-gradient(top, #599bb3 5%, #408c99 100%);"
    "  background:-ms-linear-gradient(top, #599bb3 5%, #408c99 100%);"
    "  background:linear-gradient(to bottom, #599bb3 5%, #408c99 100%);"
    "  filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#599bb3', endColorstr='#408c99',GradientType=0);"
    "  background-color:#599bb3;"
    "  -moz-border-radius:8px;"
    "  -webkit-border-radius:8px;"
    "  border-radius:8px;"
    "  display:inline-block;"
    "  cursor:pointer;"
    "  color:#ffffff;"
    "  font-family:Arial;"
    "  font-size:50px;"
    "  font-weight:bold;"
    "  padding:13px 56px;"
    "  text-decoration:none;"
    "  text-shadow:0px 1px 0px #3d768a;"
    "}"
    ".myButton:hover {"
    "  background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #408c99), color-stop(1, #599bb3));"
    "  background:-moz-linear-gradient(top, #408c99 5%, #599bb3 100%);"
    "  background:-webkit-linear-gradient(top, #408c99 5%, #599bb3 100%);"
    "  background:-o-linear-gradient(top, #408c99 5%, #599bb3 100%);"
    "  background:-ms-linear-gradient(top, #408c99 5%, #599bb3 100%);"
    "  background:linear-gradient(to bottom, #408c99 5%, #599bb3 100%);"
    "  filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#408c99', endColorstr='#599bb3',GradientType=0);"
    "  background-color:#408c99;"
    "}"
    ".myButton:active {position:relative;top:1px;}"
    ".myButtonScan {"
    "  -moz-box-shadow: 0px 10px 14px -7px #3e7327;"
    "  -webkit-box-shadow: 0px 10px 14px -7px #3e7327;"
    "  box-shadow: 0px 10px 14px -7px #3e7327;"
    "  background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #77b55a), color-stop(1, #72b352));"
    "  background:-moz-linear-gradient(top, #77b55a 5%, #72b352 100%);"
    "  background:-webkit-linear-gradient(top, #77b55a 5%, #72b352 100%);"
    "  background:-o-linear-gradient(top, #77b55a 5%, #72b352 100%);"
    "  background:-ms-linear-gradient(top, #77b55a 5%, #72b352 100%);"
    "  background:linear-gradient(to bottom, #77b55a 5%, #72b352 100%);"
    "  filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#77b55a', endColorstr='#72b352',GradientType=0);"
    "  background-color:#77b55a;"
    "  -moz-border-radius:4px;"
    "  -webkit-border-radius:4px;"
    "  border-radius:4px;"
    "  border:1px solid #4b8f29;"
    "  display:inline-block;"
    "  cursor:pointer;"
    "  color:#ffffff;"
    "  font-family:Arial;"
    "  font-size:45px;"
    "  font-weight:bold;"
    "  padding:6px 54px;"
    "  text-decoration:none;"
    "  text-shadow:0px 1px 0px #5b8a3c;"
    "}"
    ".myButtonScan:hover {"
    "  background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #72b352), color-stop(1, #77b55a));"
    "  background:-moz-linear-gradient(top, #72b352 5%, #77b55a 100%);"
    "  background:-webkit-linear-gradient(top, #72b352 5%, #77b55a 100%);"
    "  background:-o-linear-gradient(top, #72b352 5%, #77b55a 100%);"
    "  background:-ms-linear-gradient(top, #72b352 5%, #77b55a 100%);"
    "  background:linear-gradient(to bottom, #72b352 5%, #77b55a 100%);"
    "  filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#72b352', endColorstr='#77b55a',GradientType=0);"
    "  background-color:#72b352;"
    "}"
    ".myButtonScan:active {  position:relative;  top:1px;}"
    ".myButtonWifi {"
    "-moz-box-shadow:inset 0px 1px 0px 0px #ffffff;"
    "  -webkit-box-shadow:inset 0px 1px 0px 0px #ffffff;"
    "  box-shadow:inset 0px 1px 0px 0px #ffffff;"
    "  background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffffff), color-stop(1, #f6f6f6));"
    "  background:-moz-linear-gradient(top, #ffffff 5%, #f6f6f6 100%);"
    "  background:-webkit-linear-gradient(top, #ffffff 5%, #f6f6f6 100%);"
    "  background:-o-linear-gradient(top, #ffffff 5%, #f6f6f6 100%);"
    "  background:-ms-linear-gradient(top, #ffffff 5%, #f6f6f6 100%);"
    "  background:linear-gradient(to bottom, #ffffff 5%, #f6f6f6 100%);"
    "  filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffffff', endColorstr='#f6f6f6',GradientType=0);"
    "  background-color:#ffffff;"
    "  -moz-border-radius:6px;"
    "  -webkit-border-radius:6px;"
    "border-radius:6px;"
    "  border:1px solid #dcdcdc;"
    "  display:inline-block;"
    "  cursor:pointer;"
    "color:#666666;"
    "font-family:Arial;"
    "font-size:40px;"
    "font-weight:bold;"
    "padding:3px 76px;"
    "text-decoration:none;"
    "text-shadow:0px 1px 0px #ffffff;"
    "}"
    ".myButtonWifi:hover {"
    "background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #f6f6f6), color-stop(1, #ffffff));"
    "background:-moz-linear-gradient(top, #f6f6f6 5%, #ffffff 100%);"
    "background:-webkit-linear-gradient(top, #f6f6f6 5%, #ffffff 100%);"
    "background:-o-linear-gradient(top, #f6f6f6 5%, #ffffff 100%);"
    "background:-ms-linear-gradient(top, #f6f6f6 5%, #ffffff 100%);"
    "background:linear-gradient(to bottom, #f6f6f6 5%, #ffffff 100%);"
    "filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#f6f6f6', endColorstr='#ffffff',GradientType=0);"
    "background-color:#f6f6f6;"
    "}"
    ".myButtonWifi:active {  position:relative;  top:1px;}"

    ".wifiActual {"
    "-moz-box-shadow:inset 0px 1px 0px 0px #d9fbbe;"
    "-webkit-box-shadow:inset 0px 1px 0px 0px #d9fbbe;"
    "box-shadow:inset 0px 1px 0px 0px #d9fbbe;"
    "background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #b8e356), color-stop(1, #a5cc52));"
    "background:-moz-linear-gradient(top, #b8e356 5%, #a5cc52 100%);"
    "background:-webkit-linear-gradient(top, #b8e356 5%, #a5cc52 100%);"
    "background:-o-linear-gradient(top, #b8e356 5%, #a5cc52 100%);"
    "background:-ms-linear-gradient(top, #b8e356 5%, #a5cc52 100%);"
    "background:linear-gradient(to bottom, #b8e356 5%, #a5cc52 100%);"
    "filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#b8e356', endColorstr='#a5cc52',GradientType=0);"
    "background-color:#b8e356;"
    "-moz-border-radius:6px;"
    "-webkit-border-radius:6px;"
    "border-radius:6px;"
    "border:1px solid #83c41a;"
    "display:inline-block;"
    "cursor:pointer;"
    "color:#ffffff;"
    "font-family:Arial;"
    "font-size:40px;"
    "font-weight:bold;"
    "padding:3px 76px;"
    "text-decoration:none;"
    "text-shadow:0px 1px 0px #86ae47;"
    "}"
    ".wifiActual:hover {"
    "background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #a5cc52), color-stop(1, #b8e356));"
    "background:-moz-linear-gradient(top, #a5cc52 5%, #b8e356 100%);"
    "background:-webkit-linear-gradient(top, #a5cc52 5%, #b8e356 100%);"
    "background:-o-linear-gradient(top, #a5cc52 5%, #b8e356 100%);"
    "background:-ms-linear-gradient(top, #a5cc52 5%, #b8e356 100%);"
    "background:linear-gradient(to bottom, #a5cc52 5%, #b8e356 100%);"
    "filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#a5cc52', endColorstr='#b8e356',GradientType=0);"
    "background-color:#a5cc52;"
    "}"
    ".wifiActual:active {"
    "position:relative;"
    "top:1px;"
    "}    "
    "</style>"
    "<script>"
    "    function selec_wifi(obj) {"
    "        document.querySelector('#ssid').value = obj.innerText;"
    "        var ssid = obj.innerText;"
    "        if (ssid.indexOf('OBD_AP') >= 0) {"
    "            var pass = 'dbTrust01';"
    "            document.querySelector('#pass').value = pass;"
    "        } else {"
    "            document.querySelector('#pass').value = '';"
    "        }"
    "    }"
    "</script>"
    "<body>"
    "    <form action='guardar_conf' method='get' style='text-align: center;'>"
    "        <h1 style='font-size: 60px;'>Configurar Concentrador</h1>"
    "        <div style='width: 100%;'>"
    "            <div style='width: 24%;float:left; text-align: right;'>"
    "                <label style='padding-right: 8px;' for='ssid'>SSID:</label><br>"
    "                <label style='padding-right: 8px;' for='pass'>PASS:</label><br>"
    "                <label style='padding-right: 8px;' for='empresa'>EMP.:</label><br>"
    "            </div>"
    "            <div style='width: 74%;float: right; text-align: left;padding: 1%;'>"
    "                <input id='ssid' class='input1 myinput' name='ssid' type='text'><br>"
    "                <input id='pass' class='input1 myinput' name='pass' type='password'>"
    "                <select class='input1 myinput' style='height: 56px;' name='empresa'>"
    "                   <option value='dbtrust'>dbTrust</option>"
    "                   <option value='tenaris'>Tenaris</option>"
    "                   <option value='cemax'>Cemax</option>"
    "                </select>"
    "            </div>"
    "        </div>"
    "        <br>"
    "        <div>"
    "            <input class='myButton' type='submit' value='GUARDAR' />"
    "        </div>"
    "    </form>"
    "    <br>"
    "    <div>"
    "        <a href='escanear'><button class='myButtonScan'>ESCANEAR REDES WI-FI</button></a><br>";
  return pagina_html;
}


String charToString(char* varChar) {

  String stringAuxChar(varChar);
  String stringAux = "";

  for (int s = 0; s < stringAuxChar.length(); s++) {
    stringAux += stringAuxChar[s];
  }
  return stringAux;
}


void verificaConfiguracion() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi(5);
  }
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  boolean banderaCambios = false;
  String macDispo = WiFi.BSSIDstr();
  HTTPClient http;
  Serial.println("URL: " + urlServer + "test/configuracion/obtener_parametros/" + WiFi.macAddress() + "/" + macDispo);
  if (http.begin(urlServer + "test/configuracion/obtener_parametros/" + WiFi.macAddress() + "/" + macDispo)) { // HTTP
    if (printSerial) {
      Serial.println("Buscando Configuraciones...");
    }
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();

        const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& root = jsonBuffer.parseObject(payload);
        // Parameters

        const char* haycambios = root["cambios"]; // "Leanne Graham"
        const char* _empresa = root["empresaSol"];
        const char* _chkAwid = root["chkAwid"];
        const char* _chkRfid = root["chkRfid"];
        const char* _bornera1 = root["comportamientoBornera1"];
        const char* _emulation = root["emulation"];
        const char* _minutesupdate = root["minutesupdate"];
        if (haycambios[0] == '1') {
          banderaCambios = true;
          grabar(100, _empresa);
          grabar(150, _chkAwid);
          grabar(200, _chkRfid);
          grabar(250, _bornera1);
          grabar(300, _emulation);
          grabar(350, _minutesupdate);
          if (printSerial) {
            Serial.println("Datos Guardados");
            Serial.print("empresa: ");
            Serial.println(_empresa);
            Serial.print("chkAwid: ");
            Serial.println(_chkAwid);
            Serial.print("chkRfid: ");
            Serial.println(_chkRfid);
            Serial.print("Bornera1: ");
            Serial.println(_bornera1);
            Serial.print("Emulation Mode: ");
            Serial.println(_emulation);
            Serial.print("Minute update: ");
            Serial.println(_minutesupdate);
          }
        } else {
          if (printSerial) {
            Serial.println("No hay cambios pendientes");
          }
        }
      }
    } else {
      if (printSerial) {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    }
  } else {
    if (printSerial) {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  http.end();
  if (banderaCambios) {
    leer(100).toCharArray(empresa, 50);
    leer(150).toCharArray(chkAwid, 50);
    leer(200).toCharArray(chkRfid, 50);
    leer(250).toCharArray(bornera1, 50);
    comportamientoBornera1 = charToString(bornera1);
    if (comportamientoBornera1.length() == 0) {
      grabar(250, "03");
      comportamientoBornera1 = "03";
    }
    leer(300).toCharArray(emulationMode, 50);
    auxEmulation = charToString(emulationMode);
    if (auxEmulation.length() == 0) {
      grabar(300, "0");
      auxEmulation = "0";
    }
    if (auxEmulation == "1") {
      emulation = true;
    } else {
      emulation = false;
    }

    leer(350).toCharArray(MinutesBeforeUpdate, 50);
    if (String(MinutesBeforeUpdate[0]).toInt() == 0) {
      grabar(350, "5");
      leer(350).toCharArray(MinutesBeforeUpdate, 50);
    }
  }
}

void verificaActualizacionFirmware() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi(5);
  }
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (printSerial) {
    Serial.println("Chequeando Update");
  }
  String stringSsid(ssid);
  String stringAuxSSID = "";

  for (int s = 0; s < stringSsid.length(); s++) {
    if (!isSpace(stringSsid[s])) {
      stringAuxSSID += stringSsid[s];
    }
  }

  String stringEmpresa(empresa);
  String stringAuxEmpresa = "";

  for (int s = 0; s < stringEmpresa.length(); s++) {
    if (!isSpace(stringEmpresa[s])) {
      stringAuxEmpresa += stringEmpresa[s];
    }
  }
  if (stringAuxEmpresa.length() == 0) {
    stringAuxEmpresa = "dbtrust";
  }

  Serial.println("URL: " + urlServer + stringAuxEmpresa + "/" + stringAuxSSID);
  t_httpUpdate_return ret = ESPhttpUpdate.update(urlServer + stringAuxEmpresa + "/" + stringAuxSSID);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }

}

void ListaMsjAdd(String msj) {
  for (int i = 0; i < 50; i ++) {
    if (listaMensajes[i] .length() == 0) {
      listaMensajes[i] = msj;
      break;
    }
  }
}

String ListaMsjNext() {
  String msj = "";
  for (int i = 0; i < 50; i ++) {
    if (listaMensajes[i].length() > 0) {
      msj = listaMensajes[i];
      listaMensajes[i] = "";
      break;
    }
  }
  return msj;
}
