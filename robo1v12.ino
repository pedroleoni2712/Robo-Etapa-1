#include <ESP8266WiFi.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>

// ================== CONFIG WiFi ==================
const char* ssid = "ROBO_CNPEM";
const char* password = "12345678";
WiFiServer server(80);

// ================== PINOS DO CNC SHIELD ==================
// Driver X
#define X_STEP D2
#define X_DIR  D5
// Driver Y
#define Y_STEP D3
#define Y_DIR  D6
// Driver Z
#define Z_STEP D4
#define Z_DIR  D7
// Driver A (Controlled by 'E' command)
#define A_STEP D12
#define A_DIR  D13

#define EN     D8 // Enable compartilhado

// Servos
#define SERVO1_PIN D9 // Your original servo
#define SERVO2_PIN D0 // Added a second servo on pin D0

Servo servo1;
Servo servo2;

// ================== OBJETOS ==================
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP, X_DIR);
AccelStepper stepperA(AccelStepper::DRIVER, A_STEP, A_DIR); // This is now controlled by 'E' commands
AccelStepper stepperY(AccelStepper::DRIVER, Y_STEP, Y_DIR);
AccelStepper stepperZ(AccelStepper::DRIVER, Z_STEP, Z_DIR);

MultiStepper eixoVertical;   // A + X
MultiStepper eixoHorizontal; // Y + Z

// ================== VARIÁVEIS DE ESTADO ==================
long posVertical[2] = {0, 0};   // X e A
long posHorizontal[2] = {0, 0}; // Y e Z
String statusMessage = "Sistema pronto.";

// Forward declaration for HTML function
void sendHTML(WiFiClient client);

// ================== CONFIG GERAL ==================
void setup() {
  Serial.begin(115200);

  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH); // Start with drivers disabled

  // Configure stepper motors
  stepperX.setMaxSpeed(400);
  stepperX.setAcceleration(200);
  stepperA.setMaxSpeed(400);
  stepperA.setAcceleration(200);
  stepperY.setMaxSpeed(400);
  stepperY.setAcceleration(200);
  stepperZ.setMaxSpeed(400);
  stepperZ.setAcceleration(200);

  // Set motor directions
  stepperX.setPinsInverted(false, false, false);
  stepperA.setPinsInverted(true,  false, false);
  stepperY.setPinsInverted(false, false, false);
  stepperZ.setPinsInverted(true,  false, false);

  // Group motors for gantry movement (used in routines)
  eixoVertical.addStepper(stepperX);
  eixoVertical.addStepper(stepperA);
  eixoHorizontal.addStepper(stepperY);
  eixoHorizontal.addStepper(stepperZ);

  // Configure and attach servos
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0); // Set to initial "Repouso" position
  servo2.write(0); // Set to initial "Repouso" position

  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("Rede WiFi criada: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Start web server
  server.begin();
}

// ================== FUNÇÕES DE MOVIMENTO GANTRY (para rotinas) ==================
void moverVertical(long passos) {
  posVertical[0] += passos; // X
  posVertical[1] += passos; // A
  digitalWrite(EN, LOW);
  eixoVertical.moveTo(posVertical);
  while(eixoVertical.run()) {
    yield(); // <-- CORREÇÃO: Impede o reset do ESP8266 em movimentos longos
  }
  digitalWrite(EN, HIGH);
}

void moverHorizontal(long passos) {
  posHorizontal[0] += passos; // Y
  posHorizontal[1] += passos; // Z
  digitalWrite(EN, LOW);
  eixoHorizontal.moveTo(posHorizontal);
  while(eixoHorizontal.run()) {
    yield(); // <-- CORREÇÃO: Impede o reset do ESP8266 em movimentos longos
  }
  digitalWrite(EN, HIGH);
}

// ================== FUNÇÃO DE MOVIMENTO INDIVIDUAL (para comando manual) ==================
void moveSingleStepper(AccelStepper &stepper, long steps) {
    digitalWrite(EN, LOW);
    stepper.setCurrentPosition(0); // Zera a posição atual para um movimento relativo limpo
    stepper.move(steps);
    while (stepper.distanceToGo() != 0) {
        stepper.run();
        yield(); // <-- CORREÇÃO: Impede o reset do ESP8266 em movimentos longos
    }
    digitalWrite(EN, HIGH);
}


