#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>

// ================== PINOS DO CNC SHIELD ==================
#define X_STEP D2
#define X_DIR  D5
#define Y_STEP D3
#define Y_DIR  D6
#define Z_STEP D4
#define Z_DIR  D7
#define A_STEP D12 // O pino A_STEP original era D1, mas D1 é TX. Usando D12 (pino A-STEP no shield v3)
#define A_DIR  D13 // O pino A_DIR original era D10. Usando D13 (pino A-DIR no shield v3)
#define EN     D8

#define SERVO1_PIN D9 // O pino D11 (Z-END) pode ser usado para Servo
#define SERVO2_PIN D0 // O pino D10 (Y-END) pode ser usado para Servo

Servo servo1;
Servo servo2;

// ================== OBJETOS ==================
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP, X_DIR);
AccelStepper stepperA(AccelStepper::DRIVER, A_STEP, A_DIR);
AccelStepper stepperY(AccelStepper::DRIVER, Y_STEP, Y_DIR);
AccelStepper stepperZ(AccelStepper::DRIVER, Z_STEP, Z_DIR);

MultiStepper eixoVertical;   // X + A
MultiStepper eixoHorizontal; // Y + Z

// ================== VARIÁVEIS DE ESTADO ==================
long posVertical[2] = {0, 0};   // X e A
long posHorizontal[2] = {0, 0}; // Y e Z

// ================== DECLARAÇÃO DE FUNÇÕES ==================
void parseAndExecuteCommand(String command);
bool parseSingleLineCommand(String input, long &Vx, long &Dx, long &Vy, long &Dy, long &Va, long &Da, long &Vz, long &Dz);
void fazerCruz();
void moverVertical(long passos);
void moverHorizontal(long passos);
void moveSingleStepper(AccelStepper &stepper, long steps);

// ================== CONFIG GERAL ==================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSistema de Controle de Motores Iniciado");

  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH); // Inicia com motores desabilitados

  stepperX.setAcceleration(800);
  stepperA.setAcceleration(800);
  stepperY.setAcceleration(800);
  stepperZ.setAcceleration(800);

  // Ajuste as inversões conforme necessário para seu gantry
  stepperX.setPinsInverted(false, false, false);
  stepperA.setPinsInverted(true,  false, false); // Invertido para gantry
  stepperY.setPinsInverted(false, false, false);
  stepperZ.setPinsInverted(true,  false, false); // Invertido para gantry

  // Agrupa motores para movimento gantry (usado pela rotina da cruz)
  eixoVertical.addStepper(stepperX);
  eixoVertical.addStepper(stepperA);
  eixoHorizontal.addStepper(stepperY);
  eixoHorizontal.addStepper(stepperZ);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);
  servo2.write(0);

  Serial.println("Sistema pronto. Aguardando comandos via Serial...");
  Serial.println("Formatos de comando:");
  Serial.println("1. Manual: X100, Y-200, S90, A180");
  Serial.println("2. Rotina: fazer_cruz");
  Serial.println("3. 8 Vars: Vx, Dx, Vy, Dy, Va, Da, Vz, Dz (ex: 800, 1000, 600, -500, 800, 1000, 600, -500)");
}

// ================== FUNÇÕES DE MOVIMENTO GANTRY (Para a Cruz) ==================
void moverVertical(long passos) {
  posVertical[0] += passos; // X
  posVertical[1] += passos; // A
  digitalWrite(EN, LOW);
  eixoVertical.moveTo(posVertical);
  while(eixoVertical.run()) {
    yield();
  }
  digitalWrite(EN, HIGH);
}

void moverHorizontal(long passos) {
  posHorizontal[0] += passos; // Y
  posHorizontal[1] += passos; // Z
  digitalWrite(EN, LOW);
  eixoHorizontal.moveTo(posHorizontal);
  while(eixoHorizontal.run()) {
    yield();
  }
  digitalWrite(EN, HIGH);
}

