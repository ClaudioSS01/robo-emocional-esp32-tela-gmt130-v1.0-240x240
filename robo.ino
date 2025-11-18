#include <Arduino_GFX_Library.h>
#include <BluetoothSerial.h>

// --- PINAGEM ---
#define TFT_SCK   27  
#define TFT_MOSI  26  
#define TFT_DC    25  
#define TFT_RST   12  
#define TFT_BL    33  

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, -1, TFT_SCK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 0, true, 240, 240, 0, 0);

BluetoothSerial SerialBT;
String deviceName = "MONSTRIX-2.0";
bool bluetoothConnected = false;

// --- CONFIGURAÇÕES ---
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
const int EYE_RADIUS = 45;
const int PUPIL_RADIUS = 18;
const int EYE_SPACING = 35;
const int BLINK_DURATION = 150;
const int EMOTION_INTERVAL = 60000;  
const int IRIS_MOVE_INTERVAL = 5000; // Move a cada 5 segundos

const int EYE_LEFT_X = SCREEN_WIDTH / 2 - EYE_RADIUS - EYE_SPACING / 2;
const int EYE_RIGHT_X = SCREEN_WIDTH / 2 + EYE_RADIUS + EYE_SPACING / 2;
const int EYE_Y = SCREEN_HEIGHT / 2 + 5;

// CORES
#define COLOR_BG        0x0841
#define COLOR_WHITE     0xFFFF
#define COLOR_BLACK     0x0000
#define COLOR_CYAN      0x07FF
#define COLOR_BLUE      0x001F
#define COLOR_GREEN     0x07E0
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20
#define COLOR_RED       0xF800
#define COLOR_PINK      0xF81F
#define COLOR_PURPLE    0x780F
#define COLOR_LIME      0xBFE0
#define COLOR_GRAY      0x7BEF

enum Mood {
    NEUTRAL, HAPPY, EXCITED, LOVE, TIRED, SAD, ANGRY, CONFUSED
};

// Variáveis globais
unsigned long lastEmotionChange = 0;
unsigned long lastBlink = 0;
unsigned long lastIrisMove = 0;
unsigned long lastStatusScroll = 0;
Mood currentMood = NEUTRAL;
int pupilOffsetX = 0;
int pupilOffsetY = 0;
int targetPupilX = 0;
int targetPupilY = 0;
uint16_t currentIrisColor = COLOR_CYAN;
int statusScrollPos = SCREEN_WIDTH;
String statusMessage = "";

// --- BLUETOOTH ---
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    if (event == ESP_SPP_SRV_OPEN_EVT) {
        bluetoothConnected = true;
        Serial.println("✅ BT conectado!");
        SerialBT.println("🤖 MONSTRIX 2.0!");
        SerialBT.println("Comandos: feliz, triste, bravo, amor, cansado, neutro, confuso, empolgado");
    } else if (event == ESP_SPP_CLOSE_EVT) {
        bluetoothConnected = false;
        Serial.println("❌ BT desconectado!");
    }
}

void updateStatusMessage() {
    if (bluetoothConnected) {
        statusMessage = "    CONECTADO | Envie comandos via Bluetooth    ";
    } else {
        statusMessage = "    BLUETOOTH: " + deviceName + " | Aguardando conexao...    ";
    }
}

void processBluetoothCommand() {
    if (SerialBT.available()) {
        String command = SerialBT.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "feliz" || command == "happy") {
            drawEmotion(HAPPY, "FELIZ", COLOR_YELLOW);
            SerialBT.println("😊 FELIZ");
        } else if (command == "triste" || command == "sad") {
            drawEmotion(SAD, "TRISTE", COLOR_BLUE);
            SerialBT.println("😢 TRISTE");
        } else if (command == "bravo" || command == "angry") {
            drawEmotion(ANGRY, "BRAVO", COLOR_RED);
            SerialBT.println("😠 BRAVO");
        } else if (command == "amor" || command == "love") {
            drawEmotion(LOVE, "APAIXONADO", COLOR_PINK);
            SerialBT.println("❤️ AMOR");
        } else if (command == "cansado" || command == "tired") {
            drawEmotion(TIRED, "CANSADO", COLOR_PURPLE);
            SerialBT.println("😴 CANSADO");
        } else if (command == "neutro" || command == "neutral") {
            drawEmotion(NEUTRAL, "NEUTRAL", COLOR_CYAN);
            SerialBT.println("😐 NEUTRO");
        } else if (command == "confuso" || command == "confused") {
            drawEmotion(CONFUSED, "CONFUSO", COLOR_LIME);
            SerialBT.println("🤔 CONFUSO");
        } else if (command == "empolgado" || command == "excited") {
            drawEmotion(EXCITED, "EMPOLGADO", COLOR_ORANGE);
            SerialBT.println("🤩 EMPOLGADO");
        } else if (command == "help" || command == "ajuda") {
            SerialBT.println("📋 Comandos: feliz, triste, bravo, amor, cansado, neutro, confuso, empolgado");
        } else {
            SerialBT.println("❓ Digite 'help'");
        }
        
        lastEmotionChange = millis();
    }
}