// ================== ROTINA DE TESTE: CRUZ ==================
void fazerCruz() {
  statusMessage = "Executando Rotina da Cruz...";
  
  // Move 6400 passos no eixo vertical (X+A) antes de começar
  moverVertical(6400);
  delay(1000); // Pausa por 1 segundo após o movimento inicial

  long passosMetade = 700; // Passos para desenhar a cruz

  servo1.write(180); // Lower servo
  delay(500);

  // Horizontal movements
  moverHorizontal(passosMetade / 2);
  delay(1000);
  moverHorizontal(-passosMetade);
  delay(1000);
  moverHorizontal(passosMetade / 2);
  delay(1000);

  // Vertical movements
  moverVertical(passosMetade / 2);
  delay(1000);
  moverVertical(-passosMetade);
  delay(1000);
  moverVertical(passosMetade / 2);
  delay(1000);

  servo1.write(0); // Raise servo
  delay(500);

  statusMessage = "Rotina da Cruz Concluída.";
  Serial.println("Cruz concluída!");
}

// ================== PARSER DE COMANDOS ==================
void parseAndExecuteCommand(String command) {
  command.toUpperCase();
  if (command.length() < 2) {
    statusMessage = "Erro: Comando inválido.";
    return;
  }

  char cmdType = command.charAt(0);
  long cmdValue = command.substring(1).toInt();

  switch (cmdType) {
    case 'X':
      moveSingleStepper(stepperX, cmdValue);
      statusMessage = "Motor X movido " + String(cmdValue) + " passos.";
      break;
    case 'Y':
      moveSingleStepper(stepperY, cmdValue);
      statusMessage = "Motor Y movido " + String(cmdValue) + " passos.";
      break;
    case 'Z':
      moveSingleStepper(stepperZ, cmdValue);
      statusMessage = "Motor Z movido " + String(cmdValue) + " passos.";
      break;
    case 'E': // 'E' command now controls the 'A' stepper
      moveSingleStepper(stepperA, cmdValue);
      statusMessage = "Motor E(A) movido " + String(cmdValue) + " passos.";
      break;
    case 'S': // Servo 1
      if (cmdValue >= 0 && cmdValue <= 180) {
        servo1.write(cmdValue);
        statusMessage = "Servo 1 movido para " + String(cmdValue) + "°.";
      } else {
        statusMessage = "Erro: Ângulo do servo 1 inválido.";
      }
      break;
    case 'A': // Servo 2
      if (cmdValue >= 0 && cmdValue <= 180) {
        servo2.write(cmdValue);
        statusMessage = "Servo 2 movido para " + String(cmdValue) + "°.";
      } else {
        statusMessage = "Erro: Ângulo do servo 2 inválido.";
      }
      break;
    default:
      statusMessage = "Erro: Comando '" + String(cmdType) + "' desconhecido.";
      break;
  }
}

// ================== LOOP PRINCIPAL ==================
void loop() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  String request = client.readStringUntil('\r');
  client.flush();

  bool actionTaken = false;

  // --- Processar Ações ---
  if (request.indexOf("GET /?comando=") >= 0) {
    int cmdPos = request.indexOf('=');
    int endPos = request.indexOf(' ', cmdPos);
    String command = request.substring(cmdPos + 1, endPos);
    command.trim();
    if (command.length() > 0) {
      parseAndExecuteCommand(command);
    }
    actionTaken = true;
  } else if (request.indexOf("/servo1_on") != -1) {
    servo1.write(180);
    statusMessage = "Servo 1 Ativado (180°).";
    actionTaken = true;
  } else if (request.indexOf("/servo1_off") != -1) {
    servo1.write(0);
    statusMessage = "Servo 1 em Repouso (0°).";
    actionTaken = true;
  } else if (request.indexOf("/servo2_on") != -1) {
    servo2.write(180);
    statusMessage = "Servo 2 Ativado (180°).";
    actionTaken = true;
  } else if (request.indexOf("/servo2_off") != -1) {
    servo2.write(0);
    statusMessage = "Servo 2 em Repouso (0°).";
    actionTaken = true;
  } else if (request.indexOf("/fazer_cruz") != -1) {
    fazerCruz(); // This function updates its own status
    actionTaken = true;
  }

  // --- Enviar Resposta ---
  if (actionTaken) {
    // Redirect to the root page to refresh the view and show the new status
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /");
    client.println();
  } else {
    // Serve the main HTML page
    sendHTML(client);
  }

  delay(1);
  client.stop();
}

