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

## 📁 디렉토리 구조

```
ESP32Watch/
├── .vscode/                # VS Code 설정 파일
├── include/                 # 헤더 파일들 (현재 'README' 만 있음)
├── lib/                     # 외부 라이브러리들
│   ├── GifCode/
│   │   ├── GifCode.c        # GIF 관련 소스
│   │   ├── GifCode.h        # GIF 관련 헤더
│   ├── TFT_eSPI/            # TFT 디스플레이 드라이버
│   ├── cert/                # 인증서 관련
│   │   └── cert.h           # 인증서 파일
│   ├── lvgl/                # LVGL 라이브러리
│   └── lv_lib_gif/          # GIF 라이브러리 관련
├── src/                     # 소스 코드 (주요 프로그램)
│   └── main.cpp             # 메인 소스 코드
├── test/                    # 테스트 관련 파일
│   ├── .gitignore           # Git 무시 목록
│   ├── README.md            # 프로젝트 설명
│   ├── huge_app.csv         # 테스트 데이터
│   └── platformio.ini       # PlatformIO 설정
├── platformio.ini           # 프로젝트 설정
└── README.md                # 프로젝트 설명
```

---

## 👤 개발자

| 프로필 | 역할  | 담당 부분 | 기술 스택 |
|--------|-------|----------|-----------|
| ![강송구](https://github.com/user-attachments/assets/986e1819-2d0d-4715-97ce-590ea6495421) <br> [강송구](https://github.com/StrongThrow) | 팀장  | HW, SW 개발 | ESP32, LVGL, OTA, API |

---

## 📚 참고 자료

- [ESP32 OTA 공식 문서](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)  
- [SquareLine Studio 사용법](https://squareline.io/)  
- [OpenWeatherMap API](https://openweathermap.org/api)

---

> ✨ 본 프로젝트는 OTA, API 연동, UI 구현, 터치 처리, 네트워크 설정까지 포함된  
> **ESP32 기반 통합 임베디드 시스템**입니다.
