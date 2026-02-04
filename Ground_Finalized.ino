#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define RADIO_CE_PIN  9
#define RADIO_CSN_PIN 10

RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);

const byte COMMAND_ADDRESS[6] = "IN001";
const byte TELEMETRY_ADDRESS[6] = "OUT01";

struct Command {
  uint8_t cmd; // 1=старт
} c;

struct Telemetry {
  int8_t tilt;
  int8_t pan;
  uint8_t mode;
} t;

void Start() {
  c.cmd = 1;
  radio.stopListening();
  radio.write(&c, sizeof(c));
  radio.startListening();
  Serial.println("SEND: START");
}

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);

  radio.openWritingPipe(COMMAND_ADDRESS);     // шлём команды
  radio.openReadingPipe(1, TELEMETRY_ADDRESS);  // слушаем телеметрию
  radio.startListening();

  Serial.println("Station ready. Type 's' and press Enter.");
}

void loop() {
  // отправка команды
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 's' || ch == 'S') {
      Start();
    }
  }

  // прием телеметрии
  if (radio.available()) {
    radio.read(&t, sizeof(t));
    Serial.print("mode=");
    Serial.print(t.mode);
    Serial.print(" tilt=");
    Serial.print((int)t.tilt);
    Serial.print(" pan=");
    Serial.println((int)t.pan);
  }
}
