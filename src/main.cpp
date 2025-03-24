#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <TFT_Touch.h>
#include "SPIFFS.h"
//#include <GifCode.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
//#include "lv_gif.h"
#include <esp_log.h>
#include <time.h>
#include <EEPROM.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"

//OTA 설정
// GitHub API URL (최신 릴리스 정보 가져오기)
#define URL_fw_Bin "https://raw.githubusercontent.com/StrongThrow/ESP32Watch/main/firmware.bin"
#define URL_fw_Version "https://raw.githubusercontent.com/StrongThrow/ESP32Watch/main/bin_version.txt"
String FirmwareVer = {
  "1.0.0"
};
String Version = "";

//와이파이 리스트
static lv_obj_t * wifi_list;

//API 설정
const String ip_api_url = "http://ip-api.com/json";  // IP 기반 위치 API
const String weather_api_base_url = "https://api.openweathermap.org/data/2.5/weather";
const String weather_api_key = "7494719d37679814e89c151fcbcc4054";  // OpenWeather API 키
String weather_before= "";

// NTP 서버 설정
const char* ntpServer = "pool.ntp.org";        // NTP 서버 주소
const long  gmtOffset_sec = 32400;             // 한국 시간대 설정 (UTC+9, 9시간을 초로 변환 = 9*60*60)
const int   daylightOffset_sec = 0;            // 일광절약시간 사용하지 않음 (한국은 미사용)

// 시간 관련 변수
struct tm timeinfo;                            // 시간 정보를 저장하는 구조체
unsigned long lastNTPSync = 0;                 // 마지막 NTP 동기화 시점 저장
unsigned long lastTimeUpdate = 0;              // 마지막 시간 업데이트 시점 저장
const unsigned long NTP_SYNC_INTERVAL = 3600000 / 2;  // NTP 동기화 주기 (1시간 = 3600000밀리초)
const unsigned long TIME_UPDATE_INTERVAL = 1000;   // 화면 업데이트 주기 (1초 = 1000밀리초)

void syncNTP();
void update_weather_data();

//애니매이션 관련 변수
uint8_t animation_cnt = 0; //애니매이션 카운트 변수

// 시간, 날짜 저장 변수
int currentYear = 0;     // 현재 년도
uint8_t currentMonth = 0;    // 현재 월 (1-12)
uint8_t currentDay = 0;      // 현재 일
uint8_t currentHour = 0;     // 현재 시간 (0-23)
uint8_t currentMinute = 0;   // 현재 분 (0-59)
uint8_t currentSecond = 0;   // 현재 초 (0-59)
uint8_t currentWeekday = 0;  // 현재 요일 (0=일요일, 1=월요일, ..., 6=토요일)
bool colonFlag = false;        // 콜론 표시 플래그

//계수기 디바운싱 방지 변수
unsigned long lastPressTime1 = 0; // 버튼 1의 마지막 클릭 시간
unsigned long lastPressTime2 = 0; // 버튼 2의 마지막 클릭 시간
const uint8_t debounceDelay = 200; // 디바운싱 딜레이 (200ms)

// 날씨 관련 변수
String city_name = "";  // 현재 위치의 도시 이름
String weather_description = "";  // 날씨 설명
String weather_icon_id = "";  // 날씨 아이콘 ID
float feels_like; // 체감온도
float temp_min; // 최저 온도
float temp_max; // 최고 온도
float wind_speed; // 풍속
uint8_t info_cnt = 0; // 날씨 정보 출력 카운트 

float temperature;
int humidity;
unsigned long last_update_time = 0;

// 요일 문자열 배열 
const char* weekdays[] = {"일요일!", "월요일!", "화요일!", "수요일!", "목요일!", "금요일!", "토요일!"};

/*Don't forget to set Sketchbook location in File/Preferencesto the path of your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */


// 시스템 관련 변수
static bool tabview_visible = true; //탭뷰바 보이기/숨기기 플래그
uint8_t num1, num2 = 0; //카운트 변수

//와이파이 SSID, PW 변수
char WiFissid[20];
char WiFipw[20];

//EEPROM 설정
#define EEPROM_SIZE 40
#define SSID_ADDR 0
#define PW_ADDR 20


// OTA 업데이트 함수
void firmwareUpdate(void) {
    WiFiClientSecure client;                  // HTTPS 클라이언트 생성
    client.setCACert(rootCACertificate);      // 인증서 설정 (rootCACertificate는 cert.h에서 가져옴)

    // HTTP 업데이트 설정
    t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin); // 펌웨어 다운로드 및 업데이트 수행

    // 업데이트 결과 처리
    switch (ret) {
        case HTTP_UPDATE_FAILED:              // 업데이트 실패
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:          // 업데이트 없음
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
        case HTTP_UPDATE_OK:                  // 업데이트 성공
            Serial.println("HTTP_UPDATE_OK");
            break;
    }
}

