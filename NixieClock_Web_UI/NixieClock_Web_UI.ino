/*
  Arduino IDE 1.8.13 версия прошивки 1.2.4 релиз от 05.10.24
  Специльно для проекта "Часы на ГРИ v2. Альтернативная прошивка"
  Страница проекта - https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab & Psyx86.

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете версию 2.7.4!

  В "Инструменты -> Управлять библиотеками..." необходимо предварительно установить указанные версии библиотек:
  GyverPortal 3.6.6
  EEManager 2.0.1

  В "Инструменты -> Flash Size" необходимо выбрать распределение памяти в зависимости от установленного объёма FLASH:
  1МБ - FS:64KB OTA:~470KB(только обновление esp по OTA).
  1МБ - FS:512KB OTA:~246KB(только локальные файлы FS и обновление прошивки часов).
  2МБ - FS:1MB OTA:~512KB(обновление esp по OTA, обновление прошивки часов и локальные файлы FS).
  4МБ - FS:2MB OTA:~1019KB(обновление esp по OTA, обновление прошивки часов и локальные файлы FS).
  8МБ - FS:6MB OTA:~1019KB(обновление esp по OTA, обновление прошивки часов и локальные файлы FS).

  Папку с плагином "ESP8266LittleFS" необходимо поместить в .../Program Files/Arduino/tools, затем нужно перезапустить Arduino IDE(если была запущена).
  Сначала загружаете прошивку, затем "Инструменты -> ESP8266 LittleFS Data Upload".

  Подключение к часам производится по шине I2C, так-же необходимо отключить внутреннюю подтяжку шины в часах, а внешнюю подтяжку подключить к 3.3в(подтяжка на шине должна быть строго только в одном месте!)
  Питать модуль ESP8266 от вывода 3v3 ардуино нельзя! Нужно использовать линейный стабилизатор или DC-DC преобразователь на 3.3в!
*/
#include "config.h"

#define GP_NO_DNS
#define GP_NO_MDNS

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS);

struct settingsData {
  boolean nameAp;
  boolean nameMenu;
  boolean namePrefix;
  boolean namePostfix;
  uint8_t weatherCity;
  float weatherLat;
  float weatherLon;
  uint8_t climateType[3];
  uint8_t climateSend;
  uint8_t climateBar;
  uint8_t climateTime;
  boolean climateAvg;
  boolean ntpSync;
  boolean ntpDst;
  uint8_t ntpTime;
  int8_t ntpGMT;
  char host[20];
  char name[20];
  char ssid[64];
  char pass[64];
  char multi[MAX_CLOCK * 2][20];
} settings;

#include <EEManager.h>
EEManager memory(settings, 3000);

//переменные
GPdate mainDate; //основная дата
GPtime mainTime; //основное время
GPtime alarmTime; //время будильника

char buffMultiIp[20]; //буфер ip адреса
char buffMultiName[20]; //буфер имени

boolean clockUpdate = true; //флаг запрета обновления часов
boolean otaUpdate = true; //флаг запрета обновления есп
boolean alarmSvgImage = false; //флаг локальных изображений будильника
boolean timerSvgImage = false; //флаг локальных изображений таймера/секундомера
boolean radioSvgImage = false; //флаг локальных изображений радиоприемника

int8_t wifiScanState = 2; //статус сканирования сети
uint32_t wifiScanTimer = 0; //таймер начала поиска сети
uint8_t wifiStatus = WL_IDLE_STATUS; //статус соединения wifi
uint32_t wifiInterval = 5000; //интервал переподключения к wifi

uint8_t timeState = 0; //флаг состояния актуальности времени
uint32_t secondsTimer = 0; //таймер счета секундных интервалов

int8_t syncState = -2; //флаг состояния синхронизации времени
uint8_t syncNtpTimer = 0; //таймер запроса времени с ntp сервера

int8_t playbackTimer = -1; //таймер остановки воспроизведения
uint8_t waitTimer = 0; //таймер ожидания опроса шины

int8_t clockState = 0; //флаг состояния соединения с часами
uint8_t uploadState = 0; //флаг состояния загрузки файла прошивки часов

uint8_t climateTimer = 0; //таймер обновления микроклимата
uint8_t climateCountAvg = 0; //счетчик циклов обновления микроклимата
int16_t climateTempAvg = 0; //буфер средней температуры микроклимата
uint16_t climateHumAvg = 0; //буфер средней влажности микроклимата
uint16_t climatePressAvg = 0; //буфер среднего давления микроклимата

boolean climateLocal = false; //флаг локальных скриптов графика
int8_t climateState = -1; //флаг состояние активации микроклимата

#include "NTP.h"
#include "WIRE.h"
#include "WEATHER.h"
#include "UPDATER.h"
#include "CLOCKBUS.h"

int16_t weatherArrMain[2][WEATHER_BUFFER];
int16_t weatherArrExt[1][WEATHER_BUFFER];
uint32_t weatherDates[WEATHER_BUFFER];

int16_t climateArrMain[2][CLIMATE_BUFFER];
int16_t climateArrExt[1][CLIMATE_BUFFER];
uint32_t climateDates[CLIMATE_BUFFER];

const char *climateNamesMain[] = {"Температура", "Влажность"};
const char *climateNamesExt[] = {"Давление"};

const char *climateFsData[] = {"/gp_data/PLOT_STOCK.js.gz"};
const char *alarmFsData[] = {"/alarm_add.svg", "/alarm_set.svg", "/alarm_dis.svg"};
const char *timerFsData[] = {"/timer_play.svg", "/timer_stop.svg", "/timer_pause.svg", "/timer_up.svg", "/timer_down.svg"};
const char *radioFsData[] = {"/radio_backward.svg", "/radio_left.svg", "/radio_right.svg", "/radio_forward.svg", "/radio_mode.svg", "/radio_power.svg"};

