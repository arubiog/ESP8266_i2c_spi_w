/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will provide temperature reading SPI communication
 *  Daisy chain two SPI devices MAX31855 Thermocouple sensor
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */ 
#include <Wire.h>  
#include <SPI.h> 
#include <ESP8266WiFi.h>



int val[14];
#define i2cT0 0x48
#define i2cT1 0x49
#define i2cT2 0x4A
#define i2cT3 0x4B
#define i2cT4 0x4C
#define i2cT5 0x4D

int sclpin = 14;    // select the clock pin for the Temerature
int misopin=12;
int cs1pin=13;
int cs2pin=0;
int in1pin=A0;
int buzzpin=2;
int ledpin=16;
int TPin = 4;    // select the input pin for the Temerature
int ClockPin = 5; // select the input pin for the Temerature

int offset=0;
int TempI=-1;
int TempE=-2;
int Adc=-1;
int buzz_value=0;
int led_value=0;
int Temp0=-2;
int Temp1=-2;
int Temp2=-2;
int Temp3=-2;
int Temp4=-2;
int Temp5=-2;



IPAddress espIP;
char espIPadr [10];
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

// FUNCTIONS
int I2Ctemp(int ad,int delay1) {   //reemplazar por la direccion en hex o Dec
  int temperature = 0;      
  Wire.begin(); // join i2c bus (address optional for master)
  Wire.beginTransmission(ad);
  //Wire.write(0x00); // 
  //Wire.write(0x20); // CTRL_REG1 (20h)
  Wire.requestFrom(ad, 2);
  temperature = Wire.read();
  Wire.endTransmission();
  delay(delay1);
  return(temperature);  
}

int spiRead(int cspin)   {
  int value = -5;
  digitalWrite(cspin,LOW);
  delay(2);
  digitalWrite(cspin,HIGH);
  delay(220);
  /* Read the chip and return the raw temperature value */
  /* Bring CS pin low to allow us to read the data from
   the conversion process */
  digitalWrite(cspin,LOW);
  /* Cycle the clock for dummy bit 15 */
  digitalWrite(sclpin,HIGH);
  delay(1);
  digitalWrite(sclpin,LOW);
  /*
   Read bits 14-3 from MAX6675 for the Temp. Loop for each bit reading
   the value and storing the final value in 'temp'
   */
  for (int i=30; i>=17; i--) {
    digitalWrite(sclpin,HIGH);
    value += digitalRead(misopin) << i;
    val[i]= digitalRead(misopin);
    //Serial.println(value,BIN);
    digitalWrite(sclpin,LOW);
    Serial.print (val[i]) ;
  }
  digitalWrite(cspin,HIGH);
  // check bit D2 if HIGH no sensor
  if ((value & 0x04) == 0x04){ Serial.println("VCC"); return -1;}
  if ((value & 0x02) == 0x02){ Serial.println("GND"); return -1;}
  if ((value & 0x01) == 0x01){ Serial.println("OPEN CIRCUIT"); return -1;}
  // shift right three places
  //for ( int j = 32; j>=0; j-- ) // output each array element's value 
  //{
  //    Serial.print (val[j],BIN) ;
  // } 
  //Serial.println(value);
  //Serial.print ("next") ;
  return value >> 3;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  // prepare GPIO
  pinMode(buzzpin, OUTPUT);
  digitalWrite(buzzpin, buzz_value);
  pinMode(ledpin, OUTPUT);
  digitalWrite(ledpin, led_value);
  //SPI
  pinMode(misopin, INPUT);
  pinMode(sclpin, OUTPUT);
  pinMode(cs1pin, OUTPUT);
  pinMode(cs2pin, OUTPUT);

  //i2c
  Wire.begin(); // join i2c bus (address optional for master)
  Wire.beginTransmission(i2cT0);
  Wire.beginTransmission(i2cT1);
  Wire.beginTransmission(i2cT2);
  Wire.beginTransmission(i2cT3);
  Wire.beginTransmission(i2cT4);
  Wire.beginTransmission(i2cT5);
  
  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledpin, false);
    delay(500);
    digitalWrite(ledpin, true);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // Start the server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  espIP=WiFi.localIP();
  Serial.println(espIP);
  Serial.println(WiFi.BSSIDstr());
  Serial.println(WiFi.softAPmacAddress()); 
}


void loop() {
  Temp0 = I2Ctemp(i2cT0, 100);   // read the value from the sensor:
  Temp1 = I2Ctemp(i2cT1, 100);   // read the value from the sensor:
  Temp2 = I2Ctemp(i2cT2, 100);   // read the value from the sensor:
  Temp3 = I2Ctemp(i2cT3, 100);   // read the value from the sensor:
  Temp4 = I2Ctemp(i2cT4, 100);   // read the value from the sensor:
  Temp5 = I2Ctemp(i2cT5, 100);   // read the value from the sensor:
  
  TempI=(spiRead(cs1pin)*0.25);
  TempE=(spiRead(cs2pin)*0.25);
  delay(1000);
  Adc = analogRead(in1pin);
  delay(20);   
  // Set GPIO2 according to the request
  digitalWrite(buzzpin, buzz_value);
  digitalWrite(ledpin, led_value);
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) return;
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()) Serial.println("wait");
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  // Match the request
  if (req.indexOf("/buzz0") != -1) {buzz_value=1;} //buzz
  else if (req.indexOf("/buzz1") != -1) {buzz_value=0;}
  else if (req.indexOf("/led0") != -1) {led_value=0;}
  else if (req.indexOf("/led1") != -1) {led_value=1;}
  else if (req.indexOf("/") != -1) {delay(10);} //read
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
  client.flush();
  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>";
  s += " IP     : "; s += (espIP);
  s += "<br/>";
  s += " Temp I : ";s += (TempI+offset);
  s += "<br/>";
  s += " Temp E : ";s += (TempE+offset);
  s += "<br/>";
  s += " Temp I2C : ";
  s += (Temp0);
  s += " / ";
  s += (Temp1);
  s += " / ";
  s += (Temp2);
  s += " / ";
  s += (Temp3);
  s += " / ";
  s += (Temp4);
  s += " / ";
  s += (Temp5);
  s += "<br/>";
  
  s += " ADC    : ";s += (Adc);
  s += "<br/>";
  s += " BUZZ   : "; s += (buzz_value);
  s += "<br/>";
  s += " LED    : "; s += (led_value);
  s += "     : "; s += (cs1pin);
  s += "<br/>";
  s += "</html>\n";
  // Send the response to the client
  client.print(s);
}
