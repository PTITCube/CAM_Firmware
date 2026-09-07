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

void setup() {
    Serial.begin(115200);
    Serial.println("Khoi dong ESP32-CAM...");

    camera_config_t config;
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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }
    
    Serial.println("Camera init OK.");
    Serial.println("Sẵn sàng nhận lệnh 'CAPTURE' từ ESP32 OBC...");
}

void loop() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "CAPTURE") {
            camera_fb_t * fb = esp_camera_fb_get();
            if (!fb) {
                // Có thể báo lỗi về cho OBC
                return;
            }
            
            // 1. Gửi kích thước ảnh
            Serial.printf("SIZE:%d\n", fb->len);
            
            // 2. Chờ OBC gửi lệnh OK
            long startTime = millis();
            bool okReceived = false;
            while (millis() - startTime < 3000) {
                if (Serial.available()) {
                    String ack = Serial.readStringUntil('\n');
                    ack.trim();
                    if (ack == "OK") {
                        okReceived = true;
                        break;
                    }
                }
                delay(10);
            }
            
            // 3. Nếu OBC đồng ý, gửi raw bytes
            if (okReceived) {
                Serial.write(fb->buf, fb->len);
                
                // Chờ OBC báo DONE (có thể bỏ qua bước này nếu không cần thiết)
                startTime = millis();
                while (millis() - startTime < 15000) {
                    if (Serial.available()) {
                        String ack = Serial.readStringUntil('\n');
                        ack.trim();
                        if (ack == "DONE") {
                            break;
                        }
                    }
                    delay(10);
                }
            }
            
            // 4. Giải phóng buffer
            esp_camera_fb_return(fb);
        }
    }
}
