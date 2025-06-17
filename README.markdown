# Hệ Thống Điều Khiển Thiết Bị Bằng Đồng Hồ Thông Minh

## Mô tả

Dự án này phát triển hệ thống điều khiển đèn trong nhà thông qua đồng hồ thông minh. Hệ thống bao gồm:

- **Hệ thống điều khiển thiết bị**: Điều khiển bật/tắt đèn.
- **Hệ thống Gateway**: Trung gian xử lý và chuyển tiếp lệnh.
- **Hệ thống đồng hồ thông minh**: Gửi lệnh điều khiển.

Tín hiệu hồng ngoại từ điện thoại được gửi đến hệ thống điều khiển thiết bị, sau đó thông qua Firebase và Gateway sử dụng Bluetooth Low Energy để kết nối với đồng hồ, chuyển tiếp đến đồng hồ. Đồng hồ truyền lệnh đến Gateway, MCU xử lý và gửi ngược lại để điều khiển đèn.

## Tính năng chính

- Điều khiển bật/tắt đèn qua tín hiệu hồng ngoại.
- Giao tiếp giữa ESP32_NodeMCU, ESP32-C6, ESP8266 và Firebase.
- Hiển thị trạng thái thiết bị trên màn hình OLED của đồng hồ.
- Điều khiển thiết bị qua Firebase Realtime Database.

## Phần cứng

- **ESP32_NodeMCU**: Vi điều khiển cho đồng hồ.
- **LED phát hồng ngoại**: Phát tín hiệu IR.
- **Cảm biến âm thanh, MPU6050**: Phát hiện búng tay và định hướng.
- **Màn hình OLED 0.96 inch**: Hiển thị trạng thái.
- **Pin Li-Po 3.7V 2500mAh**: Nguồn cho đồng hồ.
- **Mạch sạc TP4056**: Sạc pin.
- **Mạch tăng áp TP3608**: Tăng điện áp.
- **ESP32-C6**: Vi điều khiển Gateway.
- **ESP8266**: Vi điều khiển hệ thống điều khiển thiết bị.
- **Module relay 5V**: Điều khiển đèn.
- **LED thu hồng ngoại**: Nhận tín hiệu IR.

## Phần mềm

- **Arduino IDE**: Lập trình ESP32 và ESP8266.
- **Firebase Realtime Database**: Đồng bộ dữ liệu thời gian thực.

## Hướng dẫn sử dụng

## Hướng dẫn sử dụng

Thực hiện các thao tác sau để sử dụng sản phẩm:

- **Bước 1**: Cấp nguồn đúng theo hướng dẫn trong phần thiết lập các hệ thống.
- **Bước 2**: Kết nối hệ thống điều khiển đèn và Gateway với Wi-Fi tại hộ gia đình bạn.
- **Bước 3**: Kết nối Bluetooth giữa Gateway và đồng hồ.
- **Bước 4**: Sau khi thiết lập Wi-Fi và Bluetooth thành công, đồng hồ sẽ hiển thị thông tin và có thể phát tín hiệu hồng ngoại. Chúng ta có thể sử dụng tín hiệu hồng ngoại từ remote hoặc điện thoại.

## Cấu trúc thư mục

```
DA1_System for Controlling Devices Using a Smartwatch/
├── Fireware/               # Mã nguồn firmware
│   ├── Equipment_control_system/  # Mã cho hệ thống điều khiển thiết bị (ESP8266)
│   ├── Gateway_system/            # Mã cho hệ thống Gateway (ESP32-C6)
│   └── Swatch_watch_system/       # Mã cho đồng hồ thông minh (ESP32_NodeMCU)
├── Handware_Design/        # Thiết kế phần cứng
│   ├── Equipment_control_and_gateway_system/  # Sơ đồ mạch hệ thống điều khiển thiết bị và Gateway
│   ├── Gaber_File/                   # File Gerber
│   └── Swatch_watch_system/          # Sơ đồ mạch đồng hồ thông minh
└── Presentation/          # Slide trình bày
```

## Tác giả

- [Đỗ Phúc Duy] - [Email: 22161102@student.hcmute.edu.vn]
