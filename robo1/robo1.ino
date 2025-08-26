#include <AccelStepper.h>
#include <Servo.h>

// ======== MAPEAMENTO DE PINOS PARA LOLIN D1 R1 + CNC SHIELD V3 ========
// Este mapeamento é o padrão e está correto para a sua placa.
// Os nomes D0, D1, D2, etc., são os rótulos impressos na placa D1 R1.

// --- EIXO X ---
#define X_STEP_PIN D2   // GPIO5
#define X_DIR_PIN  D5   // GPIO4

// --- EIXO Y ---
#define Y_STEP_PIN D3   // GPIO14
#define Y_DIR_PIN  D6   // GPIO12

// --- EIXO Z ---
//#define Z_STEP_PIN D7   // GPIO13
//#define Z_DIR_PIN  D8   // GPIO15

// --- HABILITAÇÃO DOS DRIVERS ---
#define ENABLE_PIN D8  // GPIO16 - Controla todos os drivers ao mesmo tempo

// --- SERVO ---
#define SERVO_PIN  D4   // GPIO2

// Inicializa os objetos para controle dos motores
AccelStepper motorX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper motorY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
//AccelStepper motorZ(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);

// Inicializa o objeto para controle do servo
Servo myServo;

void setup() {
  // Inicia a comunicação com o computador para debug e envio de comandos
  Serial.begin(115200);
  delay(1000); // Pequena pausa para estabilizar a comunicação

  // Configura o pino de habilitação como saída
  pinMode(ENABLE_PIN, OUTPUT);
  // Ativa os drivers dos motores de passo. Em drivers comuns (A4988/DRV8825), LOW = Habilitado
  digitalWrite(ENABLE_PIN, LOW);

  // Configura a velocidade máxima e aceleração para cada motor
  // Você pode ajustar estes valores conforme a necessidade do seu projeto
  motorX.setMaxSpeed(1000);
  motorX.setAcceleration(500);

  motorY.setMaxSpeed(1000);
  motorY.setAcceleration(500);

  //motorZ.setMaxSpeed(1000);
  //motorZ.setAcceleration(500);

  // Anexa o servo ao pino D4 (GPIO2)
  myServo.attach(SERVO_PIN);

  // Imprime um menu de opções no Monitor Serial
  Serial.println("\n=== Sistema Pronto: LOLIN D1 R1 + CNC Shield ===");
  Serial.println("Envie um comando pelo Serial Monitor:");
  Serial.println("1 - Testar motor do Eixo X");
  Serial.println("2 - Testar motor do Eixo Y");
  Serial.println("3 - Testar motor do Eixo Z");
  Serial.println("4 - Testar Servo Motor");
}

void loop() {
  // Verifica se algum comando foi enviado pelo Serial Monitor
  if (Serial.available()) {
    char command = Serial.read(); // Lê o comando enviado

    switch (command) {
      case '1':
        Serial.println("Movendo Eixo X...");
        motorX.moveTo(800); // Move 800 passos para frente
        while (motorX.isRunning()) motorX.run(); // Aguarda o movimento terminar
        delay(500);
        motorX.moveTo(0);   // Move de volta para a posição inicial
        while (motorX.isRunning()) motorX.run(); // Aguarda o movimento terminar
        Serial.println("Teste do Eixo X concluído.");
        break;

      case '2':
        Serial.println("Movendo Eixo Y...");
        motorY.moveTo(800);
        while (motorY.isRunning()) motorY.run();
        delay(500);
        motorY.moveTo(0);
        while (motorY.isRunning()) motorY.run();
        Serial.println("Teste do Eixo Y concluído.");
        break;

      case '3':
        Serial.println("Movendo Eixo Z...");
        //motorZ.moveTo(800);
     //   while (motorZ.isRunning()) motorZ.run();
        delay(500);
       // motorZ.moveTo(0);
     //   while (motorZ.isRunning()) motorZ.run();
        Serial.println("Teste do Eixo Z concluído.");
        break;

      case '4':
        Serial.println("Movendo Servo...");
        myServo.write(0);   // Posição 0 graus
        delay(500);
        myServo.write(90);  // Posição 90 graus
        delay(500);
        myServo.write(180); // Posição 180 graus
        delay(500);
        myServo.write(0);   // Retorna para a posição 0
        Serial.println("Teste do Servo concluído.");
        break;
    }
  }
}