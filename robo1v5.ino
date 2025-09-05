#include <AccelStepper.h>
#include <Servo.h>

// =================================================================
//      CÓDIGO COMPLETO E CORRIGIDO PARA CONTROLE DE
//       MOTORES DE PASSO (X, Y) E SERVO MOTOR
// =================================================================
// Versão: 5 (Correção no parse dos comandos do Serial)
// - Agora remove \r, \n e espaços antes de processar o comando.
// - Comando X16000 agora funciona corretamente (não corta para 1600).
// =================================================================


// --- EIXO X ---
#define X_STEP_PIN D2
#define X_DIR_PIN  D5

// --- EIXO Y ---
#define Y_STEP_PIN D3
#define Y_DIR_PIN  D6

// --- HABILITAÇÃO DOS DRIVERS ---
#define ENABLE_PIN D8

// --- SERVO ---
//#define SERVO_PIN  D7

// Inicializa os objetos para controle dos motores
AccelStepper motorX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper motorY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);

// Inicializa o objeto para controle do servo
Servo myServo;

// Variáveis para ler comandos avançados do Serial Monitor
String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // começa desabilitado

  // Configuração inicial dos motores
  motorX.setMaxSpeed(1000);
  motorX.setAcceleration(500);
  motorY.setMaxSpeed(1000);
  motorY.setAcceleration(500);

  // myServo.attach(SERVO_PIN);

  inputString.reserve(20);

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
    if (inChar == '\n' || inChar == '\r') { // aceita Enter como final
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

void imprimirMenu() {
  Serial.println("\n===== MENU DE COMANDOS ATUALIZADO =====");
  Serial.println("--- Testes Simples ---");
  Serial.println("1 - Testar motor do Eixo X (ida e volta)");
  Serial.println("2 - Testar motor do Eixo Y (ida e volta)");
  Serial.println("3 - Testar Servo Motor (varredura 0-180)");
  Serial.println("4 - Testar Eixos X e Y juntos");
  Serial.println("\n--- Comandos Avançados (Envie e aperte Enter) ---");
  Serial.println("X[passos] -> Move o motor X. Ex: X16000 ou X-50000");
  Serial.println("Y[passos] -> Move o motor Y. Ex: Y-40000");
  Serial.println("S[angulo] -> Move o servo para um ângulo. Ex: S90");
  Serial.println("V[velocidade] -> Altera a velocidade dos motores. Ex: V2000");
  Serial.println("=========================================");
}

void processarComando() {
  inputString.trim(); // <<< remove espaços, \r, \n extras

  if (inputString.length() == 1) {
    switch (inputString.charAt(0)) {
      case '1':
        Serial.println("Habilitando drivers e movendo Eixo X...");
        digitalWrite(ENABLE_PIN, LOW);
        delay(10);

        motorX.move(800);
        while (motorX.distanceToGo() != 0) motorX.run();
        delay(500);
        motorX.move(-800);
        while (motorX.distanceToGo() != 0) motorX.run();

        digitalWrite(ENABLE_PIN, HIGH);
        Serial.println("Teste do Eixo X concluído. Motores desabilitados.");
        break;

      case '2':
        Serial.println("Habilitando drivers e movendo Eixo Y...");
        digitalWrite(ENABLE_PIN, LOW);
        delay(10);

        motorY.move(800);
        while (motorY.distanceToGo() != 0) motorY.run();
        delay(500);
        motorY.move(-800);
        while (motorY.distanceToGo() != 0) motorY.run();

        digitalWrite(ENABLE_PIN, HIGH);
        Serial.println("Teste do Eixo Y concluído. Motores desabilitados.");
        break;

      case '3':
        Serial.println("Movendo Servo...");
        myServo.write(0); delay(500);
        myServo.write(90); delay(500);
        myServo.write(180); delay(500);
        myServo.write(90);
        Serial.println("Teste do Servo concluído.");
        break;

      case '4':
        Serial.println("Habilitando drivers e movendo Eixos X e Y juntos...");
        digitalWrite(ENABLE_PIN, LOW);
        delay(10);

        motorX.moveTo(1600);
        motorY.moveTo(1600);
        while (motorX.isRunning() || motorY.isRunning()) {
          motorX.run();
          motorY.run();
        }
        delay(500);
        motorX.moveTo(0);
        motorY.moveTo(0);
        while (motorX.isRunning() || motorY.isRunning()) {
          motorX.run();
          motorY.run();
        }

        digitalWrite(ENABLE_PIN, HIGH);
        Serial.println("Teste conjunto concluído. Motores desabilitados.");
        break;

      default:
        Serial.println("Comando simples inválido.");
        break;
    }
    return;
  }

  char commandChar = toupper(inputString.charAt(0));
  long value = atol(inputString.substring(1).c_str());

  switch (commandChar) {
    case 'X':
      Serial.print("Comando: Mover X por ");
      Serial.print(value);
      Serial.println(" passos.");

      digitalWrite(ENABLE_PIN, LOW);
      delay(10);

      motorX.move(value);
      while (motorX.distanceToGo() != 0) motorX.run();

      digitalWrite(ENABLE_PIN, HIGH);
      Serial.println("Movimento X concluído. Motores desabilitados.");
      break;

    case 'Y':
      Serial.print("Comando: Mover Y por ");
      Serial.print(value);
      Serial.println(" passos.");

      digitalWrite(ENABLE_PIN, LOW);
      delay(10);

      motorY.move(value);
      while (motorY.distanceToGo() != 0) motorY.run();

      digitalWrite(ENABLE_PIN, HIGH);
      Serial.println("Movimento Y concluído. Motores desabilitados.");
      break;

    case 'S':
      if (value >= 0 && value <= 180) {
        Serial.print("Comando: Mover Servo para ");
        Serial.print(value);
        Serial.println(" graus.");
        myServo.write(value);
      } else {
        Serial.println("Ângulo do servo deve ser entre 0 e 180.");
      }
      break;

    case 'V':
      Serial.print("Comando: Alterar velocidade máxima para ");
      Serial.println(value);
      motorX.setMaxSpeed(value);
      motorY.setMaxSpeed(value);
      motorX.setAcceleration(value / 2);
      motorY.setAcceleration(value / 2);
      break;

    default:
      Serial.println("Comando avançado inválido.");
      break;
  }
}