// --- DESENHO ---
void drawGradientBackground() {
    for (int y = 40; y < SCREEN_HEIGHT; y++) {
        uint16_t color = gfx->color565(0, 0, 20 + ((y - 40) * 40 / (SCREEN_HEIGHT - 40)));
        gfx->drawFastHLine(0, y, SCREEN_WIDTH, color);
    }
}

void drawStatusBar() {
    gfx->fillRect(0, 0, SCREEN_WIDTH, 38, COLOR_BLACK);
    gfx->setTextColor(bluetoothConnected ? COLOR_GREEN : COLOR_YELLOW);
    gfx->setTextSize(1);
    gfx->setCursor(statusScrollPos, 5);
    gfx->print(statusMessage);
    
    gfx->setCursor(5, 5);
    gfx->setTextColor(bluetoothConnected ? COLOR_GREEN : COLOR_GRAY);
    gfx->print("BT");
    
    if (bluetoothConnected) {
        gfx->fillCircle(230, 10, 4, COLOR_GREEN);
    } else {
        gfx->drawCircle(230, 10, 4, COLOR_GRAY);
    }
    
    gfx->drawFastHLine(0, 38, SCREEN_WIDTH, COLOR_CYAN);
    
    gfx->setTextSize(2);
    gfx->setTextColor(currentIrisColor);
    String moodText = "";
    switch (currentMood) {
        case NEUTRAL: moodText = "NEUTRAL"; break;
        case HAPPY: moodText = "FELIZ"; break;
        case EXCITED: moodText = "EMPOLGADO"; break;
        case LOVE: moodText = "APAIXONADO"; break;
        case TIRED: moodText = "CANSADO"; break;
        case SAD: moodText = "TRISTE"; break;
        case ANGRY: moodText = "BRAVO"; break;
        case CONFUSED: moodText = "CONFUSO"; break;
    }
    int textWidth = moodText.length() * 12;
    gfx->setCursor((SCREEN_WIDTH - textWidth) / 2, 20);
    gfx->print(moodText);
}

void scrollStatusBar() {
    statusScrollPos -= 2;
    int messageWidth = statusMessage.length() * 6;
    if (statusScrollPos < -messageWidth) {
        statusScrollPos = SCREEN_WIDTH;
    }
    drawStatusBar();
}

void moveIrisRandomly() {
    int maxOffset = EYE_RADIUS - PUPIL_RADIUS - 10;
    targetPupilX = random(-maxOffset, maxOffset + 1);
    targetPupilY = random(-maxOffset, maxOffset + 1);
    Serial.print("Nova posição: X=");
    Serial.print(targetPupilX);
    Serial.print(" Y=");
    Serial.println(targetPupilY);
}

// ✅ MOVIMENTO INSTANTÂNEO (SEM ANIMAÇÃO SUAVE)
void updateIrisPosition() {
    pupilOffsetX = targetPupilX;
    pupilOffsetY = targetPupilY;
}

void drawEye(int x, int y, uint16_t irisColor, bool isOpen = true) {
    if (!isOpen) {
        gfx->fillCircle(x, y, EYE_RADIUS, COLOR_BG);
        gfx->drawFastHLine(x - EYE_RADIUS + 5, y, (EYE_RADIUS - 5) * 2, COLOR_WHITE);
        return;
    }
    
    // ✅ DESENHO DIRETO SEM startWrite/endWrite
    gfx->fillCircle(x, y, EYE_RADIUS, COLOR_WHITE);
    gfx->fillCircle(x + pupilOffsetX, y + pupilOffsetY, PUPIL_RADIUS + 8, irisColor);
    gfx->fillCircle(x + pupilOffsetX, y + pupilOffsetY, PUPIL_RADIUS, COLOR_BLACK);
    gfx->fillCircle(x + pupilOffsetX - 6, y + pupilOffsetY - 6, 5, COLOR_WHITE);
    gfx->drawCircle(x, y, EYE_RADIUS, COLOR_BLACK);
}