// 펌웨어 버전 확인 함수
int FirmwareVersionCheck(void) {
    String payload;                          // 서버 응답 저장
    int httpCode;                            // HTTP 응답 코드 저장
    String fwurl = "";                       // 버전 파일 URL 설정
    fwurl += URL_fw_Version;
    fwurl += "?";                            // 캐시 방지를 위한 랜덤 매개변수 추가
    fwurl += String(rand());
    Serial.println(fwurl);

    // HTTPS 클라이언트 생성
    WiFiClientSecure* client = new WiFiClientSecure;
    client->setInsecure(); // 인증서 검증 무시
    if (client) {
        client->setCACert(rootCACertificate); // 인증서 설정

        // HTTP 요청 처리
        HTTPClient https;
        if (https.begin(*client, fwurl)) {  // HTTPS 연결 시작
            Serial.print("[HTTPS] GET...\n");
            delay(100);
            httpCode = https.GET();         // HTTP GET 요청
            delay(100);

            if (httpCode == HTTP_CODE_OK) { // HTTP 응답 성공
                payload = https.getString(); // 응답 데이터 저장
            } else {
                Serial.print("Error in downloading version file:");
                Serial.println(httpCode);  // 오류 코드 출력
            }
            https.end();
        }
        delete client; // 클라이언트 해제
    }

    // 펌웨어 버전 비교
    if (httpCode == HTTP_CODE_OK) {
        payload.trim();
        if (payload.equals(FirmwareVer)) { // 버전 동일 여부 확인
            Serial.printf("\nDevice already on latest firmware version: %s\n", FirmwareVer.c_str());
            return 0; // 최신 상태
        } else {
            Serial.println(payload);       // 새로운 버전 출력
            Version = payload;
            Serial.println("New firmware detected");
            return 1; // 업데이트 필요
        }
    }
    return 0; // 실패 처리
}


//와이파이 상태 확인 함수 
void checkWiFi(){
    if(WiFi.status() == WL_CONNECTED){
        // Wi-Fi가 연결되어 있으면 "연결됨" 아이콘 출력
        lv_label_set_text(ui_Label4, LV_SYMBOL_WIFI); // LVGL 내장 Wi-Fi 아이콘
    } else {
         // Wi-Fi가 끊겼다면 "연결 끊김" 아이콘 출력
        lv_label_set_text(ui_Label4, LV_SYMBOL_WARNING); // LVGL 내장 경고 아이콘
    }
}

static void wifiListClick(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED){
        //LV_LOG_USER("Clicked: %s", lv_list_get_btn_text(obj));

        //클릭된 버튼의 텍스트를 가져옴
        lv_label_set_text(ui_Label33, lv_list_get_btn_text(wifi_list, obj));

        //키보드가 있는 스크린으로 변경
        lv_scr_load_anim(ui_Screen8, LV_SCR_LOAD_ANIM_FADE_OUT, 500, 0, false);
        
    }
}

//와이파이 스캔하고 리스트로 만드는 함수
void doScan(){

    //기존의 와이파이 연결 해제
    WiFi.disconnect();

    //와이파이 스테이션 모드
    WiFi.mode(WIFI_STA);

    // 검색한 AP의 갯수
    int n = WiFi.scanNetworks();
    Serial.print("스캔한 와이파이 갯수 : "); Serial.println(n);

    //기존의 리스트 내역 삭제
    lv_obj_clean(wifi_list);
    //제목 추가
    lv_list_add_text(wifi_list, "WiFi Lists");

    //AP가 검출 되지 않았을 때
    if(n == 0){
        lv_obj_t * noWiFi;
        noWiFi = lv_list_add_text(wifi_list, "No WiFi APs found");
    //AP가 하나 이상 검출되었을 때
    } else{
        
        for(int i = 0; i < n; i++){
            //AP의 이름을 버튼으로 생성
            lv_obj_t * wifiBtn;
            wifiBtn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, WiFi.SSID(i).c_str());
            //리스트에 SSID 추가, 각 항목을 선택하면 wifiListClick 함수 실행
            lv_obj_add_event_cb(wifiBtn, wifiListClick, LV_EVENT_CLICKED, NULL);
        }
    }
    //와이파이 검색결과를 메모리에서 삭제
    WiFi.scanDelete();
}


