#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

// Пины
#define RADIO_CE_PIN   9
#define RADIO_CSN_PIN  10
#define SERVO_TILT_PIN  7
#define SERVO_TURN_PIN   8
#define LASER_PIN  4

// Радио
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
const byte COMMAND_ADDRESS[6] = "IN001";
const byte TELEMETRY_ADDRESS[6] = "OUT01";

// Параметры скана
const int MIN_ANGLE = -40;
const int MAX_ANGLE =  40;
const int ANG_STEP = 10;
const int DELAY = 1000;

// Сервы (0 градусов по нашей системе = 90 градусов на серве)
const int CENTER_POS   = 90;

Servo sTilt;
Servo sTurn;

// Пакет телеметрии
struct TelemetryPacket {
  int8_t tilt;   // -40..40
  int8_t pan;    // -40..40
  uint8_t mode;  // 1..4 (какой скан)
} t;

// Команда=
struct Command {
  uint8_t cmd;
} c;

void set_pos(int tiltDeg, int turnDeg) {
  sTilt.write(CENTER_POS   + tiltDeg);
  sTurn.write(CENTER_POS   + turnDeg);
  t.tilt = tiltDeg;
  t.pan  = turnDeg;
}

void sendTelemetry(uint8_t mode) {
  t.mode = mode;

  radio.stopListening();
  radio.write(&t, sizeof(t));
  radio.startListening();
}

void startPos() {
  set_pos(0, 0);
  sendTelemetry(0);
}

void scanVert() {
  // mode = 1: pan = 0, меняем tilt
  for (int a = MIN_ANGLE; a <= MAX_ANGLE; a += ANG_STEP) {
    set_pos(a, 0);
    sendTelemetry(1);
    delay(DELAY);
  }
}

void scanHor() {
  // mode = 2: tilt = 0, меняем pan
  for (int a = MIN_ANGLE; a <= MAX_ANGLE; a += ANG_STEP) {
    set_pos(0, a);
    sendTelemetry(2);
    delay(DELAY);
  }
}

void scanDiag1() {
  // mode = 3: (-40,-40) -> (40,40)
  int tilt = MIN_ANGLE;
  int pan  = MIN_ANGLE;
  while (tilt <= MAX_ANGLE && pan <= MAX_ANGLE) {
    set_pos(tilt, pan);
    sendTelemetry(3);
    delay(DELAY);
    tilt += ANG_STEP;
    pan  += ANG_STEP;
  }
}

void scanDiag2() {
  // mode = 4: (-40,40) -> (40,-40)
  int tilt = MIN_ANGLE;
  int pan  = MAX_ANGLE;
  while (tilt <= MAX_ANGLE && pan >= MIN_ANGLE) {
    set_pos(tilt, pan);
    sendTelemetry(4);
    delay(DELAY);
    tilt += ANG_STEP;
    pan  -= ANG_STEP;
  }
}

void setup() {
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH); // лазер включен (можно выключать вне скана)

  sTilt.attach(SERVO_TILT_PIN);
  sTurn.attach(SERVO_TURN_PIN);

  // Радио
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);

  radio.openReadingPipe(1, COMMAND_ADDRESS); // слушаем команды
  radio.openWritingPipe(TELEMETRY_ADDRESS);    // шлём телеметрию
  radio.startListening();

  startPos();
}

void loop() {
  if (radio.available()) {
    radio.read(&c, sizeof(c));

    if (c.cmd == 1) { // старт
      // делаем 4 скана
      scanVert();
      scanHor();
      scanDiag1();
      scanDiag2();

      // возвращаемся в 0,0 и снова ждём
      startPos();
    }
  }
}
