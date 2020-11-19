///////////////////////////////////////////////////////////////////////////
//  SkyNando-WiFi
//
//  Modificación de firmware del Bridge de RoboRemo para crear una comunicación entre la montura del telescopio EQ6/HEQ5 y el PC o teléfono móvil.
//  Se han introducido pequeños cambios para mejorar el funcionamiento del programa original (básicamente responer a la dirección + puerto UDP origen, llamadas y parámetros de funciones y manejo de los buffers)
//  
//  Se elimina la compilación condicional, se configura como estación de Acceso (AP) y protocolo UDP.
//  El programa se transfiere a un ESP-01S
//  
//  Modificación by Nandorroloco (2020)
//
///////////////////////////////////////////////////////////////////////
//
// Disclaimer: Don't use this code for life support systems
// or any other situations where system failure may affect
// user or environmental safety.
//
// 

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/////////////////////////////////////////////////////////////

#define UART_BAUD 9600
#define packTimeout 2               // 2 ms es suficiente
#define bufferSize (UDP_TX_PACKET_MAX_SIZE + 1)  // ajustado a la definición

WiFiUDP udp;

//////////   Wifi config

char ssid[] = "SkyNando-WiFi";      // You will connect your phone to this Access Point
char pw[] = "12345678";           // and this is the password (must have eight characters) 
//char pw[] = "";                     // ... without encription "open"
#define canal  4                    // Usamos otro canal que no sea el 1
#define oculto 0                    // no ocultamos la red  (hidden false)
#define max_conn 4                  // maximo de conexiones podría llegar hasta 8

IPAddress AP_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress netmask(255,255,255,0);

const int port = 11880;                 // udp this port

//////////////////////////////////////////////////////////////////////////

// #define DEBUG_DIY            // Sólo sirve cuando el ESP-01s está en el programador, no activar el debug conectado a la montura.

unsigned int remoteport;
IPAddress remoteIp;

uint8_t buf1[bufferSize];
uint16_t i1=0;

uint8_t buf2[bufferSize];
uint16_t i2=0;

///////////////////////  inicialización

void setup() {

  delay(500);
  
  Serial.begin(UART_BAUD);   // UART: 9600bps

  Serial.println();
  Serial.print("SSID: ");
  Serial.println(ssid);  
  Serial.print("@ip device: ");
  Serial.println(AP_ip);
  Serial.print("Netmask: ");
  Serial.println(netmask);
 
  Serial.println( WiFi.softAPConfig(AP_ip, gateway, netmask)? "Ok APconfig": "KO APConfig" ); // configure ip address for softAP 
//  Serial.println( WiFi.softAP(ssid) ? "Ok SoftAP": "KO SoftAP" ); // configure ssid for softAP witout encription.
//  Serial.println( WiFi.softAP(ssid, pw) ? "Ok SoftAP": "KO SoftAP" ); // configure ssid and password for softAP
  Serial.println( WiFi.softAP(ssid, pw, canal, oculto, max_conn ) ? "Ok SoftAP": "KO SoftAP" ); // configure softAP all parameters
  Serial.print("@IP Soft-AP = ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Starting UDP Server");
  Serial.print("port: ");
  Serial.println(port);
  udp.begin(port); // start UDP server 
}


void loop() {

  // if there’s data available, read a packet
  int packetSize = udp.parsePacket();
  if(packetSize>0) 
  {
    remoteIp = udp.remoteIP(); // store the ip of the remote device
    remoteport = udp.remotePort();    // añadido se debe contestar en el mismo puerto que ha habido el envío
#ifdef DEBUG_DIY
  Serial.println();
  Serial.print("remoteport: ");
  Serial.println(remoteport);
#endif
    i1 = udp.read(buf1, bufferSize);
    buf1[i1]= 0x00;                   // terminador, centinela
    Serial.write(buf1, i1);           // El tamaño de paquete no tiene porqué ser igual a los bytes leidos de mensaje
  }

  if(Serial.available()) {                        /////////// begin Serial
    while(1) {
      if(Serial.available()) 
      {
        buf2[i2] = (char)Serial.read(); // read char from UART
        if( i2 < UDP_TX_PACKET_MAX_SIZE ) {       // si hay más carácteres que el tamaño del buffer, se escribirá en la última posición. Se evita hacer cálculos en la comparación.
          i2++;
        }
      } 
      else
      { 
        delay(packTimeout);                       // espera 2 ms
        if(!Serial.available()) break;            // salida del bucle de lectura 
      }
    }
    // now send to WiFi:  
    udp.beginPacket(remoteIp, remoteport);        // remote IP and remoteport  (Pueden haber varios dispositivos interactuando con la montura
    buf2[i2] = 0x00;                              // terminador, centinela  (útil por si se escribe por el puerto serie)
    udp.write(buf2, i2);
    udp.endPacket();
#ifdef DEBUG_DIY
  Serial.println("Enviando"); 
  Serial.print("IP = ");
  Serial.println(remoteIp);
  Serial.print("out->");
  Serial.println((char*)buf2);
#endif

  i2 = 0;
  }                                               ////////// end Serial

}
