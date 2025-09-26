// =================================================================
//  CONTROLE VIA WEB SERVER (ACCESS POINT)
//  VERSÃO V15-WEB - 4 MOTORES + 2 SERVOS
//  => ROTINA DA CRUZ ATUALIZADA (ESTILO SEPARADO)
// =================================================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AccelStepper.h>
#include <Servo.h>

// --- CREDENCIAIS DA REDE WI-FI QUE SERÁ CRIADA ---
const char *ssid = "Controle Motores CNC"; // Nome da rede Wi-Fi que o Wemos irá criar
const char *pass = "12345678";             // Senha da rede (mínimo 8 caracteres)

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
#define SERVO2_PIN D10

// --- OBJETOS ---
ESP8266WebServer server(80);
AccelStepper motorX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper motorY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
AccelStepper motorZ(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);
AccelStepper motorE(AccelStepper::DRIVER, E_STEP_PIN, E_DIR_PIN);
Servo myServo;
Servo myServo2;

// ===================== PARÂMETROS =====================
const int VEL_FIXA = 400;
const long PASSOS_POR_VOLTA = 3200;
const long PASSOS_TESTE = 800;

// Posições dos servos
const int SERVO_POS_REPOUSO = 0;   // Posição para levantar a ferramenta
const int SERVO_POS_ACIONADO = 180; // Posição para abaixar a ferramenta

// Variável para armazenar o status
String statusMessage = "Sistema pronto.";
// ======================================================