// EEPROM에 데이터 저장하는 함수
void saveCredentialsToEEPROM(const char* ssid, const char* password) {
    EEPROM.begin(EEPROM_SIZE); // EEPROM 사용 시작
    for (int i = 0; i < 19; i++) {
        EEPROM.write(SSID_ADDR + i, i < strlen(ssid) ? ssid[i] : '\0');
        EEPROM.write(PW_ADDR + i, i < strlen(password) ? password[i] : '\0');
    }
    EEPROM.commit(); // 변경 내용 저장
    EEPROM.end(); // EEPROM 사용 종료
}

// EEPROM에서 SSID와 비밀번호 읽기
void loadCredentialsFromEEPROM(char* ssid, char* password) {
    for (int i = 0; i < 19; i++) {
        ssid[i] = EEPROM.read(SSID_ADDR + i);
        password[i] = EEPROM.read(PW_ADDR + i);
    }
    ssid[19] = '\0';
    password[19] = '\0';
}

// 버튼을 눌렀을때의 함수===================================================================================================

//계수기 관련 함수===================================================================================================
//디바운싱 해결 해야함

//1번 계수기
void ui_event_Button15(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        unsigned long currentTime = millis();
        if (currentTime - lastPressTime1 > debounceDelay) { // 디바운싱 확인
            lastPressTime1 = currentTime;
            num1++;
            lv_label_set_text_fmt(ui_Label23, "%d", num1);
        }
    }
}

void ui_event_Button17(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        unsigned long currentTime = millis();
        if (currentTime - lastPressTime1 > debounceDelay) {
            lastPressTime1 = currentTime;
            num1 = 0;
            lv_label_set_text_fmt(ui_Label23, "%d", num1);
        }
    }
}

// 버튼 16과 18도 동일한 방식으로 디바운싱 처리
void ui_event_Button16(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        unsigned long currentTime = millis();
        if (currentTime - lastPressTime2 > debounceDelay) {
            lastPressTime2 = currentTime;
            num2++;
            lv_label_set_text_fmt(ui_Label56, "%d", num2);
        }
    }
}

void ui_event_Button18(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        unsigned long currentTime = millis();
        if (currentTime - lastPressTime2 > debounceDelay) {
            lastPressTime2 = currentTime;
            num2 = 0;
            lv_label_set_text_fmt(ui_Label56, "%d", num2);
        }
    }
}
//===================================================================================================================

//OTA 버전 확인 함수
void ui_event_Button13(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        //업데이트 확인
        int updateAvailable = FirmwareVersionCheck();
        //업데이트가 필요하면
        if(updateAvailable == 1){
            lv_obj_clear_flag(ui_Panel21, LV_OBJ_FLAG_HIDDEN);  // 버전 업 버튼 보임
            String VersionUp = FirmwareVer + " -> " + Version;
            lv_label_set_text(ui_Label50, VersionUp.c_str());
            lv_label_set_text(ui_Label49, FirmwareVer.c_str());
        }
        //업데이트가 필요하지 않으면
        else{
            lv_label_set_text(ui_Label48, "최신 버전입니다.");
            lv_label_set_text(ui_Label49, Version.c_str());
        }
    }
}
void ui_event_Button14(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        lv_obj_clear_flag(ui_Panel23, LV_OBJ_FLAG_HIDDEN);  // 업데이트 창 활성화
        lv_obj_add_flag(ui_Button7, LV_OBJ_FLAG_HIDDEN);  // 뒤로가기 버튼 비활성화
        firmwareUpdate(); // 업데이트 실행
    }
}

void ui_event_Button7(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
        lv_obj_add_flag(ui_Panel21, LV_OBJ_FLAG_HIDDEN);  // 버전 업 버튼 숨김
        lv_label_set_text(ui_Label49, "Click!");
        lv_label_set_text(ui_Label48, "업데이트 확인!");
    }
}

void ui_event_Button6(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen4_screen_init);
    }
}

void ui_event_Button3(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        
        //스크린 전환
        lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_FADE_OUT, 500, 0, false);
        doScan();
    }
}

void ui_event_Button4(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {

        //AP 정보 가져오기
        String ssid = WiFi.SSID();
        String ipAddress = WiFi.localIP().toString();
        String gateway = WiFi.gatewayIP().toString();
        int signalStrength = WiFi.RSSI();

        // Label 업데이트
        lv_label_set_text(ui_Label38, ssid.c_str()); // AP 이름
        lv_label_set_text_fmt(ui_Label40, "%s", ipAddress.c_str()); // IP 주소
        lv_label_set_text_fmt(ui_Label42, "%s", gateway.c_str()); // 게이트웨이
        lv_label_set_text_fmt(ui_Label44, "%d dBm", signalStrength); // 신호 강도

        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen3_screen_init);
    }
}

