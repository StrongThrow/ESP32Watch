# ⌚ ESP32 Watch

**ESP32-2432S028 개발 보드를 활용한 IoT 스마트워치 프로젝트**  
WiFi를 통해 인터넷에 연결하여 **NTP 기반 시각 표시**,  
**OpenWeatherMap API 기반 날씨 정보 출력**,  
**GitHub 기반 OTA 펌웨어 업데이트**,  
**LVGL 기반 터치 UI 인터페이스** 기능을 포함합니다.

---

## 📸 동작 영상

<img src="https://github.com/user-attachments/assets/896ca317-4351-419e-9756-195e1659dfa0" width="500"/>

---

## 🔧 주요 기능

- ✅ NTP 기반 시간 동기화  
- ✅ OpenWeatherMap API 연동 날씨 표시  
- ✅ WiFi 연결 및 설정 UI  
- ✅ GitHub 연동 OTA 펌웨어 업데이트 기능  
- ✅ LVGL 기반 터치 UI (SquareLine Studio 설계)

---

## 📂 시스템 구성

```
[Touch Input / WiFi 설정]
        ↓
[ESP32 MCU]
  ├── UI 출력 (LVGL)
  ├── 시간 동기화 (NTP)
  ├── 날씨 API 연동
  └── OTA 업데이트 수행
```

---

## 🛠️ 개발 환경

| 항목     | 내용                                      |
|----------|-------------------------------------------|
| 보드     | ESP32-2432S028 (2.8" TFT, XPT2046 Touch)  |
| 개발툴   | VS Code + PlatformIO                      |
| UI 툴    | SquareLine Studio + LVGL                  |
| 언어     | C++ (Arduino Core for ESP32)              |
| OTA 서버 | GitHub Release `.bin` 파일                |

---

## 🧪 예시 화면

### 📶 WiFi 연결 설정

<img src="https://github.com/user-attachments/assets/81c55287-0b7f-45cd-8831-bb864df67b8c" width="400"/>

---

### 🛠 OTA 체크 및 정보 표시

<img src="https://github.com/user-attachments/assets/fcf5d898-4a45-4b6c-992c-2f8acafe5fc6" width="400"/>

---

## 📁 디렉토리 구조(수정중)

```
ESP32_Watch/
├── src/
│   ├── main.cpp
│   ├── wifi_manager.cpp/.h
│   ├── ota_manager.cpp/.h
│   ├── lvgl_gui.cpp/.h
│   └── weather_api.cpp/.h
├── include/
├── data/
├── platformio.ini
```

---

## 👤 개발자

| 이름 | 역할 | 기술 스택 |
|------|------|------------|
| [강송구](https://github.com/Throwball99) | HW + SW 개발 | ESP32, LVGL, OTA, API |

---

## 📚 참고 자료

- [ESP32 OTA 공식 문서](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)  
- [SquareLine Studio 사용법](https://squareline.io/)  
- [OpenWeatherMap API](https://openweathermap.org/api)

---

> ✨ 본 프로젝트는 OTA, API 연동, UI 구현, 터치 처리, 네트워크 설정까지 포함된  
> **ESP32 기반 통합 임베디드 시스템**입니다.
