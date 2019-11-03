#define TINY_GSM_MODEM_SIM808
#define SerialMon Serial
#define SerialAT Serial1

#include <SoftwareSerial.h>
SoftwareSerial SerialAT(11,10); // RX, TX

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
//#define LOGGING  // <- Logging is for the HTTP library

// Range to attempt to autobaud
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Add a reception delay - may be needed for a fast processor at a slow baud rate
//#define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "Robi";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[]  = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// Server details
const char server[] = "arduino-sim808.free.beeceptor.com";
const char resource[] = "/api/v1/data";
const int  port = 80;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>



#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
HttpClient http(client, server, port);

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  SerialMon.println("Wait...");

  SerialAT.begin(9600);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  
  while(sendCommand("AT", "OK\r\n")==0);
  while(sendCommand("AT+CGPSPWR=1", "OK\r\n")==0);
  while(sendCommand("AT+CGPSRST=0", "OK\r\n")==0);
}

void loop() {
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected()) {
      SerialMon.println("GPRS connected");
    }
#endif

  SerialMon.print(F("Performing HTTP GET request... "));
  int err = http.post(resource, "application/x-www-form-urlencoded", "name=Alice&age=12");
  if (err != 0) {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  SerialMon.print(F("Body length is: "));
  SerialMon.println(body.length());

  // Shutdown

  http.stop();
  SerialMon.println(F("Server disconnected"));


#if TINY_GSM_USE_GPRS
    modem.gprsDisconnect();
    SerialMon.println(F("GPRS disconnected"));
#endif

  // Do nothing forevermore
  while (true) {
    delay(1000);
  }
}

int sendCommand(String command, String expected)
{
  int x =0, cnt = 0, paisi = 0;
  char output[100];
  memset(output, '\0', 100);
  SerialAT.println(command);
  delay(500);
  while (SerialAT.available()) 
  {
    output[x] = SerialAT.read();
    if(output[x] == '\n' && paisi == 0) {
      cnt = x+1;
      paisi = 1;
    }
    SerialMon.write(output[x]);//Forward what Serial received to Software Serial Port
    x++;
  }
  String out = String(output+cnt);
  if(expected.equals(out)){
    SerialMon.println("mila gese!");
    return 1;
  } 
  else 
  {
    SerialMon.print("Output: ");
    SerialMon.println(out);
    SerialMon.print("Expected: ");
    SerialMon.println(expected);
  }
  return 0;
}
