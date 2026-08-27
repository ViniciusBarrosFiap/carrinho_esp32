/*
  ============================================================
  CARRINHO BLUETOOTH COM DABBLE - VERSAO ESP32
  COM SENSOR ULTRASSONICO ANTICOLISAO (HC-SR04)
  ============================================================
  Ponte H:    L298N
  Placa:      ESP32 (DevKit V1 ou similar)
  Bluetooth:  Bluetooth Classic INTERNO do ESP32
              (NAO precisa de modulo HC-05/HC-06!)
  App:        Dabble (iOS / Android)
  Sensor:     HC-SR04 (ultrassonico)

  IMPORTANTE - DIFERENCAS EM RELACAO AO ARDUINO:
    1) O ESP32 ja tem radio Bluetooth embutido. A biblioteca
       Dabble usa esse radio diretamente (Bluetooth Classic),
       entao o modulo HC-05/HC-06 e os pinos 12/11 de RX/TX
       NAO sao mais necessarios. Remova o modulo do circuito.
    2) Dabble.begin() no ESP32 recebe apenas um NOME (que vai
       aparecer na lista de Bluetooth do celular), diferente
       do Arduino que recebia baud rate + pinos.
    3) Os pinos 6 a 11 do ESP32 sao usados internamente para
       a memoria flash (SPI) e NAO devem ser usados como GPIO.
       Por isso os pinos dos motores foram trocados para
       GPIOs livres e seguros.
    4) Evite pinos "input-only" (34, 35, 36, 39) para saidas
       digitais como IN1-IN4, pois eles nao tem resistor de
       pull e nao podem ser usados como OUTPUT.

  ATENCAO - SENSOR ULTRASSONICO (HC-SR04):
    O pino ECHO do HC-SR04 trabalha em 5V, mas o ESP32 so
    tolera 3.3V nos seus GPIOs. Ligar o ECHO direto pode
    DANIFICAR o ESP32. Use um divisor de tensao:

      ECHO (5V) --[ resistor 1k ]--+--> GPIO do ESP32
                                   |
                              [ resistor 2k ]
                                   |
                                  GND

    Isso reduz o sinal de 5V para aproximadamente 3.3V.
    O pino TRIG pode ser ligado direto (ele recebe 3.3V do
    ESP32, que o HC-SR04 aceita normalmente).

  MAPEAMENTO DE PINOS (ESP32):
    IN1  = GPIO 25 (Motor A)
    IN2  = GPIO 26 (Motor A)
    IN3  = GPIO 27 (Motor B)
    IN4  = GPIO 14 (Motor B)
    TRIG = GPIO 33 (Sensor ultrassonico - saida)
    ECHO = GPIO 32 (Sensor ultrassonico - entrada, com divisor de tensao)

    L298N -> ESP32
    ENA / ENB: pode ligar direto no 3.3V/5V ou em jumper
               (sem controle de velocidade neste exemplo)
    GND (L298N) -> GND (ESP32)
    VCC logica do L298N -> 3.3V ou 5V do ESP32, conforme o
               modulo (confira o datasheet do seu L298N)

  CONTROLE:
    No app Dabble, use o modulo "Gamepad"
    Botao cima     = Frente (bloqueado se houver obstaculo perto)
    Botao baixo    = Re
    Botao esquerda = Esquerda
    Botao direita  = Direita
    Nenhum botao   = Parar

  COMPORTAMENTO ANTICOLISAO:
    O sensor mede a distancia continuamente. Se houver um
    obstaculo mais proximo que DISTANCIA_MINIMA_CM, o comando
    de ir para FRENTE e ignorado e o carrinho para, mesmo que
    o botao de cima esteja pressionado no app. Re, esquerda e
    direita continuam funcionando normalmente, para permitir
    que o usuario afaste o carrinho do obstaculo.
  ============================================================
*/

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

// IMPORTANTE: use a biblioteca "DabbleESP32" (da STEMpedia), NAO a
// biblioteca "Dabble" comum (essa e so para Arduino Uno/Nano/Mega
// e usa SoftwareSerial, que nao existe no ESP32).
#include <DabbleESP32.h>

// --- Pinos da Ponte H (adaptados para ESP32) ---
#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14

// --- Pinos do sensor ultrassonico HC-SR04 ---
#define TRIG_PIN 33
#define ECHO_PIN 32

// --- Distancia minima (em cm) para considerar risco de colisao ---
#define DISTANCIA_MINIMA_CM 15