void drawTiredEye(int x, int y, uint16_t irisColor) {
    gfx->fillCircle(x, y, EYE_RADIUS, COLOR_WHITE);
    gfx->fillCircle(x + pupilOffsetX, y + pupilOffsetY, PUPIL_RADIUS + 8, irisColor);
    gfx->fillCircle(x + pupilOffsetX, y + pupilOffsetY, PUPIL_RADIUS, COLOR_BLACK);
    gfx->fillCircle(x + pupilOffsetX - 6, y + pupilOffsetY - 6, 5, COLOR_WHITE);
    gfx->drawCircle(x, y, EYE_RADIUS, COLOR_BLACK);
    gfx->fillRect(x - EYE_RADIUS - 2, y - EYE_RADIUS - 2, (EYE_RADIUS * 2) + 4, EYE_RADIUS + 2, COLOR_BG);
    gfx->drawLine(x - EYE_RADIUS, y, x + EYE_RADIUS, y, COLOR_WHITE);
    gfx->drawLine(x - EYE_RADIUS, y + 1, x + EYE_RADIUS, y + 1, COLOR_WHITE);
    gfx->drawLine(x - EYE_RADIUS, y + 2, x + EYE_RADIUS, y + 2, COLOR_WHITE);
}

void drawBrow(int x, int y, int angle) {
    int x1, y1, x2, y2;
    if (angle > 0) {
        x1 = x - 25; y1 = y - EYE_RADIUS - 8;
        x2 = x + 25; y2 = y - EYE_RADIUS - 15;
    } else if (angle < 0) {
        x1 = x - 25; y1 = y - EYE_RADIUS - 15;
        x2 = x + 25; y2 = y - EYE_RADIUS - 8;
    } else {
        x1 = x - 25; y1 = y - EYE_RADIUS - 12;
        x2 = x + 25; y2 = y - EYE_RADIUS - 12;
    }
    gfx->drawLine(x1, y1, x2, y2, COLOR_WHITE);
    gfx->drawLine(x1, y1 + 1, x2, y2 + 1, COLOR_WHITE);
}

void drawMouth(int centerX, int centerY, Mood mood, uint16_t color) {
    int mouthY = EYE_Y + EYE_RADIUS + 35;
    gfx->fillRect(centerX - 60, mouthY - 25, 120, 50, COLOR_BG);
    
    switch (mood) {
        case HAPPY:
        case EXCITED:
            gfx->fillEllipse(centerX, mouthY + 5, 45, 20, color);
            gfx->fillRect(centerX - 50, mouthY - 20, 100, 25, COLOR_BG);
            if (mood == EXCITED) gfx->fillEllipse(centerX, mouthY + 10, 30, 15, COLOR_BLACK);
            break;
        case LOVE:
            gfx->fillEllipse(centerX, mouthY, 35, 18, COLOR_PINK);
            gfx->fillRect(centerX - 40, mouthY - 18, 80, 18, COLOR_BG);
            break;
        case SAD:
        case TIRED:
            gfx->fillEllipse(centerX, mouthY - 10, 40, 20, color);
            gfx->fillRect(centerX - 45, mouthY, 90, 25, COLOR_BG);
            break;
        case ANGRY:
            gfx->fillRect(centerX - 35, mouthY - 3, 70, 6, COLOR_RED);
            break;
        default:
            gfx->drawFastHLine(centerX - 30, mouthY, 60, color);
            gfx->drawFastHLine(centerX - 30, mouthY + 1, 60, color);
            break;
    }
}

void drawCheeks(Mood mood) {
    if (mood == HAPPY || mood == EXCITED || mood == LOVE) {
        gfx->fillCircle(EYE_LEFT_X - 40, EYE_Y + 20, 12, COLOR_PINK);
        gfx->fillCircle(EYE_RIGHT_X + 40, EYE_Y + 20, 12, COLOR_PINK);
    }
}

void drawHeartSymbols() {
    for (int i = 0; i < 3; i++) {
        int x = 30 + i * 90;
        int y = 50 + (i % 2) * 30;
        int size = 8;
        gfx->fillCircle(x - size/2, y, size/2, COLOR_PINK);
        gfx->fillCircle(x + size/2, y, size/2, COLOR_PINK);
        gfx->fillTriangle(x - size, y, x + size, y, x, y + size, COLOR_PINK);
    }
}

