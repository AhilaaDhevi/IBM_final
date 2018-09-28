int button1 = 2; // push button is connected D4 -> GPIO2
int count = 0;
int automatic = 0;    // Automatic mode
int gas;

#include <SPI.h>                                           
#include <Wire.h>  
#include <ESP8266WiFi.h>                                        
#include <Adafruit_GFX.h>                                  
#include <Adafruit_SSD1306.h>
#define OLED_RESET LED_BUILTIN 
Adafruit_SSD1306 display(OLED_RESET);                      
                                                           
#if (SSD1306_LCDHEIGHT != 32)                              
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define TINY_GSM_MODEM_SIM808
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
SoftwareSerial SerialAT(14, 12); // RX, TX  14-D5 12-D6
//GPS settings
String latlongtab[5];
#define DEBUG true
String state, timegps, latitude, longitude; 
// Your GPRS credentials
const char apn[]  = "www";
const char user[] = "";
const char pass[] = "";

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

TinyGsmClient gsmclient(modem);

#define ORG "3r90l2" // "quickstart" or use your organisation
#define DEVICE_ID "nodemcu-01"      
#define DEVICE_TYPE "esp8266" // your device type or not used for "quickstart"
#define TOKEN "nodemcu123" // your device token or not used for "quickstart"
//-------- Customise the above values --------

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[] = "iot-2/evt/status/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;
WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);


                                       
void setup() {
  Serial.begin(115200);
  pinMode(button1, INPUT); // declare push button as input
  delay(10);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
    // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(10);
  sendData("AT+CGNSPWR=1",1000,DEBUG);       //Initialize GPS device
  delay(50);
  sendData("AT+CGNSSEQ=RMC",1000,DEBUG);
  delay(150);
  Serial.println(F("Initializing modem..."));
        modem.init();

        String modemInfo = modem.getModemInfo();
        Serial.print(F("Modem: "));
        Serial.println(modemInfo);
        Serial.print(F("Waiting for network..."));
        if (!modem.waitForNetwork()) 
        {
        Serial.println(" fail");
        delay(10000);
        return;
        }
        Serial.println(" OK");
        Serial.print(F("Connecting to "));
        Serial.print(apn);
        if (!modem.gprsConnect(apn, user, pass)) 
        {
        Serial.println(" fail");
        delay(10000);
        return;
        }
        Serial.println(" OK");
        
         display.clearDisplay();
  display.setCursor(0,0);
  String command1 = "SELECT MODE";
  display.println(command1);
  display.display();
  delay(5000);
}

void loop() {
       
       automatic = digitalRead(button1);
       Serial.println(automatic);
       
     if (automatic == HIGH || count == 1) 
     {
      count=1;
      Serial.println(automatic);
      Serial.print("Automatic Mode");
      display.clearDisplay();
      display.setCursor(0,0);
      String command = "AUTOMATIC MODE";
      display.println(command);
      display.display();
      sendtoIBMcloud();
      delay (1000);
      }   
}

void sendTabData(String command, const int timeout, boolean debug){
 
    SerialAT.println(command); 
    long int time = millis();
    int i = 0;   
    
    while((time+timeout) > millis()){
      while(SerialAT.available()){       
        char c = SerialAT.read();
        if(c != ','){ //read characters until you find comma, if found increment
          latlongtab[i]+=c;
          delay(100);
        }else{
          i++;        
        }
        if(i == 5){
          delay(100);
          goto exitL;
        }       
      }    
    }exitL:    
    if(debug){ 
      /*or you just can return whole table,in case if its not global*/ 
      state = latlongtab[1];     //state = recieving data - 1, not recieving - 0
      timegps = latlongtab[2];
      latitude = latlongtab[3];  //latitude
      longitude = latlongtab[4]; //longitude
    }    
}
String sendData(String command, const int timeout, boolean debug){
  
    String response = "";    
    SerialAT.println(command); 
    long int time = millis();
    int i = 0;  
     
    while( (time+timeout) > millis()){
      while(SerialAT.available()){       
        char c = SerialAT.read();
        response+=c;
      }  
    }    
    if(debug){
      Serial.print(response);
    }    
    return response;
}

void sendtoIBMcloud()
{ 
        if (!!!client.connected()) 
        {
        Serial.print("Reconnecting client to "); Serial.println(server);
        while ( ! (ORG == "quickstart" ? client.connect(clientId) : client.connect(clientId, authMethod, token))) {
        Serial.print(".");
        delay(500);
        }
        Serial.println();
        }

          sendTabData("AT+CGNSINF",1000,DEBUG);    //send demand of gps localization
          if(state != 0){
          Serial.println("State: "+state+" Time: "+timegps+"  Latitude: "+latitude+" Longitude "+longitude);
          for(int j=0;j<5;j++)
         {
          latlongtab[j] = "\0";
         }
         }else{
         Serial.println("GPS initializing");
         }
        
        gas=digitalRead(13);//Read Gas value from digital pin D7 -> GPIO13
        Serial.println(gas);//Print the value to serial port
        delay(500);

        if (gas == 1)
        {
          display.clearDisplay();
          display.setCursor(0,0);
          String command = "AUTOMATIC MODE";
          String command2 = "LPG not detected";
          //display.println(command);
          display.println(command2);
          display.println(latitude);
          display.println(longitude);
          display.display();
        }
        else if (gas == 0)
        {
         display.clearDisplay();
          display.setCursor(0,0);
          String command = "AUTOMATIC MODE";
          String command2 = "LPG detected!!!";
          //display.println(command);
          display.println(command2);
          display.println(latitude);
          display.println(longitude);
          display.display(); 
        }
        String payload = "{\"d\":{\"lat\":";
        payload += latitude;
        payload += ",\"lon\":";
        payload += longitude;
        payload += ",\"gas\":";
        payload += gas;
        payload += "}}";
  
        Serial.print("Sending payload: "); Serial.println(payload);
    
        if (client.publish(topic, (char*) payload.c_str())) {
        Serial.println("Publish ok");
        } else {
        Serial.println("Publish failed");
        }

       // Do nothing forevermore
        
          delay(5000);
      
     
}



