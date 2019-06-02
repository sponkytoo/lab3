/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "app_commands.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
//NL EDIT
static const char* APPID_KEY = "ed3da58111974261002c2af4f8e8e81f";
char jsonBuffer[1024];
char cityBuffer[128];


// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
 */

APP_DATA appData;


// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
 */


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

int8_t _APP_PumpDNS(const char * hostname, IPV4_ADDR *ipv4Addr);



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize(void) {
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    APP_Commands_Init();

    //NL EDIT
    memset(jsonBuffer, 0, sizeof (jsonBuffer));
    memset(cityBuffer, 0, sizeof (cityBuffer));
    appData.host = "api.openweathermap.org";
    appData.port = 80;
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks(void) {
    SYS_STATUS tcpipStat;
    const char *netName, *netBiosName;
    static IPV4_ADDR dwLastIP[2] = {
        {-1},
        {-1}
    };
    IPV4_ADDR ipAddr;
    TCPIP_NET_HANDLE netH;
    int i, nNets;

    /* Check the application's current state. */
    switch (appData.state) {
            /* Application's initial state. */
        case APP_STATE_INIT:
        {
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            if (tcpipStat < 0) { // some error occurred
                SYS_CONSOLE_MESSAGE(" APP: TCP/IP stack initialization failed!\r\n");
                appData.state = APP_TCPIP_ERROR;
            } else if (tcpipStat == SYS_STATUS_READY) {
                // now that the stack is ready we can check the
                // available interfaces
                nNets = TCPIP_STACK_NumberOfNetworksGet();
                for (i = 0; i < nNets; i++) {

                    netH = TCPIP_STACK_IndexToNet(i);
                    netName = TCPIP_STACK_NetNameGet(netH);
                    netBiosName = TCPIP_STACK_NetBIOSName(netH);

#if defined(TCPIP_STACK_USE_NBNS)
                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS enabled\r\n", netName, netBiosName);
#else
                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS disabled\r\n", netName, netBiosName);
#endif  // defined(TCPIP_STACK_USE_NBNS)

                }
                appData.state = APP_TCPIP_WAIT_FOR_IP;

            }
            break;
        }
        case APP_TCPIP_WAIT_FOR_IP:

            // if the IP address of an interface has changed
            // display the new value on the system console
            nNets = TCPIP_STACK_NumberOfNetworksGet();

            for (i = 0; i < nNets; i++) {
                netH = TCPIP_STACK_IndexToNet(i);
                if (!TCPIP_STACK_NetIsReady(netH)) {
                    continue; // interface not ready yet! , 
                    //looking for another interface, that can be used for communication.
                }
                // Now. there is a ready interface that we can use
                ipAddr.Val = TCPIP_STACK_NetAddress(netH);
                // display the changed IP address
                if (dwLastIP[i].Val != ipAddr.Val) {
                    dwLastIP[i].Val = ipAddr.Val;

                    SYS_CONSOLE_MESSAGE(TCPIP_STACK_NetNameGet(netH));
                    SYS_CONSOLE_MESSAGE(" IP Address: ");
                    SYS_CONSOLE_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
                    //NL EDIT
                    //SYS_CONSOLE_MESSAGE("Waiting for command type: openurl <url>\r\n");
                    SYS_CONSOLE_MESSAGE("Waiting for command type: requestWeather <city>\r\n");
                }
                appData.state = APP_TCPIP_WAITING_FOR_COMMAND;
            }
            break;

        case APP_TCPIP_WAITING_FOR_COMMAND:
        {
            SYS_CMD_READY_TO_READ();

            if (APP_URL_Buffer[0] != '\0') {
                TCPIP_DNS_RESULT result;
                //NL EDIT
                snprintf(cityBuffer, 128, APP_URL_Buffer);
                SYS_CONSOLE_PRINT("cityBuffer: %s\r\n", cityBuffer);

                result = TCPIP_DNS_Resolve(appData.host, TCPIP_DNS_TYPE_A);
                if (result == TCPIP_DNS_RES_NAME_IS_IPADDRESS) {
                    IPV4_ADDR addr;
                    TCPIP_Helper_StringToIPAddress(appData.host, &addr);
                    appData.socket = TCPIP_TCP_ClientOpen(IP_ADDRESS_TYPE_IPV4,
                            TCPIP_HTTP_SERVER_PORT,
                            (IP_MULTI_ADDRESS*) & addr);
                    if (appData.socket == INVALID_SOCKET) {
                        SYS_CONSOLE_MESSAGE("Could not start connection\r\n");
                        appData.state = APP_TCPIP_WAITING_FOR_COMMAND;
                    }
                    SYS_CONSOLE_MESSAGE("Starting connection\r\n");
                    appData.state = APP_TCPIP_WAIT_FOR_CONNECTION;
                    break;
                }
                if (result < 0) {
                    SYS_CONSOLE_MESSAGE("Error in DNS aborting\r\n");
                    APP_URL_Buffer[0] = '\0';
                    break;
                }
                appData.state = APP_TCPIP_WAIT_ON_DNS;
                APP_URL_Buffer[0] = '\0';
            } else {
                appData.state = APP_TCPIP_WAIT_FOR_IP;
            }

        }
            break;

        case APP_TCPIP_WAIT_ON_DNS:
        {
            IPV4_ADDR addr;
            switch (_APP_PumpDNS(appData.host, &addr)) {
                case -1:
                {
                    // Some sort of error, already reported
                    appData.state = APP_TCPIP_WAITING_FOR_COMMAND;
                }
                    break;
                case 0:
                {
                    // Still waiting
                }
                    break;
                case 1:
                {
                    appData.socket = TCPIP_TCP_ClientOpen(IP_ADDRESS_TYPE_IPV4,
                            appData.port,
                            (IP_MULTI_ADDRESS*) & addr);
                    if (appData.socket == INVALID_SOCKET) {
                        SYS_CONSOLE_MESSAGE("Could not start connection\r\n");
                        appData.state = APP_TCPIP_WAITING_FOR_COMMAND;
                    }
                    SYS_CONSOLE_MESSAGE("Starting connection\r\n");
                    appData.state = APP_TCPIP_WAIT_FOR_CONNECTION;
                }
                    break;
            }
        }
            break;

        case APP_TCPIP_WAIT_FOR_CONNECTION:
        {
            char buffer[MAX_URL_SIZE];
            if (!TCPIP_TCP_IsConnected(appData.socket)) {
                break;
            }
            if (TCPIP_TCP_PutIsReady(appData.socket) == 0) {
                break;
            }
            //NL EDIT
            char pathBuffer[128];
            snprintf(pathBuffer, 128, "data/2.5/weather?q=%s&APPID=%s", cityBuffer, APPID_KEY);
            appData.path = pathBuffer;

            sprintf(buffer, "GET /%s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Connection: close\r\n\r\n", appData.path ? appData.path : "null", appData.host);
            TCPIP_TCP_ArrayPut(appData.socket, (uint8_t*) buffer, strlen(buffer));
            appData.state = APP_TCPIP_WAIT_FOR_RESPONSE;
        }
            break;

        case APP_TCPIP_WAIT_FOR_RESPONSE:
        {
            char buffer[80];
            memset(buffer, 0, sizeof (buffer));
            if (!TCPIP_TCP_IsConnected(appData.socket)) {
                SYS_CONSOLE_MESSAGE("\r\nConnection Closed\r\n");
                //NL EDIT
                appData.state = APP_STATE_JSON_PARSE_RETRIEVED_DATA;
                break;
            }
            if (TCPIP_TCP_GetIsReady(appData.socket)) {
                TCPIP_TCP_ArrayGet(appData.socket, (uint8_t*) buffer, sizeof (buffer) - 1);
                //NL EDIT
                strncat(jsonBuffer, buffer, strlen(buffer));
            }
        }
            break;

            //NL EDIT
        case APP_STATE_JSON_PARSE_RETRIEVED_DATA:
        {
            char* resultingJson;
            char* pos;

            pos = strstr(jsonBuffer, "{\"");
            *(&resultingJson) = pos;

            SYS_CONSOLE_PRINT("resultingJson: \r\n %s \r\n", resultingJson);

            //Find Humidity
            char* mainHumidityJson;
            char* mainHumidtyBuffer;

            pos = strstr(resultingJson, "humidity");
            *(&mainHumidityJson) = pos + 10;
            mainHumidtyBuffer = strtok(mainHumidityJson, ",");

            //Find Pressure
            char* mainPressureJson;
            char* mainPressureBuffer;

            pos = strstr(resultingJson, "pressure");
            *(&mainPressureJson) = pos + 10;
            mainPressureBuffer = strtok(mainPressureJson, ",");


            //Find Temperature
            char* mainTemperatureJson;
            char* mainTemperatureBuffer;
            float Temperature;

            pos = strstr(resultingJson, "temp");
            *(&mainTemperatureJson) = pos + 6;
            mainTemperatureBuffer = strtok(mainTemperatureJson, ",");
            Temperature = atof(mainTemperatureBuffer) - 273.15;

            //Find main weather
            char* mainMainWeatherJson;
            char* mainMainWeatherBuffer;

            pos = strstr(resultingJson, "main");
            *(&mainMainWeatherJson) = pos + 7;
            mainMainWeatherBuffer = strtok(mainMainWeatherJson, "\"");


            SYS_CONSOLE_PRINT("\r\nCurrent Weather in %s \r\nHumidity: %s\r\nPressure: %s\r\nTemperature: %2.2f\r\nMain Weather: %s \r\n\r\n", 
                    cityBuffer, mainHumidtyBuffer, mainPressureBuffer, Temperature, mainMainWeatherBuffer);

            TCPIP_TCP_Close(appData.socket);
            jsonBuffer[0]=0;
            appData.state = APP_TCPIP_WAITING_FOR_COMMAND;
        }
            break;

            /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

int8_t _APP_PumpDNS(const char * hostname, IPV4_ADDR *ipv4Addr) {
    IP_MULTI_ADDRESS mAddr;
    int8_t retval = -1;

    TCPIP_DNS_RESULT result = TCPIP_DNS_IsResolved(hostname, &mAddr, IP_ADDRESS_TYPE_IPV4);
    switch (result) {
        case TCPIP_DNS_RES_OK:
        {
            // We now have an IPv4 Address
            // Open a socket
            ipv4Addr->Val = mAddr.v4Add.Val;
            retval = 1;
            break;
        }
        case TCPIP_DNS_RES_PENDING:
            retval = 0;
            break;
        case TCPIP_DNS_RES_SERVER_TMO:
        case TCPIP_DNS_RES_NO_IP_ENTRY:
        default:
            SYS_CONSOLE_MESSAGE("TCPIP_DNS_IsResolved returned failure\r\n");
    }

    return retval;

}

/*******************************************************************************
 End of File
 */

