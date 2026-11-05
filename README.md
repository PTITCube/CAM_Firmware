# ESP32_CAM_Firmware

**Firmware độc lập cho module ESP32-CAM**

Firmware này dành riêng cho bo mạch AI-Thinker ESP32-CAM. Nó đóng vai trò như một camera slave độc lập, giao tiếp với máy tính trung tâm (ESP32 Dev Module / OBC) qua giao tiếp **UART2** (thay vì UART0) để tránh xung đột với cổng nạp/debug USB và GPS trên OBC.

### Sơ đồ nối dây giữa ESP32-CAM và ESP32 OBC:

| Chân ESP32-CAM | Chân ESP32 (OBC) | Chức năng |
|:---|:---|:---|
| **GPIO 14 (TX2)** | **GPIO 16 (RX2)** | Dữ liệu truyền từ Camera sang OBC |
| **GPIO 15 (RX2)** | **GPIO 17 (TX2)** | Lệnh từ OBC sang Camera |
| **GND** | **GND** | Mass chung |
| **5V / 3.3V** | **Nguồn ngoài / 5V** | Nguồn cấp cho Camera |

> **Lưu ý:**
> - Cổng UART0 của ESP32-CAM (GPIO 1 TX, GPIO 3 RX) được giữ nguyên cho việc nạp code và theo dõi Serial Monitor (Baud: 115200).
> - Khi OBC gửi lệnh `p\n` qua UART2, ESP32-CAM sẽ chụp ảnh, phản hồi `SIZE:<kích thước>`, chờ tín hiệu `OK\n`, truyền dữ liệu ảnh thô và kết thúc khi nhận `DONE\n`.
