#ifndef INCLUDES_H
#define INCLUDES_H

#include "version.h"

#include "../../ActoSenso/Nodes/_common/debug.h"

#include "defines.h"
#include "../../ActoSenso/Nodes/_common/defines.h"

#include "enums.h"
#include "../../ActoSenso/Nodes/_common/enums.h"

#include "../../ActoSenso/Nodes/_common/variables.cpp"

#include <cstdlib>
#include <string>


#include <TFT_eSPI.h>  
#include "Free_Fonts.h" //include the header file

#include <rpcWiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <RPCmDNS.h>
#include <WiFiManager.h>
#include <millisDelay.h>
#include <WiFiUdp.h>

#include <PubSubClient.h>

#include "RTC_SAMD51.h"
#include "DateTime.h"

#include "SAMDTimerInterrupt.h"
#include "SAMD_ISR_Timer.h"

#include <Arduino_GFX_Library.h>

#include <ArduinoJson.h>
#include <TimeChangeRules.h>
#include "Button2.h"

#include "structs.h"

#endif