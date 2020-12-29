
//include library that use
#include <WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "cckarn._";
const char* password = "1909802728861";

// Config MQTT Server
#define mqtt_server "192.168.43.5"
#define mqtt_port 1883
#define mqtt_user "TEST"
#define mqtt_password "12345"

WiFiClient espClient;
PubSubClient client(espClient);

//set the PIN of LED used
int LED_PIN = 2;

void setup() {

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(10);

  //setup WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
    
}

void loop() {

  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("/esp/red");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      return;
    }
  }
  client.loop();
}

//For recieve massage from Node-Red to control OUTPUT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  int i = 0;

  while (i < length) msg += (char)payload[i++];

  //Check LED state
  if (msg == "Lightstate") {
    client.publish("/esp/red", (digitalRead(LED_PIN) ? "Light state ON" : "Light state OFF"));
    Serial.println("Send to check LED state!");
    return;
  }

  //Control LED to 'on'
  if (msg == "on") {
    client.publish("/esp/red", "Light State-ON");
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Send on!");
    return;
  }

  //Control LED to 'off'
  if (msg == "off") {
    client.publish("/esp/red", "Light State-OFF");
    digitalWrite(LED_PIN, LOW);
    Serial.println("Send off!");
    return;
  }
  
}