// ================== ROTINA DA CRUZ ==================
void fazerCruz() {
  Serial.println("Executando Rotina da Cruz...");

  long passosMetade = 700;

  servo1.write(180); // Abaixa servo
  delay(500);

  // Movimentos horizontais (Y + Z)
  moverHorizontal(passosMetade / 2);
  delay(1000);
  moverHorizontal(-passosMetade);
  delay(1000);
  moverHorizontal(passosMetade / 2);
  delay(1000);

  // Movimentos verticais (X + A)
  moverVertical(passosMetade / 2);
  delay(1000);
  moverVertical(-passosMetade);
  delay(1000);
  moverVertical(passosMetade / 2);
  delay(1000);

  servo1.write(0); // Levanta servo
  delay(500);

  Serial.println("Rotina da Cruz Concluída.");
}

// ================== MOVIMENTO SIMPLES (Manual) ==================
void moveSingleStepper(AccelStepper &stepper, long steps) {
  digitalWrite(EN, LOW);
  stepper.setCurrentPosition(0);
  stepper.move(steps);
  // Define velocidade e aceleração padrão para movimentos manuais
  stepper.setMaxSpeed(800); 
  stepper.setAcceleration(400);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    yield();
  }
  digitalWrite(EN, HIGH);
}

// ================== LER LINHA ÚNICA (8 Variáveis) ==================
bool parseSingleLineCommand(String input, long &Vx, long &Dx, long &Vy, long &Dy, long &Va, long &Da, long &Vz, long &Dz) {
  input.trim();
  input.replace(",", " "); // Substitui vírgulas por espaços
  input.replace(";", " "); // Substitui ponto e vírgula por espaços
  
  // Remove espaços duplicados
  while (input.indexOf("  ") >= 0) {
    input.replace("  ", " ");
  }
  
  long values[8];
  int count = 0;
  
  int lastPos = 0;
  for (int i = 0; i < input.length() && count < 8; i++) {
    // Encontra um separador (espaço) ou o fim da string
    if (input.charAt(i) == ' ' || i == input.length() - 1) {
      String token;
      // Pega o último token se a string terminar sem espaço
      if (i == input.length() - 1 && input.charAt(i) != ' ') {
        token = input.substring(lastPos);
      } else {
        token = input.substring(lastPos, i);
      }
      
      token.trim();
      if (token.length() > 0) {
        values[count++] = token.toInt();
      }
      lastPos = i + 1; // Próxima posição inicial do token
    }
  }
  
  // Verifica se exatamente 8 valores foram encontrados
  if (count != 8) {
    // Não imprime erro aqui, pois pode ser um comando manual
    return false;
  }
  
  Vx = values[0];
  Dx = values[1];
  Vy = values[2];
  Dy = values[3];
  Va = values[4];
  Da = values[5];
  Vz = values[6];
  Dz = values[7];
  
  return true;
}

// ================== PARSER COMANDO MANUAL ==================
void parseAndExecuteCommand(String command) {
  command.toUpperCase();
  if (command.length() < 2) {
    Serial.println("Erro: Comando inválido.");
    return;
  }

  char cmdType = command.charAt(0);
  long cmdValue = command.substring(1).toInt();

  switch (cmdType) {
    case 'X':
      moveSingleStepper(stepperX, cmdValue);
      Serial.println("Motor X movido " + String(cmdValue) + " passos.");
      break;
    case 'Y':
      moveSingleStepper(stepperY, cmdValue);
      Serial.println("Motor Y movido " + String(cmdValue) + " passos.");
      break;
    case 'Z':
      moveSingleStepper(stepperZ, cmdValue);
      Serial.println("Motor Z movido " + String(cmdValue) + " passos.");
      break;
    case 'E': // 'E' é comumente usado para Extrusora, que aqui é o motor 'A'
    case 'A':
      moveSingleStepper(stepperA, cmdValue);
      Serial.println("Motor A(E) movido " + String(cmdValue) + " passos.");
      break;
    case 'S':
      if (cmdValue >= 0 && cmdValue <= 180) {
        servo1.write(cmdValue);
        Serial.println("Servo 1 movido para " + String(cmdValue) + "°.");
      } else {
        Serial.println("Erro: Ângulo do servo 1 inválido (0-180).");
      }
      break;
    // 'A' já está sendo usado para o motor A. Vamos usar 'B' para o Servo 2
    case 'B': 
      if (cmdValue >= 0 && cmdValue <= 180) {
        servo2.write(cmdValue);
        Serial.println("Servo 2 movido para " + String(cmdValue) + "°.");
      } else {
        Serial.println("Erro: Ângulo do servo 2 inválido (0-180).");
      }
      break;
    default:
      Serial.println("Erro: Comando '" + String(cmdType) + "' desconhecido.");
      break;
  }
}

