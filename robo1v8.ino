// =================================================================
//  CONTROLE COM ACCELSTEPPER + BLYNK IoT (v1.x)
//  VERSÃO NÃO-BLOQUEANTE E ATUALIZADA V8
// =================================================================

// -> Encontre estas informações na aba "Device Info" do seu dispositivo no Blynk
#define BLYNK_TEMPLATE_ID "TMPL23VP6xLBv"
#define BLYNK_TEMPLATE_NAME "Teste"
#define BLYNK_AUTH_TOKEN "FVVYAPzMEosJWP2eCs8wQKTXh5a6YRN-"

// Habilita a comunicação serial para debug do Blynk
#define BLYNK_PRINT Serial 

#include <AccelStepper.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// --- CREDENCIAIS WI-FI ---
char ssid[] = "pedro";          // <<< SSID da rede
char pass[] = "1234567p";       // <<< Senha do Wi-Fi

// --- PINAGEM ---
#define X_STEP_PIN D2
#define X_DIR_PIN  D5
#define Y_STEP_PIN D3
#define Y_DIR_PIN  D6
#define ENABLE_PIN D8
#define SERVO_PIN  D7

// --- OBJETOS ---
AccelStepper motorX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper motorY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
Servo myServo;

// ===================== PARÂMETROS =====================
const int VEL_FIXA = 3000;
const long PASSOS_POR_VOLTA = 3200;
// ======================================================

void aplicarPerfilFixo(AccelStepper &m) {
  m.setMaxSpeed(VEL_FIXA);
  m.setAcceleration(VEL_FIXA / 2);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);

  aplicarPerfilFixo(motorX);
  aplicarPerfilFixo(motorY);

  myServo.attach(SERVO_PIN);

  Serial.println("Conectando ao Blynk IoT...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// ================== HANDLERS DO BLYNK ==================

// V0 -> Campo de texto para comandos gerais
BLYNK_WRITE(V0) {
  String cmd = param.asStr();
  cmd.trim();
  if (cmd.length() == 0) return;

  char commandChar = toupper(cmd.charAt(0));
  long value = atol(cmd.substring(1).c_str());

  switch (commandChar) {
    case 'V':
      digitalWrite(ENABLE_PIN, LOW);
      motorX.move(PASSOS_POR_VOLTA);
      Blynk.virtualWrite(V4, "OK (Volta completa X)");
      break;
    case 'X':
      digitalWrite(ENABLE_PIN, LOW);
      motorX.move(value);
      Blynk.virtualWrite(V4, "OK (Movendo X)");
      break;
    case 'Y':
      digitalWrite(ENABLE_PIN, LOW);
      motorY.move(value);
      Blynk.virtualWrite(V4, "OK (Movendo Y)");
      break;
    case 'S':
      if (value >= 0 && value <= 180) {
        myServo.write(value);
        Blynk.virtualWrite(V4, "Servo -> " + String(value));
      } else {
        Blynk.virtualWrite(V4, "Servo: valor inválido");
      }
      break;
    default:
      Blynk.virtualWrite(V4, "Comando inválido!");
  }
}

// V1 -> Botão -> Teste X (ida e volta)
BLYNK_WRITE(V1) {
  if (param.asInt()) {
    Blynk.virtualWrite(V4, "Iniciando Teste X...");
    digitalWrite(ENABLE_PIN, LOW);
    
    motorX.move(800);
    motorX.runToPosition();
    delay(300);
    motorX.move(-800);
    motorX.runToPosition();
    
    digitalWrite(ENABLE_PIN, HIGH);
    Blynk.virtualWrite(V4, "Teste X Concluído");
  }
}

// V2 -> Botão -> Teste Y (ida e volta)
BLYNK_WRITE(V2) {
  if (param.asInt()) {
    Blynk.virtualWrite(V4, "Iniciando Teste Y...");
    digitalWrite(ENABLE_PIN, LOW);

    motorY.move(800);
    motorY.runToPosition();
    delay(300);
    motorY.move(-800);
    motorY.runToPosition();

    digitalWrite(ENABLE_PIN, HIGH);
    Blynk.virtualWrite(V4, "Teste Y Concluído");
  }
}

// V3 -> Botão -> Teste X e Y juntos (ida e volta)
BLYNK_WRITE(V3) {
  if (param.asInt()) {
    Blynk.virtualWrite(V4, "Iniciando Teste XY...");
    digitalWrite(ENABLE_PIN, LOW);

    motorX.move(800);
    motorY.move(800);
    while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
      motorX.run();
      motorY.run();
    }

    delay(300);

    motorX.move(-800);
    motorY.move(-800);
    while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
      motorX.run();
      motorY.run();
    }

    digitalWrite(ENABLE_PIN, HIGH);
    Blynk.virtualWrite(V4, "Teste XY Concluído");
  }
}

// V5 -> Text Input -> Movimenta X e Y juntos pelo valor digitado
BLYNK_WRITE(V5) {
  long passos = param.asLong();
  if (passos == 0) {
    Blynk.virtualWrite(V4, "Digite um valor diferente de 0");
    return;
  }

  Blynk.virtualWrite(V4, "Movendo X e Y juntos: " + String(passos) + " passos");
  digitalWrite(ENABLE_PIN, LOW);

  motorX.move(passos);
  motorY.move(passos);
}

// V6 -> Slider -> Move X e Y juntos conforme o valor do slider
BLYNK_WRITE(V6) {
  long passos = param.asLong();
  if (passos == 0) {
    Blynk.virtualWrite(V4, "Slider em 0 (sem movimento)");
    return;
  }

  Blynk.virtualWrite(V4, "Slider XY -> " + String(passos) + " passos");
  digitalWrite(ENABLE_PIN, LOW);

  motorX.move(passos);
  motorY.move(passos);
}

// ======================================================

void loop() {
  Blynk.run();
  motorX.run();
  motorY.run();

  if (motorX.distanceToGo() == 0 && motorY.distanceToGo() == 0) {
    digitalWrite(ENABLE_PIN, HIGH);
  }
}