const char *tempSensList[] = {"DS3231", "AHT", "SHT", "BMP/BME", "DS18B20", "DHT"};
const char *sensDataList[] = {"CLOCK", "AHT", "SHT", "BMP", "BME"};
const char *alarmModeList[] = {"Отключен", "Однократно", "Ежедневно", "По будням"};
const char *alarmDaysList[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
const char *statusNtpList[] = {"Отсутсвует подключение к сети", "Подключение к серверу...", "Ожидание ответа...", "Синхронизировано", "Рассинхронизация", "Сервер не отвечает"};
const char *statusWeatherList[] = {"Отсутсвует подключение к сети", "Ошибка при запросе данных", "Данные успешно получены", "Идёт запрос на сервер...", "Ожидание ответа..."};
const char *statusTimerList[] = {"Отключен", "Секундомер", "Таймер", "Ошибка"};

String wifiScanList = "Нет сетей"; //список найденых wifi сетей
String sensorsList = "Отсутсвует"; //список подключенных сенсоров температуры
String dotModeList = "Выключены,Статичные,Мигают раз в секунду,Мигают два раза в секунду"; //список режимов основных разделительных точек
String backlModeList = "Выключена"; //список режимов подсветки
String alarmDotModeList = "Выключены"; //список режимов разделительных точек будильника
String playerVoiceList = "Алёна,Филипп"; //список голосов для озвучки
String flipModeList = "Без анимации,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат"; //список режимов смены минут
String secsModeList = "Без анимации,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе"; //список режимов смены секунд

#if (LED_BUILTIN == TWI_SDA_PIN) || (LED_BUILTIN == TWI_SCL_PIN)
#undef STATUS_LED
#define STATUS_LED -1
#endif

void GP_PAGE_TITLE(const String& name) {
  GP.PAGE_TITLE(((settings.namePrefix) ? (settings.name + String(" - ")) : "") + name + ((settings.namePostfix) ? (String(" - ") + settings.name) : ""));
}
void GP_LABEL_BLOCK_W(const String& val, const String& name = "", PGM_P st = GP_GREEN, int size = 0, bool bold = 0) {
  GP.TAG_RAW(F("label class='display'"), val, name, GP_WHITE, size, bold, 0, st);
}
String GP_FLOAT_DEC(float val, uint16_t dec) {
  String data = "";
  if (!dec) data += (int)round(val);
  else data += String(val, (uint16_t)dec);
  return data;
}
void GP_SLIDER_MAX(const String& lable, const String& min_lable, const String& max_lable, const String& name, float value = 0, float min = 0, float max = 100, float step = 1, uint8_t dec = 0, PGM_P st = GP_GREEN, bool dis = 0, bool oninp = 0) {
  String data = "";
  data += F("<lable style='color:#fff;position:relative;z-index:1;left:17px;bottom:1px;width:0px;pointer-events:none'");
  if (dis) data += F(" class='dsbl'");
  data += '>';
  data += lable;
  data += F("</lable>\n<input type='range' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += F("' value='");
  data += value;
  data += F("' min='");
  data += min;
  data += F("' max='");
  data += max;
  data += F("' step='");
  data += step;
  data += F("' style='background-image:linear-gradient(");
  data += FPSTR(st);
  data += ',';
  data += FPSTR(st);
  data += F(");background-size:0% 100%;height:30px;width:100%;max-width:430px;margin:10px 4px;border-radius:20px;box-shadow:0 0 15px rgba(0, 0, 0, 0.7)' onload='GP_change(this)' ");
  if (oninp) data += F("oninput='GP_change(this);GP_click(this)'");
  else data += F("onchange='GP_click(this)' oninput='GP_change(this)'");
  data += F(" onmousewheel='GP_wheel(this);GP_change(this);GP_click(this)' ");
  if (dis) data += F("class='dsbl' disabled");
  data += F(">\n<output align='center' id='");
  data += name;
  data += F("_val' name='");
  data += min_lable;
  data += ',';
  data += max_lable;
  data += F("' style='position:relative;right:70px;margin-right:-55px;background:none;display:inline-flex;justify-content:end;pointer-events:none'");
  if (dis) data += F(" class='dsbl'");
  data += F(">");
  data += GP_FLOAT_DEC(value, dec);
  data += F("</output>\n");
  GP.SEND(data);
}
String GP_SPINNER_BTN(const String& name, float step, PGM_P st, uint8_t dec, bool dis) {
  String data = "";
  data += F("<input type='button' class='spinBtn ");
  data += (step > 0) ? F("spinR") : F("spinL");
  data += F("' name='");
  data += name;
  data += F("' min='");
  data += step;
  data += F("' max='");
  data += dec;
  data += F("' onmouseleave='if(_pressId)clearInterval(_spinInt);_spinF=_pressId=null' onmousedown='_pressId=this.name;_spinInt=setInterval(()=>{GP_spin(this);_spinF=1},");
  data += 200;
  data += F(")' onmouseup='clearInterval(_spinInt)' onclick='if(!_spinF)GP_spin(this);_spinF=0' value='");
  data += (step > 0) ? '+' : '-';
  data += F("' ");
  if (st != GP_GREEN) {
    data += F(" style='background:");
    data += FPSTR(st);
    data += F(";'");
  }
  if (dis) data += F(" disabled");
  data += F(">\n");
  return data;
}
void GP_SPINNER_MAIN(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  String data = "";
  data += F("<div id='spinner' class='spinner'>\n");
  data += GP_SPINNER_BTN(name, -step, st, dec, dis);
  data += F("<input type='number' name='");
  data += name;
  data += F("' id='");
  data += name;
  if (w.length()) {
    data += F("' style='width:");
    data += w;
  }
  data += F("' step='");
  data += GP_FLOAT_DEC(step, dec);
  data += F("' onkeyup='GP_spinw(this)' onkeydown='GP_spinw(this)' onchange='");
  if (!dec) data += F("GP_spinc(this);");
  data += F("GP_click(this);GP_spinw(this)' value='");
  data += GP_FLOAT_DEC(value, dec);
  if (!isnan(min)) {
    data += F("' min='");
    data += GP_FLOAT_DEC(min, dec);
  }
  if (!isnan(max)) {
    data += F("' max='");
    data += GP_FLOAT_DEC(max, dec);
  }
  data += F("' ");
  if (dis) data += F("disabled ");
  if (!w.length()) data += F("class='spin_inp'");
  data += F(">\n");
  data += GP_SPINNER_BTN(name, step, st, dec, dis);
  data += F("</div>\n");
  GP.SEND(data);
}
void GP_SPINNER_MID(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  GP.SEND("<div style='margin-left:-10px;margin-right:-10px;'>\n"); GP_SPINNER_MAIN(name, value, min, max, step, dec, st, w, dis); GP.SEND("</div>\n");
}
void GP_SPINNER_LEFT(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  GP.SEND("<div style='margin-left:-10px;'>\n"); GP_SPINNER_MAIN(name, value, min, max, step, dec, st, w, dis); GP.SEND("</div>\n");
}
void GP_SPINNER_RIGHT(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  GP.SEND("<div style='margin-right:-10px;'>\n"); GP_SPINNER_MAIN(name, value, min, max, step, dec, st, w, dis); GP.SEND("</div>\n");
}
void GP_BUTTON_MINI_LINK(const String& url, const String& text, PGM_P color) {
  GP.SEND(String("<button class='miniButton' style='background:") + FPSTR(color) + ";line-height:100%;' onclick='location.href=\"" + url + "\";'>" + text + "</button>\n");
}
void GP_TEXT_LINK(const String& url, const String& text, const String& id, PGM_P color) {
  String data = "";
  data += F("<style>a:link.");
  data += id;
  data += F("_link{color:");
  data += FPSTR(color);
  data += F(";text-decoration:none;} a:visited.");
  data += id;
  data += F("_link{color:");
  data += FPSTR(color);
  data += F(";} a:hover.");
  data += id;
  data += F("_link{filter:brightness(0.75);}</style>\n<a href='");
  data += url;
  data += F("' class='");
  data += id + "_link";
  data += F("'>");
  data += text;
  data += F("</a>\n");
  GP.SEND(data);
}
void GP_UI_LINK(const String& url, const String& name, PGM_P color) {
  String data = "";
  data += F("<a href='http://");
  data += url;
  data += "'";
  if (WiFi.localIP().toString().equals(url)) {
    data += F(" class='sbsel' style='background:");
    data += FPSTR(color);
    data += F(" !important;'");
  }
  data += ">";
  data += name;
  data += F("</a>\n");
  GP.SEND(data);
}
void GP_CHECK_ICON(const String& name, const String& uri, bool state = 0, int size = 30, PGM_P st_0 = GP_GRAY, PGM_P st_1 = GP_GREEN, bool dis = false) {
  String data = "";
  data += F("<style>#__");
  data += name;
  data += F(" span::before{background-color:");
  data += FPSTR(st_0);
  data += F(";border:none;padding:");
  data += (size / 2) + 2;
  data += F("px;}</style>\n");

  data += F("<style>#__");
  data += name;
  data += F(" input:checked+span::before{background-color:");
  data += FPSTR(st_1);
  data += F(";background-image:none;}</style>\n");

  data += F("<label id='__");
  data += name;
  data += F("' class='check_c'");
  data += F(" style='-webkit-mask:center/contain no-repeat url(");
  data += uri;
  data += F(");display:inline-block;width:");
  data += size;
  data += F("px;'>");
  data += F("<input type='checkbox' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += "' ";
  if (state) data += F("checked ");
  if (dis) data += F("disabled ");
  data += F("onclick='GP_click(this)' style='height:");
  data += size;
  data += F("px;'><span></span></label>\n"
            "<input type='hidden' value='0' name='");
  data += name;
  data += "'>\n";
  GP.SEND(data);
}
void GP_HR(PGM_P st, int width = 0) {
  String data = "";
  data += F("<hr style='border-color:");
  data += FPSTR(st);
  data += F(";margin:5px ");
  data += width;
  data += F("px'>\n");
  GP.SEND(data);
}
void GP_HR_TEXT(const String& text, const String& name, PGM_P st_0, PGM_P st_1) {
  String data = "";

  data += F("<label id='");
  data += name;
  data += F("' class='thinText' style='color:");
  data += FPSTR(st_0);
  data += F("'>");
  data += text;
  data += F("</label>\n");
  data += F("<hr style='border-color:");
  data += FPSTR(st_1);
  data += F(";margin-top:-17px;padding-bottom:17px'>\n");

  GP.SEND(data);
}
void GP_LINE_LED(const String& name, bool state = 0, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
  String data = "";

  data += F("<style>#__");
  data += name;
  data += F(" input:checked+span::before{background-color:");
  data += FPSTR(st_1);
  data += F(";background-image:none}\n");

  data += F("#__");
  data += name;
  data += F(" span::before{background-color:");
  data += FPSTR(st_0);
  data += F(";border:none;display:inline-block;width:100px;height:0px;cursor:default;filter:brightness(1)!important;box-shadow:0 0 15px rgba(0, 0, 0, 0.7);}\n");

  data += F("#__");
  data += name;
  data += F(" input[type=checkbox]{cursor:default;}</style>\n");

  data += F("<label id='__");
  data += name;
  data += F("' class='check_c' style='display:block;height:30px;margin-top:-13px;cursor:default'><input type='checkbox' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += "' ";
  if (state) data += F("checked ");
  data += F("disabled ");
  data += F("onclick='GP_click(this)'><span></span></label>\n"
            "<input type='hidden' value='0' name='");
  data += name;
  data += "'>\n";
  GP.SEND(data);
}
void GP_LINE_BAR(const String& name, int value = 0, int min = 0, int max = 100, int step = 1, PGM_P st = GP_GREEN) {
  String data = "";

  data += F("<input type='range' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += F("' value='");
  data += value;
  data += F("' min='");
  data += min;
  data += F("' max='");
  data += max;
  data += F("' step='");
  data += step;
  data += F("' style='filter:brightness(1);box-shadow:0 0 15px rgba(0, 0, 0, 0.7);background-color:#1a1a1a;background-image:linear-gradient(");
  data += FPSTR(st);
  data += ',';
  data += FPSTR(st);
  data += F(");background-size:0% 100%;display:block;width:124px;height:8px;margin-top:3px;margin-bottom:6px;cursor:default' onload='GP_change(this)' disabled>\n");

  data += F("<output style='display:none' id='");
  data += name;
  data += F("_val'></output>\n");

  GP.SEND(data);
}
void GP_PLOT_STOCK_BEGIN(boolean local = 0) {
  String data = "";
  if (local) data += F("<script src='/gp_data/PLOT_STOCK.js'></script>\n<script src='/gp_data/PLOT_STOCK_DARK.js'></script>\n");
  else data += F("<script src='https://code.highcharts.com/stock/highstock.js'></script>\n<script src='https://code.highcharts.com/themes/dark-unica.js'></script>\n");
  GP.SEND(data);
}
void GP_PLOT_STOCK_ADD(uint32_t time, int16_t val, uint8_t dec) {
  String data = "";
  data += '[';
  data += time;
  data += F("000");
  data += ',';
  if (dec) data += (float)val / dec;
  else data += val;
  data += "],\n";
  GP.SEND(data);
}
void GP_PLOT_STOCK_DARK(const String& id, const char** labels, uint32_t* times, int16_t* vals_0, int16_t* vals_1, uint8_t size, uint8_t dec = 0, uint16_t height = 400, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
  String data = "";

  data += F("<div class='chartBlock' style='width:95%;height:");
  data += height;
  data += F("px' id='");
  data += id;
  data += F("'></div>");

  data += F("<script>Highcharts.setOptions({colors:['");
  data += FPSTR(st_0);
  data += F("','");
  data += FPSTR(st_1);
  data += F("']});\nHighcharts.stockChart('");
  data += id;
  data += F("',{chart:{},\n"
            "rangeSelector:{buttons:[\n"
            "{count:1,type:'minute',text:'1M'},\n"
            "{count:1,type:'hour',text:'1H'},\n"
            "{count:1,type:'day',text:'1D'},\n"
            "{type:'all',text:'All'}],\n"
            "inputEnabled:false,selected:3},\n"
            "time:{useUTC:true},\n"
            "credits:{enabled:false},series:[\n"
           );

  if (vals_0 != NULL) {
    data += F("{name:'");
    data += labels[0];
    data += F("',data:[\n");
    GP.SEND(data);
    data = "";
    for (uint16_t s = 0; s < size; s++) {
      GP_PLOT_STOCK_ADD(times[s], vals_0[s], dec);
    }
    data += "]},\n";
  }
  if (vals_1 != NULL) {
    data += F("{name:'");
    data += labels[1];
    data += F("',data:[\n");
    GP.SEND(data);
    data = "";
    for (uint16_t s = 0; s < size; s++) {
      GP_PLOT_STOCK_ADD(times[s], vals_1[s], dec);
    }
    data += "]},\n";
  }
  data += F("]});</script>\n");
  GP.SEND(data);
}
void GP_BLOCK_SHADOW_BEGIN(void) {
  GP.SEND(F("<div style='box-shadow:0 0 15px rgb(0 0 0 / 45%);border-radius:25px;margin:5px 10px 5px 10px;'>\n"));
}
void GP_BLOCK_SHADOW_END(void) {
  GP.SEND(F("</div>\n"));
}
void GP_FOOTER_BEGIN(void) {
  GP.SEND("<div style='flex-grow:1;display:block;padding:0px;'></div>\n<footer>");
}
void GP_FOOTER_END(void) {
  GP.SEND("</footer>");
}
void GP_BUILD_END(void) {
  GP.SEND(F("</div>\n<div id='onlBlock' class='onlBlock'>Нет соединения</div>\n"));
  GP.JS_BOTTOM();
  GP.PAGE_END();
}
void GP_FIX_SCRIPTS(void) {
  GP.SEND(F(
            "<script>var _err=0;\n"
            "function GP_send(req,r=null,upd=null){\n"
            "var xhttp=new XMLHttpRequest();\n"
            "xhttp.open(upd?'GET':'POST',req,true);\n"
            "xhttp.send();\n"
            "xhttp.timeout=_tout;\n"
            "xhttp.onreadystatechange=function(){\n"
            "if(this.status||(++_err>=5)){onlShow(!this.status);_err=0;}\n"
            "if(this.status||upd){\n"
            "if(this.readyState==4&&this.status==200){\n"
            "if(r){\n"
            "if(r==1)location.reload();\n"
            "else location.href=r;}\n"
            "if(upd)GP_apply(upd,this.responseText);}}}}\n"
            "function GP_spinc(arg){\n"
            "if (arg.className=='spin_inp'){\n"
            "arg.value-=arg.value%arg.step;}}\n"
            "function GP_change(arg){\n"
            "arg.style.backgroundSize=(arg.value-arg.min)*100/(arg.max-arg.min)+'% 100%';\n"
            "const _output=getEl(arg.id+'_val');\n"
            "const _range=_output.name.split(',');\n"
            "if((arg.value<=Number(arg.min))&&_range[0]){_output.value=_range[0];}\n"
            "else if((arg.value>=Number(arg.max))&&_range[1]){_output.value=_range[1];}\n"
            "else _output.value=arg.value;}</script>\n"
          )
         );
}
void GP_FIX_STYLES(void) {
  GP.SEND(F(
            "<style>.headbar{z-index:3;}\n" //фикс меню в мобильной версии
            ".onlBlock{z-index:3;background:#810000bf;width:15px;height:180px;border-radius:25px 0 0 25px;writing-mode:vertical-lr;text-align:center;}\n" //фикс плашки офлайн
            ".display{border-radius:5px;}\n" //фикс лейбл блоков
            ".sblock{display:flex;flex-direction:column;min-height:98%;margin:0;}\n" //фикс меню
            ".sblock>a{border-radius:25px;}\n" //фикс кнопок меню
            ".spinBtn{font-size:24px!important;padding-left:3.5px;padding-top:0.5px;}\n" //фикс кнопок спинера
            ".check_c>span::before{border-color:#444;background-color:#2a2d35}\n" //фикс чекбоксов
            ".check_c>input:checked+span::before{border-color:#e67b09;background-color:#e67b09}\n" //фикс чекбоксов
            ".miniButton{padding:1px 7px;}\n" //фикс кнопок
            "input[type='submit'],input[type='button'],button{line-height:90%;border-radius:28px;}\n" //фикс кнопок
            "input[type='text'],input[type='password'],input[type='time'],input[type='date'],select,textarea{text-align:center;appearance:none;}\n" //фикс положения текста
            "input[type='time'],input[type='date']{height:34px;border:none!important;}\n" //фикс выбора времени и даты
            "input[type='number']{text-align:center;}\n" //фикс ввода чисел
            "input[type=range]:disabled{filter:brightness(0.6);}\n" //фикс слайдеров
            "input[type=range]::-moz-range-thumb{-moz-appearance:none;border:none;height:0px;width:0px;}\n" //фикс слайдеров
            "output{min-width:50px;border-radius:5px;}\n" //фикс слайдеров
            "select:disabled{filter:brightness(0.6);}\n" //фикс выпадающего списка
            "select{width:200px;cursor:pointer;}\n" //фикс выпадающего списка
            "#ubtn {min-width:34px;border-radius:25px;line-height:160%;}\n" //фикс кнопок загрузки
            "#grid .block{margin:15px 10px;}</style>\n" //фикс таблицы
            "<style type='text/css'>@media screen and (max-width:1100px){\n.grid{display:block;}\n#grid .block{margin:20px 10px;width:unset;}}</style>\n" //отключить таблицу при ширине экрана меньше 1050px
          )
         );
}

void build(void) {
  static boolean listInit = false;

  GP.BUILD_BEGIN(UI_MAIN_THEME, 500);
  GP_FIX_SCRIPTS(); //фикс скрипта проверки онлайна
  GP_FIX_STYLES(); //фикс стилей страницы

  if (updaterState()) {
    GP_PAGE_TITLE("Обновление");
    GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки часов", UI_BLOCK_COLOR);
    if (!updaterFlash()) {
      GP.SPAN(getUpdaterState(), GP_CENTER, "syncUpdate", GP_YELLOW); //описание
    }
    else {
      GP.SPAN("<big><b>Подключение...</b></big>", GP_CENTER, "syncUpdate", UI_INFO_COLOR); //описание
      GP.SPAN(String("<small>Не выключайте устройство до завершения обновления!</small>") + ((deviceInformation[HARDWARE_VERSION]) ? "" : "<br><small>Для входа в режим прошивки кратковременно нажмите ресет на микроконтроллере часов.</small>"), GP_CENTER, "syncWarn", GP_RED); //описание
      GP.UPDATE("syncUpdate,syncWarn", 300);
    }
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
    GP.BLOCK_END();
  }
  else if (deviceInformation[HARDWARE_VERSION] && (deviceInformation[HARDWARE_VERSION] != HW_VERSION)) {
    GP_PAGE_TITLE("Ошибка совместимости");
    GP.BLOCK_BEGIN(GP_THIN, "", "Предупреждение", UI_BLOCK_COLOR);
    GP.SPAN("<big><b>Эта версия веб-интерфейса не может взаимодействовать с этим устройством!</b></big>", GP_CENTER, "", UI_INFO_COLOR);
    GP.BREAK();
    GP.LABEL("Clock HW: 0x" + String(deviceInformation[HARDWARE_VERSION], HEX));
    GP.BREAK();
    GP.LABEL("WebUI HW: 0x" + String(HW_VERSION, HEX));
    if (otaUpdate) {
      GP.HR(UI_LINE_COLOR);
      GP.BUTTON_MINI_LINK("/ota_update", "Обновить прошивку", UI_BUTTON_COLOR);
    }
    GP.BLOCK_END();
  }
  else if (busRebootState()) {
    GP.PAGE_TITLE("Перезагрузка");

    GP.BLOCK_BEGIN(GP_THIN, "", "Перезагрузка устройства", UI_BLOCK_COLOR);
    GP.SPAN("<big><b>Выполняется перезагрузка, подождите...</b></big>", GP_CENTER, "syncReboot", UI_INFO_COLOR); //описание
    GP.SPAN("<small>Не выключайте устройство до завершения перезагрузки!</small>", GP_CENTER, "syncWarn", GP_RED); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
    GP.UPDATE("syncReboot,syncWarn");
    GP.BLOCK_END();
  }
  else {
    if (!listInit && deviceInformation[HARDWARE_VERSION]) {
      listInit = true;

      if (deviceInformation[BACKL_TYPE]) {
        backlModeList += ",Статичная,Дыхание";
      }
      if (deviceInformation[BACKL_TYPE] >= 3) {
        backlModeList += ",Дыхание со сменой цвета при затухании,Бегущий огонь,Бегущий огонь со сменой цвета,Бегущий огонь с радугой,Бегущий огонь с конфетти,Волна,Волна со сменой цвета,Волна с радугой,Волна с конфетти,Плавная смена цвета,Радуга,Конфетти";
      }

      if (deviceInformation[NEON_DOT] != 3) {
        dotModeList += ",Динамичные(плавно мигают)";
      }
      if (deviceInformation[NEON_DOT] == 2) {
        dotModeList += ",Неонки маятник(мигают раз в секунду),Неонки маятник(плавно мигают)";
      }
      if (deviceInformation[DOTS_PORT_ENABLE]) {
        dotModeList += ",Мигающие,Бегущие,Змейка,Резинка";
        if ((deviceInformation[DOTS_NUM] > 4) || (deviceInformation[DOTS_TYPE] == 2)) {
          dotModeList += ",Одинарный маятник";
        }
        if ((deviceInformation[DOTS_NUM] > 4) && (deviceInformation[DOTS_TYPE] == 2)) {
          dotModeList += ",Двойной маятник";
        }
      }
      alarmDotModeList = dotModeList + ",Без реакции";

      for (uint8_t i = 2; i < deviceInformation[PLAYER_MAX_VOICE]; i++) {
        playerVoiceList += ",Голос_";
        playerVoiceList += i;
      }
    }

    //обновления блоков
    String updateList = "barTime";

    //начать меню
    GP.UI_MENU("Nixie clock", UI_MENU_COLOR);
    if (settings.nameMenu && settings.name[0]) {
      GP.LABEL(settings.name, "", UI_MENU_NAME_COLOR);
      GP_HR(UI_MENU_LINE_COLOR, 6);
    }

    //ссылки меню
    GP.UI_LINK("/", "Главная");
    GP.UI_LINK("/settings", "Настройки");
    if (climateState > 0) GP.UI_LINK("/climate", "Микроклимат");
    if (weatherGetValidStatus()) GP.UI_LINK("/weather", "Погода");
    if (deviceInformation[RADIO_ENABLE]) GP.UI_LINK("/radio", "Радио");
    if (otaUpdate || clockUpdate) GP.UI_LINK("/update", "Обновление");
    GP.UI_LINK("/information", "Об устройстве");
    GP.UI_LINK("/network", "Сетевые настройки");

    //ссылки часов
    if (settings.multi[0][0] != '\0') {
      GP_HR(UI_MENU_LINE_COLOR, 6);
      for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
        if (settings.multi[i][0] != '\0') {
          GP_UI_LINK(settings.multi[i], (settings.multi[i + 1][0] != '\0') ? settings.multi[i + 1] : settings.multi[i], UI_MENU_COLOR);
        }
        else break;
      }
    }

    GP_HR(UI_MENU_LINE_COLOR, 6);

    //состояние соединения
    updateList += ",bar_clock";
    GP_BLOCK_SHADOW_BEGIN();
    GP.LABEL("Статус часов", "", UI_MENU_TEXT_COLOR, 15);
    GP_LINE_LED("bar_clock", (clockState != 0), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
    GP_BLOCK_SHADOW_END();

    if (!deviceInformation[DS3231_ENABLE] && (rtc_status != RTC_NOT_FOUND)) {
      updateList += ",bar_rtc";
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Статус RTC", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_LED("bar_rtc", (rtc_status != RTC_BAT_LOW), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
      GP_BLOCK_SHADOW_END();
    }
    if (ntpGetRunStatus()) {
      updateList += ",bar_ntp";
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Статус NTP", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_LED("bar_ntp", (ntpGetSyncStatus()), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
      GP_BLOCK_SHADOW_END();
    }
    if (wifiStatus == WL_CONNECTED) {
      updateList += ",bar_wifi";
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Сигнал WiFi", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_BAR("bar_wifi", constrain(2 * (WiFi.RSSI() + 100), 0, 100), 0, 100, 1, UI_MENU_WIFI_COLOR);
      GP_BLOCK_SHADOW_END();
    }

    GP_FOOTER_BEGIN();
    GP.HR(UI_MENU_LINE_COLOR);
    GP_TEXT_LINK("https://github.com/radon-lab/", "@radon_lab", "user", "#bbb");
    GP.BREAK();
    GP_TEXT_LINK("https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/", "Обсуждение на форуме", "forum", "#e67b09");
    GP_FOOTER_END();

    GP.UI_BODY(); //начать основное окно

    GP.BOX_BEGIN(GP_JUSTIFY, "auto;padding-left:2%;padding-right:2%");
    GP.LABEL_BLOCK(encodeTime(mainTime), "barTime", UI_BAR_CLOCK_COLOR, 18, 1);

    GP.BOX_BEGIN(GP_RIGHT, "100%");
    if (climateState > 0 && (!settings.climateBar || !weatherGetValidStatus())) {
      updateList += ",barTemp";
      GP.LABEL_BLOCK(String(climateGetTempFloat(), 1) + "°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
      if (climateGetHum()) {
        updateList += ",barHum";
        GP.LABEL_BLOCK(String(climateGetHum()) + "%", "barHum", UI_BAR_HUM_COLOR, 18, 1);
      }
      if (climateGetPress()) {
        updateList += ",barPress";
        GP.LABEL_BLOCK(String(climateGetPress()) + "mm.Hg", "barPress", UI_BAR_PRESS_COLOR, 18, 1);
      }
    }
    else if (weatherGetValidStatus()) {
      updateList += ",weatherTemp,weatherHum,weatherPress";
      GP.LABEL_BLOCK(String(sens.wetherTemp / 10.0, 1) + "°С", "weatherTemp", UI_BAR_TEMP_COLOR, 18, 1);
      GP.LABEL_BLOCK(String(sens.wetherHum) + "%", "weatherHum", UI_BAR_HUM_COLOR, 18, 1);
      GP.LABEL_BLOCK(String(sens.wetherPress) + "mm.Hg", "weatherPress", UI_BAR_PRESS_COLOR, 18, 1);
    }
    else {
      GP.LABEL_BLOCK("-.-°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
    }
    GP.BOX_END();

    GP.BOX_END();
    GP.HR(UI_BAR_LINE_COLOR);

    if (playbackTimer > -1) playbackTimer = 0; //сбросили воспроизведение

    if (ui.uri("/")) { //основная страница
      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        GP_PAGE_TITLE("Главная");
        M_GRID(
          GP.BLOCK_BEGIN(GP_THIN, "", "Настройка времени", UI_BLOCK_COLOR);
          M_BOX(GP.LABEL("Время", "", UI_LABEL_COLOR); GP.TIME("mainTime"););
          M_BOX(GP.LABEL("Дата", "", UI_LABEL_COLOR); GP.DATE("mainDate"););
          M_BOX(GP.LABEL("Формат", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.LABEL("24ч", "", UI_LABEL_COLOR);  GP.SWITCH("mainTimeFormat", mainSettings.timeFormat, UI_SWITCH_COLOR); GP.LABEL("12ч", "", UI_LABEL_COLOR);););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Часовой пояс", "", UI_LABEL_COLOR); GP.SELECT("syncGmt", "GMT-12,GMT-11,GMT-10,GMT-9,GMT-8,GMT-7,GMT-6,GMT-5,GMT-4,GMT-3,GMT-2,GMT-1,GMT+0,GMT+1,GMT+2,GMT+3,GMT+4,GMT+5,GMT+6,GMT+7,GMT+8,GMT+9,GMT+10,GMT+11,GMT+12", settings.ntpGMT + 12, 0););
          M_BOX(GP.LABEL("Автосинхронизация", "", UI_LABEL_COLOR); GP.SWITCH("syncAuto", settings.ntpSync, UI_SWITCH_COLOR, (boolean)(ntpGetStatus() != NTP_SYNCED)););
          M_BOX(GP.LABEL("Учитывать летнее время", "", UI_LABEL_COLOR); GP.SWITCH("syncDst", settings.ntpDst, UI_SWITCH_COLOR, (boolean)(ntpGetStatus() != NTP_SYNCED)););
          GP.HR(UI_LINE_COLOR);
          GP.BUTTON("syncTime", (ntpGetStatus() != NTP_SYNCED) ? "Время с устройства" : "Синхронизация с сервером", "", UI_BUTTON_COLOR);
          GP.BLOCK_END();

          GP.BLOCK_BEGIN(GP_THIN, "", "Эффекты", UI_BLOCK_COLOR);
          M_BOX(GP.LABEL("Глюки", "", UI_LABEL_COLOR); GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR););
          M_BOX(GP.LABEL("Точки", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList, fastSettings.dotMode););
          M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", flipModeList, fastSettings.flipMode););
          M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("fastSecsFlip", secsModeList, fastSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Подсветка", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList, fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
          M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]);););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL((deviceInformation[PLAYER_TYPE]) ? "Озвучивать действия" : "Звук кнопок", "", UI_LABEL_COLOR); GP.SWITCH("mainSound", mainSettings.knockSound, UI_SWITCH_COLOR););
          M_BOX(GP.LABEL("Громкость", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER("mainSoundVol", mainSettings.volumeSound, 0, 15, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]);););
          GP.BLOCK_END();
        );
      }

      if (deviceInformation[ALARM_TYPE]) {
        if (alarm.reload >= 2) alarm.reload = 0;

        updateList += ",mainReload";
        GP.RELOAD("mainReload");

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        if (alarm.set) { //если режим настройки будильника
          GP_PAGE_TITLE("Настройка будильника");

          String alarmSoundList;
          for (uint8_t i = 0; i < deviceInformation[PLAYER_MAX_SOUND]; i++) {
            if (i) alarmSoundList += ',';
            alarmSoundList += String("Звук №") + (i + 1);
          }

          String alarmRadioList;
          if (deviceInformation[RADIO_ENABLE]) {
            for (uint8_t i = 0; i < 10; i++) {
              if (i) alarmRadioList += ',';
              alarmRadioList += String("CH") + i + String(" ") + String(radioSettings.stationsSave[i] / 10.0, 1);
            }
          }
          else alarmRadioList += "Пусто";

          M_BOX(GP_CENTER, GP.TIME("alarmTime", alarmTime); GP.SELECT("alarmMode", "Выключен,Однократно,Ежедневно,По будням,Выбрать дни", alarm_data[alarm.now][ALARM_DATA_MODE]););
          GP.BREAK();

          GP_HR_TEXT("Дни недели", "", UI_LINE_COLOR, UI_HINT_COLOR);
          GP.BLOCK_BEGIN(GP_DIV_RAW, "430px;margin:10px 0px");
          GP.TABLE_BORDER(false);
          GP.TABLE_BEGIN("50px,50px,50px,50px,50px,50px,50px");
          GP.TR(GP_CENTER);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ПН", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ВТ", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("СР", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ЧТ", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ПТ", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("СБ", "", UI_ALARM_WEEK_2_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ВС", "", UI_ALARM_WEEK_2_COLOR, 0);

          uint8_t alarmDays = alarm_data[alarm.now][ALARM_DATA_DAYS];
          GP.TR(GP_CENTER);
          for (uint8_t i = 1; i < 8; i++) {
            GP.TD(GP_CENTER);
            alarmDays >>= 1;
            GP.CHECK(String("alarmDay/") + i, (boolean)(alarmDays & 0x01));
            updateList += String(",alarmDay/") + i;
          }
          GP.TABLE_END();
          GP.BLOCK_END();

          GP_HR_TEXT("Настройка звука", "", UI_LINE_COLOR, UI_HINT_COLOR);
          M_BOX(GP_CENTER, GP.SELECT("alarmSoundType", (deviceInformation[RADIO_ENABLE]) ? "Мелодия,Радиостанция" : "Мелодия", (boolean)alarm_data[alarm.now][ALARM_DATA_RADIO], 0, (boolean)!deviceInformation[RADIO_ENABLE], true););
          M_BOX(GP_CENTER,
                GP.SELECT("alarmSound", alarmSoundList, alarm_data[alarm.now][ALARM_DATA_SOUND], 0, (boolean)(deviceInformation[RADIO_ENABLE] && alarm_data[alarm.now][ALARM_DATA_RADIO]));
                GP.SELECT("alarmRadio", alarmRadioList, alarm_data[alarm.now][ALARM_DATA_STATION], 0, (boolean)(!deviceInformation[RADIO_ENABLE] || !alarm_data[alarm.now][ALARM_DATA_RADIO]));
               );
          M_BOX(GP_CENTER, GP_SLIDER_MAX("Громкость", "авто", "макс", "alarmVol", alarm_data[alarm.now][ALARM_DATA_VOLUME], 0, 15, 1, 0, UI_SLIDER_COLOR, (boolean)((!deviceInformation[RADIO_ENABLE] || !alarm_data[alarm.now][ALARM_DATA_RADIO]) && !deviceInformation[PLAYER_TYPE])););

          boolean alarmDelStatus = (boolean)(alarm.all > 1);

          GP.HR(UI_LINE_COLOR);
          M_BOX(GP_CENTER,
                GP.BUTTON_MINI("alarmBack", (alarm.set == 1) ? "Назад" : "Добавить", "", UI_ALARM_BACK_COLOR, "210px!important", false, true);
                GP.BUTTON_MINI("alarmDel", (alarmDelStatus) ? ((alarm.set == 1) ? "Удалить" : "Отмена") : "Отключить", "", (alarmDelStatus) ? UI_ALARM_DEL_COLOR : UI_ALARM_DIS_COLOR, "210px!important", false, !alarmDelStatus);
               );
        }
        else { //иначе режим отображения
          String reloadList;

          for (uint8_t i = 0; i < alarm.all; i++) {
            uint8_t alarmHour = alarm_data[i][ALARM_DATA_HOUR];
            uint8_t alarmMins = alarm_data[i][ALARM_DATA_MINS];
            uint8_t nowWeekDay = 0;

            String alarmTime;
            alarmTime += alarmHour;
            alarmTime += ':';
            if (alarmMins < 10) alarmTime += '0';
            alarmTime += alarmMins;

            uint8_t alarmMode = alarm_data[i][ALARM_DATA_MODE];

            String alarmStatus = " ";
            if (alarmMode >= 3) {
              boolean alarmDaysFirst = false;
              uint8_t alarmDays = (alarmMode != 3) ? alarm_data[i][ALARM_DATA_DAYS] : 0x3E;
              nowWeekDay = getWeekDay(mainDate.year, mainDate.month, mainDate.day); //получить день недели
              for (uint8_t dl = 0; dl < 7; dl++) {
                if (alarmDays & (0x01 << nowWeekDay)) {
                  nowWeekDay = dl;
                  break;
                }
                if (++nowWeekDay > 7) nowWeekDay = 1;
              }
              if (alarmMode != 3) {
                for (uint8_t dw = 0; dw < 7; dw++) {
                  alarmDays >>= 1;
                  if (alarmDays & 0x01) {
                    if (alarmDaysFirst) alarmStatus += ", ";
                    else alarmDaysFirst = true;
                    alarmStatus += alarmDaysList[dw];
                  }
                }
              }
            }
            if (alarmMode < 4) {
              alarmStatus += alarmModeList[alarmMode];
            }

            if (alarmMode) {
              if ((mainTime.hour > alarmHour) && nowWeekDay) nowWeekDay -= 1;
              if (mainTime.hour < alarmHour) alarmHour -= mainTime.hour;
              else if (mainTime.hour != alarmHour) alarmHour = (24 - mainTime.hour) + alarmHour;
              else alarmHour = 0;

              if ((mainTime.minute > alarmMins) && alarmHour) alarmHour -= 1;
              if (mainTime.minute < alarmMins) alarmMins -= mainTime.minute;
              else if (mainTime.minute != alarmMins) alarmMins = (60 - mainTime.minute) + alarmMins;
              else alarmMins = 0;

              alarmStatus += " | Через ";
              if (nowWeekDay) {
                alarmStatus += nowWeekDay;
                alarmStatus += "д ";
              }
              if (alarmHour) {
                alarmStatus += alarmHour;
                alarmStatus += "ч ";
              }
              alarmStatus += alarmMins;
              alarmStatus += "мин";
            }

            GP.BLOCK_BEGIN(GP_THIN, "", "", UI_ALARM_BLOCK_COLOR);
            if (alarmSvgImage) {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.ICON_FILE_BUTTON(String("alarmSet/") + i, alarmFsData[1], 40, UI_ALARM_SET_COLOR););
            }
            else {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.BUTTON_MINI(String("alarmSet/") + i, " ≡ ", "", UI_ALARM_SET_COLOR, ""););
            }
            M_BOX(GP_LEFT, GP.BOLD(alarmStatus, "", UI_ALARM_INFO_COLOR););
            GP.BLOCK_END();
            if (i) reloadList += ',';
            reloadList += String("alarmSet/") + i;
          }

          if (!alarm.status) {
            if (alarm.all < MAX_ALARMS) {
              if (alarmSvgImage) {
                GP.ICON_FILE_BUTTON("alarmAdd", alarmFsData[0], 50, UI_ALARM_ADD_COLOR);
              }
              else {
                M_BOX(GP_CENTER, GP.BUTTON("alarmAdd", "✚", "", UI_ALARM_ADD_COLOR, "80px;font-size:30px"););
              }
            }
          }
          else {
            if (alarmSvgImage) {
              GP.ICON_FILE_BUTTON("alarmDis", alarmFsData[2], 50, UI_ALARM_DIS_COLOR);
            }
            else {
              M_BOX(GP_CENTER, GP.BUTTON("alarmDis", "✖", "", UI_ALARM_DIS_COLOR, "80px;font-weight:bold"););
            }
          }
          GP.RELOAD_CLICK(reloadList);
        }
        GP.BLOCK_END();
      }

      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        if (deviceInformation[TIMER_ENABLE] && deviceInformation[EXT_BTN_ENABLE]) {
          updateList += ",mainTimer,mainTimerState";

          GP.BLOCK_BEGIN(GP_THIN, "", "Таймер/Секундомер", UI_BLOCK_COLOR);

          GP.BLOCK_BEGIN(GP_DIV_RAW , "280px");
          GP.BLOCK_BEGIN(GP_THIN, "", "", UI_TIMER_BLOCK_COLOR);
          GP.LABEL(getTimerState(), "mainTimerState", UI_TIMER_INFO_COLOR, 0, 1);

          GP.TABLE_BEGIN("15%,15%,15%", GP_ALS(GP_CENTER, GP_CENTER, GP_CENTER), "200px");
          GP.TR();
          for (uint8_t i = 0; i < 6; i++) {
            String btn = String("timerHour/") + i;
            GP.TD();
            if (timerSvgImage) {
              GP.ICON_FILE_BUTTON(btn, (i < 3) ? timerFsData[3] : timerFsData[4], 40, UI_TIMER_SET_COLOR);
            }
            else {
              GP.BUTTON_MINI(btn, (i < 3) ? " ▲ " : " ▼ ", "", UI_TIMER_SET_COLOR);
            }
            if (i == 2) {
              GP.TR();
              GP.TD(GP_CENTER, 3);
              M_BOX(GP_CENTER, GP.LABEL(convertTimerTime(), "mainTimer", UI_TIMER_TIME_COLOR, 40, 1););
              GP.TR();
            }
          }
          GP.TABLE_END();
          GP.BLOCK_END();
          GP.BLOCK_END();

          if (timerSvgImage) {
            M_BOX(GP_CENTER,
                  GP.ICON_FILE_BUTTON("timerControl/0", timerFsData[0], 60, UI_TIMER_CTRL_COLOR);
                  GP.ICON_FILE_BUTTON("timerControl/1", timerFsData[1], 60, UI_TIMER_CTRL_COLOR);
                  GP.ICON_FILE_BUTTON("timerControl/2", timerFsData[2], 60, UI_TIMER_CTRL_COLOR);
                 );
          }
          else {
            M_BOX(GP_CENTER,
                  GP.BUTTON_MINI("timerControl/0", "⠀⠀►⠀⠀", "", UI_TIMER_CTRL_COLOR);
                  GP.BUTTON_MINI("timerControl/1", "⠀⠀❑⠀⠀", "", UI_TIMER_CTRL_COLOR);
                  GP.BUTTON_MINI("timerControl/2", "⠀⠀||⠀⠀", "", UI_TIMER_CTRL_COLOR);
                 );
          }
          GP.BLOCK_END();

          GP.UPDATE_CLICK("mainTimer", "timerHour/0,timerHour/1,timerHour/2,timerHour/3,timerHour/4,timerHour/5");
          GP.UPDATE_CLICK("mainTimer,mainTimerState", "timerControl/0,timerControl/1,timerControl/2");
        }
      }
    }
    else if (ui.uri("/settings")) { //настройки
      GP_PAGE_TITLE("Настройки");

      String showModeList = "Пусто,Дата,Год,Дата и год"; //список режимов автопоказа
      if (deviceInformation[LAMP_NUM] < 6) showModeList += "(недоступно)";
      if (deviceInformation[SENS_TEMP]) {
        showModeList += ",Температура,Влажность,Давление,Температура и влажность";
        if (deviceInformation[LAMP_NUM] < 6) showModeList += "(недоступно)";
      }
      if ((climateState != 0) && (!settings.climateSend || !deviceInformation[SENS_TEMP])) {
        showModeList += ",Температура(есп),Влажность(есп),Давление(есп),Температура и влажность";
        if (deviceInformation[LAMP_NUM] < 6) showModeList += "(недоступно)";
        else showModeList += "(есп)";
      }
      if (weatherGetValidStatus() && (settings.climateSend || !deviceInformation[SENS_TEMP])) {
        showModeList += ",Температура(погода),Влажность(погода),Давление(погода),Температура и влажность";
        if (deviceInformation[LAMP_NUM] < 6) showModeList += "(недоступно)";
        else showModeList += "(погода)";
      }

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Автопоказ", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Включить", "", UI_LABEL_COLOR); GP.SWITCH("mainAutoShow", (boolean)!(mainSettings.autoShowTime & 0x80), UI_SWITCH_COLOR););
        M_BOX(GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("mainAutoShowTime", mainSettings.autoShowTime & 0x7F, 1, 15, 1, 0, UI_SPINNER_COLOR););
        M_BOX(GP.LABEL("Эффект", "", UI_LABEL_COLOR); GP.SELECT("mainAutoShowFlip", "Основной эффект,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат", mainSettings.autoShowFlip););
        GP.BREAK();
        GP_HR_TEXT("Отображение", "hint4", UI_LINE_COLOR, UI_HINT_COLOR);
        GP.HINT("hint4", "Источник и время в секундах"); //всплывающая подсказка
      for (uint8_t i = 0; i < 5; i++) {
      M_BOX(
        M_BOX(GP_LEFT, GP.LABEL(String(i + 1), "hint4", UI_LABEL_COLOR); GP.SELECT(String("extShowMode/") + i, showModeList, extendedSettings.autoShowModes[i]););
        GP_SPINNER_MID(String("extShowTime/") + i, extendedSettings.autoShowTimes[i], 1, 5, 1, 0, UI_SPINNER_COLOR);
      );
      }
      GP.BREAK();
      GP_HR_TEXT("Дополнительно", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Коррекция, °C", "", UI_LABEL_COLOR); GP_SPINNER_MID("mainTempCorrect", mainSettings.tempCorrect / 10.0, -12.7, 12.7, 0.1, 1, UI_SPINNER_COLOR, "", (boolean)(climateState <= 0)););
      M_BOX(GP.LABEL("Корректировать", "", UI_LABEL_COLOR); GP.SELECT("climateCorrectType", "Ничего," + climateGetSensList(), extendedSettings.tempCorrectSensor););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Индикаторы", UI_BLOCK_COLOR);
      GP.LABEL("Яркость", "", UI_HINT_COLOR);
      M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); GP.SLIDER_C("mainIndiBrtDay", mainSettings.indiBrightDay, 5, 30, 1, 0, UI_SLIDER_COLOR););
      M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); GP.SLIDER_C("mainIndiBrtNight", mainSettings.indiBrightNight, 5, 30, 1, 0, UI_SLIDER_COLOR););
      GP.BREAK();
      GP_HR_TEXT("Эффекты", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Глюки", "", UI_LABEL_COLOR); GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", flipModeList, fastSettings.flipMode););
      M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("fastSecsFlip", secsModeList, fastSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
      GP.BREAK();
      GP_HR_TEXT("Антиотравление", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Период, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("mainBurnTime", mainSettings.burnTime, 10, 180, 5, 0, UI_SPINNER_COLOR););
      M_BOX(GP.LABEL("Метод", "", UI_LABEL_COLOR); GP.SELECT("mainBurnFlip", "Перебор всех индикаторов,Перебор одного индикатора,Перебор одного индикатора с отображением времени", mainSettings.burnMode););
      GP.BREAK();
      GP_HR_TEXT("Время смены яркости", "hint1", UI_LINE_COLOR, UI_HINT_COLOR);
      GP.HINT("hint1", "Одинаковое время - отключить смену яркости или активировать датчик освещения"); //всплывающая подсказка
      M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP_SPINNER_LEFT("mainTimeBrightS", mainSettings.timeBrightStart, 0, 23, 1, 0, UI_SPINNER_COLOR); GP_SPINNER_RIGHT("mainTimeBrightE", mainSettings.timeBrightEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
      GP.BREAK();
      GP_HR_TEXT("Режим сна", "hint2", UI_LINE_COLOR, UI_HINT_COLOR);
      GP.HINT("hint2", "0 - отключить режим сна для выбранного промежутка времени"); //всплывающая подсказка
      M_BOX(GP_CENTER, GP.LABEL("День", "", UI_LABEL_COLOR); GP_SPINNER_LEFT("mainSleepD", mainSettings.timeSleepDay, 0, 90, 15, 0, UI_SPINNER_COLOR); GP_SPINNER_RIGHT("mainSleepN", mainSettings.timeSleepNight, 0, 30, 5, 0, UI_SPINNER_COLOR); GP.LABEL("Ночь", "", UI_LABEL_COLOR););
      GP.BLOCK_END();
      );

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Подсветка", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
        M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList, fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
        GP.BREAK();
        GP_HR_TEXT("Яркость", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); GP.SLIDER_C("mainBacklBrightDay", mainSettings.backlBrightDay, 10, 250, 10, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); GP.SLIDER_C("mainBacklBrightNight", mainSettings.backlBrightNight, 0, 250, 10, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Точки", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList, fastSettings.dotMode););
        GP.BREAK();
        GP_HR_TEXT("Яркость", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); GP.SLIDER_C("mainDotBrtDay", mainSettings.dotBrightDay, 10, 250, 10, 0, UI_SLIDER_COLOR, (boolean)(deviceInformation[NEON_DOT] == 3)););
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); GP.SLIDER_C("mainDotBrtNight", mainSettings.dotBrightNight, 0, (deviceInformation[NEON_DOT] == 3) ? 1 : 250, (deviceInformation[NEON_DOT] == 3) ? 1 : 10, 0, UI_SLIDER_COLOR););
        GP.BLOCK_END();
      );

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Звуки", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL((deviceInformation[PLAYER_TYPE]) ? "Озвучивать действия" : "Звук кнопок", "", UI_LABEL_COLOR); GP.SWITCH("mainSound", mainSettings.knockSound, UI_SWITCH_COLOR););
        M_BOX(GP.LABEL("Голос озвучки", "", UI_LABEL_COLOR); GP.SELECT("mainVoice", "Алёна,Филипп", mainSettings.voiceSound, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
        M_BOX(GP_JUSTIFY, GP.LABEL("Громкость", "", UI_LABEL_COLOR); GP.SLIDER("mainSoundVol", mainSettings.volumeSound, 0, 15, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]););
        GP.BREAK();
        GP_HR_TEXT("Звук смены часа", "hint3", UI_LINE_COLOR, UI_HINT_COLOR);
        GP.HINT("hint3", "Одниаковое время - отключить звук смены часа"); //всплывающая подсказка
        M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP_SPINNER_LEFT("mainHourSoundS", mainSettings.timeHourStart, 0, 23, 1, 0, UI_SPINNER_COLOR); GP_SPINNER_RIGHT("mainHourSoundE", mainSettings.timeHourEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
        GP.BREAK();
        GP_HR_TEXT("Озвучка смены часа", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Температура", "", UI_LABEL_COLOR); GP.SWITCH("mainHourTemp", mainSettings.hourSound & 0x80, UI_SWITCH_COLOR, (boolean)!(deviceInformation[PLAYER_TYPE] && (climateState > 0))););
        M_BOX(GP.LABEL("Новый час", "", UI_LABEL_COLOR); GP.SELECT("mainHourSound", "Автоматически,Только мелодия,Только озвучка,Мелодия и озвучка", mainSettings.hourSound & 0x03, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Автоотключение, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("extAlarmTimeout", extendedSettings.alarmTime, 1, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););

        GP.BREAK();
        GP_HR_TEXT("Дополнительно", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Повтор сигнала, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("extAlarmWaitTime", extendedSettings.alarmWaitTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
        M_BOX(GP.LABEL("Отключить звук, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("extAlarmSoundTime", extendedSettings.alarmSoundTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););

        GP.BREAK();
        GP_HR_TEXT("Индикация", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Активный", "", UI_LABEL_COLOR); GP.SELECT("extAlarmDotOn", alarmDotModeList, extendedSettings.alarmDotOn, 0, (boolean)!deviceInformation[ALARM_TYPE]););
        M_BOX(GP.LABEL("Ожидание", "", UI_LABEL_COLOR); GP.SELECT("extAlarmDotWait", alarmDotModeList, extendedSettings.alarmDotWait, 0, (boolean)!deviceInformation[ALARM_TYPE]););
        GP.BLOCK_END();
      );
    }
    else if (ui.uri("/climate")) { //микроклимат
      GP_PAGE_TITLE("Микроклимат");

      uint16_t heightSize = 500;
      if (climateGetPress()) heightSize = 300;

      GP.BLOCK_BEGIN(GP_THIN, "", "Микроклимат", UI_BLOCK_COLOR);
      GP_PLOT_STOCK_BEGIN(climateLocal);

      if (climateGetHum()) {
        GP_PLOT_STOCK_DARK("climateDataMain", climateNamesMain, climateDates, climateArrMain[0], climateArrMain[1], CLIMATE_BUFFER, 10, heightSize, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
      }
      else {
        GP_PLOT_STOCK_DARK("climateDataMain", climateNamesMain, climateDates, climateArrMain[0], NULL, CLIMATE_BUFFER, 10, heightSize, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
      }
      if (climateGetPress()) {
        GP_PLOT_STOCK_DARK("climateDataExt", climateNamesExt, climateDates, climateArrExt[0], NULL, CLIMATE_BUFFER, 0, heightSize, UI_BAR_PRESS_COLOR);
      }
      GP.BREAK();
      GP.BLOCK_END();

      GP.GRID_BEGIN();
      GP.BLOCK_BEGIN(GP_THIN, "", "Замер", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("Усреднение", "", UI_LABEL_COLOR); GP.SWITCH("climateAvg", settings.climateAvg, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("climateTime", settings.climateTime, 1, 60, 1, 0, UI_SPINNER_COLOR););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Датчики", UI_BLOCK_COLOR);
      String dataList = "";
      uint8_t dataAll = 0;
      for (uint8_t i = 0; i < 4; i++) {
        if (i) dataList += ',';
        dataList += sensDataList[i + (((i == 3) && sens.hum[3]) ? 1 : 0)];
        dataList += ": ";
        dataList += (sens.search & (0x01 << i)) ? String(sens.temp[i] / 10.0, 1) : "--";
        if (sens.search & (0x01 << i)) dataAll++;
      }
      M_BOX(GP.LABEL("Температура", "", UI_LABEL_COLOR); GP.SELECT("climateTemp", dataList, settings.climateType[0], 0, (boolean)(dataAll <= 1)););

      dataList = "";
      dataAll = 0;
      for (uint8_t i = 0; i < 4; i++) {
        if (i) dataList += ',';
        dataList += sensDataList[i + (((i == 3) && sens.hum[3]) ? 1 : 0)];
        dataList += ": ";
        dataList += (sens.hum[i]) ? String(sens.hum[i]) : "--";
        if (sens.hum[i]) dataAll++;
      }
      M_BOX(GP.LABEL("Влажность", "", UI_LABEL_COLOR); GP.SELECT("climateHum", dataList, settings.climateType[2], 0, (boolean)(dataAll <= 1)););
      GP.BLOCK_END();
      GP.GRID_END();

      GP.GRID_BEGIN();
      GP.BLOCK_BEGIN(GP_THIN, "", "Отображение", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("Данные в баре", "", UI_LABEL_COLOR); GP.SELECT("climateBar", "Датчик,Погода", settings.climateBar, 0, (boolean)(!weatherGetValidStatus()), true););
      M_BOX(GP.LABEL("Данные в часах", "", UI_LABEL_COLOR); GP.SELECT("climateSend", "Датчик,Погода", settings.climateSend, 0, (boolean)(!weatherGetValidStatus() || !deviceInformation[SENS_TEMP]), true););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Отправка", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("Тип датчика", "", UI_LABEL_COLOR); GP.NUMBER("", sensorsList, INT32_MAX, "", true););
      M_BOX(GP.LABEL("Отображение", "", UI_LABEL_COLOR); GP.SELECT("climateMainSens", climateGetSensList(), extendedSettings.tempMainSensor, 0, (boolean)(deviceInformation[BTN_EASY_MAIN_MODE])););
      GP.BLOCK_END();
      GP.GRID_END();
    }
    else if (ui.uri("/weather")) { //погода
      GP_PAGE_TITLE("Погода");

      GP.BLOCK_BEGIN(GP_THIN, "", "Погода на сутки", UI_BLOCK_COLOR);
      GP_PLOT_STOCK_BEGIN(climateLocal);
      GP_PLOT_STOCK_DARK("weatherDataMain", climateNamesMain, weatherDates, weatherArrMain[0], weatherArrMain[1], WEATHER_BUFFER, 10, 300, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
      GP_PLOT_STOCK_DARK("weatherDataExt", climateNamesExt, weatherDates, weatherArrExt[0], NULL, WEATHER_BUFFER, 10, 300, UI_BAR_PRESS_COLOR);
      GP.BREAK();
      GP.BLOCK_END();

      uint8_t time_start = (weatherDates[0] % 86400UL) / 3600UL;

      GP.BLOCK_BEGIN(GP_THIN, "", "Погода по часам", UI_BLOCK_COLOR);
      for (uint8_t i = 0; i < WEATHER_BUFFER; i++) {
        if (i) {
          GP.HR(UI_MENU_LINE_COLOR);
          GP.BREAK();
        }
        M_BOX(
          GP.LABEL(((time_start >= 10) ? String(time_start) : ('0' + String(time_start))) + ":00", "", GP_DEFAULT, 30);
          GP.LABEL(String(weatherArrMain[0][i] / 10.0, 1) + "°С", "", UI_BAR_TEMP_COLOR);
          GP.LABEL(String(weatherArrMain[1][i] / 10) + "%", "", UI_BAR_HUM_COLOR);
          GP.LABEL(String(weatherArrExt[0][i] / 10) + "mm.Hg", "", UI_BAR_PRESS_COLOR);
        );
        GP.BREAK();
        if (++time_start > 23) time_start = 0;
      }
      GP.BLOCK_END();
    }
    else if (ui.uri("/radio")) { //радиоприемник
      GP_PAGE_TITLE("Радио");

      updateList += ",radioVol,radioFreq,radioPower";

      GP.BLOCK_BEGIN(GP_THIN, "", "Радиоприёмник", UI_BLOCK_COLOR);
      GP.BLOCK_BEGIN(GP_DIV_RAW, "450px");
      if (!radioSvgImage) {
        M_BOX(M_BOX(GP_LEFT, GP.BUTTON_MINI("radioMode", "Часы ⇋ Радио", "", UI_RADIO_BACK_COLOR);); M_BOX(GP_RIGHT, GP.LABEL("Питание", "", UI_LABEL_COLOR); GP.SWITCH("radioPower", radioSettings.powerState, UI_RADIO_POWER_2_COLOR);););
      }
      M_BOX(GP_CENTER, GP_SLIDER_MAX("Громкость", "мин", "макс", "radioVol", radioSettings.volume, 0, 15, 1, 0, UI_RADIO_VOL_COLOR, false, true););
      M_BOX(GP_CENTER, GP_SLIDER_MAX("Частота", "", "", "radioFreq", radioSettings.stationsFreq / 10.0, 87.5, 108, 0.1, 1, UI_RADIO_FREQ_1_COLOR, false, true););
      GP.BLOCK_END();

      if (radioSvgImage) {
        GP.SEND("<style>#radioMode .i_mask{margin-left:5px;margin-right:4px;}\n"
                "#radioFreqDown .i_mask{margin-left:0px;margin-right:3px;}\n"
                "#radioFreqUp .i_mask{margin-left:3px;margin-right:0px;}</style>\n"
               );
        M_BOX(GP_CENTER,
              GP.ICON_FILE_BUTTON("radioMode", radioFsData[4], 40, UI_RADIO_BACK_COLOR);
              GP.ICON_FILE_BUTTON("radioSeekDown", radioFsData[0], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioFreqDown", radioFsData[1], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioFreqUp", radioFsData[2], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioSeekUp", radioFsData[3], 30, UI_RADIO_FREQ_2_COLOR);
              GP_CHECK_ICON("radioPower", radioFsData[5], radioSettings.powerState, 50, UI_RADIO_POWER_1_COLOR, UI_RADIO_POWER_2_COLOR);
             );
      }
      else {
        M_BOX(GP_CENTER,
              GP.BUTTON("radioSeekDown", "|◄◄", "", UI_RADIO_FREQ_2_COLOR, "100px");
              GP.BUTTON("radioFreqDown", "◄", "", UI_RADIO_FREQ_2_COLOR, "100px");
              GP.BUTTON("radioFreqUp", "►", "", UI_RADIO_FREQ_2_COLOR, "100px");
              GP.BUTTON("radioSeekUp", "►►|", "", UI_RADIO_FREQ_2_COLOR, "100px");
             );
      }
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Станции", UI_BLOCK_COLOR);
      GP.TABLE_BEGIN("20%,30%,20%,30%", GP_ALS(GP_RIGHT, GP_LEFT, GP_RIGHT, GP_LEFT));
      for (int i = 0; i < 10; i += 2) {
        M_TR(
          GP.BUTTON_MINI(String("radioCh/") + i, String("CH") + i, "", UI_RADIO_CHANNEL_COLOR),
          GP.NUMBER_F(String("radioSta/") + i, "Пусто", (radioSettings.stationsSave[i]) ? (radioSettings.stationsSave[i] / 10.0) : NAN, 1),
          GP.BUTTON_MINI(String("radioCh/") + (i + 1), String("CH") + (i + 1), "", UI_RADIO_CHANNEL_COLOR),
          GP.NUMBER_F(String("radioSta/") + (i + 1), "Пусто", (radioSettings.stationsSave[i + 1]) ? (radioSettings.stationsSave[i + 1] / 10.0) : NAN, 1)
        );
      }
      GP.TABLE_END();
      GP.BLOCK_END();

      GP.UPDATE_CLICK("radioSta/0,radioSta/1,radioSta/2,radioSta/3,radioSta/4,radioSta/5,radioSta/6,radioSta/7,radioSta/8,radioSta/9,radioFreq",
                      "radioSta/0,radioSta/1,radioSta/2,radioSta/3,radioSta/4,radioSta/5,radioSta/6,radioSta/7,radioSta/8,radioSta/9,radioCh/0,radioCh/1,radioCh/2,radioCh/3,radioCh/4,radioCh/5,radioCh/6,radioCh/7,radioCh/8,radioCh/9,");
    }
    else if (ui.uri("/update") && (otaUpdate || clockUpdate)) { //обновление ESP
      GP_PAGE_TITLE("Обновление");

      GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
      GP.SPAN("Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).", GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.BREAK();
      String formatText = "Поддерживаемые форматы файлов: ";
      if (clockUpdate) formatText += "hex";
      if (otaUpdate) {
        if (clockUpdate) formatText += ", ";
        formatText += "bin и bin.gz.";
        GP.SPAN("Файловую систему можно получить в Arduino IDE: Инструменты -> ESP8266 LittleFS Data Upload, в логе необходимо найти: [LittleFS] upload, файл находится по этому пути.", GP_CENTER, "", UI_INFO_COLOR); //описание
        GP.BREAK();
      }
      else formatText += ".";
      GP.SPAN(formatText, GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.BREAK();
      GP_HR_TEXT("Загрузить файлы", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (clockUpdate) {
        M_BOX(GP.LABEL("Прошивка часов", "", UI_LABEL_COLOR); GP.FILE_UPLOAD("updater", "", ".hex", UI_BUTTON_COLOR););
      }
      if (otaUpdate) {
        M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE("", UI_BUTTON_COLOR, true););
        M_BOX(GP.LABEL("Файловая система ESP", "", UI_LABEL_COLOR); GP.OTA_FILESYSTEM("", UI_BUTTON_COLOR, true););
      }
      GP.BLOCK_END();
    }
    else if (ui.uri("/information")) { //информация о системе
      GP_PAGE_TITLE("Об устройстве");

      GP.BLOCK_BEGIN(GP_THIN, "", "Системная информация", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("Уровень сигнала", "", UI_LABEL_COLOR); GP.LABEL("📶 " + String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + '%', "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Режим модема", "", UI_LABEL_COLOR); GP.LABEL(WiFi.getMode() == WIFI_AP ? "AP" : (WiFi.getMode() == WIFI_STA ? "STA" : "AP_STA"), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("MAC адрес", "", UI_LABEL_COLOR); GP.LABEL(WiFi.macAddress(), "", UI_INFO_COLOR););

      if (wifiStatus == WL_CONNECTED) {
        M_BOX(GP.LABEL("Маска подсети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.subnetMask().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("Шлюз", "", UI_LABEL_COLOR); GP.LABEL(WiFi.gatewayIP().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("SSID сети", "", UI_LABEL_COLOR); GP.LABEL(StrLengthConstrain(WiFi.SSID(), 12), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("IP сети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.localIP().toString(), "", UI_INFO_COLOR););
      }
      if (WiFi.getMode() != WIFI_STA) {
        M_BOX(GP.LABEL("SSID точки доступа", "", UI_LABEL_COLOR); GP.LABEL(StrLengthConstrain((settings.nameAp) ? (AP_SSID + String(" - ") + settings.name) : AP_SSID, 12), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("IP точки доступа", "", UI_LABEL_COLOR); GP.LABEL(WiFi.softAPIP().toString(), "", UI_INFO_COLOR););
      }

      GP.BREAK();
      GP_HR_TEXT("Память устройства", "", UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL("Фрагментировано(Heap)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getHeapFragmentation()) + '%', "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Свободно(Heap)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFreeHeap() / 1000.0, 3) + " kB", "", UI_INFO_COLOR););

      M_BOX(GP.LABEL("Всего(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFlashChipSize() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Занято(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getSketchSize() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Свободно(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFreeSketchSpace() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););

      GP.BREAK();
      GP_HR_TEXT("О системе", "", UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL("ID чипа", "", UI_LABEL_COLOR); GP.LABEL("0x" + String(ESP.getChipId(), HEX), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Частота процессора", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getCpuFreqMHz()) + F(" MHz"), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Циклов в секунду", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getCycleCount()), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Время работы", "", UI_LABEL_COLOR); GP.LABEL(getTimeFromMs(millis()), "", UI_INFO_COLOR););

      GP.BREAK();
      GP_HR_TEXT("Версия ПО", "", UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL("SDK", "", UI_LABEL_COLOR); GP.LABEL(ESP.getSdkVersion(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("CORE", "", UI_LABEL_COLOR); GP.LABEL(ESP.getCoreVersion(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("GyverPortal", "", UI_LABEL_COLOR); GP.LABEL(GP_VERSION, "", UI_INFO_COLOR););

      M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.LABEL(ESP_FIRMWARE_VERSION, "", UI_INFO_COLOR););
      if (deviceInformation[HARDWARE_VERSION]) {
        M_BOX(GP.LABEL("Прошивка часов", "", UI_LABEL_COLOR); GP.LABEL(String(deviceInformation[FIRMWARE_VERSION_1]) + "." + String(deviceInformation[FIRMWARE_VERSION_2]) + "." + String(deviceInformation[FIRMWARE_VERSION_3]), "", UI_INFO_COLOR););
      }
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Устройство", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("Имя", "", UI_LABEL_COLOR); GP.TEXT("extDeviceName", "Без названия", settings.name, "", 19););
      GP.BREAK();
      GP_HR_TEXT("Отображение", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Меню", "", UI_LABEL_COLOR); GP.SWITCH("extDeviceMenu", settings.nameMenu, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Префикс", "", UI_LABEL_COLOR); GP.SWITCH("extDevicePrefix", settings.namePrefix, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Постфикс", "", UI_LABEL_COLOR); GP.SWITCH("extDevicePostfix", settings.namePostfix, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Точка доступа", "", UI_LABEL_COLOR); GP.SWITCH("extDeviceAp", settings.nameAp, UI_SWITCH_COLOR););

      GP.BREAK();
      GP_HR_TEXT("Групповое управление", "", UI_LINE_COLOR, UI_HINT_COLOR);
      for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
        if (i) {
          GP.HR(UI_MENU_LINE_COLOR);
        }
        if (settings.multi[i][0] != '\0') {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("", "IP адрес", settings.multi[i], "", 20, "", true); GP.TEXT("", "Название", (settings.multi[i + 1][0] != '\0') ? settings.multi[i + 1] : settings.multi[i], "", 20, "", true););
            GP.BUTTON_MINI(String("extMultiDel/") + i, "Удалить", "", UI_BUTTON_COLOR, "115px!important", false, true);
          );
        }
        else {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("extMultiIp", "IP адрес", "", "", 15); GP.TEXT("extMultiName", "Название", "", "", 19););
            GP.BUTTON_MINI("extMultiAdd", "Добавить", "", UI_BUTTON_COLOR, "115px!important", false, true);
          );
          buffMultiIp[0] = '\0';
          buffMultiName[0] = '\0';
          break;
        }
      }

      String rtcStatus = "Не обнаружен...";
      GP.BREAK();
      GP_HR_TEXT("Модуль RTC", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (deviceInformation[DS3231_ENABLE]) {
        rtcStatus = "Подключен к часам";
      }
      else if (rtc_status != RTC_NOT_FOUND) {
        M_BOX(GP.LABEL("Коррекция", "", UI_LABEL_COLOR); GP.NUMBER("syncAging", "-128..127", rtc_aging););
        GP.UPDATE_CLICK("syncAging", "syncAging");
        rtcStatus = (rtc_status != RTC_ONLINE) ? "Батарея разряжена" : "Работает исправно";
      }
      M_BOX(GP.LABEL("Состояние", "", UI_LABEL_COLOR); GP.NUMBER("", rtcStatus, INT32_MAX, "", true););

      GP.BREAK();
      GP_HR_TEXT("Управление", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.BUTTON("resetButton", "Сброс настроек", "", UI_BUTTON_COLOR); GP.BUTTON("rebootButton", "Перезагрузка", "", UI_BUTTON_COLOR););
      GP.BLOCK_END();

      GP.CONFIRM("extReset", "Сбросить все настройки устройства?");
      GP.CONFIRM("extReboot", "Перезагрузить устройство?");

      GP.UPDATE_CLICK("extReset", "resetButton");
      GP.UPDATE_CLICK("extReboot", "rebootButton");
      GP.RELOAD_CLICK(String("extReset,extReboot,extDeviceMenu,extDevicePrefix,extDevicePostfix") + ((settings.nameMenu || settings.namePrefix || settings.namePostfix || (settings.multi[0][0] != '\0')) ? ",extDeviceName" : ""));
    }
    else { //подключение к роутеру
      GP_PAGE_TITLE("Сетевые настройки");

      GP.BLOCK_BEGIN(GP_THIN, "", "Локальная сеть WIFI", UI_BLOCK_COLOR);
      if ((wifiStatus == WL_CONNECTED) || wifiInterval) {
        GP.FORM_BEGIN("/network");
        if (wifiStatus == WL_CONNECTED) {
          GP.TEXT("", "", settings.ssid, "", 0, "", true);
          GP.BREAK();
          GP.TEXT("", "", WiFi.localIP().toString(), "", 0, "", true);
          GP.SPAN("Подключение установлено", GP_CENTER, "", UI_INFO_COLOR); //описание
        }
        else {
          GP.SPAN(getWifiState(), GP_CENTER, "syncNetwork", UI_INFO_COLOR); //описание
          updateList += ",syncNetwork";
        }

        GP.HR(UI_LINE_COLOR);
        if (ui.uri("/connection")) {
          GP.BUTTON_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR);
        }
        else {
          GP.SUBMIT("Отключиться", UI_BUTTON_COLOR);
        }
        GP.FORM_END();
      }
      else {
        if (wifiScanState < 0) wifiScanState = -wifiScanState;

        updateList += ",syncReload";
        GP.RELOAD("syncReload");

        GP.FORM_BEGIN("/connection");
        if (ui.uri("/manual")) {
          GP.TEXT("wifiSsid", "SSID", settings.ssid, "", 64);
          GP.BREAK();
          GP.PASS_EYE("wifiPass", "Пароль", settings.pass, "100%", 64);
          GP.BREAK();
          GP_TEXT_LINK("/network", "Список сетей", "net", UI_LINK_COLOR);
          GP.HR(UI_LINE_COLOR);
          GP.SEND("<div style='max-width:300px;justify-content:center' class='inliner'>\n");
          GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
          GP.BUTTON("extClear", "✕", "", (!settings.ssid[0] && !settings.pass[0]) ? GP_GRAY : UI_BUTTON_COLOR, "65px", (boolean)(!settings.ssid[0] && !settings.pass[0]), true);
          GP.SEND("</div>\n");
        }
        else {
          GP.SELECT("wifiNetwork", wifiScanList, 0, 0, (boolean)(wifiScanState != 1));
          GP.BREAK();
          GP.PASS_EYE("wifiPass", "Пароль", settings.pass, "100%", 64);
          GP.BREAK();
          GP_TEXT_LINK("/manual", "Ручной режим", "net", UI_LINK_COLOR);
          GP.HR(UI_LINE_COLOR);
          GP.SEND("<div style='max-width:300px;justify-content:center' class='inliner'>\n");
          if (wifiScanState != 1) GP.BUTTON("", "Подключиться", "", GP_GRAY, "", true);
          else GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
          GP.BUTTON("extScan", "<big><big>↻</big></big>", "", UI_BUTTON_COLOR, "65px", false, true);
          GP.SEND("</div>\n");
        }
        GP.FORM_END();
      }
      GP.BLOCK_END();

      if (ui.uri("/network")) { //сетевые настройки
        updateList += ",syncStatus,weatherStatus";

        GP.BLOCK_BEGIN(GP_THIN, "", "Сервер NTP", UI_BLOCK_COLOR);
        GP.TEXT("syncHost", "Хост", settings.host, "", 19);
        GP.BREAK();
        GP.SELECT("syncPer", String("Каждые 15 мин,Каждые 30 мин,Каждый 1 час") + ((settings.ntpDst) ? "" : ",Каждые 2 часа,Каждые 3 часа"), (settings.ntpDst && (settings.ntpTime > 2)) ? 2 : settings.ntpTime);
        GP.SPAN(getNtpState(), GP_CENTER, "syncStatus", UI_INFO_COLOR); //описание
        GP.HR(UI_LINE_COLOR);
        GP.BUTTON("syncCheck", "Синхронизировать сейчас", "", (!ntpGetRunStatus()) ? GP_GRAY : UI_BUTTON_COLOR, "", (boolean)(!ntpGetRunStatus()));
        GP.BLOCK_END();

        GP.UPDATE_CLICK("syncStatus", "syncCheck");

        GP.BLOCK_BEGIN(GP_THIN, "", "Регион погоды", UI_BLOCK_COLOR);
        GP.SELECT("weatherCity", String(weatherCityList) + ",- По координатам -", settings.weatherCity, 0, false, true);
        GP.BREAK();
        M_BOX(GP_CENTER, "209px",
              GP.NUMBER_F("weatherLat", "Широта", (settings.weatherCity < WEATHER_CITY_ARRAY) ? weatherCoordinatesList[0][settings.weatherCity] : settings.weatherLat, 4, "", (boolean)(settings.weatherCity < WEATHER_CITY_ARRAY));
              GP.NUMBER_F("weatherLon", "Долгота", (settings.weatherCity < WEATHER_CITY_ARRAY) ? weatherCoordinatesList[1][settings.weatherCity] : settings.weatherLon, 4, "", (boolean)(settings.weatherCity < WEATHER_CITY_ARRAY));
             );
        GP.SPAN(getWeatherState(), GP_CENTER, "weatherStatus", UI_INFO_COLOR); //описание
        GP.HR(UI_LINE_COLOR);
        GP.BUTTON("weatherUpdate", "Обновить погоду", "", (!weatherGetRunStatus()) ? GP_GRAY : UI_BUTTON_COLOR, "", (boolean)(!weatherGetRunStatus()));
        GP.BLOCK_END();

        GP.UPDATE_CLICK("weatherStatus", "weatherUpdate");
      }
    }

    GP.UPDATE(updateList);
    GP.UI_END(); //завершить окно панели управления
  }
  GP_BUILD_END();
}

void buildUpdater(bool UpdateEnd, const String & UpdateError) {
  GP.BUILD_BEGIN(UI_MAIN_THEME, 500);
  GP_FIX_SCRIPTS(); //фикс скрипта проверки онлайна
  GP_FIX_STYLES(); //фикс стилей страницы

  GP.PAGE_TITLE("Обновление");

  GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
  if (!UpdateEnd) {
    GP.SPAN("<b>Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).</b><br>Поддерживаемые форматы файлов bin и bin.gz.", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.OTA_FIRMWARE("", UI_BUTTON_COLOR, true); GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
  }
  else if (UpdateError.length()) {
    GP.SPAN("<big><b>Произошла ошибка при обновлении...</b></big><br><small>[" + UpdateError + "]</small>", GP_CENTER, "", GP_RED); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP_BUTTON_MINI_LINK("/ota_update", "⠀<big><big>↻</big></big>⠀", UI_BUTTON_COLOR); GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
  }
  else {
    GP.SPAN("<big><b>Выполняется обновление прошивки...</b></big>", GP_CENTER, "syncUpdate", UI_INFO_COLOR); //описание
    GP.SPAN("<small>Не выключайте устройство до завершения обновления!</small>", GP_CENTER, "syncWarn", GP_RED); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
    GP.UPDATE("syncUpdate,syncWarn");
  }
  GP.BLOCK_END();

  GP_BUILD_END();
}

void action() {
  if (ui.click()) {
    if (ui.clickSub("sync")) {
      if (ui.click("syncGmt")) {
        settings.ntpGMT = ui.getInt("syncGmt") - 12;
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("syncAuto", settings.ntpSync)) {
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("syncDst", settings.ntpDst)) {
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncTime")) {
        if (ntpGetSyncStatus()) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        else {
          mainTime = ui.getSystemTime(); //запросить время браузера
          mainDate = ui.getSystemDate(); //запросить дату браузера
          busSetComand(WRITE_TIME_DATE);
          if (rtc_status != RTC_NOT_FOUND) busSetComand(WRITE_RTC_TIME); //отправить время в RTC
        }
      }

      if (ui.click("syncHost")) {
        if (ui.getString("syncHost").length() > 0) strncpy(settings.host, ui.getString("syncHost").c_str(), 20); //копируем себе
        else strncpy(settings.host, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
        settings.host[19] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncPer")) {
        settings.ntpTime = ui.getInt("syncPer");
        if (settings.ntpTime > (sizeof(ntpSyncTime) - 1)) settings.ntpTime = sizeof(ntpSyncTime) - 1;
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncCheck")) {
        ntpRequest(); //запросить текущее время
        syncState = 0; //сбросили флаг синхронизации
      }

      if (ui.click("syncAging")) {
        if (rtc_status != RTC_NOT_FOUND) {
          rtc_aging = constrain(ui.getInt("syncAging"), -128, 127);
          busSetComand(WRITE_RTC_AGING);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("weather")) {
      if (ui.click("weatherCity")) {
        settings.weatherCity = ui.getInt("weatherCity");
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("weatherLat")) {
        settings.weatherLat = (ui.getString("weatherLat").length()) ? ui.getFloat("weatherLat") : NAN;
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("weatherLon")) {
        settings.weatherLon = (ui.getString("weatherLon").length()) ? ui.getFloat("weatherLon") : NAN;
        memory.update(); //обновить данные в памяти
      }

      if (ui.click("weatherUpdate")) {
        weatherCheck(); //запросить прогноз погоды
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("alarm")) {
      if (alarm.set) { //если режим настройки будильника
        if (ui.click("alarmVol")) {
          alarm_data[alarm.now][ALARM_DATA_VOLUME] = ui.getInt("alarmVol");
          busSetComand(WRITE_SELECT_ALARM, ALARM_VOLUME);
          if ((!alarm_data[alarm.now][ALARM_DATA_RADIO] || !deviceInformation[RADIO_ENABLE]) && alarm_data[alarm.now][ALARM_DATA_MODE]) busSetComand(WRITE_TEST_ALARM_VOL);
        }
        else if (playbackTimer > -1) playbackTimer = 0; //сбросили воспроизведение
        if (ui.click("alarmSoundType")) {
          alarm_data[alarm.now][ALARM_DATA_RADIO] = ui.getBool("alarmSoundType");
          busSetComand(WRITE_SELECT_ALARM, ALARM_RADIO);
          busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          busSetComand(WRITE_SELECT_ALARM, ALARM_VOLUME);
        }
        if (ui.click("alarmSound")) {
          if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_SOUND] = ui.getInt("alarmSound");
            if ((!deviceInformation[PLAYER_TYPE] || alarm_data[alarm.now][ALARM_DATA_VOLUME]) && (!deviceInformation[RADIO_ENABLE] || !radioSettings.powerState)) {
              busSetComand(WRITE_TEST_ALARM_SOUND);
              playbackTimer = 5;
            }
            busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          }
        }
        if (ui.click("alarmRadio")) {
          if (alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_STATION] = ui.getInt("alarmRadio");
            busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          }
        }
        if (ui.clickTime("alarmTime", alarmTime)) {
          alarm_data[alarm.now][ALARM_DATA_HOUR] = alarmTime.hour;
          alarm_data[alarm.now][ALARM_DATA_MINS] = alarmTime.minute;
          busSetComand(WRITE_SELECT_ALARM, ALARM_TIME);
        }
        if (ui.click("alarmMode")) {
          alarm_data[alarm.now][ALARM_DATA_MODE] = ui.getInt("alarmMode");
          busSetComand(WRITE_SELECT_ALARM, ALARM_MODE);
        }
        if (ui.clickSub("alarmDay")) {
          uint8_t day = constrain(ui.clickNameSub(1).toInt(), 1, 7);
          if (ui.getBool(String("alarmDay/") + day)) alarm_data[alarm.now][ALARM_DATA_DAYS] |= (0x01 << day);
          else alarm_data[alarm.now][ALARM_DATA_DAYS] &= ~(0x01 << day);
          busSetComand(WRITE_SELECT_ALARM, ALARM_DAYS);
        }
        if (ui.click("alarmDel") && !alarm.reload) {
          if (alarm.all > 1) {
            alarm.all--;
            busSetComand(DEL_ALARM, alarm.now);
            busSetComand(READ_ALARM_ALL);
          }
          else {
            alarm_data[alarm.now][ALARM_DATA_MODE] = 0;
            busSetComand(WRITE_SELECT_ALARM, ALARM_MODE);
          }
          alarm.set = 0;
        }
        if (ui.click("alarmBack")) {
          alarm.set = 0;
        }
      }
      else {
        if (ui.clickSub("alarmSet")) {
          alarm.now = constrain(ui.clickNameSub(1).toInt(), 0, MAX_ALARMS - 1);
          alarm.set = 1;
        }
        if (ui.click("alarmAdd") && !alarm.reload) {
          if (alarm.all < MAX_ALARMS) {
            alarm.now = alarm.all;
            alarm.all++;
            alarm.set = 2;
            busSetComand(NEW_ALARM);
            busSetComand(READ_ALARM_ALL);
          }
        }
        if (ui.click("alarmDis")) {
          busSetComand(WRITE_ALARM_DISABLE);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("fast")) {
      if (ui.clickInt("fastDot", fastSettings.dotMode)) {
        busSetComand(WRITE_FAST_SET, FAST_DOT_MODE);
      }
      if (ui.clickInt("fastFlip", fastSettings.flipMode)) {
        busSetComand(WRITE_FAST_SET, FAST_FLIP_MODE);
      }
      if (ui.clickInt("fastSecsFlip", fastSettings.secsMode)) {
        busSetComand(WRITE_FAST_SET, FAST_SECS_MODE);
      }
      if (ui.clickInt("fastBackl", fastSettings.backlMode)) {
        busSetComand(WRITE_FAST_SET, FAST_BACKL_MODE);
      }
      if (ui.click("fastColor")) {
        uint8_t color = constrain(ui.getInt("fastColor"), 0, 28);
        fastSettings.backlColor = (color > 25) ? (color + 227) : (color * 10);
        busSetComand(WRITE_FAST_SET, FAST_BACKL_COLOR);
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("main")) {
      if (ui.clickDate("mainDate", mainDate)) {
        busSetComand(WRITE_DATE);
      }
      if (ui.clickTime("mainTime", mainTime)) {
        busSetComand(WRITE_TIME);
      }
      if (ui.clickBool("mainTimeFormat", mainSettings.timeFormat)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_FORMAT);
      }
      if (ui.clickBool("mainGlitch", mainSettings.glitchMode)) {
        busSetComand(WRITE_MAIN_SET, MAIN_GLITCH_MODE);
      }

      if (ui.clickInt("mainIndiBrtDay", mainSettings.indiBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_D);
      }
      if (ui.clickInt("mainIndiBrtNight", mainSettings.indiBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_N);
      }
      if (ui.clickInt("mainBurnFlip", mainSettings.burnMode)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BURN_MODE);
      }
      if (ui.clickInt("mainBurnTime", mainSettings.burnTime)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BURN_TIME);
      }

      if (ui.click("mainAutoShow")) {
        if (ui.getBool("mainAutoShow")) mainSettings.autoShowTime &= 0x7F;
        else mainSettings.autoShowTime |= 0x80;
        busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
      }
      if (ui.click("mainAutoShowTime")) {
        if (mainSettings.autoShowTime & 0x80) {
          mainSettings.autoShowTime = ui.getInt("mainAutoShowTime") | 0x80;
        }
        else {
          mainSettings.autoShowTime = ui.getInt("mainAutoShowTime");
          busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
        }
      }
      if (ui.clickInt("mainAutoShowFlip", mainSettings.autoShowFlip)) {
        busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_FLIP);
      }
      if (ui.click("mainTempCorrect")) {
        mainSettings.tempCorrect = constrain((int16_t)(ui.getFloat("mainTempCorrect") * 10), -127, 127);
        busSetComand(WRITE_MAIN_SET, MAIN_TEMP_CORRECT);
      }

      if (ui.clickInt("mainTimeBrightS", mainSettings.timeBrightStart)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_BRIGHT_S);
      }
      if (ui.clickInt("mainTimeBrightE", mainSettings.timeBrightEnd)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_BRIGHT_E);
      }

      if (ui.clickInt("mainHourSoundS", mainSettings.timeHourStart)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_HOUR_S);
      }
      if (ui.clickInt("mainHourSoundE", mainSettings.timeHourEnd)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_HOUR_E);
      }

      if (ui.clickInt("mainSleepD", mainSettings.timeSleepDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_SLEEP_D);
      }
      if (ui.clickInt("mainSleepN", mainSettings.timeSleepNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_SLEEP_N);
      }

      if (ui.clickInt("mainDotBrtDay", mainSettings.dotBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_DOT_BRIGHT_D);
      }
      if (ui.clickInt("mainDotBrtNight", mainSettings.dotBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_DOT_BRIGHT_N);
      }

      if (ui.clickInt("mainBacklBrightDay", mainSettings.backlBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BACKL_BRIGHT_D);
      }
      if (ui.clickInt("mainBacklBrightNight", mainSettings.backlBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BACKL_BRIGHT_N);
      }

      if (ui.click("mainSound")) {
        mainSettings.knockSound = ui.getBool("mainSound");
        busSetComand(WRITE_MAIN_SET, MAIN_KNOCK_SOUND);
      }
      if (ui.clickInt("mainVoice", mainSettings.voiceSound)) {
        busSetComand(WRITE_MAIN_SET, MAIN_VOICE_SOUND);
        busSetComand(WRITE_TEST_MAIN_VOICE);
      }
      if (ui.clickInt("mainSoundVol", mainSettings.volumeSound)) {
        busSetComand(WRITE_MAIN_SET, MAIN_VOLUME_SOUND);
        busSetComand(WRITE_TEST_MAIN_VOL);
      }
      if (ui.click("mainHourTemp")) {
        if (ui.getBool("mainHourTemp")) mainSettings.hourSound |= 0x80;
        else mainSettings.hourSound &= ~0x80;
        busSetComand(WRITE_MAIN_SET, MAIN_HOUR_SOUND);
      }
      if (ui.click("mainHourSound")) {
        mainSettings.hourSound = (mainSettings.hourSound & 0x80) | constrain(ui.getInt("mainHourSound"), 0, 3);
        busSetComand(WRITE_MAIN_SET, MAIN_HOUR_SOUND);
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("ext")) {
      if (ui.clickSub("extShowMode")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t mode = ui.getInt(String("extShowMode/") + pos);
        extendedSettings.autoShowModes[pos] = mode;
        busSetComand(WRITE_EXTENDED_SHOW_MODE, pos);
      }
      if (ui.clickSub("extShowTime")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t time = ui.getInt(String("extShowTime/") + pos);
        extendedSettings.autoShowTimes[pos] = time;
        busSetComand(WRITE_EXTENDED_SHOW_TIME, pos);
      }

      if (ui.click("extAlarmTimeout")) {
        extendedSettings.alarmTime = ui.getInt("extAlarmTimeout");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_TIMEOUT);
      }
      if (ui.click("extAlarmWaitTime")) {
        extendedSettings.alarmWaitTime = ui.getInt("extAlarmWaitTime");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_WAIT);
      }
      if (ui.click("extAlarmSoundTime")) {
        extendedSettings.alarmSoundTime = ui.getInt("extAlarmSoundTime");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_TIMEOUT_SOUND);
      }
      if (ui.click("extAlarmDotOn")) {
        extendedSettings.alarmDotOn = ui.getInt("extAlarmDotOn");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_DOT_ON);
      }
      if (ui.click("extAlarmDotWait")) {
        extendedSettings.alarmDotWait = ui.getInt("extAlarmDotWait");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_DOT_WAIT);
      }

      if (ui.click("extDeviceName")) {
        strncpy(settings.name, ui.getString("extDeviceName").c_str(), 20); //копируем себе
        settings.name[19] = '\0'; //устанавливаем последний символ

        for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
          if (settings.multi[i][0] != '\0') {
            if (WiFi.localIP().toString().equals(settings.multi[i])) {
              strncpy(settings.multi[i + 1], settings.name, 20); //копируем себе
              settings.multi[i + 1][19] = '\0'; //устанавливаем последний символ
              break;
            }
          }
          else break;
        }

        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDeviceAp", settings.nameAp)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDeviceMenu", settings.nameMenu)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDevicePrefix", settings.namePrefix)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDevicePostfix", settings.namePostfix)) {
        memory.update(); //обновить данные в памяти
      }

      if (ui.clickSub("extMultiDel")) {
        uint8_t multiNum = constrain(ui.clickNameSub(1).toInt(), 0, ((MAX_CLOCK - 1) * 2));
        if (WiFi.localIP().toString().equals(settings.multi[multiNum])) {
          for (uint8_t i = 0; i < (MAX_CLOCK * 2); i++) settings.multi[i][0] = '\0';
        }
        else {
          for (; multiNum < ((MAX_CLOCK - 1) * 2); multiNum++) {
            strncpy(settings.multi[multiNum], settings.multi[multiNum + 2], 20); //копируем себе
          }
          settings.multi[((MAX_CLOCK - 1) * 2)][0] = '\0'; //устанавливаем последний символ
          settings.multi[((MAX_CLOCK - 1) * 2) + 1][0] = '\0'; //устанавливаем последний символ

          if (settings.multi[2][0] == '\0') {
            settings.multi[0][0] = '\0'; //устанавливаем последний символ
            settings.multi[1][0] = '\0'; //устанавливаем последний символ
          }
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("extMultiAdd")) {
        if (buffMultiIp[0] != '\0') { //если строка не пустая
          if (!WiFi.localIP().toString().equals(buffMultiIp)) { //если не собственный адрес
            for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
              if (settings.multi[i][0] == '\0') { //если ячейка не заполнена
                if (!i) { //если список пуст
                  strncpy(settings.multi[i], WiFi.localIP().toString().c_str(), 20); //копируем себе
                  settings.multi[i][19] = '\0'; //устанавливаем последний символ
                  strncpy(settings.multi[i + 1], settings.name, 20); //копируем себе
                  settings.multi[i + 1][19] = '\0'; //устанавливаем последний символ
                  i += 2; //сместили указатель
                }
                strncpy(settings.multi[i], buffMultiIp, 20); //копируем себе
                settings.multi[i][19] = '\0'; //устанавливаем последний символ
                strncpy(settings.multi[i + 1], buffMultiName, 20); //копируем себе
                settings.multi[i + 1][19] = '\0'; //устанавливаем последний символ
                memory.update(); //обновить данные в памяти
                break;
              }
              else if (String(settings.multi[i]).equals(buffMultiIp)) {
                break;
              }
            }
          }
        }
      }

      if (ui.click("extMultiIp")) {
        strncpy(buffMultiIp, ui.getString("extMultiIp").c_str(), 20); //копируем себе
        buffMultiIp[19] = '\0'; //устанавливаем последний символ
      }
      if (ui.click("extMultiName")) {
        strncpy(buffMultiName, ui.getString("extMultiName").c_str(), 20); //копируем себе
        buffMultiName[19] = '\0'; //устанавливаем последний символ
      }

      if (ui.click("extReset")) {
        if (ui.getBool("extReset")) {
          resetMainSettings(); //устанавливаем настройки по умолчанию
          memory.updateNow(); //обновить данные в памяти
          busRebootDevice(DEVICE_RESET);
        }
      }
      if (ui.click("extReboot")) {
        if (ui.getBool("extReboot")) {
          memory.updateNow(); //обновить данные в памяти
          busRebootDevice(DEVICE_REBOOT);
        }
      }

      if (ui.click("extClear")) {
        settings.ssid[0] = '\0'; //устанавливаем последний символ
        settings.pass[0] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("extScan")) {
        if (wifiScanState > 0) { //начинаем поиск
          wifiScanList = "Поиск...";
          wifiScanState = 127;
          wifiScanTimer = millis();
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("timer")) {
      if (!timer.mode || (timer.mode == 0x82)) {
        if (ui.clickSub("timerHour")) {
          switch (ui.clickNameSub(1).toInt()) {
            case 0: if (timer.hour < 17) timer.hour++; else timer.hour = 0; break; //час прибавить
            case 1: if (timer.mins < 59) timer.mins++; else timer.mins = 0; break; //минута прибавить
            case 2: if (timer.secs < 59) timer.secs++; else timer.secs = 0; break; //секунда прибавить
            case 3: if (timer.hour > 0) timer.hour--; else timer.hour = 17; break; //час убавить
            case 4: if (timer.mins > 0) timer.mins--; else timer.mins = 59; break; //минута убавить
            case 5: if (timer.secs > 0) timer.secs--; else timer.secs = 59; break; //секунда убавить
          }
        }
      }
      if (ui.clickSub("timerControl")) {
        switch (ui.clickNameSub(1).toInt()) {
          case 0: //запуск
            if (!timer.mode) {
              timer.count = (timer.hour * 3600) + (timer.mins * 60) + timer.secs;
              if (timer.count) {
                timer.mode = 2;
                if (timer.count) timer.time = timer.count;
                busSetComand(WRITE_TIMER_SET);
              }
              else timer.mode = 1;
              busSetComand(WRITE_TIMER_TIME);
            }
            else if (timer.mode & 0x80) {
              timer.mode &= 0x7F;
              busSetComand(WRITE_TIMER_STATE);
            }
            busSetComand(WRITE_TIMER_MODE);
            break;
          case 1: //остановка
            if (timer.mode) {
              timer.mode = 0;
              timer.count = 0;
              busSetComand(WRITE_TIMER_TIME);
              busSetComand(SET_UPDATE);
            }
            else {
              timer.hour = timer.mins = timer.secs = 0;
            }
            break;
          case 2: //пауза
            if (timer.mode && !(timer.mode & 0x80)) {
              timer.mode |= 0x80;
              busSetComand(WRITE_TIMER_STATE);
            }
            break;
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("climate")) {
      if (ui.clickInt("climateMainSens", extendedSettings.tempMainSensor)) {
        busSetComand(WRITE_EXTENDED_SHOW_SET, EXT_SHOW_SENS);
      }
      if (ui.clickInt("climateCorrectType", extendedSettings.tempCorrectSensor)) {
        busSetComand(WRITE_EXTENDED_SHOW_SET, EXT_SHOW_CORRECT);
      }

      if (ui.clickInt("climateBar", settings.climateBar)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickInt("climateSend", settings.climateSend)) {
        if (!settings.climateSend) climateSendData(); //отправить данные
        else weatherSendData(); //отправить данные
        memory.update(); //обновить данные в памяти
      }

      if (ui.clickInt("climateTime", settings.climateTime)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("climateAvg", settings.climateAvg)) {
        memory.update(); //обновить данные в памяти
      }

      if (ui.clickSub("climateTemp")) {
        uint8_t dataType = ui.getInt("climateTemp");
        if (sens.search & (0x01 << dataType)) {
          settings.climateType[0] = dataType;
          sens.mainTemp = sens.temp[settings.climateType[0]];
          climateSendData(); //отправить данные
          memory.update(); //обновить данные в памяти
        }
      }
      if (ui.clickSub("climateHum")) {
        uint8_t dataType = ui.getInt("climateHum");
        if (sens.hum[dataType]) {
          settings.climateType[2] = dataType;
          sens.mainHum = sens.hum[settings.climateType[2]];
          climateSendData(); //отправить данные
          memory.update(); //обновить данные в памяти
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("radio")) {
      if (ui.click("radioPower")) {
        radioSettings.powerState = ui.getBool("radioPower");
        busSetComand(WRITE_RADIO_POWER);
      }
      if (radioSettings.powerState) {
        if (ui.click("radioMode")) {
          busSetComand(WRITE_RADIO_MODE);
        }
        if (ui.click("radioSeekDown")) {
          busSetComand(RADIO_SEEK_DOWN);
        }
        if (ui.click("radioSeekUp")) {
          busSetComand(RADIO_SEEK_UP);
        }
        if (ui.click("radioFreqDown")) {
          busSetComand(READ_RADIO_FREQ);
          busSetComand(RADIO_FREQ_DOWN);
        }
        if (ui.click("radioFreqUp")) {
          busSetComand(READ_RADIO_FREQ);
          busSetComand(RADIO_FREQ_UP);
        }
        if (ui.click("radioVol")) {
          radioSettings.volume = constrain(ui.getInt("radioVol"), 0, 15);
          busSetComand(WRITE_RADIO_VOL);
        }
        if (ui.click("radioFreq")) {
          uint16_t stationFreq = (uint16_t)(ui.getFloat("radioFreq") * 10);
          radioSettings.stationsFreq = constrain(stationFreq, 870, 1080);
          busSetComand(WRITE_RADIO_FREQ);
        }
      }
      if (ui.clickSub("radioCh")) {
        uint8_t stationNum = constrain(ui.clickNameSub(1).toInt(), 0, 9);
        if (radioSettings.stationsSave[stationNum]) {
          radioSettings.stationsFreq = radioSettings.stationsSave[stationNum];
          if (!radioSettings.powerState) {
            radioSettings.powerState = true;
            busSetComand(WRITE_RADIO_POWER);
          }
          busSetComand(WRITE_RADIO_FREQ);
        }
      }
      if (ui.clickSub("radioSta")) {
        uint8_t stationNum = constrain(ui.clickNameSub(1).toInt(), 0, 9);
        uint16_t stationFreq = (uint16_t)(ui.getFloat(String("radioSta/") + stationNum) * 10);
        stationFreq = (stationFreq) ? constrain(stationFreq, 870, 1080) : 0;
        if (radioSettings.stationsSave[stationNum] != stationFreq) {
          radioSettings.stationsSave[stationNum] = stationFreq;
          busSetComand(WRITE_RADIO_STA, stationNum);
        }
      }
    }
  }
  /**************************************************************************/
  if (ui.form()) {
    if (!wifiInterval && (wifiStatus != WL_CONNECTED)) {
      if (ui.form("/connection")) {
        wifiInterval = 1; //устанавливаем интервал переподключения
        if (!ui.copyStr("wifiSsid", settings.ssid, 64)) { //копируем из строки
          int network = 0; //номер сети из списка
          if (ui.copyInt("wifiNetwork", network)) strncpy(settings.ssid, WiFi.SSID(network).c_str(), 64); //копируем из списка
          else wifiInterval = 0; //сбрасываем интервал переподключения
        }
        settings.ssid[63] = '\0'; //устанавливаем последний символ
        ui.copyStr("wifiPass", settings.pass, 64); //копируем пароль сети
        settings.pass[63] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
    }
    else if (ui.form("/network")) {
      wifiInterval = 0; //сбрасываем интервал переподключения
      wifiStatus = 255; //отключаемся от точки доступа
      settings.ssid[0] = '\0'; //устанавливаем последний символ
      settings.pass[0] = '\0'; //устанавливаем последний символ
      memory.update(); //обновить данные в памяти
    }
  }
  /**************************************************************************/
  if (ui.update()) {
    if (ui.updateSub("weather")) {
      if (ui.update("weatherStatus")) { //если было обновление
        ui.answer(getWeatherState());
      }

      if (ui.update("weatherTemp")) { //если было обновление
        ui.answer(String(sens.wetherTemp / 10.0, 1) + "°С");
      }
      if (ui.update("weatherHum")) { //если было обновление
        ui.answer(String(sens.wetherHum) + "%");
      }
      if (ui.update("weatherPress")) { //если было обновление
        ui.answer(String(sens.wetherPress) + "mm.Hg");
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("sync")) {
      if (ui.update("syncStatus")) { //если было обновление
        ui.answer(getNtpState());
      }

      if (ui.update("syncUpdate")) { //если было обновление
        ui.answer((busRebootFail()) ? " " : ((!busRebootState()) ? getUpdaterState() : "<big><b>Выполняется перезагрузка, подождите...</b></big>"));
      }
      if (ui.update("syncReboot") && !busRebootState()) { //если было обновление
        ui.answer((busRebootFail()) ? " " : "<big><b>Перезагрузка завершена!</b></big>");
      }
      if (ui.update("syncWarn") && !updaterState() && !busRebootState()) { //если было обновление
        ui.answer((busRebootFail()) ? "<big><b>Ошибка перезагрузки!</b></big>" : " ");
      }

      if (ui.update("syncNetwork")) { //если было обновление
        ui.answer(getWifiState());
      }
      if (ui.update("syncReload") && (wifiScanState < 0)) { //если было обновление
        ui.answer(1);
        wifiScanState = -wifiScanState;
      }

      if (ui.update("syncAging")) { //если было обновление
        ui.answer(rtc_aging);
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("bar")) {
      if (ui.update("barTime")) { //если было обновление
        ui.answer(encodeTime(mainTime));
        waitTimer = 0; //установили таймер ожидания
      }

      if (ui.update("barTemp")) { //если было обновление
        ui.answer(String(climateGetTempFloat(), 1) + "°С");
      }
      if (ui.update("barHum")) { //если было обновление
        ui.answer(String(climateGetHum()) + "%");
      }
      if (ui.update("barPress")) { //если было обновление
        ui.answer(String(climateGetPress()) + "mm.Hg");
      }

      if (ui.update("bar_clock")) { //если было обновление
        ui.answer((boolean)(clockState != 0));
      }
      if (ui.update("bar_rtc")) { //если было обновление
        ui.answer((boolean)(rtc_status != RTC_BAT_LOW));
      }
      if (ui.update("bar_ntp")) { //если было обновление
        ui.answer((boolean)(ntpGetSyncStatus()));
      }

      if (ui.update("bar_wifi")) { //если было обновление
        ui.answer(constrain(2 * (WiFi.RSSI() + 100), 0, 100));
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("main")) {
      if (ui.update("mainTimerState")) { //если было обновление
        ui.answer(getTimerState());
        if (!timer.mode) busSetComand(READ_TIMER_STATE);
      }
      if (ui.update("mainTimer")) { //если было обновление
        ui.answer(convertTimerTime());
      }
      if (ui.update("mainReload") && (alarm.reload >= 2)) { //если было обновление
        ui.answer(1);
        alarm.reload = 0;
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("ext")) {
      if (ui.update("extReset")) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("extReboot")) { //если было обновление
        ui.answer(1);
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("radio")) {
      if (ui.update("radioVol")) { //если было обновление
        ui.answer(constrain((int8_t)radioSettings.volume, 0, 15));
      }
      if (ui.update("radioFreq")) { //если было обновление
        ui.answer(constrain(radioSettings.stationsFreq, 870, 1080) / 10.0, 1);
      }
      if (ui.update("radioPower")) { //если было обновление
        ui.answer(radioSettings.powerState);
        busSetComand(READ_RADIO_POWER);
      }
      if (ui.updateSub("radioSta")) {
        uint8_t stationNum = constrain(ui.updateNameSub(1).toInt(), 0, 9);
        ui.answer((radioSettings.stationsSave[stationNum]) ? String(radioSettings.stationsSave[stationNum] / 10.0, 1) : " ");
      }
    }
  }
  /**************************************************************************/
  if (ui.upload()) {
    updaterSetStatus(UPDATER_UPL_ABORT); //установили флаг ошибки загрузки файла
    ui.saveFile(LittleFS.open("/update/firmware.hex", "w"));
  }
  if (ui.uploadEnd()) {
    if (ui.fileName().endsWith(".hex") || ui.fileName().endsWith(".HEX")) {
      updaterSetIdle(); //сбросили флаг ошибки
      Serial.println("Updater load file: " + ui.fileName());
      if (deviceInformation[HARDWARE_VERSION]) busSetComand(UPDATE_FIRMWARE);
      else updaterStart(); //запуск обновления
    }
    else {
      updaterSetStatus(UPDATER_NOT_HEX); //установили флаг ошибки расширения
      LittleFS.remove("/update/firmware.hex"); //удаляем файл
      Serial.println("Updater file extension error " + ui.fileName());
    }
  }
  if (ui.uploadAbort()) {
    updaterSetStatus(UPDATER_UPL_ABORT); //установили флаг ошибки загрузки файла
    LittleFS.remove("/update/firmware.hex"); //удаляем файл
    Serial.println F("Updater file upload abort");
  }
}

//-----------------------Получить состояние подключения wifi-----------------------------
String getWifiState(void) { //получить состояние подключения wifi
  String data = "<big><big>";
  if (!settings.ssid[0]) data += "Некорректное имя сети!";
  else {
    if (wifiStatus == WL_CONNECTED) data += "Подключено к \"";
    else if (!wifiInterval) data += "Не удалось подключиться к \"";
    else data += "Подключение к \"";
    data += String(settings.ssid);
    if ((wifiStatus == WL_CONNECTED) || !wifiInterval) data += "\"";
    else data += "\"...";
  }
  data += "</big></big>";
  return data;
}
//--------------------------Получить состояние загрузчика--------------------------------
String getUpdaterState(void) { //получить состояние загрузчика
  String data = "<big><b>";
  switch (updaterStatus()) {
    case UPDATER_IDLE: data += "Обновление завершено!"; break;
    case UPDATER_ERROR: data += "Сбой при загрузке прошивки!"; break;
    case UPDATER_TIMEOUT: data += "Время ожидания истекло!"; break;
    case UPDATER_NO_FILE: data += "Ошибка!<br><small>Файл повреждён или имеет неверный формат!</small>"; break;
    case UPDATER_NOT_HEX: data += "Ошибка!<br><small>Расширение файла не поддерживается!</small>"; break;
    case UPDATER_UPL_ABORT: data += "Ошибка!<br><small>Загрузка файла прервана!</small>"; break;
    default: data += (updaterProgress()) ? ("Загрузка прошивки..." + String(constrain(map(updaterProgress(), 0, 252, 0, 100), 0, 100)) + "%") : "Подключение..."; break;
  }
  data += "</b></big>";
  updaterSetIdle();
  return data;
}
//------------------------------Получить состояние ntp-----------------------------------
String getNtpState(void) { //получить состояние ntp
  String data = "";
  if (!ntpGetAttempts()) data += statusNtpList[ntpGetStatus()];
  else {
    data += "Попытка подключения[";
    data += ntpGetAttempts();
    data += "]...";
  }
  return data;
}
//----------------------------Получить состояние погоды-----------------------------------
String getWeatherState(void) { //получить состояние погоды
  String data = "";
  if (!weatherGetAttempts()) data += statusWeatherList[weatherGetStatus()];
  else {
    data += "Попытка запроса[";
    data += weatherGetAttempts();
    data += "]...";
  }
  return data;
}
//----------------------------Получить состояние таймера---------------------------------
String getTimerState(void) { //получить состояние таймера
  String data = statusTimerList[timer.mode & 0x03];
  if (((timer.mode & 0x03) == 2) && !timer.count) data += " - тревога";
  else if (timer.mode & 0x80) data += " - пауза";
  return data;
}
//------------------------Преобразовать время в формат ЧЧ:ММ:СС--------------------------
String convertTimerTime(void) { //преобразовать время в формат ЧЧ:ММ:СС
  String data = "";

  uint8_t buff = 0;
  if (timer.mode) buff = timer.count / 3600;
  else buff = timer.hour;
  if (buff < 10) data += '0';
  data += buff;
  data += ':';

  if (timer.mode) buff = (timer.count / 60) % 60;
  else buff = timer.mins;
  if (buff < 10) data += '0';
  data += buff;
  data += ':';

  if (timer.mode) buff = timer.count % 60;
  else buff = timer.secs;
  if (buff < 10) data += '0';
  data += buff;

  return data;
}
//--------------------------------------------------------------------
String encodeTime(GPtime data) {
  String str = "";

  if (mainSettings.timeFormat) {
    if (data.hour > 12) data.hour -= 12;
    else if (!data.hour) data.hour = 12;
  }

  str += data.hour / 10;
  str += data.hour % 10;
  str += ':';
  str += data.minute / 10;
  str += data.minute % 10;
  str += ':';
  str += data.second / 10;
  str += data.second % 10;

  return str;
}
//--------------------------------------------------------------------
String getTimeFromMs(uint32_t data) {
  data /= 1000;

  uint8_t second = data % 60;
  data /= 60;
  uint8_t minute = data % 60;
  data /= 60;
  uint16_t hour = data % 24;
  data /= 24;

  String str = "";

  str += data;
  str += ':';
  str += hour;
  str += ':';
  str += minute / 10;
  str += minute % 10;
  str += ':';
  str += second / 10;
  str += second % 10;

  return str;
}
//--------------------------------------------------------------------
String StrLengthConstrain(String data, uint8_t size) {
  if (data.length() > size) {
    data.remove(size);
    data += "…";
  }
  return data;
}
//--------------------------------------------------------------------
String climateGetSensList(void) {
  String str = "";

  if (deviceInformation[SENS_TEMP]) {
    str += "Датчик в часах,";
    if (weatherGetValidStatus() && settings.climateSend) str += "Данные о погоде";
    else str += "Датчик в есп";
  }
  else if (climateState != 0) str += "Датчик в есп,Данные о погоде";
  else str += "Данные о погоде,Датчик в есп";

  return str;
}
//--------------------------------------------------------------------
int16_t climateGetTemp(void) {
  return sens.mainTemp + mainSettings.tempCorrect;
}
float climateGetTempFloat(void) {
  return (climateGetTemp()) ? (climateGetTemp() / 10.0) : 0;
}
uint16_t climateGetPress(void) {
  return sens.mainPress;
}
uint8_t climateGetHum(void) {
  return sens.mainHum;
}
//--------------------------------------------------------------------
void climateSet(void) {
  static boolean firstStart;

  if (!firstStart) {
    firstStart = true;
    climateState = 0;

    if (sens.status) {
      if (!(sens.status & (0x01 << settings.climateType[0]))) {
        settings.climateType[0] = 0;
        for (uint8_t i = 0; i < 4; i++) {
          if (sens.status & (0x01 << i)) {
            settings.climateType[0] = i;
            climateState = 1;
            break;
          }
        }
      }
      else climateState = 1;
      sens.search = sens.status;

      if (!sens.press[settings.climateType[1]]) {
        settings.climateType[1] = 0;
        for (uint8_t i = 0; i < 4; i++) {
          if (sens.press[i]) {
            settings.climateType[1] = i;
            break;
          }
        }
      }

      if (!sens.hum[settings.climateType[2]]) {
        settings.climateType[2] = 0;
        for (uint8_t i = 0; i < 4; i++) {
          if (sens.hum[i]) {
            settings.climateType[2] = i;
            break;
          }
        }
      }

      sensorsList = "";
      memory.update(); //обновить данные в памяти
    }

    for (uint8_t i = 0; i < 4; i++) {
      if (sens.status & (0x01 << i)) {
        if (i) {
          if (sensorsList.length() > 0) sensorsList += "+";
          sensorsList += tempSensList[i];
        }
        else sensorsList += (sens.err) ? "Ошибка" : tempSensList[sens.type];
      }
      else if (!i && deviceInformation[SENS_TEMP]) sensorsList = "Ошибка";
    }
  }

  sens.mainTemp = sens.temp[settings.climateType[0]];
  sens.mainPress = sens.press[settings.climateType[1]];
  sens.mainHum = sens.hum[settings.climateType[2]];
}

void climateAdd(int16_t temp, int16_t hum, int16_t press, uint32_t unix) {
  if (climateDates[CLIMATE_BUFFER - 1] > unix) {
    climateDefault(temp, hum, press, unix);
  }
  else {
    GPaddInt(temp, climateArrMain[0], CLIMATE_BUFFER);
    if (hum) {
      GPaddInt(hum * 10, climateArrMain[1], CLIMATE_BUFFER);
    }
    if (press) {
      GPaddInt(press, climateArrExt[0], CLIMATE_BUFFER);
    }
    GPaddUnix(unix, climateDates, CLIMATE_BUFFER);
  }
}
//--------------------------------------------------------------------
void climateReset(void) {
  climateTempAvg = 0;
  climateHumAvg = 0;
  climatePressAvg = 0;
  climateCountAvg = 0;
}
//--------------------------------------------------------------------
void climateDefault(int16_t temp, int16_t hum, int16_t press, uint32_t unix) {
  for (uint8_t i = 0; i < CLIMATE_BUFFER; i++) {
    climateAdd(temp, hum, press, unix);
  }
}
//--------------------------------------------------------------------
void climateUpdate(void) {
  static int8_t firstStart = -1;

  uint32_t unixNow = GPunix(mainDate.year, mainDate.month, mainDate.day, mainTime.hour, mainTime.minute, 0, 0);

  if (firstStart < timeState) {
    firstStart = timeState;
    climateDefault(climateGetTemp(), climateGetHum(), climateGetPress(), unixNow);
    climateReset(); //сброс усреднения
  }
  else {
    climateTempAvg += climateGetTemp();
    climatePressAvg += climateGetPress();
    climateHumAvg += climateGetHum();

    if (++climateCountAvg >= settings.climateTime) {
      if (settings.climateAvg && (climateCountAvg > 1)) {
        if (climateTempAvg) climateTempAvg /= climateCountAvg;
        if (climatePressAvg) climatePressAvg /= climateCountAvg;
        if (climateHumAvg) climateHumAvg /= climateCountAvg;
        climateAdd(climateTempAvg, climateHumAvg, climatePressAvg, unixNow);
      }
      else climateAdd(climateGetTemp(), climateGetHum(), climateGetPress(), unixNow);
      climateReset(); //сброс усреднения
    }
  }
}
//--------------------------------------------------------------------
void climateSendData(void) {
  if (climateState != 0) { //если датчик обнаружен
    if (!deviceInformation[SENS_TEMP]) busSetComand(WRITE_SENS_DATA, SENS_DATA_MAIN); //отправить данные
    else if (!settings.climateSend || !weatherGetValidStatus()) busSetComand(WRITE_SENS_DATA, SENS_DATA_EXT); //отправить данные
  }
}
//--------------------------------------------------------------------
void weatherCheck(void) {
  if (settings.weatherCity < WEATHER_CITY_ARRAY) weatherSetCoordinates(settings.weatherCity); //установить город
  else weatherSetCoordinates(settings.weatherLat, settings.weatherLon); //установить координаты
  weatherSendRequest(); //запросить прогноз погоды
}
//--------------------------------------------------------------------
void weatherAveragData(void) {
  int8_t time_diff = mainTime.hour - ((weatherDates[0] % 86400UL) / 3600UL);
  if (time_diff < 0) time_diff += 24;
  uint8_t time_now = constrain(time_diff, 0, 23);
  uint8_t time_next = constrain(time_now + 1, 0, 23);

  if (!weatherGetGoodStatus() && (time_now > 12)) {
    weatherResetValidStatus(); //сбросили статус погоды
    sens.wetherTemp = 0x7FFF; //сбросили температуру погоды
    sens.wetherHum = 0; //сбросили влажность погоды
    sens.wetherPress = 0; //сбросили давление погоды
  }
  else {
    sens.wetherTemp = map(mainTime.minute, 0, 59, weatherArrMain[0][time_now], weatherArrMain[0][time_next]); //температура погоды
    sens.wetherHum = map(mainTime.minute, 0, 59, weatherArrMain[1][time_now], weatherArrMain[1][time_next]) / 10; //влажность погоды
    sens.wetherPress = map(mainTime.minute, 0, 59, weatherArrExt[0][time_now], weatherArrExt[0][time_next]) / 10; //давление погоды
  }

  weatherSendData(); //отправить данные
}
//--------------------------------------------------------------------
void weatherSendData(void) {
  if (!climateState && !deviceInformation[SENS_TEMP]) busSetComand(WRITE_WEATHER_DATA, SENS_DATA_MAIN); //отправить данные
  else if (!climateState || !deviceInformation[SENS_TEMP] || settings.climateSend) busSetComand(WRITE_WEATHER_DATA, SENS_DATA_EXT); //отправить данные
}
//--------------------------------------------------------------------
boolean checkFsData(const char** data, int8_t size) {
  File file;
  while (size > 0) {
    size--;
    file = LittleFS.open(data[size], "r");
    if (!file) {
      Serial.print(data[size]);
      Serial.println F(" not found");
      return false;
    }
    else file.close();
  }
  return true;
}
//--------------------------------------------------------------------
void timeUpdate(void) {
  if ((millis() - secondsTimer) >= 1000) {
    if (!secondsTimer) secondsTimer = millis();
    else { //счет времени
      if (++mainTime.second > 59) { //секунды
        mainTime.second = 0; //сбросили секунды
        if (++mainTime.minute > 59) { //минуты
          mainTime.minute = 0; //сбросили минуты
          if (++mainTime.hour > 23) { //часы
            mainTime.hour = 0; //сбросили часы
            if (++mainDate.day > maxDays(mainDate.year, mainDate.month)) { //дата
              mainDate.day = 1; //сбросили день
              if (++mainDate.month > 12) { //месяц
                mainDate.month = 1; //сбросили месяц
                if (++mainDate.year > 2099) { //год
                  mainDate.year = 2000; //сбросили год
                }
              }
            }
          }
          if (weatherGetRunStatus()) weatherCheck(); //запросить прогноз погоды
        }
        if (weatherGetValidStatus()) weatherAveragData(); //усреднить показания погоды

        if ((rtc_status != RTC_NOT_FOUND) && !(mainTime.minute % 15) && !settings.ntpSync) busSetComand(READ_RTC_TIME); //отправить время в RTC
        else busSetComand(READ_TIME_DATE, 0); //прочитали время из часов

        if (settings.ntpSync) {
          if (!settings.ntpDst) {
            if (!syncNtpTimer) {
              syncNtpTimer = ntpSyncTime[settings.ntpTime];
              ntpRequest(); //запросить время с ntp сервера
            }
            else syncNtpTimer--;
          }
          else {
            if (!(mainTime.minute % ntpSyncTime[(settings.ntpDst && (settings.ntpTime > 2)) ? 2 : settings.ntpTime])) {
              ntpRequest(); //запросить время с ntp сервера
            }
          }
        }
      }
      if (climateState != 0) {
        if (!climateTimer) {
          climateTimer = 59;
          sens.status = 0;
          if (deviceInformation[SENS_TEMP]) busSetComand(WRITE_CHECK_SENS);
          else sens.update |= SENS_EXT;
        }
        else climateTimer--;
      }
      secondsTimer += 1000; //прибавили секунду
    }
    if (timer.mode) busSetComand(READ_TIMER_TIME);
    if (!waitTimer) { //если пришло время опросить статус часов
      waitTimer = 4; //установили таймер ожидания
      if (clockState > 0) clockState--; //если есть попытки подключения
      else if (clockState < 0) clockState = 3; //иначе первый запрос состояния
      busSetComand(READ_STATUS); //запрос статуса часов
    }
    else waitTimer--;
    if (playbackTimer > -1) {
      if (!playbackTimer) busSetComand(WRITE_STOP_SOUND); //остановка воспроизведения
      playbackTimer--;
    }
#if STATUS_LED == 1
    if ((wifiStatus != WL_CONNECTED) && wifiInterval) digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
#elif STATUS_LED == 2
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
#endif
  }

  if (ntpUpdate()) { //обработка ntp
    if (settings.ntpSync || !syncState) {
      syncNtpTimer = ntpSyncTime[settings.ntpTime];
      busSetComand(SYNC_TIME_DATE); //проверить и отправить время ntp сервера
    }
  }

  if (weatherUpdate()) {
    weatherGetUnixData(weatherDates, WEATHER_BUFFER);
    weatherGetParseData(weatherArrMain[0], WEATHER_GET_TEMP, WEATHER_BUFFER);
    weatherGetParseData(weatherArrMain[1], WEATHER_GET_HUM, WEATHER_BUFFER);
    weatherGetParseData(weatherArrExt[0], WEATHER_GET_PRESS, WEATHER_BUFFER);
    if (weatherGetValidStatus()) weatherAveragData();
  }
}

void deviceUpdate(void) {
  if (deviceStatus) { //если статус обновился
    for (uint8_t i = 0; i < STATUS_MAX_DATA; i++) { //проверяем все флаги
      if (deviceStatus & 0x01) { //если флаг установлен
        switch (i) { //выбираем действие
          case STATUS_UPDATE_MAIN_SET: busSetComand(READ_MAIN_SET); break;
          case STATUS_UPDATE_FAST_SET: busSetComand(READ_FAST_SET); break;
          case STATUS_UPDATE_RADIO_SET: busSetComand(READ_RADIO_SET); break;
          case STATUS_UPDATE_ALARM_SET: busSetComand(READ_ALARM_ALL); break;
          case STATUS_UPDATE_TIME_SET: busSetComand(READ_TIME_DATE, 1); break;
          case STATUS_UPDATE_SENS_DATA: if (deviceInformation[SENS_TEMP]) busSetComand(READ_SENS_DATA); break;
        }
      }
      deviceStatus >>= 1; //сместили буфер флагов
    }
    deviceStatus = 0; //сбрасываем все флаги
  }

  switch (sens.update) { //если все датчики опрошены
    case SENS_EXT:
      sens.update = 0; //сбрасываем флаги опроса
      busSetComand(CHECK_INTERNAL_AHT);
      busSetComand(CHECK_INTERNAL_SHT);
      busSetComand(CHECK_INTERNAL_BME);
      break;
    case (SENS_AHT | SENS_SHT | SENS_BME):
      sens.update = 0; //сбрасываем флаги опроса
      climateSet(); //установить показания датчиков
      climateUpdate(); //обновляем показания графиков
      climateSendData(); //отправить данные
      break;
  }
}
//--------------------------------------------------------------------
void wifiUpdate(void) {
  static uint32_t timerWifi = millis(); //таймер попытки подключения к wifi

  if ((wifiScanState == 127) && (millis() - wifiScanTimer) >= 100) { //если необходимо начать поиск
    wifiScanState = 0; //сбрасываем статус
    WiFi.scanNetworksAsync(wifiScanResult); //начинаем поиск
  }

  if (wifiStatus != WiFi.status()) { //если изменился статус
    if (wifiStatus == 255) { //если нужно отключиться
      Serial.println F("Wifi disconnecting...");
      ntpStop(); //остановили ntp
      weatherDisconnect(); //отключились от сервера погоды
      WiFi.disconnect(); //отключаем wifi
      if (WiFi.getMode() != WIFI_AP_STA) wifiStartAP(); //включаем точку доступа
    }
    wifiStatus = WiFi.status();
    switch (wifiStatus) {
      case WL_CONNECTED:
        timerWifi = millis(); //сбросили таймер
        wifiInterval = 300000; //устанавливаем интервал отключения точки доступа
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, HIGH); //выключаем индикацию
#endif
        ntpStart(); //запустить ntp
        weatherCheck(); //запросить прогноз погоды

        Serial.print F("Wifi connected, IP address: ");
        Serial.println(WiFi.localIP());
        break;
      case WL_IDLE_STATUS:
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
        Serial.println F("Wifi idle status");
        break;
      default:
        if ((wifiStatus == WL_DISCONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL)) {
          timerWifi = millis(); //сбросили таймер
          if (wifiStatus == WL_NO_SSID_AVAIL) wifiInterval = 30000; //устанавливаем интервал переподключения
          else wifiInterval = 5000; //устанавливаем интервал переподключения
          WiFi.disconnect(); //отключаем wifi
        }
        else {
          wifiInterval = 0; //сбрасываем интервал переподключения
#if STATUS_LED == 1
          digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
          Serial.println F("Wifi connect error...");
        }
        ntpStop(); //остановили ntp
        weatherDisconnect(); //отключились от сервера погоды
        break;
    }
  }

  if (wifiInterval && ((millis() - timerWifi) >= wifiInterval)) {
    if (wifiStatus == WL_CONNECTED) { //если подключены
      wifiInterval = 0; //сбрасываем интервал переподключения
      WiFi.mode(WIFI_STA); //отключили точку доступа
      Serial.println F("Wifi access point disabled");
    }
    else { //иначе новое поключение
      wifiStatus = WiFi.begin(settings.ssid, settings.pass); //подключаемся к wifi
      if (wifiStatus != WL_CONNECT_FAILED) {
        timerWifi = millis(); //сбросили таймер
        wifiInterval = 30000; //устанавливаем интервал переподключения
        Serial.print F("Wifi connecting to \"");
        Serial.print(settings.ssid);
        Serial.println F("\"...");
      }
      else {
        wifiInterval = 0; //сбрасываем интервал
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
        Serial.println F("Wifi connection failed, wrong settings");
      }
    }
  }
}
//--------------------------------------------------------------------
void wifiScanResult(int networksFound) {
  wifiScanList = "";
  if (networksFound) {
    wifiScanState = -1;
    for (int i = 0; i < networksFound; i++) {
      if (i) wifiScanList += ',';
      wifiScanList += WiFi.SSID(i);
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE) wifiScanList += " 🔒";
    }
  }
  else {
    wifiScanState = -2;
    wifiScanList = "Нет сетей";
  }
}
//--------------------------------------------------------------------
void wifiStartAP(void) {
  //настраиваем режим работы
  WiFi.mode(WIFI_AP_STA);
  Serial.println F("");

  //настраиваем точку доступа
  IPAddress local(AP_IP);
  IPAddress subnet(255, 255, 255, 0);

  //задаем настройки сети
  WiFi.softAPConfig(local, local, subnet);

  //запускаем точку доступа
  if (!WiFi.softAP((settings.nameAp) ? (AP_SSID + String(" - ") + settings.name) : AP_SSID, AP_PASS, AP_CHANNEL)) Serial.println F("Wifi access point start failed, wrong settings");
  else {
    Serial.print F("Wifi access point enable, [ ssid: ");
    Serial.print((settings.nameAp) ? (AP_SSID + String(" - ") + settings.name) : AP_SSID);
    if (AP_PASS[0] != '\0') {
      Serial.print F(" ][ pass: ");
      Serial.print(AP_PASS);
    }
    else Serial.print F(" ][ open ");
    Serial.print F(" ][ ip: ");
    Serial.print(WiFi.softAPIP());
    Serial.println F(" ]");
  }

  //начинаем поиск сетей
  WiFi.scanNetworksAsync(wifiScanResult);
}
//--------------------------------------------------------------------
void initFileSystemData(void) {
  if (!LittleFS.begin()) Serial.println F("File system error");
  else {
    Serial.println F("File system init");

    if (checkFsData(climateFsData, 1)) {
      climateLocal = true; //работаем локально
      Serial.println F("Script file found");
    }
    if (checkFsData(alarmFsData, 3)) {
      alarmSvgImage = true; //работаем локально
      Serial.println F("Alarm svg files found");
    }
    if (checkFsData(timerFsData, 5)) {
      timerSvgImage = true; //работаем локально
      Serial.println F("Timer svg files found");
    }
    if (checkFsData(radioFsData, 6)) {
      radioSvgImage = true; //работаем локально
      Serial.println F("Radio svg files found");
    }

    if (LittleFS.remove("/update/firmware.hex")) Serial.println F("Clock update file remove"); //удаляем файл прошивки

    FSInfo fs_info;
    LittleFS.info(fs_info);
    if ((fs_info.totalBytes - fs_info.usedBytes) < 120000) {
      clockUpdate = false; //выключаем обновление
      Serial.println F("Clock update disable, running out of memory");
    }
    else Serial.println F("Clock update enable");
  }

  if (ESP.getFreeSketchSpace() < ESP.getSketchSize()) {
    otaUpdate = false; //выключаем обновление
    Serial.println F("OTA update disable, running out of memory");
  }
  else Serial.println F("OTA update enable");
}
//--------------------------------------------------------------------
void resetMainSettings(void) {
  strncpy(settings.host, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
  settings.host[19] = '\0'; //устанавливаем последний символ

  settings.nameAp = DEFAULT_NAME_AP; //установить отображение имени после названия точки доступа wifi по умолчанию
  settings.nameMenu = DEFAULT_NAME_MENU; //установить отображение имени в меню по умолчанию
  settings.namePrefix = DEFAULT_NAME_PREFIX; //установить отображение имени перед названием вкладки по умолчанию
  settings.namePostfix = DEFAULT_NAME_POSTFIX; //установить отображение имени после названием вкладки по умолчанию

  strncpy(settings.name, DEFAULT_NAME, 20); //установить имя по умолчанию
  settings.name[19] = '\0'; //устанавливаем последний символ

  settings.weatherCity = DEFAULT_WEATHER_CITY; //установить город по умолчанию
  settings.weatherLat = NAN; //установить широту по умолчанию
  settings.weatherLon = NAN; //установить долготу по умолчанию

  for (uint8_t i = 0; i < sizeof(settings.climateType); i++) settings.climateType[i] = 0; //сбрасываем типы датчиков
  settings.climateBar = DEFAULT_CLIMATE_BAR; //установить режим по умолчанию
  settings.climateSend = DEFAULT_CLIMATE_SEND; //установить режим по умолчанию
  settings.climateTime = DEFAULT_CLIMATE_TIME; //установить период по умолчанию
  settings.climateAvg = DEFAULT_CLIMATE_AVG; //установить усреднение по умолчанию
  settings.ntpGMT = DEFAULT_GMT; //установить часовой по умолчанию
  settings.ntpSync = DEFAULT_SYNC; //выключаем авто-синхронизацию
  settings.ntpDst = DEFAULT_DST; //установить учет летнего времени по умолчанию
  settings.ntpTime = DEFAULT_NTP_TIME; //установить период по умолчанию
  if (settings.ntpTime > (sizeof(ntpSyncTime) - 1)) settings.ntpTime = sizeof(ntpSyncTime) - 1;
}

void setup() {
  //инициализация индикатора
#if STATUS_LED > 0
  pinMode(LED_BUILTIN, OUTPUT);
#if STATUS_LED < 2
  digitalWrite(LED_BUILTIN, HIGH);
#else
  digitalWrite(LED_BUILTIN, LOW);
#endif
#endif

  //инициализация шины
  pinMode(TWI_SDA_PIN, INPUT_PULLUP);
  pinMode(TWI_SCL_PIN, INPUT_PULLUP);
  twi_init();

  Serial.begin(115200);
  Serial.println F("");
  Serial.println F("Startup...");
  Serial.print F("Firmware version ");
  Serial.print F(ESP_FIRMWARE_VERSION);
  Serial.println F("...");

  //инициализация файловой системы
  initFileSystemData();

  //сбрасываем настройки группового управления
  for (uint8_t i = 0; i < (MAX_CLOCK * 2); i++) settings.multi[i][0] = '\0';

  //устанавливаем указатель будильниака
  alarm.now = 0;

  //восстанавливаем настройки сети
  strncpy(settings.ssid, WiFi.SSID().c_str(), 64);
  settings.ssid[63] = '\0';
  strncpy(settings.pass, WiFi.psk().c_str(), 64);
  settings.pass[63] = '\0';

  //устанавливаем настройки по умолчанию
  resetMainSettings();

  //читаем настройки из памяти
  EEPROM.begin(memory.blockSize());
  memory.begin(0, 0xBD);

  //настраиваем wifi
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  wifiStartAP();

  //подключаем конструктор и запускаем веб интерфейс
  ui.attachBuild(build);
  ui.attach(action);
  ui.start();

  //настраиваем обновление без пароля
  if (otaUpdate) {
    ui.enableOTA();
    ui.OTA.attachUpdateBuild(buildUpdater);
  }
  ui.downloadAuto(true);
  ui.uploadAuto(false);

  //остановили ntp
  ntpStop();

  //отключились от сервера погоды
  weatherDisconnect();

  //запрашиваем настройки часов
  busSetComand(READ_MAIN_SET);
  busSetComand(READ_FAST_SET);
  busSetComand(READ_EXTENDED_SET);
  busSetComand(READ_RADIO_SET);
  busSetComand(READ_ALARM_ALL);
  busSetComand(READ_SENS_INFO);
  busSetComand(READ_TIME_DATE, 0);
  busSetComand(READ_DEVICE);

  busSetComand(WRITE_RTC_INIT);

  busTimerSetInterval(1500);
}

void loop() {
  wifiUpdate(); //обработка статусов wifi

  if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) { //если связь с часами установлена
    timeUpdate(); //обработка времени
    deviceUpdate(); //обработка статусов устройства
  }

  if (!updaterFlash()) busUpdate(); //обработка шины
  else if (updaterRun()) busRebootDevice(SYSTEM_REBOOT); //загрузчик прошивки

  ui.tick(); //обработка веб интерфейса
  memory.tick(); //обработка еепром
}
