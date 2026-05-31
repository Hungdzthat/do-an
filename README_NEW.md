# 📡 SPI Communication Project

## 🎯 Mô Tả Dự Án

Dự án này là một ứng dụng **Serial Peripheral Interface (SPI)** được phát triển cho hệ thống nhúng STM32. Nó cung cấp các driver và thư viện cơ bản để giao tiếp SPI, bao gồm các tính năng:

- 🔄 Giao tiếp SPI song phương (Full-duplex)
- ⚙️ Cấu hình linh hoạt tốc độ và chế độ
- 📦 Drivers tối ưu hóa cho hiệu suất
- 🛡️ Xử lý lỗi toàn diện

---

## 📋 Yêu Cầu Hệ Thống

Trước khi cài đặt, đảm bảo bạn có:

- **STM32CubeMX** (hoặc phần mềm tương tự)
- **Keil MDK-ARM** v5.x trở lên
- **STM32 HAL Libraries**
- **ARM GCC Compiler** (nếu sử dụng GCC)
- **Git** để clone repository

---

## 💾 Hướng Dẫn Cài Đặt

### Bước 1: Clone Repository

```bash
git clone https://github.com/quytmoiz146/Do-an-1.git
cd Do-an-1
```

### Bước 2: Mở Dự Án trong STM32CubeMX

1. Mở **STM32CubeMX**
2. Chọn `File` → `Open Project`
3. Điều hướng đến `SPI.ioc` trong thư mục dự án

### Bước 3: Sinh Code

1. Chọn `Project` → `Generate Code`
2. Code sẽ được sinh ra trong thư mục `MDK-ARM`

### Bước 4: Compile Dự Án

```bash
cd MDK-ARM
# Sử dụng Keil MDK-ARM hoặc lệnh dòng lệnh:
# armcc -c --c99 --cpu=Cortex-M4 ...
```

---

## 🚀 Cách Sử Dụng

### Cấu Trúc Thư Mục

```
Do-an-1/
├── Core/                 # Mã nguồn chính của ứng dụng
│   ├── Inc/             # File header
│   ├── Src/             # File source
│   └── Startup/         # Startup code
├── Drivers/             # Driver và thư viện HAL
│   ├── CMSIS/           # Cortex-M system files
│   └── STM32*/          # STM32 HAL drivers
├── MDK-ARM/             # Dự án Keil uVision
├── SPI.ioc              # STM32CubeMX configuration
└── README.md            # File này
```

### Ví Dụ Cơ Bản

#### Khởi Tạo SPI

```c
#include "spi.h"

// Khởi tạo SPI
void SPI_Init(void) {
    // Cấu hình GPIO
    // Cấu hình SPI peripheral
    // Bật SPI
    HAL_SPI_Init(&hspi1);
}
```

#### Gửi Dữ Liệu

```c
// Gửi dữ liệu
uint8_t data_tx[] = {0x01, 0x02, 0x03};
HAL_SPI_Transmit(&hspi1, data_tx, sizeof(data_tx), HAL_MAX_DELAY);
```

#### Nhận Dữ Liệu

```c
// Nhận dữ liệu
uint8_t data_rx[3];
HAL_SPI_Receive(&hspi1, data_rx, sizeof(data_rx), HAL_MAX_DELAY);
```

#### Giao Tiếp Song Phương

```c
// Gửi và nhận cùng lúc
uint8_t tx_buffer[] = {0xAA, 0xBB};
uint8_t rx_buffer[2];
HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, 2, HAL_MAX_DELAY);
```

### Cấu Hình SPI

Các thông số có thể cấu hình trong STM32CubeMX:

| Thông Số | Mô Tả |
|----------|-------|
| **Tốc độ** | 1 MHz - 50 MHz (phụ thuộc MCU) |
| **Mode** | 0-3 (CPOL & CPHA) |
| **Data Width** | 8-bit hoặc 16-bit |
| **MSB/LSB** | Chọn thứ tự truyền |

---

## 🧪 Kiểm Tra & Debug

### Sử Dụng STM32CubeMonitor

1. Kết nối board STM32 qua **USB/ST-Link**
2. Mở **STM32CubeMonitor**
3. Chọn COM port và baud rate
4. Theo dõi dữ liệu SPI trong real-time

### Debug Breakpoints

```c
// Thêm breakpoint để kiểm tra giá trị
uint8_t received_data = HAL_SPI_Receive(...); // ← Đặt breakpoint ở đây
```

---

## 🤝 Thông Tin Đóng Góp

Chúng tôi rất hoan nghênh những đóng góp từ cộng đồng!

### Hướng Dẫn Đóng Góp

1. **Fork** repository này
2. **Tạo branch** cho tính năng của bạn:
   ```bash
   git checkout -b feature/AmazingFeature
   ```
3. **Commit** thay đổi:
   ```bash
   git commit -m "Add some AmazingFeature"
   ```
4. **Push** đến branch:
   ```bash
   git push origin feature/AmazingFeature
   ```
5. **Mở Pull Request**

### Tiêu Chuẩn Code

- ✅ Tuân thủ **MISRA C** (nếu có thể)
- ✅ Thêm **comments** cho code phức tạp
- ✅ Kiểm tra trên hardware trước khi submit
- ✅ Cập nhật documentation nếu thay đổi API

### Báo Cáo Bug

Nếu tìm thấy bug, vui lòng:
1. Kiểm tra Issue hiện có
2. Tạo Issue mới với:
   - 📝 Mô tả chi tiết vấn đề
   - 🔧 Các bước tái hiện
   - 💻 Hardware/Software được sử dụng
   - 📸 Screenshots hoặc logs (nếu có)

---

## 📄 License

Dự án này được phân phối dưới **MIT License** - xem file `LICENSE` để biết chi tiết.

---

## 📧 Liên Hệ

- **Tác giả**: quytmoiz146
- **Email**: [Thêm email của bạn]
- **GitHub**: https://github.com/quytmoiz146/Do-an-1
- **Issues**: https://github.com/quytmoiz146/Do-an-1/issues

---

## 📚 Tài Liệu Tham Khảo

- [STM32 HAL Documentation](https://www.st.com/en/embedded-software/stm32cube-mcu-packages.html)
- [SPI Protocol Overview](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface)
- [STM32CubeMX User Manual](https://www.st.com/content/dam/FAQs/MCU/STM32/STM32CubeMX_UserGuide.pdf)

---

## ✨ Lịch Sử Cập Nhật

| Version | Ngày | Mô Tả |
|---------|------|-------|
| v1.0.0 | 2026-05-31 | Release ban đầu |
| | | |

---

<div align="center">

**⭐ Nếu dự án này hữu ích, hãy đặt một sao! ⭐**

Made with ❤️ by quytmoiz146

</div>
