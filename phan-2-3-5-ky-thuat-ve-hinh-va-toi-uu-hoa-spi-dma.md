# PHẦN 2.3.5: KỸ THUẬT VẼ HÌNH VÀ TỐI ƯU HÓA XUẤT ĐỒ HỌA BẰNG SPI DMA

Trong hệ thống máy hiện sóng số cầm tay (Mini Oscilloscope), việc thu thập dữ liệu nhanh và chính xác mới chỉ giải quyết được một nửa bài toán. Nửa còn lại quyết định trực tiếp đến trải nghiệm người dùng chính là khả năng hiển thị đồ họa theo thời gian thực [6, 13]. Nếu không có các kỹ thuật vẽ hình tối ưu và cơ chế truyền tải phần cứng hiệu năng cao, giao diện hiển thị sẽ bị giật lag, sóng bị trôi hoặc đứt nét, đồng thời làm tiêu tốn quá nhiều tài nguyên của CPU [6, 17]. 

Phần này sẽ trình bày chi tiết về kỹ thuật ánh xạ tọa độ, dựng lưới chia trục (Grid), thuật toán nối điểm liên tục và phương pháp tối ưu hóa xuất đồ họa bằng cách phối hợp bộ đệm khung hình (Frame Buffer) với ngoại vi SPI kết hợp DMA trên vi điều khiển STM32F103C8T6 [13, 17, 33, 51].

---

### 2.3.5.1 Kỹ thuật Ánh xạ Tọa độ và Vẽ hình học Cơ bản

#### 1. Thuật toán Ánh xạ Tuyến tính từ Giá trị ADC sang Tọa độ Màn hình (Y-Axis Mapping)
Bộ chuyển đổi tương tự - số (ADC) của vi điều khiển STM32F103C8T6 có độ phân giải 12-bit, trả về các giá trị số nguyên rời rạc nằm trong khoảng từ $0$ đến $4095$ [16, 19]. Khoảng giá trị này tương ứng tuyến tính với dải điện áp đo được tại chân đầu vào cấu hình ADC (chân PA1) từ $0\text{V}$ đến $3.3\text{V}$ [19, 22]. Mặt khác, tín hiệu đo thực tế tại đầu vào que đo ($V_{\text{thô}}$) sau khi đi qua khối Analog Front-End (AFE) đã bị suy hao $40$ lần, dịch mức lên $+1.65\text{V}$ và khuếch đại lên $2$ lần [28, 29, 31]. Sự biến đổi tuyến tính này được biểu diễn bằng phương thức quy đổi điện áp [31]:

$$V_{\text{input\_adc}} = V_{\text{thô}} \times \frac{1}{40} \times 2 + 1.65 = \frac{V_{\text{thô}}}{20} + 1.65 \quad (\text{V})$$

Để hiển thị dạng sóng một cách trực quan trên màn hình LCD TFT ST7735 có độ phân giải đồ họa $128 \times 160$ Pixels [33], tác vụ hiển thị (`TaskDisplay`) phải thực hiện ánh xạ các giá trị số ADC ($D_{\text{ADC}} \in [0, 4095]$) thành tọa độ trục dọc $Y$ của màn hình [51]. 

Giả sử vùng đồ thị hiển thị dạng sóng (vùng Grid) có chiều cao là $H_{\text{grid}}$ Pixels (vùng vẽ sóng từ dòng $Y_{\text{start}}$ đến $Y_{\text{end}}$), và điểm mốc $0\text{V}$ của tín hiệu tương ứng với vị trí chính giữa của vùng Grid ($Y_{\text{center}}$) [30]. Khi đó, tọa độ hiển thị $Y$ của một điểm dữ liệu được xác định theo công thức:

$$Y = Y_{\text{center}} - \Delta Y$$

Trong đó, độ lệch pixel $\Delta Y$ so với đường nền trung tâm tỉ lệ thuận với độ lệch của điện áp thực tế so với điểm dịch mức $+1.65\text{V}$ (điểm giữa dải đo của ADC) [30]. Công thức chuyển đổi từ giá trị số $D_{\text{ADC}}$ sang tọa độ điểm ảnh $Y$ trên màn hình là [31, 51]:

$$Y = Y_{\text{center}} - \left( D_{\text{ADC}} - 2048 \right) \times \frac{\text{Scale}_{\text{V}}}{2048}$$

Với $\text{Scale}_{\text{V}}$ là hệ số tỉ lệ pixel trên mỗi Volt, được cập nhật tự động từ cấu trúc cấu hình hệ thống `gConfig` khi người dùng nhấn nút tăng/giảm thang đo biên độ (Volt/Div) [51, 52]. Hệ số này đảm bảo dạng sóng luôn tự co giãn vừa vặn trong dải đo hiển thị mà không vượt quá giới hạn biên của vùng Grid [66].

```
     Mức điện áp thô (V)             Giá trị ADC (12-bit)          Tọa độ màn hình Y (Pixels)
   [ +33V cực đại dương ] ---------> [  4095 (3.3V) ] -----------> [ Y_min (Giới hạn trên) ]
                                            |
   [  0V đường trung bình ] --------> [  2048 (1.65V) ] ----------> [ Y_center (Đường trung tâm) ]
                                            |
   [ -33V cực đại âm ] ------------> [   0  (0.0V) ] -----------> [ Y_max (Giới hạn dưới) ]
```

#### 2. Kỹ thuật Dựng lưới tọa độ (Grid Line Generation)
Lưới chia tọa độ (Grid) là thành phần tối quan trọng giúp người dùng ước lượng biên độ (Volt) và chu kỳ (Time) của tín hiệu [13, 32]. Vùng Grid được phân chia thành các ô vuông bằng các đường kẻ dọc và ngang đứt nét [32]. 
- **Đường kẻ dọc (Timebase Grid):** Đại diện cho các mốc thời gian, khoảng cách giữa các đường kẻ dọc tương ứng với một ô chia thời gian (Time/Div) [14].
- **Đường kẻ ngang (Voltage Grid):** Đại diện cho các mức điện áp, khoảng cách giữa các đường kẻ ngang tương ứng với một ô chia điện áp (Volt/Div) [14].

Để vẽ lưới mà không làm suy giảm hiệu năng của vi điều khiển, hệ thống thực hiện viết trực tiếp mã màu của lưới vào các tọa độ cố định trong bộ đệm khung hình. Thay vì vẽ các nét liền làm đè hoàn toàn lên dạng sóng, lưới được thiết kế dưới dạng đường đứt nét (Dotted Lines) bằng cách chỉ đặt pixel màu lưới tại các vị trí chẵn hoặc lẻ xen kẽ:

```c
void Draw_Grid_To_Buffer(uint16_t *frame_buffer) {
    // Vẽ các đường đứt nét ngang (Voltage divisions)
    for (uint16_t y = GRID_Y_START; y <= GRID_Y_END; y += DIV_HEIGHT) {
        for (uint16_t x = GRID_X_START; x <= GRID_X_END; x += 2) {
            frame_buffer[y * LCD_WIDTH + x] = COLOR_GRID; // Chỉ vẽ pixel tại tọa độ chẵn để tạo nét đứt
        }
    }
    // Vẽ các đường đứt nét dọc (Timebase divisions)
    for (uint16_t x = GRID_X_START; x <= GRID_X_END; x += DIV_WIDTH) {
        for (uint16_t y = GRID_Y_START; y <= GRID_Y_END; y += 2) {
            frame_buffer[y * LCD_WIDTH + x] = COLOR_GRID;
        }
    }
}
```

