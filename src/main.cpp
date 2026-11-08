#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"

// Cấu hình chân cho module AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_GPIO     4

// Tần số XCLK cấp cho cảm biến OV2640 (mặc định 20MHz phát xạ sóng hài gây nghẽn GPS L1 1575.42MHz)
// Hạ xuống 10MHz để dời các vạch phổ sóng hài ra xa và giảm mạnh biên độ nhiễu bức xạ RF
#define CAMERA_XCLK_FREQ  10000000

// UART2 trên ESP32-CAM: Giao tiếp với ESP32 chính (OBC)
// Dùng GPIO 14 (TX) và GPIO 15 (RX) thay vì UART0
#define OBC_UART_TX   14
#define OBC_UART_RX   15

static void sendCaptureError(const char* message) {
    Serial2.print("ERROR:");
    Serial2.println(message);
}

// Hàm debug test camera độc lập qua Serial0 (Serial Monitor trên máy tính)
void testCaptureViaSerial0() {
    Serial.println("\n========== [DEBUG TEST CAMERA (Serial0)] ==========");
    Serial.println("[DEBUG] Dang chup anh test tu cam bien OV2640...");
    
    unsigned long tStart = millis();
    digitalWrite(FLASH_LED_GPIO, HIGH);
    camera_fb_t* fb = esp_camera_fb_get();
    digitalWrite(FLASH_LED_GPIO, LOW);
    unsigned long tDuration = millis() - tStart;

    if (!fb) {
        Serial.println("[DEBUG ERROR] Khong the chup anh! (fb_get tra ve NULL)");
        Serial.println("[DEBUG ERROR] Hay kiem tra tiep xuc cap FPC cua mat cam hoac nguon cap.");
        Serial.println("===================================================\n");
        return;
    }

    Serial.printf("[DEBUG SUCCESS] Chup anh test thanh cong!\n");
    Serial.printf("  - Kich thuoc anh : %u bytes (%.2f KB)\n", fb->len, fb->len / 1024.0);
    Serial.printf("  - Do phan giai   : %u x %u\n", fb->width, fb->height);
    Serial.printf("  - Dinh dang      : %s\n", (fb->format == PIXFORMAT_JPEG) ? "JPEG" : "RAW/OTHER");
    Serial.printf("  - Thoi gian chup : %lu ms\n", tDuration);

    // Kiem tra JPEG Magic Bytes (SOI: 0xFF 0xD8, EOI: 0xFF 0xD9)
    if (fb->len >= 4 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8) {
        Serial.println("  - Header JPEG    : Hop le (0xFF 0xD8 - Start of Image)");
    } else {
        Serial.println("  - Header JPEG    : [CANH BAO] Khong dung header JPEG!");
    }

    if (fb->len >= 2 && fb->buf[fb->len - 2] == 0xFF && fb->buf[fb->len - 1] == 0xD9) {
        Serial.println("  - Footer JPEG    : Hop le (0xFF 0xD9 - End of Image)");
    } else {
        Serial.println("  - Footer JPEG    : [CANH BAO] Thieu footer JPEG!");
    }

    esp_camera_fb_return(fb);
    Serial.println("===================================================\n");
}

// Xử lý lệnh chụp và gửi ảnh sang OBC qua UART2
void handleOBCCommand() {
    Serial.println("[CAM] Nhan lenh 'p' tu OBC qua UART2. Bat dau chup...");

    camera_fb_t* frame = esp_camera_fb_get();
    if (frame == nullptr) {
        sendCaptureError("FRAME_NULL");
        Serial.println("[CAM] Loi: Khong chup duoc anh!");
        return;
    }

    digitalWrite(FLASH_LED_GPIO, HIGH);
    Serial2.printf("SIZE:%u\n", frame->len);
    Serial.printf("[CAM] Kich thuoc anh gui OBC: %u bytes\n", frame->len);

    // Chờ OBC xác nhận sẵn sàng nhận dữ liệu
    unsigned long startTime = millis();
    bool ready = false;
    while (millis() - startTime < 5000) {
        if (Serial2.available()) {
            String response = Serial2.readStringUntil('\n');
            response.trim();
            if (response == "OK") {
                ready = true;
                break;
            }
        }
        delay(1);
    }

    if (!ready) {
        sendCaptureError("ACK_TIMEOUT");
        Serial.println("[CAM] Loi: OBC khong phan hoi ACK!");
        digitalWrite(FLASH_LED_GPIO, LOW);
        esp_camera_fb_return(frame);
        return;
    }

    // Gửi dữ liệu ảnh raw qua UART2 đến OBC
    Serial.println("[CAM] Dang truyen du lieu anh qua UART2...");
    Serial2.write(frame->buf, frame->len);
    Serial2.flush();

    // Chờ OBC xác nhận hoàn tất
    startTime = millis();
    while (millis() - startTime < 15000) {
        if (Serial2.available()) {
            String response = Serial2.readStringUntil('\n');
            response.trim();
            if (response == "DONE") {
                Serial.println("[CAM] OBC xac nhan nhan anh thanh cong!");
                break;
            }
        }
        delay(1);
    }

    digitalWrite(FLASH_LED_GPIO, LOW);
    esp_camera_fb_return(frame);
}

