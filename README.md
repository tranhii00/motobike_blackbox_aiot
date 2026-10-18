# 🏍️ AIoT Motorcycle Blackbox (EDR) - Web Telemetry & Accident Reconstruction Dashboard

> **Đồ án 2 / Khóa luận tốt nghiệp:** THIẾT KẾ HỘP ĐEN AIoT THÔNG MINH PHÂN TÍCH VÀ TÁI HIỆN HIỆN TRƯỜNG TAI NẠN XE MÁY (EVENT DATA RECORDER - EDR)

Hệ thống giám sát, phân tích động lực học và tái hiện hiện trường tai nạn xe máy thời gian thực dựa trên nền tảng vi điều khiển **ESP32-S3**, mạng viễn thông **4G LTE (A7682S)**, cảm biến quán tính 6 trục **MPU-6050**, định vị vệ tinh **GNSS NEO-M8N**, thời gian thực **RTC DS3231**, và giao thức truyền thông IoT **MQTT over WebSockets**.

---

## 🌟 Tính Năng Nổi Bật Trên Web Dashboard

1. **🗺️ Bản Đồ Số Định Vị & Giám Sát Hành Trình (Leaflet & Google Maps):**
   - Định vị phương tiện thời gian thực, tự động xoay đầu xe theo góc hướng di chuyển (GPS Heading).
   - Vệt đường di chuyển thông minh (Breadcrumbs Trail) đổi màu theo dải tốc độ: Xanh lá (<30 km/h) ➔ Vàng (30-60 km/h) ➔ Đỏ (>60 km/h).
   - Vòng tròn bán kính sai số định vị (HDOP Accuracy Circle) thể hiện độ tin cậy vệ tinh.
   - Tự động thả ghim cờ đỏ nhấp nháy tại tọa độ va chạm khi xảy ra tai nạn.

2. **🏎️ Cụm Đồng Hồ Cơ Năng & Tư Thế Xe (Telemetry):**
   - Mặt đồng hồ đo vận tốc vòng cung thể thao (Speed Arc Gauge) kèm nhãn cảnh báo tốc độ.
   - Con quay mô phỏng góc nghiêng xe máy thời gian thực theo góc **Roll / Pitch** của cảm biến MPU-6050.
   - Thanh tiến trình gia tốc 3 trục ($A_x, A_y, A_z$) và chỉ số tổng lực va đập $G_{total}$ cảnh báo khi vượt ngưỡng an toàn ($> 2.5G$).

3. **🎬 Phòng Phân Tích & Tái Hiện Hiện Trường Tai Nạn 30s (Accident Reconstruction Studio):**
   - Tuân thủ tiêu chuẩn hộp đen xe cơ giới **IEEE 1616 / 49 CFR Part 563**.
   - Khung thời gian đóng băng 30 giây: **10s Trước va chạm ➔ T0 (Khoảnh khắc va chạm) ➔ 15s Sau va chạm**.
   - Thanh trượt Timeline Scrubber cho phép kéo chuột tua lại hiện trường: Xe trên bản đồ, góc nghiêng xe và đồ thị sóng nhảy theo đúng từng mili-giây!
   - Nút phát lại (Play / Pause) tự động chiếu lại như một thước phim tai nạn kèm còi hú cảnh báo.

4. **📄 Biên Bản Giám Định Hiện Trường Kỹ Thuật Số (Digital Forensic Report):**
   - Bảo đảm tính toàn vẹn chứng cứ số với mã băm pháp lý **SHA-256 Checksum**.
   - Trích xuất bảng thông số pháp y: Vận tốc va chạm ($V_{pre}$ / $V_{post}$), Lực va đập cực đại ($Peak\ G$), Góc lật xe, Tọa độ GPS có nút mở Google Maps ngoài, và Mốc giờ chuẩn RTC phần cứng.
   - Hỗ trợ in biên bản giám định hoặc xuất file **CSV**.

5. **🌙 Chuyển Đổi Giao Diện Sáng / Tối (Day / Night Modes):**
   - Tối ưu quan sát ngoài trời nắng gắt hoặc ban đêm chống lóa mắt khi lái xe.
   - Tự động ghi nhớ chế độ bằng `localStorage`.

6. **☁️ Kết Nối MQTT Cloud Linh Hoạt:**
   - Hỗ trợ cả **EMQX Public Broker** miễn phí và máy chủ riêng **EMQX Cloud Serverless** với chứng thực tài khoản an toàn.

---

## 🚀 Hướng Dẫn Chạy & Trải Nghiệm Trực Tiếp

### Cách 1: Xem trực tiếp Online trên GitHub Pages (Không cần cài đặt)
Truy cập ngay đường link GitHub Pages của dự án:
👉 **[https://tranhii00.github.io/motobike_blackbox_aiot/](https://tranhii00.github.io/motobike_blackbox_aiot/)**

### Cách 2: Chạy Offline trên máy tính
1. Tải repository này về máy tính (hoặc tải file `index.html`).
2. Nhấp đúp chuột vào file `index.html` để mở bằng trình duyệt bất kỳ (Google Chrome, Microsoft Edge, Firefox, Cốc Cốc, Safari...).
3. Bấm nút **`🚨 Test Va Chạm`** để xem hệ thống mô phỏng một vụ tai nạn xe máy trực quan!

---

## 📡 Định Dạng Gói Tin Viễn Trắc (Telemetry JSON Payload)

ESP32-S3 gửi gói tin định kỳ lên MQTT Broker qua topic `edr_blackbox/telemetry`:

```json
{
  "device": "EDR_ESP32S3_01",
  "lat": 10.776889,
  "lng": 106.700806,
  "speed": 42.5,
  "ax": 1.15,
  "ay": 0.35,
  "az": 9.81,
  "roll": 12.3,
  "pitch": -2.1,
  "status": "NORMAL",
  "sats": 11,
  "sd": "READY",
  "pwr": "ẮC QUY 12V (OK)",
  "csq": 26
}
```
*(Nếu xảy ra tai nạn, ESP32 chỉ cần đổi `"status": "CRASH_DETECTED"` để kích hoạt báo động khẩn cấp trên Web).*

---

## 👨‍💻 Tác giả & Thông tin Đề tài
* **Sinh viên thực hiện:** Trần Hữu Ý (`tranhii00`)
* **Ngành:** Kỹ thuật Máy tính / Điện tử - Viễn thông
* **Học phần:** Đồ án 2 (Định hướng Khóa luận Tốt nghiệp)
* **Khoa:** Điện - Điện tử
