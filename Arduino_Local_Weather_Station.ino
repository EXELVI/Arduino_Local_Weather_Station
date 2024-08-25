#include <NTPClient.h>
#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include "WiFiSSLClient.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include "RTC.h"

#include "arduino_secrets.h"

int timeZoneOffsetHours = 2;

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

const char *ssid = SECRET_SSID;
const char *pass = SECRET_PASS;

int status = WL_IDLE_STATUS;
WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
RTCTime currentTime;

void setup()
{

    Serial.begin(115200);
    RTC.begin();

    delay(5000);

     if (!aht.begin()) { 
        Serial.println("Could not find AHTX0 sensor!");
        while (1);
    }

    Serial.println("AHT20 found");

    if (!bmp.begin()) {
        Serial.println("Could not find BMP280 sensor!");
        while (1);
    }

    Serial.println("BMP280 found");

    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        while (true)
            ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("Please upgrade the firmware");
    }
    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
    }

    printWifiStatus();
    delay(5000);
    timeClient.begin();

    timeClient.update();

    auto unixTime = timeClient.getEpochTime();
    Serial.print("Unix time = ");
    Serial.println(unixTime);
    RTCTime timeToSet = RTCTime(unixTime);
    RTC.setTime(timeToSet);

    RTC.getTime(currentTime);
    Serial.println("The RTC was just set to: " + String(currentTime));

    server.begin();
}

void loop()
{
    WiFiClient client = server.available();

    if (client)
    {
        Serial.println("New client");
        String currentLine = "";
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);
                if (c == '\n')
                {
                    if (currentLine.length() == 0)
                    {
                       
                        break;
                    }
                    else
                    {
                     if (currentLine.startsWith("GET /data"))
                        {
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:application/json");
                            client.println();

                            JSONVar myObject;
                          

                            String jsonString = JSON.stringify(myObject);
                            client.println(jsonString);

                            break;
                        }
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }
            }

        }
        client.stop();
        Serial.println("Client disconnected");
    }

}


void printWifiStatus()
{
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    long rssi = WiFi.RSSI();

    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");

}
