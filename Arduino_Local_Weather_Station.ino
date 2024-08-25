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

    if (!aht.begin())
    {
        Serial.println("Could not find AHTX0 sensor!");
        while (1)
            ;
    }

    Serial.println("AHT20 found");

    if (!bmp.begin())
    {
        Serial.println("Could not find BMP280 sensor!");
        while (1)
            ;
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

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    float temperatureBMP = bmp.readTemperature();
    float pressureBMP = bmp.readPressure() / 100.0F;

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
                        client.println("<!doctype html>");
                        client.println("<html lang=\"en\">");
                        client.println("  <head>");
                        client.println("    <meta charset=\"utf-8\">");
                        client.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("    <title>Stazione Meteo Locale</title>");
                        client.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
                        client.println("    <style>");
                        client.println("      body {");
                        client.println("        padding-top: 20px;");
                        client.println("      }");
                        client.println("      .data-display {");
                        client.println("        font-size: 1.2rem;");
                        client.println("      }");
                        client.println("      .timestamp {");
                        client.println("        font-style: italic;");
                        client.println("        color: gray;");
                        client.println("      }");
                        client.println("    </style>");
                        client.println("  </head>");
                        client.println("  <body>");
                        client.println("    <div class=\"container\">");
                        client.println("      <h1 class=\"text-center\">Local Weather Station</h1>");
                        client.println("      <div class=\"row mt-5\">");
                        client.println("        <div class=\"col-md-6\">");
                        client.println("          <h3>AHT20</h3>");
                        client.println("          <p class=\"data-display\">Temperatura: <span id=\"aht20-temp\">--</span> °C</p>");
                        client.println("          <p class=\"data-display\">Umidità: <span id=\"aht20-humidity\">--</span> %</p>");
                        client.println("        </div>");
                        client.println("        <div class=\"col-md-6\">");
                        client.println("          <h3>BMP280</h3>");
                        client.println("          <p class=\"data-display\">Temperatura: <span id=\"bmp280-temp\">--</span> °C</p>");
                        client.println("          <p class=\"data-display\">Pressione: <span id=\"bmp280-pressure\">--</span> hPa</p>");
                        client.println("        </div>");
                        client.println("      </div>");
                        client.println("      <div class=\"row mt-4\">");
                        client.println("        <div class=\"col-md-12 text-center\">");
                        client.println("          <p class=\"timestamp\">Ultimo aggiornamento: <span id=\"last-update\">--</span></p>");
                        client.println("        </div>");
                        client.println("      </div>");
                        client.println("    </div>");
                        client.println("");
                        client.println("    <script>");
                        client.println("      function fetchWeatherData() {");
                        client.println("        fetch('/data')");
                        client.println("          .then(response => response.json())");
                        client.println("          .then(data => {");
                        client.println("");
                        client.println("            document.getElementById('aht20-temp').innerText = data.AHT20.temp.toFixed(2);");
                        client.println("            document.getElementById('aht20-humidity').innerText = data.AHT20.humidity.toFixed(2);");
                        client.println("            document.getElementById('bmp280-temp').innerText = data.BMP280.temp.toFixed(2);");
                        client.println("            document.getElementById('bmp280-pressure').innerText = data.BMP280.pressure.toFixed(2);");
                        client.println("");
                        client.println("            const date = new Date(data.RTC * 1000); ");
                        client.println("            document.getElementById('last-update').innerText = date.toLocaleString();");
                        client.println("          })");
                        client.println("          .catch(error => console.error('Errore nel recupero dei dati:', error));");
                        client.println("      }");
                        client.println("");
                        client.println("      setInterval(fetchWeatherData, 10000);");
                        client.println("");
                        client.println("      fetchWeatherData();");
                        client.println("    </script>");
                        client.println("");
                        client.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>");
                        client.println("  </body>");
                        client.println("</html>");
                        client.println("");
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
                            JSONVar AHT20;
                            AHT20["temp"] = temp.temperature;
                            AHT20["humidity"] = humidity.relative_humidity;
                            myObject["AHT20"] = AHT20;

                            JSONVar BMP280;
                            BMP280["temp"] = temperatureBMP;
                            BMP280["pressure"] = pressureBMP;
                            myObject["BMP280"] = BMP280;

                            RTC.getTime(currentTime);
                            myObject["RTC"] = String(currentTime.getUnixTime());
                          

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