//EEPROM에 와이파이 SSID, PW 저장
void ui_event_Button5(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        saveCredentialsToEEPROM(WiFissid, WiFipw);
        delay(1000);
        ESP.restart();
    }
}

void ui_event_Button8(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen7, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen7_screen_init);
    }
}

void ui_event_Button9(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
    }
}
void ui_event_Button11(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
    }
}

void ui_event_Button12(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
    }
}

//와이파이 비밀번호 입력 후 연결버튼을 눌렀을 때 호출되는 함수
void ui_event_Button10(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        

        //와이파이 SSID, 비밀번호 저장

        strncpy(WiFissid, lv_label_get_text(ui_Label33), sizeof(WiFissid) - 1);
        WiFissid[sizeof(WiFissid) - 1] = '\0';

        strncpy(WiFipw, lv_textarea_get_text(ui_TextArea1), sizeof(WiFipw) - 1);
        WiFipw[sizeof(WiFipw) - 1] = '\0';

        //와이파이 저장 버튼 활성화
        lv_obj_add_flag(ui_Button11, LV_OBJ_FLAG_HIDDEN);  // 뒤로가기 버튼 숨김
        lv_obj_clear_flag(ui_Button5, LV_OBJ_FLAG_HIDDEN);  // 저장 버튼 보임

        //와이파이 연결
        Serial.println(WiFissid);
        Serial.println(WiFipw);
        WiFi.begin(WiFissid, WiFipw);
        Serial.printf("Connecting to WiFi: %s\n", WiFissid);

        int retryCount = 0;
        while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
            delay(1000);
            Serial.print(".");
            retryCount++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi Connected!");
            syncNTP();
            update_weather_data();

        } else {
            Serial.println("\nWiFi Connection Failed.");
        }

        //AP 정보 가져오기
        String ssid = WiFi.SSID();
        String ipAddress = WiFi.localIP().toString();
        String gateway = WiFi.gatewayIP().toString();
        int signalStrength = WiFi.RSSI();

        // Label 업데이트
        lv_label_set_text(ui_Label38, ssid.c_str()); // AP 이름
        lv_label_set_text_fmt(ui_Label40, "%s", ipAddress.c_str()); // IP 주소
        lv_label_set_text_fmt(ui_Label42, "%s", gateway.c_str()); // 게이트웨이
        lv_label_set_text_fmt(ui_Label44, "%d dBm", signalStrength); // 신호 강도

        //화면 전환
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);

        // const char * WiFissid = lv_label_get_text(ui_Label33);
        // const char * WiFipw = lv_textarea_get_text(ui_TextArea1);

        
        //delay(5);

    }
}


// =======================================================================================================================

// IP 기반 위치 확인
String fetch_city_name() {
    HTTPClient http;
    http.begin(ip_api_url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error) {
            Serial.println("Failed to parse IP location JSON!");
            return "";
        }

        city_name = doc["regionName"].as<String>();
        Serial.println("Detected city: " + city_name);


        return city_name;
    } else {
        Serial.printf("Failed to fetch IP location, error: %s\n", http.errorToString(httpCode).c_str());
        return "";
    }
}

// OpenWeather API 데이터 가져오기
String fetch_weather_data() {

    String city = fetch_city_name();
    char city_name[city.length() + 1];
    lv_label_set_text(ui_Label2, city_name);

    String api_url = weather_api_base_url + "?q=" + city + "&APPID=" + weather_api_key + "&units=metric";

    HTTPClient http;
    http.begin(api_url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        return http.getString();  // JSON 데이터 반환
    } else {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        return "";
    }
}

// OpenWeather API 데이터 파싱
bool parse_weather_data(const String& json_data) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, json_data);

    if (error) {
        Serial.println("Failed to parse JSON!");
        return false;
    }

    // 데이터 파싱
    weather_description = doc["weather"][0]["description"].as<String>();
    weather_icon_id = doc["weather"][0]["icon"].as<String>();
    temperature = doc["main"]["temp"];
    humidity = doc["main"]["humidity"];
    feels_like = doc["main"]["feels_like"]; // 체감온도 추출
    temp_min = doc["main"]["temp_min"]; // 최저 온도 추출
    temp_max = doc["main"]["temp_max"]; // 최고 온도 추출
    wind_speed = doc["wind"]["speed"]; // 풍속 추출

    return true;
}


