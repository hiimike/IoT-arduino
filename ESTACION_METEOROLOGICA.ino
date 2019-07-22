#include <SFE_BMP180.h>
#include <Wire.h>
#include <DHT.h>
#include <SPI.h>
//WIFI Interface
#include <SoftwareSerial.h>
#define RX 10
#define TX 11
//Se declara una instancia de la librería
DHT dht(2, DHT11); // Inicializamos el sensor DHT11
SFE_BMP180 pressure;
//Se declaran las variables. Es necesario tomar en cuenta una presión inicial
//esta será la presión que se tome en cuenta en el cálculo de la diferencia de altura
double PresionBase;
//Leeremos presión y temperatura. Calcularemos la diferencia de altura
double Presion = 0;
double Altura = 0;
double Temperatura = 0;
char status;
float humedad=0;
float h=0;
float t=0;
float f=0;
int UVOUT = A5; //Output from the sensor
int REF_3V3 = A4; //3.3V power on the Arduino board

//anemometro
float veloc1= 0;// entrada A0
int tiempo=0;
int cnt=0;
float v1=0;
float v2=0;

/*** Configuracion del host y wifi TX y RX **/

String AP = "";       // MODIFICAR RED WIFI
String PASS = ""; // MODIFICAR  CLAVE
String HOST = "";//HOST IP
String PORT = "80";//HOST PORT
int countTrueCommand;
int countTimeCommand; 
boolean found = false; 
SoftwareSerial esp8266(RX,TX); //indicar puertos seriales rx y tx en la placa
char buf1[25];
/******/

void setup() {
Serial.begin(9600);
Wire.begin();
analogReference(INTERNAL);// pone como referencia interna 1.1V
esp8266.begin(115200);//MODULO WIFI
dht.begin();// Comenzamos el sensor DHT
//Se inicia el sensor y se hace una lectura inicial
SensorStart();
//Serial.println("Estableciendo comunicacion con los Sensores");
sendCommand("AT",5,"OK");
sendCommand("AT+CWMODE=1",5,"OK");
sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");
}
void loop() {
 v1 =(analogRead(2)); // lectura de sensor a2
   veloc1= (v1*0.190); // 0,190 corresponde a la pendiente de la curva aca deben poner el numero que calcularon
 delay(1000); 

  
//Se hace lectura del sensor
ReadSensor();
int uvLevel = averageAnalogRead(UVOUT);
int refLevel = averageAnalogRead(REF_3V3);
 //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
 float outputVoltage = 3.3 / refLevel * uvLevel;  
 float uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);
humedad = dht.readHumidity();
h = dht.readHumidity();
t = dht.readTemperature();
f = dht.readTemperature(true);

float hif = dht.computeHeatIndex(f, h);
float hic = dht.computeHeatIndex(t, h, false);

//*** enviar datos a la pagina web por medio del modulo wifi *//
String getData = "GET /PostData?place_id=2&humedad="+String(h)+"&temperatura="+String(t)+"&sens_term="+String(hic)+"&presion="+String(Presion)+"&altura="+String(Altura)+"&viento="+String(veloc1)+"&ra_solar="+String(uvIntensity);
 Serial.print(getData);
 sendCommand("AT+CIPMUX=1",1,"OK");
 sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,1,"OK");
 sendCommand("AT+CIPSEND=0," +String(getData.length()+4),1,">");
 esp8266.println(getData);delay(1000);countTrueCommand++;
 sendCommand("AT+CIPCLOSE=0",1,"OK");
//Se imprimen las variables
/*Serial.println("\n ////// ");
Serial.print("\nTemperatura: ");
Serial.print(t);
Serial.print("°C / Humedad: ");
Serial.print(h);
Serial.print("% / Sensacion termica ");
Serial.print(hic);
Serial.print("°C ");
Serial.print(hif);
Serial.println("°F ");
Serial.print("\nPresion Atmosferica: ");
Serial.print(Presion);
Serial.println(" milibares ");
Serial.print("\nAltura relativa: ");
Serial.print(Altura);
Serial.println(" metros");
Serial.print("\nUV Salida: ");
Serial.print(uvLevel);
Serial.print(" UV voltaje: ");
Serial.print(outputVoltage);
Serial.print(" UV Intensidad(mW/cm^2): ");
Serial.print(uvIntensity);  
Serial.println(); 
Serial.print("\nAnemometro:");
Serial.print(veloc1);  //muestra la velocidad del viento en el LCD
Serial.print("Km/h");
Serial.println("\n\n ////// ");

delay(10000);*/
//Cada 10 segundos hará una nueva lectura
}

int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 
  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;
  return(runningValue);  
}
//The Arduino Map function but for floats
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

}
void SensorStart() {
//Secuencia de inicio del sensor
pressure.begin();
/*if (pressure.begin())
Serial.println("BMP180 init success");
else
{
Serial.println("BMP180 init fail (disconnected?)\n\n");
while (1);
}*/
//Se inicia la lectura de temperatura
status = pressure.startTemperature();
if (status != 0)  {
delay(status);
//Se lee una temperatura inicial
status = pressure.getTemperature(Temperatura);
if (status != 0)    {
//Se inicia la lectura de presiones
status = pressure.startPressure(3);
if (status != 0)     
{
delay(status);
//Se lee la presión inicial incidente sobre el sensor en la primera ejecución
status = pressure.getPressure(PresionBase, Temperatura);
}
}
}
}
void ReadSensor() {
//En este método se hacen las lecturas de presión y temperatura y se calcula la altura
//Se inicia la lectura de temperatura
status = pressure.startTemperature();
if (status != 0)
{
delay(status);
//Se realiza la lectura de temperatura
status = pressure.getTemperature(Temperatura);
if (status != 0)
{
//Se inicia la lectura de presión
status = pressure.startPressure(3);
if (status != 0)
{
delay(status);
//Se lleva a cabo la lectura de presión,</span>
//considerando la temperatura que afecta el desempeño del sensor</span>
status = pressure.getPressure(Presion, Temperatura);
if (status != 0)
{
//Cálculo de la altura en base a la presión leída en el Setup
Altura = pressure.altitude(Presion, PresionBase);
}
else Serial.println("Error en la lectura de presion\n");
}
else Serial.println("Error iniciando la lectura de presion\n");
}
else Serial.println("Error en la lectura de temperatura\n");
}
else Serial.println("Error iniciando la lectura de temperatura\n");
}


/*** ENVIAR COMANDOS AL MODULO WIFI ***/
void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }
  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("OYI");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  
  found = false;
 }