#### 3. Thuật toán Nối điểm liên tục dựng dạng sóng (Waveform Reconstruction)
Nếu hệ thống chỉ ánh xạ các mẫu dữ liệu rời rạc thu được và chấm các điểm đơn lẻ (Dot Mode) lên màn hình, dạng sóng sẽ gặp lỗi hiển thị nghiêm trọng khi tần số của tín hiệu đo tăng cao. Ở tần số cao, khoảng cách về mặt điện áp giữa hai mẫu lấy liên tiếp rất lớn, dẫn đến việc các chấm điểm ảnh bị tách rời xa nhau theo trục dọc $Y$, tạo ra các khoảng trống và khiến dạng sóng bị đứt đoạn, cực kỳ khó quan sát.

Để giải quyết vấn đề này, đồ án ứng dụng **Thuật toán nối điểm đoạn thẳng Bresenham**. Thuật toán này sử dụng hoàn toàn các phép toán số nguyên (phép cộng, trừ và dịch bit), loại bỏ hoàn toàn các phép tính dấu phẩy động phức tạp hay phép chia, giúp tốc độ thực thi đạt mức tối đa trên lõi Cortex-M3 của STM32 [3, 19]. Thuật toán liên tục tính toán sai số tích lũy để quyết định bước đi dọc theo trục có độ dốc lớn hơn, vẽ nên một đường thẳng mịn kết nối giữa tọa độ của hai mẫu dữ liệu kế tiếp: $(X_i, Y_i)$ và $(X_{i+1}, Y_{i+1})$.

```c
void Draw_Line_Bresenham(int x0, int y0, int x1, int y1, uint16_t color, uint16_t *buffer) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // Ghi màu pixel trực tiếp vào bộ đệm khung hình tại tọa độ (x0, y0)
        if (x0 >= 0 && x0 < LCD_WIDTH && y0 >= 0 && y0 < LCD_HEIGHT) {
            buffer[y0 * LCD_WIDTH + x0] = color;
        }
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}
```

---

### 2.3.5.2 Tối ưu hóa Bộ đệm Quét màn hình (Frame Buffer) trên STM32

#### 1. Khái niệm và Thử thách Tài nguyên RAM
Trong kiến trúc xuất đồ họa truyền thống, mỗi khi cần thay đổi một chi tiết nhỏ trên màn hình, CPU sẽ gửi các lệnh thiết lập vùng vẽ và gửi mã màu của từng pixel qua đường truyền bus SPI [17]. Việc này đòi hỏi CPU phải can thiệp liên tục để kiểm tra trạng thái bận của ngoại vi SPI, gây lãng phí một lượng lớn chu kỳ máy và khiến hệ thống bị nghẽn (Busy Waiting) [17, 64].

Để giải quyết triệt để, đồ án xây dựng cơ chế **Bộ đệm khung hình (Frame Buffer)** trong vùng RAM của vi điều khiển [17, 51]. Toàn bộ các thao tác vẽ lưới, viết ký tự thông số, và nối điểm sóng đều được thực hiện trực tiếp thông qua các truy cập bộ nhớ RAM với tốc độ cực nhanh [51]. Sau khi toàn bộ khung hình được dựng hoàn chỉnh trên Frame Buffer, bộ điều khiển DMA sẽ tự động chịu trách nhiệm truyền tải khối dữ liệu lớn này sang màn hình TFT mà không cần CPU can thiệp [17, 51].

Tuy nhiên, cấu trúc phần cứng của chip STM32F103C8T6 đặt ra một thử thách rất lớn: **Dung lượng RAM tối đa chỉ có 20 KB** [19]. 
Nếu sử dụng bộ đệm khung hình toàn phần (Full Frame Buffer) cho màn hình ST7735 có độ phân giải $128 \times 160$ Pixels và mã hóa màu 16-bit (RGB 565, 2 bytes/pixel) [33], dung lượng bộ đệm yêu cầu sẽ là:

$$\text{Size}_{\text{Full}} = 128 \times 160 \times 2 \text{ Bytes} = 40,960 \text{ Bytes} = 40\text{ KB}$$

Con số 40 KB này vượt gấp đôi tổng dung lượng RAM vật lý 20 KB hiện có của chip [19], chưa tính đến việc RAM còn phải chia sẻ cho các biến hệ thống, bộ đệm vòng ADC và ngăn xếp của các tác vụ FreeRTOS [19, 59, 60]. Do đó, bắt buộc hệ thống phải áp dụng các kỹ thuật tối ưu hóa bộ đệm khung hình.