void setup() {
    // 1. Hạ xung nhịp CPU từ 240MHz xuống 80MHz (Giảm nhiễu nền RF và tiết kiệm điện năng)
    setCpuFrequencyMhz(80);

    // 2. Tắt triệt để Wi-Fi và Bluetooth để tránh rò sóng 2.4GHz và triệt tiêu gai dòng
    WiFi.mode(WIFI_OFF);
    btStop();

    pinMode(FLASH_LED_GPIO, OUTPUT);
    digitalWrite(FLASH_LED_GPIO, LOW);

    // UART0 (Serial) kết nối với PC qua USB/FTDI để debug
    Serial.begin(115200);
    delay(500);

    // UART2 (Serial2) kết nối với OBC
    Serial2.begin(115200, SERIAL_8N1, OBC_UART_RX, OBC_UART_TX);

    Serial.println("\n==========================================");
    Serial.println("       ESP32-CAM DUAL-UART FIRMWARE       ");
    Serial.println("       (LOW EMI / GPS COMPATIBLE)         ");
    Serial.println("==========================================");
    Serial.printf("CPU Frequency   : %d MHz (Giam tu 240MHz)\n", getCpuFrequencyMhz());
    Serial.printf("Camera XCLK     : %d MHz (Giam tu 20MHz de tranh gay nhieu GPS L1)\n", CAMERA_XCLK_FREQ / 1000000);
    Serial.println("Wi-Fi / BT      : DISABLED (Khong phat song RF)");
    Serial.println("UART0 (USB/FTDI): Debug & Test Monitor");
    Serial.println("UART2 (GPIO14/15): Ket noi OBC (ESP32)");
    Serial.println("------------------------------------------");
    Serial.println("Menu debug tren Serial Monitor (Serial0):");
    Serial.println("  - Nhap 't' hoac 'p': Test chup anh truc tiep");
    Serial.println("  - Nhap 'f'         : Bat/Tat den Flash LED");
    Serial.println("  - Nhap 's'         : Xem thong tin xung nhip & he thong");
    Serial.println("==========================================\n");

    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = CAMERA_XCLK_FREQ;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[ERROR] Camera init that bai: 0x%x\n", err);
        Serial2.printf("ERROR:CAMERA_INIT:0x%x\n", err);
        return;
    }
    
    Serial.println("[OK] Camera init thanh cong. San sang!");
    Serial2.println("CAMERA_READY");
}

void loop() {
    // 1. Kiểm tra lệnh debug từ Serial0 (máy tính qua USB/FTDI)
    if (Serial.available() > 0) {
        char debugCmd = Serial.read();
        if (debugCmd == 't' || debugCmd == 'T' || debugCmd == 'p' || debugCmd == 'P') {
            testCaptureViaSerial0();
        } else if (debugCmd == 'f' || debugCmd == 'F') {
            static bool flashState = false;
            flashState = !flashState;
            digitalWrite(FLASH_LED_GPIO, flashState ? HIGH : LOW);
            Serial.printf("[DEBUG] Flash LED: %s\n", flashState ? "ON" : "OFF");
        } else if (debugCmd == 's' || debugCmd == 'S') {
            Serial.println("\n--- [SYSTEM & EMI STATUS] ---");
            Serial.printf("  - CPU Frequency : %d MHz\n", getCpuFrequencyMhz());
            Serial.printf("  - Camera XCLK   : %d MHz\n", CAMERA_XCLK_FREQ / 1000000);
            Serial.println("  - Wi-Fi Radio   : OFF");
            Serial.println("  - Bluetooth     : OFF");
            Serial.println("-----------------------------\n");
        }
    }

    // 2. Chờ lệnh từ ESP32 chính (OBC) qua UART2
    if (Serial2.available() > 0) {
        String command = Serial2.readStringUntil('\n');
        command.trim();

        if (command == "p" || command == "P") {
            handleOBCCommand();
        }
    }
}
