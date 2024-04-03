// #include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Servo.h>
#include <PubSubClient.h>

//WiFi config
const char* ssid = "Tenda_D7BEA8";
const char* pwd = "tt174020";
ESP8266WiFiMulti WiFiMulti;

//mqtt config
const char* mqtt_broker = "121.41.80.56";
const int mqtt_port = 1883;
const char* topic = "/lock";
WiFiClient espclient;
PubSubClient client(espclient); //必须传一个client作为参数

//servo config
#define servo_pin 2     //GPIO2--D4 //和灯共用一个pin，但是控制舵机的脉宽只有一小部分。
#define servo_ON 60     //开门的角度
#define servo_OFF 150   //回位的角度
Servo servo;

//LED config
#define led_OFF 199 //200即占空比100%时不亮，占空比越低越亮
#define led_ON  100

void setup() {
  analogWriteFreq(50);
  analogWriteRange(200);  //0.1ms

  Serial.begin(115200);
  Serial.println();

  //LED
  analogWrite(LED_BUILTIN, led_ON); //注意LED_BUILTIN也是GPIO2

  //servo
  servo.attach(servo_pin, 500, 2500);
  servo.write(servo_OFF);
  delay(100);

  //wifi
  WiFiMulti.addAP(ssid, pwd);
  WiFiMulti.addAP("Hi nova 9z", "12345678");
  WiFiMulti.addAP("donkeycar", "donkeycar");
  connect_wifi_multi();
  // connect_wifi();

  //mqtt
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  connect_mqtt();
  client.subscribe(topic);
}

void loop() {
  // Serial.println("loop"); //test if it's blocked: flash frequently
  while (!client.loop()) {
    // reconnect_wifi();
    connect_wifi_multi();
    connect_mqtt();
    client.subscribe(topic);
    analogWrite(LED_BUILTIN, led_ON);
  }
}


void connect_wifi_multi() {
  analogWrite(LED_BUILTIN, led_ON);
  Serial.printf("connecting to WiFi......");
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
  }
  Serial.printf("connected to %s\n", WiFi.SSID().c_str());
  analogWrite(LED_BUILTIN, led_OFF);
}

void connect_wifi() {
  analogWrite(LED_BUILTIN, led_ON);
  Serial.printf("connecting to %s......", ssid);
  WiFi.begin(ssid, pwd);
  while (!WiFi.isConnected()) {
    delay(500);
  }
  Serial.println("connected");
  analogWrite(LED_BUILTIN, led_OFF);
}

void reconnect_wifi() {
  Serial.printf("reconnecting to %s......", ssid);
  analogWrite(LED_BUILTIN, led_ON);
  WiFi.disconnect();  //清楚连接信息，防止路由器重启后信道变化
  WiFi.begin(ssid, pwd);
  while (!WiFi.isConnected()) {
    delay(500);
  }
  Serial.println("connected");
  analogWrite(LED_BUILTIN, led_OFF);
  delay(20);
}

void connect_mqtt() {
  analogWrite(LED_BUILTIN, led_ON);
  while (!client.connected()) {
    String client_id = "esp8266-client-" + WiFi.hostname(); //or String(WiFi.macAddress());
    Serial.printf("client %s is connecting to broker %s ... ", client_id.c_str(), mqtt_broker);
    if (client.connect(client_id.c_str())) {
      Serial.println("connected");
      delay(20);
    } else {
      Serial.printf("failed with state %d\n", client.state());
      delay(2000);
    }
  }
  analogWrite(LED_BUILTIN, led_OFF);
  delay(20);
}

void callback(char *topic, uint8_t *payload, unsigned int length) {
  analogWrite(LED_BUILTIN, led_ON);
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("receive message from topic %s: %s\n", topic, msg.c_str());
  Serial.println("--------------------------");

  if (act_on_msg(msg)) {
    // delay(1000);
    // analogWrite(LED_BUILTIN, 5);  //这里本只想控制灯但是这个脉宽在舵机控制范围内，所以它被驱动
    analogWrite(LED_BUILTIN, led_OFF);
  }
  //Error, need reconnect or reset so keep the LED on
}

bool act_on_msg(String msg) {
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
