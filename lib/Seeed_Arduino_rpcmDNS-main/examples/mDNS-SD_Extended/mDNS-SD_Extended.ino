/*
  ESP8266 mDNS-SD responder and query sample

  This is an example of announcing and finding services.
  
  Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash the sketch to two ESP8266 boards
  - The last one powered on should now find the other.
 */

#include <rpcWiFi.h>
#include <RPCmDNS.h>

const char* ssid     = "yourNetwork";
const char* password = "yourPassword";

void setup() {
    Serial.begin(115200);
    while(!Serial){

    }

    Serial.printf("Setup Start\r\n");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print(".");
        WiFi.begin(ssid, password);
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (!MDNS.begin("WioTerminal_Browser")) {
        Serial.println("Error setting up MDNS responder!");
        while(1){
            delay(1000);
        }
    }

    Serial.printf("Setup End\r\n");
}

void loop() {
     browseService("http", "tcp");
     delay(1000);
     browseService("arduino", "tcp");
     delay(1000);
     browseService("workstation", "tcp");
     delay(1000);
     browseService("smb", "tcp");
     delay(1000);
     browseService("afpovertcp", "tcp");
     delay(1000);
     browseService("ftp", "tcp");
     delay(1000);
     browseService("ipp", "tcp");
     delay(1000);
     browseService("printer", "tcp");
     delay(1000);
}

void browseService(const char * service, const char * proto){
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
        Serial.println("no services found");
    } else {
        Serial.print(n);
        Serial.println(" service(s) found");
        for (int i = 0; i < n; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i));
            Serial.print(":");
            Serial.print(MDNS.port(i));
            Serial.println(")");
        }
    }
    Serial.println();
}