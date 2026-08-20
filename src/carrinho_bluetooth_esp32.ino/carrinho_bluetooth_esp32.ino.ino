#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <DabbleESP32.h>

// --- Pinos da Ponte H ---
#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14

// Nome que vai aparecer na lista de Bluetooth do celular ---
#define NOME_BLUETOOTH "CarrinhoESP32"

// Controle de estado para os prints ---
bool bluetoothConectado  = false;
String ultimoComando     = "";

// FUNCOES DE MOVIMENTO
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


//imprime comando apenas quando muda
void imprimirComando(String novoComando) {
  if (novoComando != ultimoComando) {
    Serial.print("COMANDO");
    Serial.println(novoComando);
    ultimoComando = novoComando;
  }
}


// SETUP
void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  parar();

  Serial.begin(115200);
  Serial.println("  Carrinho Bluetooth - Dabble (ESP32)");
  Serial.println("Iniciando Bluetooth interno do ESP32...");

  // O parametro e apenas o nome que aparecera no celular.
  Dabble.begin(NOME_BLUETOOTH);

  Serial.println("Bluetooth inicializado.");
  Serial.print("Procure por \"");
  Serial.print(NOME_BLUETOOTH);
  Serial.println("\" na lista de pareamento do app Dabble.");
}


// LOOP PRINCIPAL
void loop() {
  Dabble.processInput();


  if (Dabble.isAppConnected()) {
    if (!bluetoothConectado) {
      bluetoothConectado = true;

      Serial.println("App Dabble CONECTADO!");
      Serial.println("Aguardando comandos...");

    }

    // --- Le e executa os botoes do Gamepad ---
    if (GamePad.isUpPressed()) {
      imprimirComando("FRENTE");
      frente();
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

      Serial.println("App Dabble DESCONECTADO!");
      Serial.println("Aguardando nova conexao...");
    }
  }
}
