#include <Arduino.h>
#include "secrets.h"
#include <PubSubClient.h>
#include "WiFi.h"
#include <string>
#include <Wire.h>

#define DEBUG true       // switch to "false" for production
#define NB_TRYWIFI 20    // WiFi connection retries
#define durationSleep 30 // seconds

int moistureSensorPin = 34;
int moistureSensorPowerPin = 19;
float moistureSensorValue = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// **************
void loop();
void setup();
void goToSleep();
void connectToWiFi();
void connectToHass();
void initGyroComponent();
void printWakeupReason();
void sendMessageToHass(String msg);
// **************


/**
 * Establishes WiFi connection
 * @return
 */
void connectToWiFi()
{
    int _try = 0;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (DEBUG == true)
    {
        Serial.println("Connecting to Wi-Fi");
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        _try++;
        if (_try >= NB_TRYWIFI)
        {
            if (DEBUG == true)
            {
                Serial.println("Impossible to connect WiFi, going to deep sleep");
            }

            ESP.deepSleep(durationSleep * 1000000);
        }
    }
    if (DEBUG == true)
    {
        Serial.println("Connected to Wi-Fi");
    }
}

/**
 * Establishes connection to Home Assistant MQTT Broker
 * @return
 */
void connectToHass()
{
    client.setServer(MQTT_SERVER, 1883);

    // Loop until we're reconnected
    while (!client.connected())
    {
        if (DEBUG == true)
        {
            Serial.print("Attempting MQTT connection...");
        }
        // Attempt to connect
        // If you do not want to use a username and password, change next line to
        // if (client.connect("ESP32Client")) {
        if (client.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD))
        {
            if (DEBUG == true)
            {
                Serial.println("connected");
            }
        }
        else
        {
            if (DEBUG == true)
            {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds");
            }
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

/**
 * Publishes notification to MQTT topic
 * @return
 */
void sendMessageToHass(String msg)
{
    if (DEBUG == true)
    {
        Serial.println("Publishing to Hass!");
    }
    // publish the reading to Hass through MQTT
    client.publish(MQTT_PUBLISH_TOPIC, msg.c_str(), true);
    client.loop();

    if (DEBUG == true)
    {
        Serial.print("Message ");
        Serial.print(msg);
        Serial.println(" sent to Hass!");
    }
}

void setup() {
    Serial.begin(115200);

    // only print debug messages to serial if we're in debug mode
    if (DEBUG == true)
    {
        Serial.println("Motion detected! Waking up...");
        printWakeupReason(); // Print the wakeup reason for ESP32
    }

    while (!Serial)
    {
        delay(10); // will pause Zero, Leonardo, etc until serial console opens
    }

    // Init both pins
    pinMode(moistureSensorPin, INPUT);
    pinMode(moistureSensorPowerPin, OUTPUT);

    // Switch moisture sensor by sending High
    digitalWrite(moistureSensorPowerPin, HIGH);
    delay(100);

    // perform a read from the moisture sensor
    moistureSensorValue = analogRead(moistureSensorPin);
    Serial.print("moistureSensorValue: ");
    Serial.print(moistureSensorValue);

    if (moistureSensorValue > 350) {
        Serial.println(" -> wet");
    } else {
        Serial.println(" -> dry");
    }

    // connectToWiFi();
    // connectToHass();
    // sendMessageToHass("message:water_me");

    digitalWrite(moistureSensorPowerPin, LOW);
    delay(5000); // stay awake for 5 seconds
    goToSleep();
}

void goToSleep()
{
    if (DEBUG == true)
    {
        Serial.println("Going to sleep");
    }

    // Configure GPIO33 as ext0 wake up source for HIGH logic level
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);

    // Go to sleep now
    esp_deep_sleep_start();
}

/**
 * Function that prints the reason by which ESP32 has been awaken from sleep
 *
**/
void printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeupReason;
    wakeupReason = esp_sleep_get_wakeup_cause();

    switch (wakeupReason)
    {
    case 1:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case 2:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case 3:
        Serial.println("Wakeup caused by timer");
        break;
    case 4:
        Serial.println("Wakeup caused by touchpad");
        break;
    case 5:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.println("Wakeup was not caused by deep sleep");
        break;
    }
}

void loop() {}
