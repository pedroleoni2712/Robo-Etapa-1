#include <AccelStepper.h>
#include <Servo.h>

// =================================================================
//  CONTROLE COM ACCELSTEPPER — MOVIMENTOS INDEPENDENTES
//  Versão: 6 (Velocidade reduzida e comando de uma volta)
// =================================================================

// --- EIXO X ---
#define X_STEP_PIN D2
#define X_DIR_PIN  D5
// --- EIXO Y ---
#define Y_STEP_PIN D3
#define Y_DIR_PIN  D6
// --- ENABLE ---
#define ENABLE_PIN D8
// --- SERVO ---
// #define SERVO_PIN D7

AccelStepper motorX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper motorY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
Servo myServo;

String inputString = "";
bool stringComplete = false;

// ===================== PARÂMETROS AJUSTADOS =====================
const int VEL_FIXA = 2000;    // VELOCIDADE REDUZIDA PARA TESTE
const long PASSOS_POR_VOLTA = 3200; // Assumindo motor de 200 passos e 1/16 microstepping
// =============================================================

// Helper: aplica velocidade/aceleração fixa
void aplicarPerfilFixo(AccelStepper &m) {
  m.setMaxSpeed(VEL_FIXA);
  m.setAcceleration(VEL_FIXA / 2); // Aceleração também é reduzida
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // drivers desabilitados

  aplicarPerfilFixo(motorX);
  aplicarPerfilFixo(motorY);

  // myServo.attach(SERVO_PIN);

  inputString.reserve(32);
  imprimirMenu();
}

void loop() {
  if (stringComplete) {
    processarComando();
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

void imprimirMenu() {
  Serial.println("\n===== MENU DE COMANDOS (Velocidade de Teste) =====");
  Serial.println("--- Comandos Especiais ---");
  Serial.print("V - Girar Eixo X por uma volta ("); Serial.print(PASSOS_POR_VOLTA); Serial.println(" passos)");
  Serial.println("\n--- Testes Simples ---");
  Serial.println("1 - Testar Eixo X (ida e volta)");
  Serial.println("2 - Testar Eixo Y (ida e volta)");
  Serial.println("\n--- Comandos Avançados ---");
  Serial.println("X[passos] -> Move X relativo. Ex: X16000");
  Serial.println("Y[passos] -> Move Y relativo. Ex: Y-40000");
  Serial.print  ("Velocidade de teste = ");
  Serial.print(VEL_FIXA); Serial.println(" passos/s");
  Serial.println("================================================");
}

void processarComando() {
  inputString.trim();

  char commandChar = toupper(inputString.charAt(0));
  long value = atol(inputString.substring(1).c_str());

  switch (commandChar) {
    case 'V': { // NOVO COMANDO PARA DAR UMA VOLTA
      Serial.print("Girando Eixo X por uma volta (");
      Serial.print(PASSOS_POR_VOLTA); Serial.println(" passos)...");

      digitalWrite(ENABLE_PIN, LOW); delay(10);
      aplicarPerfilFixo(motorX);
      motorX.move(PASSOS_POR_VOLTA);
      while (motorX.distanceToGo() != 0) {
          motorX.run();
      }
      motorX.setCurrentPosition(0); // Zera a contagem
      digitalWrite(ENABLE_PIN, HIGH);
      Serial.println("OK (Volta completa).");
      break;
    }
    case 'X': {
      Serial.print("X "); Serial.print(value); Serial.println(" passos...");
      digitalWrite(ENABLE_PIN, LOW); delay(10);
      aplicarPerfilFixo(motorX);
      motorX.move(value);
      while (motorX.distanceToGo() != 0) {
          motorX.run();
      }
      motorX.setCurrentPosition(0);
      digitalWrite(ENABLE_PIN, HIGH);
      Serial.println("OK (X).");
      break;
    }
    case 'Y': {
      Serial.print("Y "); Serial.print(value); Serial.println(" passos...");
      digitalWrite(ENABLE_PIN, LOW); delay(10);
      aplicarPerfilFixo(motorY);
      motorY.move(value);
      while (motorY.distanceToGo() != 0) {
          motorY.run();
      }
      motorY.setCurrentPosition(0);
      digitalWrite(ENABLE_PIN, HIGH);
      Serial.println("OK (Y).");
      break;
    }
    case '1': case '2': { // Comandos de teste simples
        processarComandoSimples(commandChar);
        break;
    }
    default:
      Serial.println("Comando inválido.");
  }
}

// Função para os comandos simples de teste
void processarComandoSimples(char comando) {
    switch (comando) {
        case '1': {
            Serial.println("Teste X...");
            digitalWrite(ENABLE_PIN, LOW); delay(10);
            aplicarPerfilFixo(motorX);
            motorX.move(800);
            while (motorX.distanceToGo() != 0) motorX.run();
            delay(300);
            motorX.move(-800);
            while (motorX.distanceToGo() != 0) motorX.run();
            motorX.setCurrentPosition(0);
            digitalWrite(ENABLE_PIN, HIGH);
            Serial.println("OK (X).");
            break;
        }
        case '2': {
            Serial.println("Teste Y...");
            digitalWrite(ENABLE_PIN, LOW); delay(10);
            aplicarPerfilFixo(motorY);
            motorY.move(800);
            while (motorY.distanceToGo() != 0) motorY.run();
            delay(300);
            motorY.move(-800);
            while (motorY.distanceToGo() != 0) motorY.run();
            motorY.setCurrentPosition(0);
            digitalWrite(ENABLE_PIN, HIGH);
            Serial.println("OK (Y).");
            break;
        }
    }
}