// ================== LOOP PRINCIPAL ==================
void loop() {
  // Verifica se há dados disponíveis no Serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Lê até a nova linha
    command.trim(); // Remove espaços em branco

    if (command.length() == 0) {
      return; // Ignora comandos vazios
    }

    Serial.println("Comando recebido: \"" + command + "\"");

    long Vx, Dx, Vy, Dy, Va, Da, Vz, Dz;

    // 1. Tenta interpretar como um comando de 8 variáveis
    if (parseSingleLineCommand(command, Vx, Dx, Vy, Dy, Va, Da, Vz, Dz)) {
      Serial.println("Executando movimento simultâneo de 4 eixos...");
      Serial.printf("X (V,D): %ld, %ld | Y (V,D): %ld, %ld | A (V,D): %ld, %ld | Z (V,D): %ld, %ld\n",
                    Vx, Dx, Vy, Dy, Va, Da, Vz, Dz);

      digitalWrite(EN, LOW); // Habilita motores

      // Define velocidades e acelerações
      long speedX = constrain(abs(Vx), 0, 1600);
      long speedY = constrain(abs(Vy), 0, 1600);
      long speedA = constrain(abs(Va), 0, 1600);
      long speedZ = constrain(abs(Vz), 0, 1600);
      
      stepperX.setMaxSpeed(speedX);
      stepperY.setMaxSpeed(speedY);
      stepperA.setMaxSpeed(speedA);
      stepperZ.setMaxSpeed(speedZ);
      
      // Aceleração como metade da velocidade
      stepperX.setAcceleration(speedX / 2);
      stepperY.setAcceleration(speedY / 2);
      stepperA.setAcceleration(speedA / 2);
      stepperZ.setAcceleration(speedZ / 2);

      // Cria um objeto MultiStepper para todos os 4 motores
      MultiStepper allMotors;
      allMotors.addStepper(stepperX);
      allMotors.addStepper(stepperY);
      allMotors.addStepper(stepperA);
      allMotors.addStepper(stepperZ);

      // Define as posições alvo
      // Ordem: X, Y, A, Z (conforme adicionados ao allMotors)
      long targets[4] = {Dx, Dy, Da, Dz};

      // Zera a posição atual para movimento relativo
      stepperX.setCurrentPosition(0);
      stepperY.setCurrentPosition(0);
      stepperA.setCurrentPosition(0);
      stepperZ.setCurrentPosition(0);

      allMotors.moveTo(targets);
      
      // Executa o movimento até que todos cheguem ao destino
      // run() gerencia aceleração/desaceleração para todos os motores
      while (allMotors.run()) {
        yield(); // Cede tempo para outras tarefas do ESP8266
      }

      digitalWrite(EN, HIGH); // Desabilita motores
      Serial.println("Movimento de 4 eixos concluído.");

    } 
    // 2. Se não for 8-var, verifica se é a rotina da cruz
    else if (command.equalsIgnoreCase("cruz")) { // Modificado de "fazer_cruz" para "cruz"
      fazerCruz();
    } 
    // 3. Se não for nenhum dos anteriores, tenta interpretar como comando manual
    else {
      parseAndExecuteCommand(command);
    }
    
    Serial.println("\nAguardando novo comando...");
  }
}