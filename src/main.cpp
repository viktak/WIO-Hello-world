#define __debugSettings
#include "includes.h"

//  Wifi
WiFiClient wclient;
PubSubClient PSclient(wclient);
WiFiManager wifiManager;

//  Web server
WebServer server(80);

//  MQTT
const char* clientID = "WioTerminal-01";

//  Flags
bool needsHeartbeat = false;

//  Other global variables
config appConfig;
bool isAccessPoint = false;
bool isAccessPointCreated = false;
TimeChangeRule *tcr;        // Pointer to the time change rule
uint64_t h = 0;
int8_t page = PAGES::TEMPERATURES;

////////////////////////////////////
//  Variables to hold measurements
float livingroomTemp;
float bedroomTemp;
float guestBedroomTemp;
float officeTemp;

float piholeLoad;
float spotipiLoad;

int certExpiryViktak;
int certExpiryDiy;
int certExpiryAndThatsHowItsDone;
////////////////////////////////////


//  NTP
#ifdef __debugSettings
    const char timeServer[] = "192.168.1.3";
#else
    const char timeServer[] = "pool.ntp.org";
#endif

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
unsigned long devicetime = 0;
DateTime currentTime;
RTC_SAMD51 rtc;
unsigned int localPort = 2390;
millisDelay updateDelay;


//  Timers
char16_t buttonMillis;
unsigned long oldMillis = 0;
unsigned long heartBeatMillis = 0;

bool ntpInitialized = false;
enum CONNECTION_STATE connectionState;

WiFiUDP udp;

//  LCD
TFT_eSPI tft;
#define TITLE_ROW   10
#define ROW_1   60
#define ROW_2   80
#define ROW_3   100
#define ROW_4   120
#define LABEL   10
#define VALUE   210

//  Buttons
Button2 buttonA;
Button2 buttonB;
Button2 buttonC;

//  Pages
char * pageTitles[] = { "Temperatures", "Raspberry Pi", "Certificates"};

String WioTerminalID() {
    char ID[50];
    uint32_t *id_word0 = (uint32_t *)DEVICE_ID_WORD0;
    uint32_t *id_word1 = (uint32_t *)DEVICE_ID_WORD1;
    uint32_t *id_word2 = (uint32_t *)DEVICE_ID_WORD2;
    uint32_t *id_word3 = (uint32_t *)DEVICE_ID_WORD3;

    snprintf(ID, 30, "%02X:%02X:%02X:%02X:%02X:%02X"
        ,(*id_word0 >> 8) & 0xFF
        ,(*id_word0 >> 0) & 0xFF
        ,(*id_word3 >> 24)& 0xFF
        ,(*id_word3 >> 16)& 0xFF
        ,(*id_word3 >> 8) & 0xFF
        ,(*id_word3 >> 0) & 0xFF
    );
    return ID;
}

void DisplayData(bool clearScreen) {
    if ( clearScreen ){
        //  Clear screen
        tft.fillScreen(TFT_BLACK);         

        //  Title
        tft.setFreeFont(FMB18);
        tft.drawString(pageTitles[page], 10, TITLE_ROW);
    }


    //  Data
    switch (page){
        case PAGES::TEMPERATURES: {
            tft.setFreeFont(FMB12);
            tft.drawString("Living room:", LABEL, ROW_1);

            tft.setFreeFont(FM12);
            tft.drawFloat(livingroomTemp, 2, VALUE, ROW_1);

            tft.setFreeFont(FMB12);
            tft.drawString("Office:", LABEL, ROW_2);

            tft.setFreeFont(FM12);
            tft.drawFloat(officeTemp, 2, VALUE, ROW_2);

            tft.setFreeFont(FMB12);
            tft.drawString("Guest bedroom:", LABEL, ROW_3);

            tft.setFreeFont(FM12);
            tft.drawFloat(guestBedroomTemp, 2, VALUE, ROW_3);

            break;
        }
        case PAGES::RPIS: {
            tft.setFreeFont(FMB12);
            tft.drawString("Pi-Hole:", LABEL, ROW_1);

            tft.setFreeFont(FM12);
            tft.drawFloat(piholeLoad, 2, VALUE, ROW_1);

            tft.setFreeFont(FMB12);
            tft.drawString("Spotify:", LABEL, ROW_2);

            tft.setFreeFont(FM12);
            tft.drawFloat(spotipiLoad, 2, VALUE, ROW_2);

            break;
        }
        case PAGES::CERTS: {
            tft.setFreeFont(FMB12);
            tft.drawString("Web site:", LABEL, ROW_1);

            tft.setFreeFont(FM12);
            tft.drawFloat(certExpiryViktak, 2, VALUE, ROW_1);

            tft.setFreeFont(FMB12);
            tft.drawString("Blog:", LABEL, ROW_2);

            tft.setFreeFont(FM12);
            tft.drawFloat(certExpiryDiy, 2, VALUE, ROW_2);

            tft.setFreeFont(FMB12);
            tft.drawString("IT blog:", LABEL, ROW_3);

            tft.setFreeFont(FM12);
            tft.drawFloat(certExpiryAndThatsHowItsDone, 2, VALUE, ROW_3);

            break;
        }
    default:
        break;
    }

    
}