// --- Tempo maximo de espera pelo echo, em microssegundos ---
// (equivale a cerca de 5 metros de alcance; evita travar o
// codigo caso o sensor nao receba retorno do eco)
#define ECHO_TIMEOUT_US 30000

// Nome que vai aparecer na lista de Bluetooth do celular
#define NOME_BLUETOOTH "CarrinhoESP32"

// --- Controle de estado para os prints ---
bool bluetoothConectado   = false;
String ultimoComando      = "";
bool obstaculoDetectado   = false;

// ============================================================
// FUNCAO: mede a distancia com o HC-SR04 e retorna em cm
// Retorna -1 se nao houver leitura valida (fora de alcance)
// ============================================================
long medirDistanciaCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (duracao == 0) {
    return -1; // sem leitura (nada em alcance, ou erro)
  }

  // Velocidade do som ~0.0343 cm/us, dividido por 2 (ida e volta)
  long distanciaCm = duracao * 0.0343 / 2;
  return distanciaCm;
}

// ============================================================
// FUNCOES DE MOVIMENTO
// ============================================================

void parar() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
}

void frente() {
  parar();
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(10);
}

void tras() {
  parar();
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(10);
}

void direita() {
  parar();
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(10);
}

void esquerda() {
  parar();
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(10);
}

// ============================================================
// FUNCAO: imprime comando apenas quando muda
// ============================================================
void imprimirComando(String novoComando) {
  if (novoComando != ultimoComando) {
    Serial.print("[COMANDO] ");
    Serial.println(novoComando);
    ultimoComando = novoComando;
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  parar();

  Serial.begin(115200);

  Serial.println("============================================");
  Serial.println("  Carrinho Bluetooth - Dabble (ESP32)");
  Serial.println("  Com sensor ultrassonico anticolisao");
  Serial.println("============================================");
  Serial.println("[SISTEMA] Iniciando Bluetooth interno do ESP32...");

  // No ESP32, o Dabble usa o Bluetooth Classic embutido.
  // O parametro e apenas o nome que aparecera no celular.
  Dabble.begin(NOME_BLUETOOTH);

  Serial.println("[SISTEMA] Bluetooth inicializado.");
  Serial.print("[SISTEMA] Procure por \"");
  Serial.print(NOME_BLUETOOTH);
  Serial.println("\" na lista de pareamento do app Dabble.");
  Serial.println("--------------------------------------------");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  Dabble.processInput();

  // --- Mede a distancia do obstaculo mais proximo ---
  long distancia = medirDistanciaCm();

  if (distancia > 0 && distancia <= DISTANCIA_MINIMA_CM) {
    if (!obstaculoDetectado) {
      obstaculoDetectado = true;
      Serial.print("[SENSOR] Obstaculo detectado a ");
      Serial.print(distancia);
      Serial.println(" cm! Bloqueando movimento para frente.");
    }
  } else {
    if (obstaculoDetectado) {
      obstaculoDetectado = false;
      Serial.println("[SENSOR] Caminho livre novamente.");
    }
  }

  // --- Detecta conexao/desconexao do Dabble ---
  if (Dabble.isAppConnected()) {
    if (!bluetoothConectado) {
      bluetoothConectado = true;
      Serial.println("============================================");
      Serial.println("[BLUETOOTH] App Dabble CONECTADO!");
      Serial.println("[BLUETOOTH] Aguardando comandos...");
      Serial.println("============================================");
    }

    // --- Le e executa os botoes do Gamepad ---
    if (GamePad.isUpPressed()) {
      if (obstaculoDetectado) {
        // Obstaculo perto: ignora o comando de frente e para
        imprimirComando("PARADO (obstaculo detectado)");
        parar();
      } else {
        imprimirComando("FRENTE");
        frente();
      }
    }
    else if (GamePad.isDownPressed()) {
      imprimirComando("RE (TRAS)");
      tras();
    }
    else if (GamePad.isRightPressed()) {
      imprimirComando("DIREITA");
      direita();
    }
    else if (GamePad.isLeftPressed()) {
      imprimirComando("ESQUERDA");
      esquerda();
    }
    else {
      imprimirComando("PARADO");
      parar();
    }

  } else {
    // App desconectado
    if (bluetoothConectado) {
      bluetoothConectado = false;
      ultimoComando = "";
      parar();
      Serial.println("--------------------------------------------");
      Serial.println("[BLUETOOTH] App Dabble DESCONECTADO!");
      Serial.println("[BLUETOOTH] Aguardando nova conexao...");
      Serial.println("--------------------------------------------");
    }
  }
}