void update_weather_icon() {
    // if (weather_icon_id == weather_before){
    //     ;;
    // } else { 
        
        // 아이콘 ID에 따라 이미지 설정
        Serial.println(weather_icon_id);
        if (weather_icon_id == "01d") lv_img_set_src(ui_Image21, &ui_img_i01d_t_png);
        else if (weather_icon_id == "01n") lv_img_set_src(ui_Image21, &ui_img_i01n_t_png);
        else if (weather_icon_id == "02d") lv_img_set_src(ui_Image21, &ui_img_i02d_t_png);
        else if (weather_icon_id == "02n") lv_img_set_src(ui_Image21, &ui_img_i02n_t_png);
        else if (weather_icon_id == "03d") lv_img_set_src(ui_Image21, &ui_img_i03d_t_png);
        else if (weather_icon_id == "03n") lv_img_set_src(ui_Image21, &ui_img_i03n_t_png);
        else if (weather_icon_id == "04d") lv_img_set_src(ui_Image21, &ui_img_i04d_t_png);
        else if (weather_icon_id == "04n") lv_img_set_src(ui_Image21, &ui_img_i04n_t_png);
        else if (weather_icon_id == "09d") lv_img_set_src(ui_Image21, &ui_img_i09d_t_png);
        else if (weather_icon_id == "09n") lv_img_set_src(ui_Image21, &ui_img_i09n_t_png);
        else if (weather_icon_id == "10d") lv_img_set_src(ui_Image21, &ui_img_i10d_t_png);
        else if (weather_icon_id == "10n") lv_img_set_src(ui_Image21, &ui_img_i10n_t_png);
        else if (weather_icon_id == "11d") lv_img_set_src(ui_Image21, &ui_img_i11d_t_png);
        else if (weather_icon_id == "11n") lv_img_set_src(ui_Image21, &ui_img_i11n_t_png);
        else if (weather_icon_id == "13d") lv_img_set_src(ui_Image21, &ui_img_i13d_t_png);
        else if (weather_icon_id == "13n") lv_img_set_src(ui_Image21, &ui_img_i13n_t_png);
        else if (weather_icon_id == "50d") lv_img_set_src(ui_Image21, &ui_img_i50d_t_png);
        else if (weather_icon_id == "50n") lv_img_set_src(ui_Image21, &ui_img_i50n_t_png);


        /*
        LV_IMG_DECLARE(ui_img_i03n_t_png);    // assets\i03n_t.png
        LV_IMG_DECLARE(ui_img_i01d_t_png);    // assets\i01d_t.png
        LV_IMG_DECLARE(ui_img_i01n_t_png);    // assets\i01n_t.png
        LV_IMG_DECLARE(ui_img_i02d_t_png);    // assets\i02d_t.png
        LV_IMG_DECLARE(ui_img_i02n_t_png);    // assets\i02n_t.png
        LV_IMG_DECLARE(ui_img_i03d_t_png);    // assets\i03d_t.png
        LV_IMG_DECLARE(ui_img_i04d_t_png);    // assets\i04d_t.png
        LV_IMG_DECLARE(ui_img_i04n_t_png);    // assets\i04n_t.png
        LV_IMG_DECLARE(ui_img_i09d_t_png);    // assets\i09d_t.png
        LV_IMG_DECLARE(ui_img_i09n_t_png);    // assets\i09n_t.png
        LV_IMG_DECLARE(ui_img_i10d_t_png);    // assets\i10d_t.png
        LV_IMG_DECLARE(ui_img_i10n_t_png);    // assets\i10n_t.png
        LV_IMG_DECLARE(ui_img_i11d_t_png);    // assets\i11d_t.png
        LV_IMG_DECLARE(ui_img_i11n_t_png);    // assets\i11n_t.png
        LV_IMG_DECLARE(ui_img_i13d_t_png);    // assets\i13d_t.png
        LV_IMG_DECLARE(ui_img_i13n_t_png);    // assets\i13n_t.png
        LV_IMG_DECLARE(ui_img_i50d_t_png);    // assets\i50d_t.png
        LV_IMG_DECLARE(ui_img_i50n_t_png);    // assets\i50n_t.png

        */
        weather_before = weather_icon_id;
    //}
    
}