// ================== GERADOR DE PÁGINA HTML ==================
void sendHTML(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML><html><head>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<title>Controle de Motores</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; }");
  client.println("h1 { text-align: center; color: #333; }");
  client.println(".container { max-width: 1200px; margin: auto; }");
  client.println(".status-bar { background-color: #e7f3fe; border-left: 6px solid #2196F3; padding: 15px; margin-bottom: 20px; }");
  client.println(".main-content { display: flex; flex-wrap: wrap; gap: 20px; }");
  client.println(".card { background-color: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); padding: 20px; flex: 1; min-width: 280px; }");
  client.println("h2 { color: #2196F3; margin-top: 0; border-bottom: 1px solid #eee; padding-bottom: 10px; }");
  client.println("table { width: 100%; border-collapse: collapse; }");
  client.println("th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }");
  client.println("th { background-color: #f8f8f8; }");
  client.println("input[type='text'] { width: calc(100% - 22px); padding: 10px; border: 1px solid #ccc; border-radius: 4px; }");
  client.println("button, .btn { background-color: #2196F3; color: white; border: none; padding: 12px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; border-radius: 5px; cursor: pointer; width: 100%; margin-top: 10px; }");
  client.println("button:hover, .btn:hover { background-color: #0b7dda; }");
  client.println(".btn-secondary { background-color: #6c757d; } .btn-secondary:hover { background-color: #5a6268; }");
  client.println(".servo-controls { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }");
  client.println(".footer-card { text-align: center; }");
  client.println("</style></head><body>");
  client.println("<div class='container'>");
  client.println("<h1>Controle de Motores via Wi-Fi</h1>");
  client.println("<div class='status-bar'><b>Status:</b> " + statusMessage + "</div>");
  client.println("<div class='main-content'>");
  // Card: Menu de Comandos
  client.println("<div class='card'><h2>Menu de Comandos</h2><table>");
  client.println("<tr><th>Comando</th><th>Descrição</th></tr>");
  client.println("<tr><td>X[passos]</td><td>Move o motor X. Ex: X1600</td></tr>");
  client.println("<tr><td>Y[passos]</td><td>Move o motor Y. Ex: Y-800</td></tr>");
  client.println("<tr><td>Z[passos]</td><td>Move o motor Z.</td></tr>");
  client.println("<tr><td>E[passos]</td><td>Move o motor E (A).</td></tr>");
  client.println("<tr><td>S[angulo]</td><td>Move Servo 1 (0-180). Ex: S90</td></tr>");
  client.println("<tr><td>A[angulo]</td><td>Move Servo 2 (0-180). Ex: A180</td></tr>");
  client.println("</table></div>");
  // Card: Comando Manual
  client.println("<div class='card'><h2>Comando Manual</h2>");
  client.println("<form action='/' method='GET'><input type='text' name='comando' placeholder='Ex: X3200' required>");
  client.println("<button type='submit'>Enviar Comando</button></form></div>");
  // Card: Controle dos Servos
  client.println("<div class='card'><h2>Controle dos Servos</h2>");
  client.println("<div class='servo-controls'>");
  client.println("<a class='btn' href='/servo1_on'>Ativar Servo 1</a>");
  client.println("<a class='btn btn-secondary' href='/servo1_off'>Repouso Servo 1</a>");
  client.println("<a class='btn' href='/servo2_on'>Ativar Servo 2</a>");
  client.println("<a class='btn btn-secondary' href='/servo2_off'>Repouso Servo 2</a>");
  client.println("</div></div></div>");
  // Card: Rotinas de Teste
  client.println("<div class='card footer-card' style='margin-top: 20px;'><h2>Rotinas de Teste</h2>");
  client.println("<a class='btn' href='/fazer_cruz' style='background-color:#d9534f;'>Executar Rotina da Cruz</a></div>");
  client.println("</div></body></html>");
}