#### 2. Giải pháp Vẽ theo Phân vùng (Band-Buffering / Windowing Slicing)
Để khắc phục hạn chế về bộ nhớ, đồ án triển khai kỹ thuật **Vẽ theo phân vùng (Band-Buffering)**. Thay vì lưu trữ toàn bộ màn hình, RAM chỉ cấp phát một bộ đệm khung hình cục bộ nhỏ hơn, đại diện cho một dải (Band) nằm ngang hoặc một phân vùng cửa sổ chuyên biệt của màn hình:

- **Phân vùng Vùng đồ thị sóng (Waveform Window):** Chỉ dựng bộ đệm cho phân vùng hiển thị đồ thị dạng sóng (ví dụ: kích thước $96 \times 128$ Pixels). Dung lượng RAM cần thiết chỉ là $96 \times 128 \times 2 = 24,576$ Bytes, vẫn lớn nhưng có thể tối ưu thêm thành $96 \times 96$ Pixels ($18,432$ Bytes $\approx 18$ KB) hoặc chia nhỏ hơn nữa.
- **Kỹ thuật Cắt dải (Slicing):** Chia màn hình thành $4$ dải ngang bằng nhau, mỗi dải có kích thước $128 \times 40$ Pixels. 
  - Dung lượng RAM cho mỗi dải chỉ cần: $128 \times 40 \times 2 = 10,240 \text{ Bytes} = 10\text{ KB}$ (Vừa vặn nằm trong tầm kiểm soát của Heap 10 KB khả dụng) [59, 60].
  - Quy trình xử lý: Tác vụ `TaskDisplay` sẽ dựng hình cho dải thứ nhất (vẽ lưới, nối điểm sóng thuộc dải 1), kích hoạt truyền DMA dải 1 sang màn hình [51]. Khi DMA truyền ngầm, CPU chuyển sang dựng hình cho dải thứ 2 trên một bộ đệm phụ (Double-buffer dải nhỏ) hoặc chờ DMA xong để dựng tiếp dải 2 trên chính bộ đệm đó. Phương pháp này giúp tiết kiệm tới $75\%$ dung lượng RAM yêu cầu so với bộ đệm toàn phần mà vẫn đạt hiệu quả tương tự.

---

### 2.3.5.3 Cơ chế Truyền tải Dữ liệu Tốc độ cao qua SPI DMA

#### 1. Nguyên lý phối hợp phần cứng SPI và DMA
Sự kết hợp giữa ngoại vi truyền thông nối tiếp đồng bộ SPI1 phần cứng và bộ điều khiển truy cập bộ nhớ trực tiếp DMA1 là chìa khóa để đạt tốc độ quét màn hình ổn định từ $25$ đến $30$ FPS mà không gây quá tải CPU [13, 55].

```
+------------------+     Dữ liệu pixel     +-------------------+                 +-------------------+
|   RAM Hệ thống   |  ===================> |   Ngoại vi SPI1   |  =============> |    IC ST7735 của  |
|  (Frame Buffer)  |   (Ghi tự động ngầm)  | (Thanh ghi SPI_DR)|  (Đường truyền) |    Màn hình TFT   |
+------------------+                       +-------------------+                 +-------------------+
        ^                                            |                                     ^
        |                                            |                                     |
        +======== Điều khiển truyền ngầm ============+                                     |
                 |     Bộ điều khiển DMA1   |                                              |
                 +--------------------------+                                              |
                                                                                           |
                 +--------------------------+                                              |
                 |  Chân điều khiển GPIO    | ---------------------------------------------+
                 |  CS (PB0), DC (PB1)      |     (Đồng bộ chế độ Nhận dữ liệu hình ảnh)
                 +--------------------------+
```

