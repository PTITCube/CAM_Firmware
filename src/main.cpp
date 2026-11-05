#include <Arduino.h>
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

// UART2 trên ESP32-CAM: Giao tiếp với ESP32 chính (OBC)
// Dùng GPIO 14 (TX) và GPIO 15 (RX) thay vì UART0
#define OBC_UART_TX   14
#define OBC_UART_RX   15

static void sendCaptureError(const char* message) {
    Serial2.print("ERROR:");
    Serial2.println(message);
}

void setup() {
    pinMode(FLASH_LED_GPIO, OUTPUT);
    digitalWrite(FLASH_LED_GPIO, LOW);

    // UART0 giữ cho debug qua USB
    Serial.begin(115200);
    delay(500);

    // UART2 để giao tiếp với ESP32 chính (OBC)
    Serial2.begin(115200, SERIAL_8N1, OBC_UART_RX, OBC_UART_TX);

    Serial.println("Khoi dong ESP32-CAM...");
    Serial.println("UART2 (GPIO14-TX, GPIO15-RX) -> OBC");

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
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("ERROR:CAMERA_INIT:0x%x\n", err);
        Serial2.printf("ERROR:CAMERA_INIT:0x%x\n", err);
        return;
    }
    
    Serial.println("Camera init OK.");
    Serial2.println("CAMERA_READY");
}

void loop() {
    // Chờ lệnh từ ESP32 chính (OBC) qua UART2
    if (Serial2.available() == 0) {
        return;
    }

    String command = Serial2.readStringUntil('\n');
    command.trim();

    // Sử dụng lệnh 'p' hoặc 'P' để chụp ảnh
    if (command != "p" && command != "P") {
        return;
    }

    Serial.println("[CAM] Nhan lenh 'p' tu OBC. Bat dau chup...");

    camera_fb_t* frame = esp_camera_fb_get();
    if (frame == nullptr) {
        sendCaptureError("FRAME_NULL");
        Serial.println("[CAM] Loi: Khong chup duoc anh!");
        return;
    }

    digitalWrite(FLASH_LED_GPIO, HIGH);
    Serial2.printf("SIZE:%u\n", frame->len);
    Serial.printf("[CAM] Kich thuoc anh: %u bytes\n", frame->len);

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
    Serial.println("[CAM] Dang gui du lieu anh...");
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