void Beep(int tone, int duration) {
    analogWrite(WIO_BUZZER, tone);
    delay(duration);
    analogWrite(WIO_BUZZER, 0);
}

void buttonHandler(Button2& btn) {
    int n = PAGES::NUMBER_OF_PAGES - 1;
    switch (btn.getClickType()) {
        case SINGLE_CLICK:{
            Beep(128, 100);
            switch (btn.getAttachPin()){
            case WIO_KEY_A:
                page--;
                if ( page < 0 ) page = n;
                DisplayData(true);
                break;
            case WIO_KEY_B:
                
                break;
            case WIO_KEY_C:
                page++;
                if ( page > n ) page = 0;
                DisplayData(true);
                break;
            
            default:
                break;
            }
            break;
            default:
                Serial.print("unknown ");
                break;
            }
    }
}

void sendNTPpacket(const char* address) {
    // set all bytes in the buffer to 0
    for (int i = 0; i < NTP_PACKET_SIZE; ++i) {
        packetBuffer[i] = 0;
    }
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
 
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

unsigned long getNTPtime() {
 
    // module returns a unsigned long time valus as secs since Jan 1, 1970 
    // unix time or 0 if a problem encounted
 
    //only send data when connected
    if (WiFi.status() == WL_CONNECTED) {
        //initializes the UDP state
        //This initializes the transfer buffer
        udp.begin(WiFi.localIP(), localPort);
 
        sendNTPpacket(timeServer); // send an NTP packet to a time server
        // wait to see if a reply is available
        delay(1000);
        if (udp.parsePacket()) {
            // We've received a packet, read the data from it
            udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
 
            //the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, extract the two words:
 
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const unsigned long seventyYears = 2208988800UL;
            // subtract seventy years:
            unsigned long epoch = secsSince1900 - seventyYears;

            //  Return UTC time
            return epoch;
        }
        else {
            // were not able to parse the udp packet successfully
            // clear down the udp connection
            udp.stop();
            return 0; // zero indicates a failure
        }
        // not calling ntp time frequently, stop releases resources
        udp.stop();
    }
    else {
        // network not connected
        return 0;
    }
 
}
 
void SyncTime() {
    devicetime = getNTPtime();
    if (devicetime == 0) {
        Serial.println("Failed to get time from network time server.");
    } else {
        rtc.adjust(DateTime(devicetime));
    }
}

void SetRandomSeed() {
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

void SendHeartbeat() {

    if (PSclient.connected()){

        time_t localTime = timezones[appConfig.timeZone]->toLocal(devicetime, &tcr);
        DateTime dt = (DateTime)localTime;

        const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 200;
        DynamicJsonDocument doc(capacity);

        JsonObject sysDetails = doc.createNestedObject("System");
        sysDetails["UTC"] = rtc.now().timestamp(DateTime::TIMESTAMP_FULL);
        sysDetails["LocalTime"] = dt.timestamp(DateTime::TIMESTAMP_FULL);
        
        sysDetails["Node"] = WioTerminalID();
        sysDetails["BSSID"] = WiFi.BSSIDstr();

        JsonObject wifiDetails = doc.createNestedObject("Wifi");
        wifiDetails["SSId"] = String(WiFi.SSID());
        wifiDetails["MACAddress"] = String(WiFi.macAddress());
        wifiDetails["IPAddress"] = WiFi.localIP().toString();

        #ifdef __debugSettings
        serializeJsonPretty(doc,Serial);
        Serial.println();
        #endif

        String myJsonString;

        serializeJson(doc, myJsonString);
        PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/HEARTBEAT").c_str(), myJsonString.c_str(), false);

    }

    needsHeartbeat = false;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {

    debug("Topic:\t\t");
    debugln(topic);

    debug("Payload:\t");
    for (unsigned int i = 0; i < length; i++) {
        debug((char)payload[i]);
    }
    debugln();


    StaticJsonDocument<JSON_MQTT_COMMAND_SIZE> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        //  This is NOT a JSON object
        char str[length];
        for (unsigned int i = 0; i < length; i++) {
            str[i] = (char)payload[i];
        }
        float f = std::stof(str);
        if ( ((String)topic).indexOf("livingroomhall")>-1){
            livingroomTemp = f;
        } else if ( ((String)topic).indexOf("officehall")>-1){
            officeTemp = f;
        } else if ( ((String)topic).indexOf("guestbedroomhall")>-1){
            guestBedroomTemp = f;
        } 
    }
    else{
        //  This IS a JSON object
        #ifdef __debugSettings
        serializeJsonPretty(doc,Serial);
        Serial.println();
        #endif

        if (doc.containsKey("System")){
            JsonObject System = doc["System"];
            if (System.containsKey("Hostname")){
                if (!strcmp(System["Hostname"], "pihole")){
                    piholeLoad = System["Load1"]; 
                } else if (!strcmp(System["Hostname"], "mc2")){
                    spotipiLoad = System["Load1"]; 
                }
            }
        } else if (doc.containsKey("Certificates")){
            JsonObject certs = doc["Certificates"];

            JsonObject viktak = certs["viktak.com"];
            certExpiryViktak = viktak["ExpiresIn"];

            JsonObject diy = certs["diy.viktak.com"];
            certExpiryDiy = diy["ExpiresIn"];

            JsonObject andthatshowitsdone = certs["andthatshowitsdone.viktak.com"];
            certExpiryAndThatsHowItsDone = andthatshowitsdone["ExpiresIn"];

        }


    }
    DisplayData(false);
}

void PSReconnect() {
    PSclient.setServer(appConfig.mqttServer, appConfig.mqttPort);
    if (PSclient.connect(defaultSSID, (MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), 0, true, "offline" )){
        PSclient.setBufferSize(10240);
        PSclient.setCallback(mqtt_callback);

        //  Subscribe to all thermometer readings
        PSclient.subscribe("viktak/spiti/+/thermometers/+", 0);

        //  Subscribe to all RPi reports
        PSclient.subscribe("viktak/spiti/devices/+", 0);

        //  Subscribe to certificate expiry reports
        PSclient.subscribe("viktak/spiti/certificates", 0);

        PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), "online", true);
    }
}

