# ESP32_CAM_Firmware

**Firmware độc lập cho module ESP32-CAM**

Firmware này dành riêng cho bo mạch AI-Thinker ESP32-CAM. Nó đóng vai trò như một camera slave độc lập, hỗ trợ 2 cổng giao tiếp đồng thời:
1. **UART0 (USB/FTDI - 115200 baud):** Dành riêng cho Debug, cấu hình và Test Camera trực tiếp từ máy tính.
2. **UART2 (GPIO 14 TX, GPIO 15 RX - 115200 baud):** Dành cho giao tiếp với máy tính trung tâm (ESP32 Dev Module / OBC).

---

### Sơ đồ nối dây giữa ESP32-CAM và ESP32 OBC:

| Chân ESP32-CAM | Chân ESP32 (OBC) | Chức năng |
|:---|:---|:---|
| **GPIO 14 (TX2)** | **GPIO 16 (RX2)** | Dữ liệu truyền từ Camera sang OBC |
| **GPIO 15 (RX2)** | **GPIO 17 (TX2)** | Lệnh từ OBC sang Camera |
| **GND** | **GND** | Mass chung |
| **5V / 3.3V** | **Nguồn ngoài / 5V** | Nguồn cấp cho Camera |

---

### 🛠️ Tính năng Debug & Test độc lập qua Serial0 (Serial Monitor):
Mở Serial Monitor ở tốc độ **115200 baud** kết nối với ESP32-CAM:
* **Gõ `t` hoặc `p` + Enter:** Kích hoạt chụp một bức ảnh test trực tiếp từ cảm biến OV2640. Firmware sẽ in chi tiết:
  - Trạng thái chụp (Thành công / Lỗi)
  - Dung lượng ảnh (Bytes / KB)
  - Độ phân giải (VGA 640x480)
  - Thời gian xử lý chụp (ms)
  - Kiểm tra tính hợp lệ của Header/Footer JPEG (`0xFF 0xD8` và `0xFF 0xD9`)
* **Gõ `f` + Enter:** Bật / Tắt đèn Flash LED (GPIO 4) để kiểm tra phần cứng.

---

### Giao thức với OBC qua UART2:
* Khi OBC gửi lệnh `p\n` qua UART2, ESP32-CAM sẽ chụp ảnh, phản hồi `SIZE:<kích thước>`, chờ tín hiệu `OK\n`, truyền dữ liệu ảnh thô và kết thúc khi nhận `DONE\n`.
