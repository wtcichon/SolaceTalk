
#include <Arduino.h>
#include <Ethernet.h>
#include <dht.h>
dht DHT;
#define DHT11_PIN 7

// Enable MqttClient logs
#define MQTT_LOG_ENABLED 1
// Include library
#include <MqttClient.h>


#define LOG_PRINTFLN(fmt, ...)  printfln_P(PSTR(fmt), ##__VA_ARGS__)
#define LOG_SIZE_MAX 128

void printfln_P(const char *fmt, ...) {
  char buf[LOG_SIZE_MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf_P(buf, LOG_SIZE_MAX, fmt, ap);
  va_end(ap);
  Serial.println(buf);
}

#define HW_UART_SPEED                 57600L
#define MQTT_ID                     "SOLACE_VPN"
 #define USR "username"
#define PWD "password"
const char* MQTT_TOPIC_PUB = "topic";

MqttClient *mqtt = NULL;
EthernetClient network;
 
// ============== Object to supply system functions ================================
class System: public MqttClient::System {
public:
  unsigned long millis() const {
    return ::millis();
  }
};

// ============== Setup all objects ============================================
void setup() {
  // Setup hardware serial for logging
  Serial.begin(HW_UART_SPEED);
  while (!Serial);
  // Setup MqttClient
  MqttClient::System *mqttSystem = new System;
  MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
  MqttClient::Network * mqttNetwork = new MqttClient::NetworkClientImpl<Client>(network, *mqttSystem);
  //// Make 128 bytes send buffer
  MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
  //// Make 128 bytes receive buffer
  MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
  //// Allow up to 2 subscriptions simultaneously
  MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
  //// Configure client options
  MqttClient::Options mqttOptions;
  ////// Set command timeout to 10 seconds
  mqttOptions.commandTimeoutMs = 10000;
  //// Make client object
  mqtt = new MqttClient (
    mqttOptions, *mqttLogger, *mqttSystem, *mqttNetwork, *mqttSendBuffer,
    *mqttRecvBuffer, *mqttMessageHandlers
  );
}
IPAddress ip(192, 168, 0, 177);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// ============== Main loop ====================================================
void loop() {
  // Check connection status
  if (!mqtt->isConnected()) {
    // Close connection if exists
    network.stop();

 if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("connecting...");
    
    // Re-establish TCP connection with MQTT broker
    network.connect("vmr-mr8v6yiwid1f.messaging.solace.cloud",20902);
    // Start new MQTT connection
    LOG_PRINTFLN("Connecting");
    MqttClient::ConnectResult connectResult;
    // Connect
    {
      MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
      options.MQTTVersion = 4;
      options.clientID.cstring = (char*)MQTT_ID;
       options.username.cstring =   (char*) USR;
       options.password.cstring =  (char*) PWD; 
      options.cleansession = true;
      options.keepAliveInterval = 15; // 15 seconds
      MqttClient::Error::type rc = mqtt->connect(options, connectResult);
      if (rc != MqttClient::Error::SUCCESS) {
        LOG_PRINTFLN("Connection error: %i", rc);
        return;
      }
    }
    {
      // Add subscribe here if need
    }
  } else {
    {
       
        int chk = DHT.read11(DHT11_PIN);
        Serial.print("Temperature = ");
        Serial.println(DHT.temperature);
         Serial.println(DHT.humidity);
         String json = "{\"hum\":\""+String(DHT.humidity)+"\",\"temp\":\""+String(DHT.temperature,2)+"\"}";
       int buf_size = json.length() + 1; 
       char buf[buf_size];
       
      json.toCharArray(buf,buf_size);
      MqttClient::Message message;
      message.qos = MqttClient::QOS0;
      message.retained = false;
      message.dup = false;
      message.payload = (void*) buf;
      message.payloadLen = strlen(buf);
      mqtt->publish(MQTT_TOPIC_PUB, message);
    }
   
    mqtt->yield(30000L);
  }
}