void drawSweatDrop() {
    gfx->fillCircle(EYE_RIGHT_X + 35, EYE_Y - 15, 8, COLOR_CYAN);
    gfx->fillCircle(EYE_RIGHT_X + 35, EYE_Y - 15, 5, COLOR_WHITE);
    gfx->fillTriangle(EYE_RIGHT_X + 35 - 3, EYE_Y - 7, EYE_RIGHT_X + 35 + 3, EYE_Y - 7, EYE_RIGHT_X + 35, EYE_Y + 2, COLOR_CYAN);
}

void drawAngerSymbols() {
    gfx->drawLine(EYE_LEFT_X - 45, EYE_Y - 25, EYE_LEFT_X - 35, EYE_Y - 35, COLOR_RED);
    gfx->drawLine(EYE_LEFT_X - 40, EYE_Y - 25, EYE_LEFT_X - 30, EYE_Y - 35, COLOR_RED);
    gfx->drawLine(EYE_LEFT_X - 47, EYE_Y - 30, EYE_LEFT_X - 33, EYE_Y - 30, COLOR_RED);
}

void drawEmotion(Mood mood, String moodName, uint16_t mainColor) {
    currentMood = mood;
    currentIrisColor = mainColor;
    
    drawGradientBackground();
    
    switch (mood) {
        case LOVE: drawHeartSymbols(); break;
        case TIRED: drawSweatDrop(); break;
        case ANGRY: drawAngerSymbols(); break;
    }
    
    switch (mood) {
        case ANGRY:
            drawBrow(EYE_LEFT_X, EYE_Y, -1);
            drawBrow(EYE_RIGHT_X, EYE_Y, -1);
            break;
        case CONFUSED:
            drawBrow(EYE_LEFT_X, EYE_Y, 1);
            drawBrow(EYE_RIGHT_X, EYE_Y, 0);
            break;
        case EXCITED:
            drawBrow(EYE_LEFT_X, EYE_Y, 1);
            drawBrow(EYE_RIGHT_X, EYE_Y, 1);
            break;
        default:
            drawBrow(EYE_LEFT_X, EYE_Y, 0);
            drawBrow(EYE_RIGHT_X, EYE_Y, 0);
            break;
    }
    
    if (mood == TIRED) {
        drawTiredEye(EYE_LEFT_X, EYE_Y, mainColor);
        drawTiredEye(EYE_RIGHT_X, EYE_Y, mainColor);
    } else if (mood == HAPPY || mood == EXCITED) {
        drawEye(EYE_LEFT_X, EYE_Y - 3, mainColor);
        drawEye(EYE_RIGHT_X, EYE_Y - 3, mainColor);
    } else if (mood == SAD) {
        drawEye(EYE_LEFT_X, EYE_Y + 5, COLOR_BLUE);
        drawEye(EYE_RIGHT_X, EYE_Y + 5, COLOR_BLUE);
        gfx->fillCircle(EYE_LEFT_X, EYE_Y + EYE_RADIUS + 10, 4, COLOR_CYAN);
        gfx->fillCircle(EYE_RIGHT_X, EYE_Y + EYE_RADIUS + 10, 4, COLOR_CYAN);
    } else {
        drawEye(EYE_LEFT_X, EYE_Y, mainColor);
        drawEye(EYE_RIGHT_X, EYE_Y, mainColor);
    }
    
    drawCheeks(mood);
    drawMouth(SCREEN_WIDTH / 2, EYE_Y, mood, mainColor);
    drawStatusBar();
}

void blinkEyes() {
    int eyeY = (currentMood == HAPPY || currentMood == EXCITED) ? EYE_Y - 3 : EYE_Y;
    drawEye(EYE_LEFT_X, eyeY, currentIrisColor, false);
    drawEye(EYE_RIGHT_X, eyeY, currentIrisColor, false);
    delay(BLINK_DURATION);
}