Bộ điều khiển DMA1 Channel 3 được cấu hình liên kết trực tiếp với thanh ghi truyền dữ liệu của ngoại vi SPI1 (`SPI1->DR`) [19, 22]. Khi quá trình dựng hình trên bộ đệm khung hoàn tất, `TaskDisplay` sẽ thiết lập địa chỉ nguồn của kênh DMA là địa chỉ mảng bộ đệm trong RAM, địa chỉ đích là thanh ghi `SPI1->DR`, và độ dài truyền tải bằng kích thước vùng dữ liệu [17, 51].

Trước khi phát lệnh truyền, chân DC (Data/Command - chân PB1) được kéo lên mức cao ($1$) để báo cho IC driver ST7735 biết các byte sắp truyền tới là mã màu pixel của hình ảnh chứ không phải lệnh cấu hình [23, 36]. Chân CS (Chip Select - chân PB0) được kéo xuống mức thấp ($0$) để kích hoạt chip nhận dữ liệu trên bus SPI [22, 36].

#### 2. Phân tích Toán học về Băng thông và Giải phóng CPU
Tốc độ xung nhịp của bus ngoại vi SPI1 trên STM32F103C8T6 được thiết lập ở tần số hoạt động tối đa của bộ chia:

$$f_{\text{SPI1}} = 10\text{ MHz}$$

Với tần số này, mỗi giây đường truyền có thể tải được $10 \times 10^6$ bits dữ liệu [51].
Khi thực hiện truyền một mảng phân vùng dữ liệu hình ảnh đầy đủ tương đương $128 \times 160$ Pixels màu 16-bit (tổng cộng $327,680$ bits) [33], thời gian truyền tải vật lý thực tế trên đường truyền được tính toán như sau [51]:

$$\text{Time}_{\text{Transfer}} = \frac{\text{Tổng số bits}}{f_{\text{SPI1}}} = \frac{128 \times 160 \times 16 \text{ bits}}{10 \times 10^6 \text{ Hz}} = \frac{327,680}{10,000,000} = 0.032768 \text{ s} \approx 32.77 \text{ ms}$$

Nếu sử dụng phương pháp truyền thống (CPU ghi vòng lặp và chờ cờ trạng thái), CPU của vi điều khiển sẽ hoàn toàn bị "giam cầm" trong suốt $32.77\text{ ms}$ này chỉ để làm nhiệm vụ chuyển dữ liệu [17]. Khi đó, hệ thống sẽ mất khả năng đáp ứng thời gian thực: tác vụ thu thập dữ liệu `TaskADC` (yêu cầu chu kỳ đáp ứng $5\text{ ms}$) và tác vụ xử lý tín hiệu `TaskDSP` sẽ bị trễ, gây hiện tượng mất mẫu và méo mó dạng sóng trầm trọng [54, 55, 57].

Nhờ cơ chế DMA, sau khi `TaskDisplay` phát lệnh ghi thanh ghi khởi động kênh DMA, toàn bộ quá trình truyền tải dữ liệu từ RAM ra SPI1 sẽ do phần cứng của DMA đảm nhận ngầm [17, 51]. Tác vụ `TaskDisplay` ngay lập tức gọi hàm giải phóng CPU và tự chuyển sang trạng thái bị chặn (`Blocked`) thông qua việc chờ Semaphore hoàn thành truyền hoặc sử dụng tính năng dừng hệ điều hành [51, 55]:

```c
// Đoạn mã minh họa cấu hình xuất đồ họa bằng SPI DMA trong TaskDisplay
void TaskDisplay_Routine(void const * argument) {
    for(;;) {
        // Chờ nhận gói tin chứa thông số đã xử lý từ Queue02
        osEvent event = osMessageGet(Queue02Handle, osWaitForever);
        if (event.status == osEventMessage) {
            Signal_Data_t *data = (Signal_Data_t*)event.value.p;

            // Đảm bảo truy cập an toàn cấu hình qua Mutex
            osMutexWait(gConfigMutexHandle, osWaitForever);
            float current_v_scale = gConfig.v_divMv;
            uint32_t current_time_scale = gConfig.time_DivUs;
            osMutexRelease(gConfigMutexHandle);

            // 1. Dựng nền và vẽ lưới tọa độ đứt nét vào Frame Buffer
            Clear_Frame_Buffer(PixelBuffer);
            Draw_Grid_To_Buffer(PixelBuffer);

            // 2. Ánh xạ dữ liệu và vẽ nối điểm sóng liên tục bằng Bresenham
            for (int i = 0; i < LCD_WIDTH - 1; i++) {
                int y0 = Map_ADC_To_Y(data->adc_samples[i], current_v_scale);
                int y1 = Map_ADC_To_Y(data->adc_samples[i+1], current_v_scale);
                Draw_Line_Bresenham(i, y0, i+1, y1, COLOR_WAVE, PixelBuffer);
            }

            // 3. Vẽ các ký tự thông số đo đạc tự động (Vpp, Vrms, Freq)
            Draw_String_To_Buffer(10, 10, data->vpp_str, PixelBuffer);
            Draw_String_To_Buffer(10, 22, data->freq_str, PixelBuffer);

            // 4. Bắt đầu truyền dữ liệu qua SPI DMA
            LCD_Set_Window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
            HAL_GPIO_WritePin(GPIOB, CS_TFT_Pin, GPIO_PIN_RESET); // Chọn màn hình
            HAL_GPIO_WritePin(GPIOB, DC_TFT_Pin, GPIO_PIN_SET);   // Chế độ ghi dữ liệu

            // Khởi động kênh DMA truyền dữ liệu ngầm, TaskDisplay đi vào trạng thái Blocked
            HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)PixelBuffer, LCD_WIDTH * LCD_HEIGHT * 2);
            
            // Chờ Semaphore báo hiệu truyền xong từ ISR (DMA Transfer Complete)
            osSemaphoreWait(displayDmaSemHandle, osWaitForever);
            
            HAL_GPIO_WritePin(GPIOB, CS_TFT_Pin, GPIO_PIN_SET);   // Giải phóng màn hình
            osMailFree(Queue02Handle, data); // Giải phóng vùng nhớ của gói tin
        }
    }
}

// Trình phục vụ ngắt DMA hoàn thành truyền (DMA1 Channel 3 Interrupt Service Routine)
void DMA1_Channel3_IRQHandler(void) {
    // Gọi hàm xử lý ngắt chuẩn của thư viện HAL
    HAL_DMA_IRQHandler(&hdma_spi1_tx);
}

// Hàm Callback được gọi tự động khi DMA truyền xong toàn bộ dữ liệu ra SPI
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // Phát Semaphore đánh thức TaskDisplay tiếp tục chu kỳ tiếp theo
        osSemaphoreRelease(displayDmaSemHandle);
    }
}
```

#### 3. Đánh giá Hiệu năng Hệ thống hiển thị
Nhờ cơ chế phối hợp tối ưu này, trong suốt khoảng thời gian $32.77\text{ ms}$ màn hình đang nhận dữ liệu, tải của CPU được tối ưu hóa triệt để. Tác vụ `TaskDisplay` chỉ tiêu tốn vỏn vẹn khoảng $8\text{ ms}$ của CPU để thực thi việc dựng hình trong RAM, sau đó nhường lại toàn bộ thời gian thực thi còn lại cho hệ thống [57].

Theo kết quả tính toán tải CPU thực tế tại Mục 2.3.2.2 [56]:
- Tải CPU dành cho `TaskDisplay` chỉ chiếm khoảng **$16.0\%$** [57].
- Hệ thống vẫn còn dư thừa đến **$36.8\%$** năng lực xử lý (CPU Idle time) [58].

Điều này đảm bảo cho máy hiện sóng hoạt động cực kỳ mượt mà ở tốc độ cập nhật khung hình cao, đồng thời triệt tiêu hoàn toàn hiện tượng bỏ sót mẫu hay sai số định thời ở tác vụ thu thập dữ liệu ADC [54, 58].

---