// 날씨 데이터 업데이트
void update_weather_data() {
    if (city_name == "") {
        city_name = fetch_city_name();
        if (city_name == "") return;  // 도시를 가져오지 못하면 종료
    }

    String weather_data = fetch_weather_data();
    if (weather_data != "") {
        if (parse_weather_data(weather_data)) {
            update_weather_icon();
            int temperature_int = (int)temperature;
            lv_slider_set_value(ui_Bar1, temperature_int + 20, LV_ANIM_ON);
            lv_label_set_text_fmt(ui_Label12, "%d°C", temperature_int);

            
            // int feels_like_int = (int)feels_like;
            // lv_label_set_text_fmt(ui_Label16, "Feels like %d°C", feels_like_int);

            lv_slider_set_value(ui_Bar2, humidity, LV_ANIM_ON);
            lv_label_set_text_fmt(ui_Label13, "%d %%", humidity);
            lv_label_set_text_fmt(ui_Label5, "%s", weather_description.c_str());
            lv_label_set_text_fmt(ui_Label2, "%s", city_name.c_str());

        }
    }
}

void update_info_label(){
    info_cnt++;
    char buffer[30];
    if(info_cnt < 10){
        snprintf(buffer, sizeof(buffer), "Feels like : %.1f°C", feels_like);
        lv_label_set_text(ui_Label16, buffer);
    }else if(info_cnt >= 10 && info_cnt < 20){
        snprintf(buffer, sizeof(buffer), "Max Temp : %.1f°C", temp_max);
        lv_label_set_text(ui_Label16, buffer);
    }else if (info_cnt >= 20 && info_cnt < 30){
        snprintf(buffer, sizeof(buffer), "Min Temp : %.1f°C", temp_min);
        lv_label_set_text(ui_Label16, buffer);
    }else if (info_cnt >= 30 && info_cnt < 40){
        snprintf(buffer, sizeof(buffer), "Wind Speed : %.2fkph", wind_speed);
        lv_label_set_text(ui_Label16, buffer);
    }else if (info_cnt >= 40){
        info_cnt = 0;
    }
}


//NTP 시간,날짜 동기화
void syncNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // NTP 서버 설정
    if (getLocalTime(&timeinfo)) {  // NTP 서버에서 시간 받아오기 시도
        // 받아온 시간 정보를 각 변수에 저장
        currentYear = timeinfo.tm_year + 1900;  // 1900년 이후의 연도로 변환
        currentMonth = timeinfo.tm_mon + 1;     // 0-11월을 1-12월로 변환
        currentDay = timeinfo.tm_mday;          // 현재 일
        currentHour = timeinfo.tm_hour;         // 현재 시간
        currentMinute = timeinfo.tm_min;        // 현재 분
        currentSecond = timeinfo.tm_sec;        // 현재 초
        currentWeekday = timeinfo.tm_wday;      // 현재 요일
        Serial.println("NTP 동기화 완료");

         // 캘린더 위젯 날짜 업데이트

        lv_calendar_date_t today = {
            .year = currentYear,
            .month = currentMonth,
            .day = currentDay
        };

        // 오늘 날짜를 설정
        lv_calendar_set_today_date(ui_Calendar1, currentYear, currentMonth, currentDay);
        // 캘린더가 표시할 날짜를 업데이트
        lv_calendar_set_showed_date(ui_Calendar1, today.year, today.month);

        lastNTPSync = millis();                 // 동기화 시점 기록
    } else {
        Serial.println("NTP 동기화 실패");
    }
}