void setup() {
    delay(1); //  Needed for PlatformIO serial monitor
    Serial.begin(DEBUG_SPEED);
    while (!Serial);
    
    Serial.printf("\n\n\n\rBooting node:\t\t%u...\r\n", WioTerminalID());
    Serial.printf("Hardware ID:\t\t%s\r\nHardware version:\t%s\r\nSoftware ID:\t\t", HARDWARE_ID, HARDWARE_VERSION);
    Serial.printf("%s\r\nSoftware version:\t", SOFTWARE_ID);
    Serial.printf("%s\r\n\n", FIRMWARE_VERSION);

    strcpy(appConfig.friendlyName, "Very friendly name");
    appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
    appConfig.mode = 0;
    appConfig.mqttPort = 1883;

    strcpy(appConfig.mqttServer, "192.168.1.99");
    strcpy(appConfig.mqttTopic, "WioTerminal");

    appConfig.dst = true;
    appConfig.timeZone = 13;
    
    Serial.printf("Friendly name:\t%s\r\n", appConfig.friendlyName);
    Serial.printf("MQTT server:\t%s\r\n", appConfig.mqttServer);
    Serial.printf("MQTT topic:\t%s\r\n", appConfig.mqttTopic);

    //  Buttons
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);  

    buttonA.begin(WIO_KEY_A);
    buttonB.begin(WIO_KEY_B);
    buttonC.begin(WIO_KEY_C);

    buttonA.setClickHandler(buttonHandler);
    buttonB.setClickHandler(buttonHandler);
    buttonC.setClickHandler(buttonHandler);


    //  Wifi
    //wifiManager.resetSettings();
    wifiManager.setConfigPortalTimeout(300);
    wifiManager.autoConnect();

    //  RTC
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1) delay(10); // stop operating
    }

    // adjust time using ntp time
    rtc.adjust(DateTime(devicetime));
 
    //  LCD
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK); 
    DisplayData(true);

    //  Buzzer
    pinMode(WIO_BUZZER, OUTPUT);

    //  Start HTTP (web) server
    server.begin();
    debugln("HTTP server started.");

    //  Authenticate HTTP requests
    const char * headerkeys[] = {"User-Agent","Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize );

    //  Randomizer
    SetRandomSeed();

    SyncTime();
    //  NTP updates timer
    // start millisdelays timers as required, adjust to suit requirements
    // updateDelay.start(12 * 60 * 60 * 1000); // update time via ntp every 12 hrs    
    updateDelay.start(30 * 1000); // update time via ntp every 12 hrs    

    //  Setup completed
    Serial.println("Setup complete.");
}
 
void loop(){

    if (!WiFi.isConnected()){
        NVIC_SystemReset();
    }

    if (PSclient.connected()){
        PSclient.loop();
    } else {
        PSReconnect();
    }

    if (updateDelay.justFinished()){
        SyncTime();
        updateDelay.repeat();
    }

    buttonA.loop();
    buttonB.loop();
    buttonC.loop();

    if (millis() - heartBeatMillis > appConfig.heartbeatInterval * 1000){
        SendHeartbeat();
        heartBeatMillis = millis();
        needsHeartbeat = false;
    }

}