// Função para enviar a página HTML principal
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle de Motores</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; }
    .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    h1, h2 { color: #333; text-align: center; }
    .status { background-color: #e9ecef; border-left: 5px solid #007bff; padding: 15px; margin-bottom: 20px; border-radius: 4px; }
    .grid-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; }
    .card { background-color: #ffffff; padding: 20px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.05); }
    .card h3 { margin-top: 0; color: #007bff; }
    table { width: 100%; border-collapse: collapse; }
    th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }
    th { background-color: #f8f9fa; }
    input[type="text"] { width: calc(100% - 22px); padding: 10px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 4px; }
    .btn, input[type="submit"] {
      background-color: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 4px;
      cursor: pointer; text-decoration: none; display: inline-block; text-align: center; width: calc(100% - 30px); margin-bottom: 5px;
    }
    .btn-secondary { background-color: #6c757d; }
    .btn:hover, input[type="submit"]:hover { background-color: #0056b3; }
    .btn-secondary:hover { background-color: #5a6268; }
    .btn-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Controle de Motores via Wi-Fi</h1>
    <div class="status">
      <strong>Status:</strong> )rawliteral";
  html += statusMessage;
  html += R"rawliteral(
    </div>
    <div class="grid-container">
      <div class="card">
        <h3>Menu de Comandos</h3>
        <table>
          <tr><th>Comando</th><th>Descrição</th></tr>
          <tr><td><b>X</b>[passos]</td><td>Move o motor X. Ex: X1600</td></tr>
          <tr><td><b>Y</b>[passos]</td><td>Move o motor Y. Ex: Y-800</td></tr>
          <tr><td><b>Z</b>[passos]</td><td>Move o motor Z.</td></tr>
          <tr><td><b>E</b>[passos]</td><td>Move o motor E.</td></tr>
          <tr><td><b>S</b>[angulo]</td><td>Move Servo 1 (0-180). Ex: S90</td></tr>
          <tr><td><b>A</b>[angulo]</td><td>Move Servo 2 (0-180). Ex: A180</td></tr>
        </table>
      </div>

      <div class="card">
        <h3>Comando Manual</h3>
        <form action="/run" method="get">
          <input type="text" name="cmd" placeholder="Ex: X3200">
          <input type="submit" value="Enviar Comando">
        </form>
      </div>

      <div class="card">
        <h3>Controle dos Servos</h3>
        <div class="btn-grid">
          <a href="/run?cmd=S)rawliteral"; html += SERVO_POS_ACIONADO; html += R"rawliteral(" class="btn">Ativar Servo 1</a>
          <a href="/run?cmd=S)rawliteral"; html += SERVO_POS_REPOUSO; html += R"rawliteral(" class="btn btn-secondary">Repouso Servo 1</a>
          <a href="/run?cmd=A)rawliteral"; html += SERVO_POS_ACIONADO; html += R"rawliteral(" class="btn">Ativar Servo 2</a>
          <a href="/run?cmd=A)rawliteral"; html += SERVO_POS_REPOUSO; html += R"rawliteral(" class="btn btn-secondary">Repouso Servo 2</a>
        </div>
      </div>
      
      <div class="card">
        <h3>Rotinas de Teste</h3>
        <div class="btn-grid">
          <a href="/run?cmd=TEST_X" class="btn">Teste X</a>
          <a href="/run?cmd=TEST_Y" class="btn">Teste Y</a>
          <a href="/run?cmd=TEST_XY" class="btn">Teste XY</a>
          <a href="/run?cmd=TEST_XE" class="btn">Teste XE (Oposto)</a>
          <a href="/run?cmd=TEST_YZ" class="btn">Teste YZ (Oposto)</a>
          
          <a href="/run?cmd=ROUTINE_CROSS" class="btn" style="grid-column: 1 / -1; background-color: #28a745;">Rotina Cruz 1cm</a>

        </div>
      </div>
    </div>
  </div>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// Função para processar os comandos recebidos
void handleCommand() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    cmd.trim();
    if (cmd.length() == 0) {
      statusMessage = "Comando vazio recebido.";
      handleRoot();
      return;
    }

    // --- ROTINA DA CRUZ (ESTILO SEPARADO) ---
    if (cmd == "ROUTINE_CROSS") {
        const long passos_metade_cruz = 114; // Passos para 0.5cm, igual ao exemplo

        statusMessage = "Iniciando Rotina da Cruz...";
        server.send(200, "text/html", "Processando... <meta http-equiv='refresh' content='1;url=/' />");
        
        // 1. Abaixa o servo
        myServo.write(SERVO_POS_ACIONADO); 
        delay(500);

        // 2. Habilita os motores
        digitalWrite(ENABLE_PIN, LOW);
        
        // --- Movimentos Eixo X (Horizontal) ---
        motorX.move(passos_metade_cruz);  // Direita
        motorX.runToPosition();
        delay(200);
        motorX.move(-passos_metade_cruz); // Volta ao centro
        motorX.runToPosition();
        delay(200);
        
        motorX.move(-passos_metade_cruz); // Esquerda
        motorX.runToPosition();
        delay(200);
        motorX.move(passos_metade_cruz);  // Volta ao centro
        motorX.runToPosition();
        delay(200);

        // --- Movimentos Eixo Y (Vertical) ---
        motorY.move(passos_metade_cruz);  // Para frente ("cima")
        motorY.runToPosition();
        delay(200);
        motorY.move(-passos_metade_cruz); // Volta ao centro
        motorY.runToPosition();
        delay(200);

        motorY.move(-passos_metade_cruz); // Para trás ("baixo")
        motorY.runToPosition();
        delay(200);
        motorY.move(passos_metade_cruz);  // Volta ao centro
        motorY.runToPosition();
        delay(200);

        // 3. Desabilita os motores
        digitalWrite(ENABLE_PIN, HIGH); 
        
        // 4. Levanta o servo
        myServo.write(SERVO_POS_REPOUSO);
        delay(500);
        
        statusMessage = "Rotina da Cruz Concluída.";
        return;
    }

    // --- LÓGICA PARA TESTES PRÉ-DEFINIDOS ---
    if (cmd == "TEST_X") {
      statusMessage = "Iniciando Teste X...";
      server.send(200, "text/html", "Processando... <meta http-equiv='refresh' content='1;url=/' />");
      digitalWrite(ENABLE_PIN, LOW);
      motorX.move(PASSOS_TESTE);
      motorX.runToPosition();
      delay(300);
      motorX.move(-PASSOS_TESTE);
      motorX.runToPosition();
      digitalWrite(ENABLE_PIN, HIGH);
      statusMessage = "Teste X Concluído.";
      return;
    }
    if (cmd == "TEST_Y") {
      statusMessage = "Iniciando Teste Y...";
      server.send(200, "text/html", "Processando... <meta http-equiv='refresh' content='1;url=/' />");
      digitalWrite(ENABLE_PIN, LOW);
      motorY.move(PASSOS_TESTE);
      motorY.runToPosition();
      delay(300);
      motorY.move(-PASSOS_TESTE);
      motorY.runToPosition();
      digitalWrite(ENABLE_PIN, HIGH);
      statusMessage = "Teste Y Concluído.";
      return;
    }
    if (cmd == "TEST_XY") {
        statusMessage = "Iniciando Teste XY...";
        server.send(200, "text/html", "Processando... <meta http-equiv='refresh' content='1;url=/' />");
        digitalWrite(ENABLE_PIN, LOW);
        motorX.move(PASSOS_TESTE);
        motorY.move(PASSOS_TESTE);
        while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
          motorX.run();
          motorY.run();
        }
        delay(300);
        motorX.move(-PASSOS_TESTE);
        motorY.move(-PASSOS_TESTE);
        while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
          motorX.run();
          motorY.run();
        }
        statusMessage = "Teste XY Concluído.";
        return;
    }
    if (cmd == "TEST_XE") {
        statusMessage = "Iniciando Teste XE (Oposto)...";
        server.send(200, "text/html", "Processando... <meta http-equiv='refresh' content='1;url=/' />");
        digitalWrite(ENABLE_PIN, LOW);
        motorX.move(PASSOS_TESTE);
        motorE.move(-PASSOS_TESTE);
        while (motorX.distanceToGo() != 0 || motorE.distanceToGo() != 0) {
          motorX.run();
          motorE.run();
        }
        delay(300);
        motorX.move(-PASSOS_TESTE);
        motorE.move(PASSOS_TESTE);
        while (motorX.distanceToGo() != 0 || motorE.distanceToGo() != 0) {
          motorX.run();
          motorE.run();
        }
        statusMessage = "Teste XE Concluído.";
        return;
    }
    if (cmd == "TEST_YZ") {
        statusMessage = "Iniciando Teste YZ (Oposto)...";
        server.send(200, "text/html", "Processando... <meta http-equiv='refresh' content='1;url=/' />");
        digitalWrite(ENABLE_PIN, LOW);
        motorY.move(PASSOS_TESTE);
        motorZ.move(-PASSOS_TESTE);
        while (motorY.distanceToGo() != 0 || motorZ.distanceToGo() != 0) {
          motorY.run();
          motorZ.run();
        }
        delay(300);
        motorY.move(-PASSOS_TESTE);
        motorZ.move(PASSOS_TESTE);
        while (motorY.distanceToGo() != 0 || motorZ.distanceToGo() != 0) {
          motorY.run();
          motorZ.run();
        }
        statusMessage = "Teste YZ Concluído.";
        return;
    }

    // --- LÓGICA PARA COMANDOS DE TEXTO ---
    char commandChar = toupper(cmd.charAt(0));
    long value = atol(cmd.substring(1).c_str());
    statusMessage = "OK (Comando " + cmd + " recebido)";

    switch (commandChar) {
      case 'X':
        digitalWrite(ENABLE_PIN, LOW);
        motorX.move(value);
        break;
      case 'Y':
        digitalWrite(ENABLE_PIN, LOW);
        motorY.move(value);
        break;
      case 'Z':
        digitalWrite(ENABLE_PIN, LOW);
        motorZ.move(value);
        break;
      case 'E':
        digitalWrite(ENABLE_PIN, LOW);
        motorE.move(value);
        break;
      case 'S': // Comando para o Servo 1
        if (value >= 0 && value <= 180) {
          myServo.write(value);
          statusMessage = "Servo 1 -> " + String(value) + "°";
        } else {
          statusMessage = "Servo 1: valor inválido.";
        }
        break;
      case 'A': // Comando 'A' (Auxiliar) para o Servo 2
        if (value >= 0 && value <= 180) {
          myServo2.write(value);
          statusMessage = "Servo 2 -> " + String(value) + "°";
        } else {
          statusMessage = "Servo 2: valor inválido.";
        }
        break;
      default:
        statusMessage = "Comando inválido!";
    }
  } else {
    statusMessage = "Nenhum comando fornecido.";
  }

  // Redireciona de volta para a página inicial após processar
  server.sendHeader("Location", "/");
  server.send(303);
}

void aplicarPerfilFixo(AccelStepper &m) {
  m.setMaxSpeed(VEL_FIXA);
  m.setAcceleration(VEL_FIXA / 2);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // Motores desabilitados por padrão

  aplicarPerfilFixo(motorX);
  aplicarPerfilFixo(motorY);
  aplicarPerfilFixo(motorZ);
  aplicarPerfilFixo(motorE);

  myServo.attach(SERVO_PIN);
  myServo2.attach(SERVO2_PIN);
  myServo.write(SERVO_POS_REPOUSO);
  myServo2.write(SERVO_POS_REPOUSO);

  Serial.println("\nConfigurando Access Point...");
  WiFi.softAP(ssid, pass);
  IPAddress myIP = WiFi.softAPIP();
  
  // --- MENSAGEM MELHORADA NO MONITOR SERIAL ---
  Serial.println("==============================================");
  Serial.println(">>> Ponto de Acesso Wi-Fi Criado! <<<");
  Serial.print("Nome da Rede (SSID): ");
  Serial.println(ssid);
  Serial.print("Senha: ");
  Serial.println(pass);
  Serial.println("\nConecte-se a essa rede e acesse o IP abaixo");
  Serial.println("em seu navegador para controlar os motores:");
  Serial.print("http://");
  Serial.println(myIP);
  Serial.println("==============================================");


  // Define as rotas do servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/run", HTTP_GET, handleCommand);

  server.begin(); // Inicia o servidor
}

void loop() {
  server.handleClient(); // Processa requisições web

  // Roda os motores (essencial para o movimento não-bloqueante)
  motorX.run();
  motorY.run();
  motorZ.run();
  motorE.run();

  // Desabilita os drivers se todos os motores pararam de se mover
  if (motorX.distanceToGo() == 0 && motorY.distanceToGo() == 0 && motorZ.distanceToGo() == 0 && motorE.distanceToGo() == 0) {
    digitalWrite(ENABLE_PIN, HIGH);
  }
}