//윤년 계산 함수
bool isLeapYear(int year) {
    // 윤년 규칙: 4로 나누어 떨어지고 100으로 나누어 떨어지지 않거나, 400으로 나누어 떨어지는 해
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// ===== 각 월의 일수를 반환하는 함수 =====
int getDaysInMonth(int year, int month) {
    // 각 월의 일수 배열 (1월부터 12월까지)
    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // 2월이고 윤년이면 29일 반환
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return daysInMonth[month - 1];
}

//시간 업데이트
void updateTime() {
    currentSecond++;  // 1초 증가
    
    // 초가 60이 되면 분 증가
    if (currentSecond >= 60) {
        currentSecond = 0;
        currentMinute++;
        
        // 분이 60이 되면 시간 증가
        if (currentMinute >= 60) {
            currentMinute = 0;
            currentHour++;
            
            // 시간이 24가 되면 날짜 변경
            if (currentHour >= 24) {
                currentHour = 0;
                currentDay++;
                currentWeekday = (currentWeekday + 1) % 7;  // 요일 변경
                
                // 현재 월의 마지막 날인지 확인
                int daysInCurrentMonth = getDaysInMonth(currentYear, currentMonth);
                if (currentDay > daysInCurrentMonth) {
                    currentDay = 1;
                    currentMonth++;
                    
                    // 월이 12를 넘어가면 연도 변경
                    if (currentMonth > 12) {
                        currentMonth = 1;
                        currentYear++;
                    }
                }
            }
        }
    }
}

float calculateAngle(int unit, int maxUnit) {
    return (360.0 * unit / maxUnit); // 단위를 각도로 변환
}

float getHourAngle(int hour, int minute) {
    return calculateAngle(hour % 12 * 60 + minute, 12 * 60); // 시 단위
}

float getMinuteAngle(int minute, int second) {
    return calculateAngle(minute * 60 + second, 60 * 60); // 분 단위
}

float getSecondAngle(int second) {
    return calculateAngle(second, 60); // 초 단위
}

void updateClock(){
    float hourAngle = getHourAngle(currentHour, currentMinute) - 180;
    float minuteAngle = getMinuteAngle(currentMinute, currentSecond) - 180;
    float secondAngle = getSecondAngle(currentSecond) - 180;

    // Panel 객체 회전
    lv_obj_set_style_transform_angle(ui_Panel2, hourAngle * 10, LV_PART_MAIN);
    lv_obj_set_style_transform_angle(ui_Panel1, minuteAngle * 10, LV_PART_MAIN);
    lv_obj_set_style_transform_angle(ui_Panel5, secondAngle * 10, LV_PART_MAIN);
}


//시간 디스플레이 업데이트 함수
void updateDisplay() {
    // 시간 문자열 생성 및 표시 (HH:MM:SS 형식)
    char Hour[3];
    char Min[3];
    char Sec[3];
    
    snprintf(Hour, sizeof(Hour), "%02d", currentHour);
    lv_label_set_text(ui_Label6, Hour);


    snprintf(Min, sizeof(Min), "%02d", currentMinute);
    lv_label_set_text(ui_Label7, Min);

    snprintf(Sec, sizeof(Sec), "%02d", currentSecond);
    lv_label_set_text(ui_Label9, Sec);

    char timeString[6];  // HH:MM\0 를 위한 공간
    snprintf(timeString, sizeof(timeString), "%02d:%02d", 
             currentHour, currentMinute);
    lv_label_set_text(ui_Label3, timeString);

    
    // 날짜 표시 (YYYY/MM/DD 형식)
    char dateString[11];  // YYYY/MM/DD\0 를 위한 공간
    snprintf(dateString, sizeof(dateString), "%d/%02d/%02d", 
             currentYear, currentMonth, currentDay);
    lv_label_set_text(ui_Label10, dateString);

    char weekdayString[10];  // 요일 문자열을 위한 공간
    snprintf(weekdayString, sizeof(weekdayString), "%s", weekdays[currentWeekday]);
    lv_label_set_text(ui_Label11, weekdayString);

}



// void bar_animation() {
//     if(animation_cnt == 1){
//         lv_obj_clear_flag(ui_Panel5, LV_OBJ_FLAG_HIDDEN);  // 숨김
//         lv_obj_add_flag(ui_Panel1, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel6, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel7, LV_OBJ_FLAG_HIDDEN);
        
//     } else if(animation_cnt == 2){
//         lv_obj_clear_flag(ui_Panel1, LV_OBJ_FLAG_HIDDEN);  // 숨김
//         //lv_obj_clear_flag(ui_Panel5, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel6, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel7, LV_OBJ_FLAG_HIDDEN);
 
//     } else if (animation_cnt == 3){
//         lv_obj_clear_flag(ui_Panel6, LV_OBJ_FLAG_HIDDEN);  // 숨김
//         //lv_obj_clear_flag(ui_Panel1, LV_OBJ_FLAG_HIDDEN);
//         //lv_obj_clear_flag(ui_Panel5, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel7, LV_OBJ_FLAG_HIDDEN);

//     } else if (animation_cnt == 4){
//         lv_obj_clear_flag(ui_Panel7, LV_OBJ_FLAG_HIDDEN);  // 숨김
//         //lv_obj_clear_flag(ui_Panel1, LV_OBJ_FLAG_HIDDEN);
//         //lv_obj_clear_flag(ui_Panel6, LV_OBJ_FLAG_HIDDEN);
//         //lv_obj_clear_flag(ui_Panel5, LV_OBJ_FLAG_HIDDEN);

//     } else if (animation_cnt == 5){
//         lv_obj_add_flag(ui_Panel5, LV_OBJ_FLAG_HIDDEN);  // 숨김
//         lv_obj_add_flag(ui_Panel1, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel6, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(ui_Panel7, LV_OBJ_FLAG_HIDDEN);
//         animation_cnt = 0;
//     }
//     animation_cnt++;
// }

//탭뷰바 토글하는 함수
void bar_toggle1(lv_event_t *e) {

    //Tab buttons 영역 가져오기
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(ui_TabView1);  // ui_Tabview는 Squareline Studio에서 생성한 Tabview 객체

    if (tabview_visible) {
        // Tabview 숨기기
        lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);  // 숨김
        lv_obj_clear_flag(ui_Button2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Button1, LV_OBJ_FLAG_HIDDEN);
        tabview_visible = false;
    } else {
        // Tabview 보이기
        lv_obj_clear_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);  // 표시
        lv_obj_add_flag(ui_Button2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_Button1, LV_OBJ_FLAG_HIDDEN);
        tabview_visible = true;
    }
}

