# ESP32_CAM_Firmware

**Firmware độc lập cho module ESP32-CAM**

Firmware này chỉ dành riêng cho bo mạch ESP32-CAM. Nó đóng vai trò như một cảm biến/camera slave, giao tiếp với máy tính trung tâm (ESP32 Dev Module / OBC) qua giao tiếp UART (Serial).

Khi OBC gửi lệnh `CAPTURE` qua cổng Serial, ESP32-CAM sẽ chụp một bức ảnh và trả về tín hiệu kết quả.
