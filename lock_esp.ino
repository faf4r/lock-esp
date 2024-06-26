// #include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include <Servo.h>

// WiFi config
const char *ssid = "Tenda_D7BEA8";
const char *pwd = "tt174020";
ESP8266WiFiMulti WiFiMulti;

// mqtt config
const char *mqtt_broker = "121.41.80.56";
const int mqtt_port = 1883;
const char *topic = "/lock";
WiFiClient espclient;
PubSubClient client(espclient); // 必须传一个client作为参数

// servo config
#define servo_pin 2   // GPIO2--D4 //和灯共用一个pin？但是舵机的占空比和灯的不一样？
#define servo_ON 60   // 开门的角度
#define servo_OFF 150 // 回位的角度
Servo servo;

void setup()
{
    Serial.begin(115200);
    Serial.println();

    // LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // 初始不亮

    // servo
    servo.attach(servo_pin, 500, 2500);
    servo.write(servo_OFF);
    delay(100);

    // wifi
    WiFiMulti.addAP(ssid, pwd);
    WiFiMulti.addAP("Hi nova 9z", "12345678");
    WiFiMulti.addAP("donkeycar", "donkeycar");
    connect_wifi_multi();
    // connect_wifi();

    // mqtt
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    connect_mqtt();
    client.subscribe(topic);
}

void loop()
{
    // Serial.println("loop"); //test if it's blocked: flash frequently
    while (!client.loop()) {
        connect_wifi_multi();
        connect_mqtt();
        client.subscribe(topic);
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void connect_wifi_multi()
{
    WiFi.disconnect(); // 清除连接信息，防止路由器重启后信道变化
    digitalWrite(LED_BUILTIN, LOW);
    Serial.printf("connecting to WiFi......");
    while (WiFiMulti.run() != WL_CONNECTED) {
        delay(500);
    }
    Serial.printf("connected to %s\n", WiFi.SSID().c_str());
    digitalWrite(LED_BUILTIN, HIGH);
}

void connect_wifi()
{
    WiFi.disconnect(); // 清除连接信息，防止路由器重启后信道变化
    Serial.printf("connecting to %s......", ssid);
    WiFi.begin(ssid, pwd);
    int led_state = HIGH;
    while (!WiFi.isConnected()) {
        digitalWrite(LED_BUILTIN, !led_state);
        led_state = !led_state;
        delay(500);
    }
    Serial.println("connected");
    digitalWrite(LED_BUILTIN, HIGH);
}

void connect_mqtt()
{
    int led_state = HIGH;
    while (!client.connected()) {
        digitalWrite(LED_BUILTIN, !led_state);
        led_state = !led_state;
        String client_id = "esp8266-client-" + WiFi.hostname(); // or String(WiFi.macAddress());
        Serial.printf("client %s is connecting to broker %s ... ", client_id.c_str(), mqtt_broker);
        if (client.connect(client_id.c_str())) {
            Serial.println("connected");
            delay(20);
        } else {
            Serial.printf("failed with state %d\n", client.state());
            delay(2000);
        }
    }
    digitalWrite(LED_BUILTIN, HIGH);
    delay(20);
}

void callback(char *topic, uint8_t *payload, unsigned int length)
{
    digitalWrite(LED_BUILTIN, LOW);
    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.printf("receive message from topic %s: %s\n", topic, msg.c_str());
    Serial.println("--------------------------");

    if (act_on_msg(msg)) {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    // Error, need reconnect or reset so keep the LED on
}

bool act_on_msg(String msg)
{
    if (msg.equals("open_door")) {
        servo.write(servo_ON);
        if (!client.publish("/lock/response", "success")) {
            servo.write(servo_OFF);
            delay(200);
            return false;
        }
        delay(2000);
    }

    servo.write(servo_OFF);
    delay(200);
    return true;
}