void ui_event_Button1(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        bar_toggle1(e);
    }
}
void ui_event_Button2(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED) {
        bar_toggle1(e);
    }
}

//터치스크린 설정(XPT2046)
TFT_Touch touch = TFT_Touch(XPT2046_CS, XPT2046_CLK, XPT2046_MOSI, XPT2046_MISO);

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    uint16_t touchX = 0, touchY = 0;

    bool touched = touch.Pressed();//tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        //XPT2046에서 인식한 x좌표와 y좌표를 LVGL에 넘겨준다
        touchX = touch.X();
        touchY = touch.Y();

        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    }
}

void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

    tft.begin();          /* TFT init */
    tft.setRotation( 3 ); /* Landscape orientation, flipped */

    //LCD의 저항값을 좌표료 계산하기 위한 보정값
    touch.setCal(526, 3443, 750, 3377, 320, 240, 1);
    //값을 setRotation() 의 값과 반대로 설정해야함
    touch.setRotation(3);

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register( &indev_drv );

    
    
    ui_init();

    //와이파이 리스트 생성
    wifi_list = lv_list_create(ui_Screen2);
    lv_obj_set_size(wifi_list, 180, 220);
    lv_obj_align(wifi_list, LV_ALIGN_TOP_RIGHT, 0, 0);

    
    //EEPROM에서 데이터 로드
    EEPROM.begin(EEPROM_SIZE);
    char beginSsid[20];
    char beginPassword[20];
    

    //EEPROM에서 WiFi 정보 로드   
    loadCredentialsFromEEPROM(beginSsid, beginPassword);
    Serial.println(beginSsid);
    Serial.println(beginPassword);

    if (strlen(beginSsid) > 0 && strlen(beginPassword) > 0) {
        // 저장된 SSID와 비밀번호로 WiFi 연결
        WiFi.begin(beginSsid, beginPassword);
        Serial.printf("Connecting to WiFi: %s\n", beginSsid);

        int retryCount = 0;
        while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
            delay(1000);
            Serial.print(".");
            retryCount++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi Connected!");
            syncNTP();
            update_weather_data();

        } else {
            Serial.println("\nWiFi Connection Failed.");
        }
    } else {
        Serial.println("No WiFi credentials stored in EEPROM.");
    }

    EEPROM.end();


    

    //gif_play();
    //와이파이 연결
    //connect_to_wifi();
    

    // 태스크 생성
    //xTaskCreatePinnedToCore(gif_control_task, "GIF Control Task", 4096, NULL, 1, NULL, 1);
    //xTaskCreatePinnedToCore(ui_update_task, "UI Update Task", 4096, NULL, 1, NULL, 1);

    
    Serial.println( "Setup done" );
}

void loop()
{
    unsigned long currentMillis = millis();  // 현재 시스템 시간

        // 1시간마다 NTP 서버와 시간 동기화
        if (currentMillis - lastNTPSync >= NTP_SYNC_INTERVAL) {
            if(WiFi.status() == WL_CONNECTED){
                syncNTP();
            }
        }

        // 1초마다 작동하는 부분
        if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
            if(colonFlag){
                lv_label_set_text(ui_Label8, ":");
                colonFlag = false;
            } else{
                lv_label_set_text(ui_Label8, " ");
                colonFlag = true;
            }
            //첫 번재 화면 바 애니매이션 재생
            //bar_animation();
            //시간 업데이트
            updateTime();
            updateClock();
            //화면 갱신
            updateDisplay();
            lastTimeUpdate = currentMillis;
            checkWiFi();
            update_info_label();
        }

        // 15분마다 날씨 데이터 업데이트
        if (millis() - last_update_time > 1000 * 60 * 15) {
            if(WiFi.status() == WL_CONNECTED){
                update_weather_data();
            }
            last_update_time = millis();
        }

    lv_timer_handler(); /* let the GUI do its work */
    delay(5);

}