void setRandomMood() {
    int moodIndex = random(0, 8);
    moveIrisRandomly();
    
    switch (moodIndex) {
        case 0: drawEmotion(NEUTRAL, "NEUTRAL", COLOR_CYAN); break;
        case 1: drawEmotion(HAPPY, "FELIZ", COLOR_YELLOW); break;
        case 2: drawEmotion(EXCITED, "EMPOLGADO", COLOR_ORANGE); break;
        case 3: drawEmotion(LOVE, "APAIXONADO", COLOR_PINK); break;
        case 4: drawEmotion(TIRED, "CANSADO", COLOR_PURPLE); break;
        case 5: drawEmotion(SAD, "TRISTE", COLOR_BLUE); break;
        case 6: drawEmotion(ANGRY, "BRAVO", COLOR_RED); break;
        case 7: drawEmotion(CONFUSED, "CONFUSO", COLOR_LIME); break;
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    if (!gfx->begin()) {
        Serial.println("❌ ERRO Display!");
        while(1);
    }
    
    SerialBT.begin(deviceName);
    SerialBT.register_callback(callback);
    Serial.println("✅ BT: " + deviceName);
    
    gfx->fillScreen(COLOR_BLACK);
    
    // Splash
    gfx->fillScreen(COLOR_BG);
    gfx->setTextColor(COLOR_YELLOW);
    gfx->setTextSize(3);
    gfx->setCursor(20, 60);
    gfx->println("MONSTRIX");
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_CYAN);
    gfx->setCursor(70, 100);
    gfx->println("2.0");
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(30, 140);
    gfx->println("Robot de Emocoes");
    gfx->setCursor(15, 160);
    gfx->setTextColor(COLOR_GREEN);
    gfx->print("BT: ");
    gfx->print(deviceName);
    
    delay(3000);
    
    randomSeed(analogRead(0));
    updateStatusMessage();
    
    Serial.println("🤖 PRONTO!");
    
    drawEmotion(NEUTRAL, "PRONTO!", COLOR_GREEN);
    delay(1000);
    
    lastEmotionChange = millis();
    lastBlink = millis();
    lastIrisMove = millis();
    lastStatusScroll = millis();
    
    moveIrisRandomly();
}

void loop() {
    unsigned long now = millis();
    
    processBluetoothCommand();
    
    static unsigned long lastStatusUpdate = 0;
    if (now - lastStatusUpdate > 5000) {
        lastStatusUpdate = now;
        updateStatusMessage();
    }
    
    // Rolagem do status
    if (now - lastStatusScroll > 100) {
        lastStatusScroll = now;
        scrollStatusBar();
    }
    
    // ✅ MOVIMENTO INSTANTÂNEO DAS ÍRIS (SEM GLITCH!)
    // Só atualiza quando chegar a nova posição aleatória
    if (now - lastIrisMove > IRIS_MOVE_INTERVAL) {
        lastIrisMove = now;
        moveIrisRandomly();
        updateIrisPosition(); // Atualiza instantaneamente
        
        // Redesenha os olhos com a nova posição
        if (currentMood != TIRED && currentMood != SAD) {
            int eyeY = (currentMood == HAPPY || currentMood == EXCITED) ? EYE_Y - 3 : EYE_Y;
            drawEye(EYE_LEFT_X, eyeY, currentIrisColor);
            drawEye(EYE_RIGHT_X, eyeY, currentIrisColor);
        }
    }
    
    // Piscar
    if (now - lastBlink > random(5000, 9000)) {
        lastBlink = now;
        blinkEyes();
        delay(100);
        
        int eyeY = (currentMood == HAPPY || currentMood == EXCITED) ? EYE_Y - 3 : EYE_Y;
        if (currentMood == TIRED) {
            drawTiredEye(EYE_LEFT_X, EYE_Y, currentIrisColor);
            drawTiredEye(EYE_RIGHT_X, EYE_Y, currentIrisColor);
        } else if (currentMood == SAD) {
            drawEye(EYE_LEFT_X, EYE_Y + 5, COLOR_BLUE);
            drawEye(EYE_RIGHT_X, EYE_Y + 5, COLOR_BLUE);
            gfx->fillCircle(EYE_LEFT_X, EYE_Y + EYE_RADIUS + 10, 4, COLOR_CYAN);
            gfx->fillCircle(EYE_RIGHT_X, EYE_Y + EYE_RADIUS + 10, 4, COLOR_CYAN);
        } else {
            drawEye(EYE_LEFT_X, eyeY, currentIrisColor);
            drawEye(EYE_RIGHT_X, eyeY, currentIrisColor);
        }
    }
    
    // Mudar emoção
    if (now - lastEmotionChange > EMOTION_INTERVAL) {
        lastEmotionChange = now;
        setRandomMood();
    }
}
