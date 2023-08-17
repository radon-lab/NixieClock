//Основные настройки
#define DEFAULT_GMT 3 //часовой пояс по умолчанияю(-12..12)

#define DEFAULT_CLIMATE_AVG 1 //усреднение климатических данных на графике за указанный интервал(0 - добавлять текущие данные | 1 - добавлять усредненные данные)
#define DEFAULT_CLIMATE_TIME 30 //интервал отображения климатических данных на графике(1..60)(мин)
#define CLIMATE_BUFFER 100 //размер буфера памяти климатических данных на графике(50..250)

#define AP_IP 192, 168, 1, 1 //адрес точки доступа
#define AP_SSID "NixieClockAP" //имя точки доступа
#define AP_PASS "1234567890" //пароль точки доступа

//Настройки подключения
//   Плата:                                     SDA        SCL
//   ESP8266................................... GPIO4      GPIO5
//   ESP8266 ESP-01............................ GPIO0/D5   GPIO2/D3
//   NodeMCU 1.0, WeMos D1 Mini................ GPIO4/D2   GPIO5/D1

#define TWI_SDA_PIN D1 //пин SDA шины I2C
#define TWI_SCL_PIN D2 //пин SCL шины I2C

//Дополнительно
#define CLOCK_ADDRESS 127 //адрес шины часов
#define ESP_FIRMWARE_VERSION "1.0.2" //версия прошивки модуля esp
