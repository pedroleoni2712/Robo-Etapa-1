// =================================================================
//  CONTROLE COM ACCELSTEPPER + BLYNK IoT
//  VERSÃO V14 - 4 MOTORES + 2 SERVOS (AÇÃO MOMENTÂNEA) - COMPLETO
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
char ssid[] = "pedro";      // <<< SSID da rede
char pass[] = "1234567p";  // <<< Senha do Wi-Fi

// --- PINAGEM ---
#define X_STEP_PIN D2
#define X_DIR_PIN  D5
#define Y_STEP_PIN D3
#define Y_DIR_PIN  D6
#define Z_STEP_PIN D4
#define Z_DIR_PIN  D7
#define E_STEP_PIN D12
#define E_DIR_PIN  D13

#define ENABLE_PIN D8
#define SERVO_PIN  D9
#define SERVO2_PIN D10 // <<< ADICIONADO: Pino para o segundo servo

// --- OBJETOS ---
AccelStepper motorX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper motorY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
AccelStepper motorZ(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);
AccelStepper motorE(AccelStepper::DRIVER, E_STEP_PIN, E_DIR_PIN);
Servo myServo;
Servo myServo2;       // <<< ADICIONADO: Objeto para o segundo servo

// ===================== PARÂMETROS =====================
const int VEL_FIXA = 400;
const long PASSOS_POR_VOLTA = 3200;
const long PASSOS_TESTE = 800;

// Posições dos servos
const int SERVO_POS_REPOUSO = 0;   // Posição quando o botão está solto
const int SERVO_POS_ACIONADO = 180; // Posição quando o botão está pressionado
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
  aplicarPerfilFixo(motorZ);
  aplicarPerfilFixo(motorE);

  myServo.attach(SERVO_PIN);
  myServo2.attach(SERVO2_PIN);

  // Inicia os servos na posição de repouso
  myServo.write(SERVO_POS_REPOUSO);
  myServo2.write(SERVO_POS_REPOUSO);

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
    case 'Z':
      digitalWrite(ENABLE_PIN, LOW);
      motorZ.move(value);
      Blynk.virtualWrite(V4, "OK (Movendo Z)");
      break;
    case 'E':
      digitalWrite(ENABLE_PIN, LOW);
      motorE.move(value);
      Blynk.virtualWrite(V4, "OK (Movendo E)");
      break;
    case 'S': // Comando para o Servo 1
      if (value >= 0 && value <= 180) {
        myServo.write(value);
        Blynk.virtualWrite(V4, "Servo 1 -> " + String(value));
      } else {
        Blynk.virtualWrite(V4, "Servo 1: valor inválido");
      }
      break;
    case 'A': // Comando 'A' (Auxiliar) para o Servo 2
      if (value >= 0 && value <= 180) {
        myServo2.write(value);
        Blynk.virtualWrite(V4, "Servo 2 -> " + String(value));
      } else {
        Blynk.virtualWrite(V4, "Servo 2: valor inválido");
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
    
    motorX.move(PASSOS_TESTE);
    motorX.runToPosition();
    delay(300);
    motorX.move(-PASSOS_TESTE);
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

    motorY.move(PASSOS_TESTE);
    motorY.runToPosition();
    delay(300);
    motorY.move(-PASSOS_TESTE);
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

    motorX.move(PASSOS_TESTE);
    motorY.move(PASSOS_TESTE);
    while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
      motorX.run();
      motorY.run();
      Blynk.run();
    }

    delay(300);

    motorX.move(-PASSOS_TESTE);
    motorY.move(-PASSOS_TESTE);
    while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
      motorX.run();
      motorY.run();
      Blynk.run();
    }
    
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

  Blynk.virtualWrite(V4, "Slider XY -> " + String(passos) + " passos");
  digitalWrite(ENABLE_PIN, LOW);

  motorX.moveTo(passos);
  motorY.moveTo(passos);
}

// V7 -> Botão -> Teste X (+800) e E (-800) juntos (ida e volta)
BLYNK_WRITE(V7) {
  if (param.asInt()) {
    Blynk.virtualWrite(V4, "Iniciando Teste XE (Oposto)...");
    digitalWrite(ENABLE_PIN, LOW);

    motorX.move(800);
    motorE.move(-800);
    while (motorX.distanceToGo() != 0 || motorE.distanceToGo() != 0) {
      motorX.run();
      motorE.run();
      Blynk.run();
    }

    delay(300);

    motorX.move(-800);
    motorE.move(800);
    while (motorX.distanceToGo() != 0 || motorE.distanceToGo() != 0) {
      motorX.run();
      motorE.run();
      Blynk.run();
    }

    Blynk.virtualWrite(V4, "Teste XE Concluído");
  }
}

// V8 -> Botão -> Teste Y (+800) e Z (-800) juntos (ida e volta)
BLYNK_WRITE(V8) {
  if (param.asInt()) {
    Blynk.virtualWrite(V4, "Iniciando Teste YZ (Oposto)...");
    digitalWrite(ENABLE_PIN, LOW);

    motorY.move(800);
    motorZ.move(-800);
    while (motorY.distanceToGo() != 0 || motorZ.distanceToGo() != 0) {
      motorY.run();
      motorZ.run();
      Blynk.run();
    }

    delay(300);

    motorY.move(-800);
    motorZ.move(800);
    while (motorY.distanceToGo() != 0 || motorZ.distanceToGo() != 0) {
      motorY.run();
      motorZ.run();
      Blynk.run();
    }
    
    Blynk.virtualWrite(V4, "Teste YZ Concluído");
  }
}

// ####################################################################
// ##            CONTROLES PARA OS SERVOS (AÇÃO MOMENTÂNEA)          ##
// ####################################################################

// V9 -> Botão -> Move o Servo 1 (D9) enquanto pressionado
BLYNK_WRITE(V9) {
  int estadoBotao = param.asInt(); // Recebe 1 ao pressionar, 0 ao soltar

  if (estadoBotao == 1) {
    // Se o botão está pressionado, move para a posição ACIONADO
    myServo.write(SERVO_POS_ACIONADO);
    Blynk.virtualWrite(V4, "Servo 1: Ativado");
  } else {
    // Se o botão foi solto, volta para a posição de REPOUSO
    myServo.write(SERVO_POS_REPOUSO);
    Blynk.virtualWrite(V4, "Servo 1: Em repouso");
  }
}

// V10 -> Botão -> Move o Servo 2 (D10) enquanto pressionado
BLYNK_WRITE(V10) {
  int estadoBotao = param.asInt(); // Recebe 1 ao pressionar, 0 ao soltar

  if (estadoBotao == 1) {
    // Se o botão está pressionado, move para a posição ACIONADO
    myServo2.write(SERVO_POS_ACIONADO);
    Blynk.virtualWrite(V4, "Servo 2: Ativado");
  } else {
    // Se o botão foi solto, volta para a posição de REPOUSO
    myServo2.write(SERVO_POS_REPOUSO);
    Blynk.virtualWrite(V4, "Servo 2: Em repouso");
  }
}

// ======================================================

void loop() {
  Blynk.run();
  
  motorX.run();
  motorY.run();
  motorZ.run();
  motorE.run();

  if (motorX.distanceToGo() == 0 && motorY.distanceToGo() == 0 && motorZ.distanceToGo() == 0 && motorE.distanceToGo() == 0) {
    digitalWrite(ENABLE_PIN, HIGH);
  }
}