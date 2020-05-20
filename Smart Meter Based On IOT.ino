#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <FirebaseArduino.h>

#define FIREBASE_HOST "future-e0a51.firebaseio.com"
#define FIREBASE_AUTH "ejAoUBup3OHCTu1rvX6bRlYXXhVk8PFUfJMisHPv"
#include "DHT.h"

#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
#define OLED_RESET -1  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

BlynkTimer timer;

#define WIFI_SSID "Corona Virus"                                             // input your home or public wifi name 
#define WIFI_PASSWORD "Welcometoiot" 
int led1 = D3; 
int fireStatus;  
const int Sensor_Pin = A0;
unsigned int Sensitivity = 185;   // 185mV/A for 5A, 100 mV/A for 20A and 66mV/A for 30A Module
float Vpp = 0; // peak-peak voltage 
float Vrms = 0; // rms voltage
float Irms = 0; // rms current
float Supply_Voltage = 233.0;           // reading from DMM
float Vcc = 5.0;         // ADC reference voltage // voltage at 5V pin 
float power = 0;         // power in watt              
float Wh =0 ;             // Energy in kWh
unsigned long last_time =0;
unsigned long current_time =0;
unsigned long interval = 100;
unsigned int calibration = 100;  // V2 slider calibrates this
unsigned int pF = 85;           // Power Factor default 95
float bill_amount = 0;   // 30 day cost as present energy usage incl approx PF 
unsigned int energyTariff = 28.0; // Energy cost in INR per unit (kWh)

void getACS712() {  // for AC
  Vpp = getVPP();
  Vrms = (Vpp/2.0) *0.707; 
  Vrms = Vrms - (calibration / 10000.0);     // calibtrate to zero with slider
  Irms = (Vrms * 1000)/Sensitivity ;
  if((Irms > -0.015) && (Irms < 0.008)){  // remove low end chatter
    Irms = 0.0;
  }
  power= (Supply_Voltage * Irms) * (pF / 100.0); 
  last_time = current_time;
  current_time = millis();    
  Wh = Wh+  power *(( current_time -last_time) /3600000.0) ; // calculating energy in Watt-Hour
  bill_amount = Wh * (energyTariff/1000);
  Serial.print("Irms:  "); 
  Serial.print(String(Irms, 3));
  Serial.println(" A");
  Serial.print("Power: ");   
  Serial.print(String(power, 3)); 
  Serial.println(" W"); 
  Serial.print("  Bill Amount: INR"); 
  Serial.println(String(bill_amount, 2));
}

float getVPP()
{
  float result; 
  int readValue;                
  int maxValue = 0;             
  int minValue = 1024;          
  uint32_t start_time = millis();
  while((millis()-start_time) < 950) //read every 0.95 Sec
  {
     readValue = analogRead(Sensor_Pin);    
     if (readValue > maxValue) 
     {         
         maxValue = readValue; 
     }
     if (readValue < minValue) 
     {          
         minValue = readValue;
     }
  } 
   result = ((maxValue - minValue) * Vcc) / 1024.0;  
   return result;
 }

void displaydata() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10, 3);
  display.println("Pow  : "); 
  display.setCursor(50, 3);  
  display.println(power);
  display.setCursor(85, 3);
  display.println("W"); 
  
  display.setCursor(10, 13);
  display.println("Ener : ");  
  display.setCursor(50, 13);
  display.println(Wh);
  display.setCursor(85, 13);
  display.println("Wh");

  display.setCursor(10, 23);
  display.println("Bill : "); 
  display.setCursor(50, 23);
  display.println(bill_amount);
  display.setCursor(85, 23);
  display.println("INR");
  display.display();
  
  String fireBill = String(Wh);
  String fireConsumption = String(bill_amount);
  
  Firebase.setString("Bill_Amount", fireConsumption);
  Firebase.setString("Consumption", fireBill);
}


void setup() {
  display.begin();   
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.begin(115200); 
  Serial.println("\n Rebooted");
  WiFi.mode(WIFI_STA);
  timer.setInterval(2000L, getACS712); // get data every 2s
  pinMode(LED_BUILTIN, OUTPUT);      
  pinMode(led1, OUTPUT);  
  dht.begin(); 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.setInt("led1", 0); 
}

void loop() {

  displaydata();
  timer.run();
  displaydata();
  fireStatus = Firebase.getInt("led1");                                      // get ld status input from firebase
  if (fireStatus == 1) {                                                          // compare the input of led status received from firebase
    //Serial.println("Led Turned ON");                         
    digitalWrite(LED_BUILTIN, LOW);                                                  // make bultin led ON
    digitalWrite(led1, HIGH);                                                         // make external led ON
  } 
  else if (fireStatus == 0) {                                                  // compare the input of led status received from firebase
    //Serial.println("Led Turned OFF");
    digitalWrite(LED_BUILTIN, HIGH);                                               // make bultin led OFF
    digitalWrite(led1, LOW);                                                         // make external led OFF
  }
  else {
    Serial.println("Wrong Credential! Please send ON/OFF");
  }
  
  float Humidity = dht.readHumidity();                                              // Reading temperature or humidity takes about 250 milliseconds!
  float Temperature = dht.readTemperature();                                           // Read temperature as Celsius (the default)
    
   if (isnan(Humidity) || isnan(Temperature)) {                                                // Check if any reads failed and exit early (to try again).
    //Serial.println(F("Failed to read from DHT sensor!"));
    return;
   }
    String fireTemperature = String(Temperature);
    String fireHumidity = String(Humidity);
  
    Firebase.setString("Temperature", fireTemperature);
    Firebase.setString("Humidity", fireHumidity );

}

