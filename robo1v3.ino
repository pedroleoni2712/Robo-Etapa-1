#include <AccelStepper.h>
#include <Servo.h>

// ======== MAPEAMENTO DE PINOS CORRIGIDO PARA LOLIN D1 R1 + CNC SHIELD V3 ========
// O mapeamento foi ajustado para o padrão correto do shield para evitar conflitos.

// --- EIXO X ---
#define X_STEP_PIN D2   // GPIO5
#define X_DIR_PIN  D5  // GPIO4

// --- EIXO Y ---
#define Y_STEP_PIN D3   // GPIO14
#define Y_DIR_PIN  D6   // GPIO12

// --- HABILITAÇÃO DOS DRIVERS ---
// D8 estava em conflito com o Servo, corrigido para o pino padrão D0.
#define ENABLE_PIN D0   // GPIO16

// --- SERVO ---
// D8 estava em conflito com o Enable, corrigido para o pino padrão D4.
#define SERVO_PIN  D8  // GPIO2

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
  digitalWrite(ENABLE_PIN, LOW); // LOW = Habilitado

  // Configuração inicial dos motores
  motorX.setMaxSpeed(1000);
  motorX.setAcceleration(500);
  motorY.setMaxSpeed(1000);
  motorY.setAcceleration(500);

  myServo.attach(SERVO_PIN);
  inputString.reserve(20); // Reserva espaço na memória para a String

  // Imprime o novo menu de opções
  imprimirMenu();
}

void loop() {
  // Se um comando completo foi recebido (terminado com Enter)
  if (stringComplete) {
    processarComando(); // Chama a função para processar o comando
    inputString = "";   // Limpa a string para o próximo comando
    stringComplete = false;
  }
}

// Esta função é chamada automaticamente sempre que há dados no serial
// Ela lê os caracteres e monta a string de comando
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') { // Se o caractere for Enter (nova linha)
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
  Serial.println("X[passos] -> Move o motor X. Ex: X1600 ou X-200");
  Serial.println("Y[passos] -> Move o motor Y. Ex: Y-800");
  Serial.println("S[angulo] -> Move o servo para um ângulo. Ex: S90");
  Serial.println("V[velocidade] -> Altera a velocidade dos motores. Ex: V2000");
  Serial.println("=========================================");
}

// Função que interpreta e executa os comandos recebidos
void processarComando() {
  // Comandos de 1 caractere (testes simples)
  if (inputString.length() == 1) {
    switch (inputString.charAt(0)) {
      case '1':
        Serial.println("Movendo Eixo X...");
        motorX.move(800);
        while (motorX.distanceToGo() != 0) motorX.run();
        delay(500);
        motorX.move(-800);
        while (motorX.distanceToGo() != 0) motorX.run();
        Serial.println("Teste do Eixo X concluído.");
        break;
      case '2':
        Serial.println("Movendo Eixo Y...");
        motorY.move(800);
        while (motorY.distanceToGo() != 0) motorY.run();
        delay(500);
        motorY.move(-800);
        while (motorY.distanceToGo() != 0) motorY.run();
        Serial.println("Teste do Eixo Y concluído.");
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
        Serial.println("Movendo Eixos X e Y juntos...");
        motorX.moveTo(1600);
        motorY.moveTo(1600);
        // Roda os dois motores ao mesmo tempo até que ambos cheguem ao destino
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
        Serial.println("Teste conjunto concluído.");
        break;
      default:
        Serial.println("Comando simples inválido.");
        break;
    }
    return; // Sai da função após executar o comando simples
  }

  // Comandos avançados (com valor numérico)
  char commandChar = toupper(inputString.charAt(0));
  long value = inputString.substring(1).toInt();

  switch (commandChar) {
    case 'X':
      Serial.print("Comando: Mover X por ");
      Serial.print(value);
      Serial.println(" passos.");
      motorX.move(value);
      while (motorX.distanceToGo() != 0) {
        motorX.run();
      }
      Serial.println("Movimento X concluído.");
      break;
    case 'Y':
      Serial.print("Comando: Mover Y por ");
      Serial.print(value);
      Serial.println(" passos.");
      motorY.move(value);
      while (motorY.distanceToGo() != 0) {
        motorY.run();
      }
      Serial.println("Movimento Y concluído.");
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
      // Ajusta a aceleração proporcionalmente à nova velocidade
      motorX.setAcceleration(value / 2);
      motorY.setAcceleration(value / 2);
      break;
    default:
      Serial.println("Comando avançado inválido.");
      break;
  }
}