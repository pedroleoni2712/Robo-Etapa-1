#include <ESP8266WiFi.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>

// ================== CONFIG WiFi ==================
const char* ssid = "ROBO_CNPEM";
const char* password = "12345678";
WiFiServer server(80);

// ================== PINOS DO CNC SHIELD ==================
#define X_STEP D2
#define X_DIR  D5
#define Y_STEP D3
#define Y_DIR  D6
#define Z_STEP D4
#define Z_DIR  D7
#define A_STEP D12
#define A_DIR  D13
#define EN     D8

#define SERVO1_PIN D9
#define SERVO2_PIN D0

Servo servo1;
Servo servo2;

// ================== OBJETOS ==================
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP, X_DIR);
AccelStepper stepperA(AccelStepper::DRIVER, A_STEP, A_DIR);
AccelStepper stepperY(AccelStepper::DRIVER, Y_STEP, Y_DIR);
AccelStepper stepperZ(AccelStepper::DRIVER, Z_STEP, Z_DIR);

MultiStepper eixoVertical;   // X + A
MultiStepper eixoHorizontal; // Y + Z

String statusMessage = "Sistema pronto.";

// ================== VARIÁVEIS DE ESTADO ==================
long posVertical[2] = {0, 0};   // X e A
long posHorizontal[2] = {0, 0}; // Y e Z

// ================== DECLARAÇÃO DE FUNÇÕES ==================
void sendHTML(WiFiClient client);
void parseAndExecuteCommand(String command);
bool parseSingleLineCommand(String input, long &Vx, long &Dx, long &Vy, long &Dy, long &Va, long &Da, long &Vz, long &Dz);
void fazerCruz();
void moverVertical(long passos);
void moverHorizontal(long passos);

// ================== CONFIG GERAL ==================
void setup() {
  Serial.begin(115200);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH);

  stepperX.setAcceleration(800);
  stepperA.setAcceleration(800);
  stepperY.setAcceleration(800);
  stepperZ.setAcceleration(800);

  stepperX.setPinsInverted(false, false, false);
  stepperA.setPinsInverted(true,  false, false);
  stepperY.setPinsInverted(false, false, false);
  stepperZ.setPinsInverted(true,  false, false);

  // Agrupa motores para movimento sincronizado
  eixoVertical.addStepper(stepperX);
  eixoVertical.addStepper(stepperA);
  eixoHorizontal.addStepper(stepperY);
  eixoHorizontal.addStepper(stepperZ);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);
  servo2.write(0);

  WiFi.softAP(ssid, password);
  Serial.print("WiFi: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.begin();
}

// ================== FUNÇÕES DE MOVIMENTO GANTRY ==================
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
  statusMessage = "Executando Rotina da Cruz...";

  long passosMetade = 700;

  servo1.write(180); // Abaixa servo
  delay(500);

  // Movimentos horizontais
  moverHorizontal(passosMetade / 2);
  delay(1000);
  moverHorizontal(-passosMetade);
  delay(1000);
  moverHorizontal(passosMetade / 2);
  delay(1000);

  // Movimentos verticais
  moverVertical(passosMetade / 2);
  delay(1000);
  moverVertical(-passosMetade);
  delay(1000);
  moverVertical(passosMetade / 2);
  delay(1000);

  servo1.write(0); // Levanta servo
  delay(500);

  statusMessage = "Rotina da Cruz Concluída.";
  Serial.println("Cruz concluída!");
}

// ================== MOVIMENTO SIMPLES ==================
void moveSingleStepper(AccelStepper &stepper, long steps) {
  digitalWrite(EN, LOW);
  stepper.setCurrentPosition(0);
  stepper.move(steps);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    yield();
  }
  digitalWrite(EN, HIGH);
}

// ================== LER LINHA ÚNICA ==================
bool parseSingleLineCommand(String input, long &Vx, long &Dx, long &Vy, long &Dy, long &Va, long &Da, long &Vz, long &Dz) {
  input.trim();
  input.replace(",", " ");
  input.replace(";", " ");
  
  while (input.indexOf("  ") >= 0) {
    input.replace("  ", " ");
  }
  
  long values[8];
  int count = 0;
  
  int lastPos = 0;
  for (int i = 0; i < input.length() && count < 8; i++) {
    if (input.charAt(i) == ' ' || i == input.length() - 1) {
      String token;
      if (i == input.length() - 1 && input.charAt(i) != ' ') {
        token = input.substring(lastPos);
      } else {
        token = input.substring(lastPos, i);
      }
      
      token.trim();
      if (token.length() > 0) {
        values[count++] = token.toInt();
      }
      lastPos = i + 1;
    }
  }
  
  if (count != 8) {
    Serial.printf("Erro: esperados 8 valores, recebidos %d\n", count);
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

// ================== PARSER COMANDO ANTIGO ==================
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
    case 'E':
      moveSingleStepper(stepperA, cmdValue);
      statusMessage = "Motor E(A) movido " + String(cmdValue) + " passos.";
      break;
    case 'S':
      if (cmdValue >= 0 && cmdValue <= 180) {
        servo1.write(cmdValue);
        statusMessage = "Servo 1 movido para " + String(cmdValue) + "°.";
      } else {
        statusMessage = "Erro: Ângulo do servo 1 inválido.";
      }
      break;
    case 'A':
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
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  bool actionTaken = false;

  // === ROTINA DA CRUZ ===
  if (request.indexOf("/fazer_cruz") != -1) {
    fazerCruz();
    actionTaken = true;
  }
  // === COMANDO EM LINHA ÚNICA (AGORA COM VELOCIDADE E DISTÂNCIA) ===
  else if (request.indexOf("GET /?params=") >= 0) {
    int paramPos = request.indexOf("params=");
    int endPos = request.indexOf(' ', paramPos);
    String paramsStr = request.substring(paramPos + 7, endPos);
    
    paramsStr.replace("%20", " ");
    paramsStr.replace("%2C", ",");
    paramsStr.replace("%3B", ";");
    paramsStr.replace("+", " ");
    
    long Vx, Dx, Vy, Dy, Va, Da, Vz, Dz;
    
    if (parseSingleLineCommand(paramsStr, Vx, Dx, Vy, Dy, Va, Da, Vz, Dz)) {
      Serial.println("Parâmetros recebidos:");
      Serial.printf("Vx=%ld Dx=%ld Vy=%ld Dy=%ld Va=%ld Da=%ld Vz=%ld Dz=%ld\n",
        Vx, Dx, Vy, Dy, Va, Da, Vz, Dz);

      digitalWrite(EN, LOW);

      long speedX = constrain(abs(Vx), 0, 1600);
      long speedY = constrain(abs(Vy), 0, 1600);
      long speedA = constrain(abs(Va), 0, 1600);
      long speedZ = constrain(abs(Vz), 0, 1600);
      
      stepperX.setMaxSpeed(speedX);
      stepperY.setMaxSpeed(speedY);
      stepperA.setMaxSpeed(speedA);
      stepperZ.setMaxSpeed(speedZ);
      
      stepperX.setAcceleration(speedX / 2);
      stepperY.setAcceleration(speedY / 2);
      stepperA.setAcceleration(speedA / 2);
      stepperZ.setAcceleration(speedZ / 2);

      // === 1ª Etapa: mover Y e Z juntos ===
      {
        MultiStepper yzSteppers;
        yzSteppers.addStepper(stepperY);
        yzSteppers.addStepper(stepperZ);

        long targetsYZ[2] = {Dy, Dz};

        stepperY.setCurrentPosition(0);
        stepperZ.setCurrentPosition(0);

        yzSteppers.moveTo(targetsYZ);
        yzSteppers.runSpeedToPosition();

        Serial.println("Movimento Y e Z concluído.");
        delay(200);
      }

      // === 2ª Etapa: mover X e A juntos ===
      {
        MultiStepper xaSteppers;
        xaSteppers.addStepper(stepperX);
        xaSteppers.addStepper(stepperA);

        long targetsXA[2] = {Dx, Da};

        stepperX.setCurrentPosition(0);
        stepperA.setCurrentPosition(0);

        xaSteppers.moveTo(targetsXA);
        xaSteppers.runSpeedToPosition();

        Serial.println("Movimento X e A concluído.");
      }

      digitalWrite(EN, HIGH);

      statusMessage = "Movimento executado! X(V=" + String(Vx) + " D=" + String(Dx) + ") " +
                     "Y(V=" + String(Vy) + " D=" + String(Dy) + ") " +
                     "A(V=" + String(Va) + " D=" + String(Da) + ") " +
                     "Z(V=" + String(Vz) + " D=" + String(Dz) + ")";
      actionTaken = true;
    } else {
      statusMessage = "ERRO: Formato inválido! Insira 8 valores separados por vírgula ou espaço.";
      actionTaken = true;
    }
  }
  // === Comando Manual ===
  else if (request.indexOf("GET /?comando=") >= 0) {
    int cmdPos = request.indexOf('=');
    int endPos = request.indexOf(' ', cmdPos);
    String command = request.substring(cmdPos + 1, endPos);
    command.trim();
    if (command.length() > 0) parseAndExecuteCommand(command);
    actionTaken = true;
  }
  // === Controle Servos ===
  else if (request.indexOf("/servo1_on") != -1) {
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
  }

  // === Envio da Resposta ===
  if (actionTaken) {
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /");
    client.println();
  } else {
    sendHTML(client);
  }

  delay(1);
  client.stop();
}

// ================== PÁGINA HTML ==================
void sendHTML(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML><html><head>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<title>Controle de Motores</title>");
  client.println("<style>");
  client.println("body{font-family:Arial;background:#f0f2f5;padding:20px;}h1{text-align:center;color:#333;}");  
  client.println(".card{background:white;padding:20px;margin:10px auto;border-radius:8px;max-width:800px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}");  
  client.println("input{width:100%;padding:10px;border:1px solid #ccc;border-radius:4px;font-size:16px;box-sizing:border-box;}");  
  client.println("button,.btn{background:#2196F3;color:white;padding:12px 24px;border:none;border-radius:5px;cursor:pointer;margin-top:15px;font-size:16px;width:100%;text-decoration:none;display:block;text-align:center;}");  
  client.println("button:hover,.btn:hover{background:#0b7dda;}");
  client.println(".btn-danger{background:#d9534f;} .btn-danger:hover{background:#c9302c;}");
  client.println(".btn-secondary{background:#6c757d;} .btn-secondary:hover{background:#5a6268;}");
  client.println(".info{background:#e3f2fd;padding:10px;border-radius:4px;margin:10px 0;font-size:14px;}");
  client.println(".status{background:#c8e6c9;padding:10px;border-radius:4px;margin:10px 0;font-weight:bold;}");
  client.println("table{width:100%;margin:10px 0;border-collapse:collapse;}");
  client.println("th,td{padding:8px;text-align:left;border-bottom:1px solid #ddd;}");
  client.println("th{background:#2196F3;color:white;}");
  client.println(".servo-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:10px;}");
  client.println("</style></head><body>");
  client.println("<h1>Controle de Motores - Velocidade e Distância</h1>");
  client.println("<div class='card'>");
  client.println("<div class='status'>Status: " + statusMessage + "</div>");
  client.println("<div class='info'>");
  client.println("<b>Formato:</b> Insira os 8 valores separados por vírgula ou espaço<br>");
  client.println("<b>Ordem:</b> Vx, Dx, Vy, Dy, Va, Da, Vz, Dz<br><br>");
  client.println("<table>");
  client.println("<tr><th>Parâmetro</th><th>Significado</th><th>Exemplo</th></tr>");
  client.println("<tr><td>Vx</td><td>Velocidade motor X (passos/seg)</td><td>800 (máx 1600)</td></tr>");
  client.println("<tr><td>Dx</td><td>Distância motor X (passos)</td><td>1000</td></tr>");
  client.println("<tr><td>Vy</td><td>Velocidade motor Y (passos/seg)</td><td>600 (máx 1600)</td></tr>");
  client.println("<tr><td>Dy</td><td>Distância motor Y (passos)</td><td>-500</td></tr>");
  client.println("<tr><td>Va</td><td>Velocidade motor A (passos/seg)</td><td>400 (máx 1600)</td></tr>");
  client.println("<tr><td>Da</td><td>Distância motor A (passos)</td><td>200</td></tr>");
  client.println("<tr><td>Vz</td><td>Velocidade motor Z (passos/seg)</td><td>500 (máx 1600)</td></tr>");
  client.println("<tr><td>Dz</td><td>Distância motor Z (passos)</td><td>-300</td></tr>");
  client.println("</table>");
  client.println("<b>Exemplo completo:</b> <code>800, 1000, 600, -500, 400, 200, 500, -300</code><br>");
  client.println("<small>* Distâncias negativas movem na direção oposta<br>");
  client.println("* Velocidade máxima: 1600 passos/segundo<br>");
  client.println("* Aceleração automática: metade da velocidade de cada motor</small>");
  client.println("</div>");
  client.println("<form action='/' method='GET'>");
  client.println("<label style='display:block;margin-bottom:5px;font-weight:bold;'>Digite os 8 parâmetros:</label>");
  client.println("<input type='text' name='params' placeholder='Vx, Dx, Vy, Dy, Va, Da, Vz, Dz' required style='font-family:monospace;'>");
  client.println("<button type='submit'>Executar Movimento</button>");
  client.println("</form></div>");
  
  // Card de Comandos Manuais
  client.println("<div class='card'>");
  client.println("<h2 style='color:#2196F3;margin-top:0;'>Comando Manual</h2>");
  client.println("<p><b>Comandos disponíveis:</b> X[passos], Y[passos], Z[passos], E[passos], S[0-180], A[0-180]</p>");
  client.println("<form action='/' method='GET'>");
  client.println("<input type='text' name='comando' placeholder='Ex: X3200, S90, A180' required>");
  client.println("<button type='submit'>Enviar Comando</button>");
  client.println("</form></div>");
  
  // Card de Controle dos Servos
  client.println("<div class='card'>");
  client.println("<h2 style='color:#2196F3;margin-top:0;'>Controle dos Servos</h2>");
  client.println("<div class='servo-grid'>");
  client.println("<a class='btn' href='/servo1_on'>Ativar Servo 1</a>");
  client.println("<a class='btn btn-secondary' href='/servo1_off'>Repouso Servo 1</a>");
  client.println("<a class='btn' href='/servo2_on'>Ativar Servo 2</a>");
  client.println("<a class='btn btn-secondary' href='/servo2_off'>Repouso Servo 2</a>");
  client.println("</div></div>");
  
  // Card de Rotinas
  client.println("<div class='card'>");
  client.println("<h2 style='color:#d9534f;margin-top:0;'>Rotinas de Teste</h2>");
  client.println("<a class='btn btn-danger' href='/fazer_cruz'>Executar Rotina da Cruz</a>");
  client.println("</div>");
  
  client.println("</body></html>");
}