//Основные настройки
#define AP_IP 192, 168, 4, 4 //адрес точки доступа(по умолчанию - 192, 168, 4, 4)
#define AP_SSID "WirelessSensor" //имя точки доступа(минимум 1 символ)(без пробелов)
#define AP_PASS "1234567890" //пароль точки доступа(минимум 8 символов)("" - открытая сеть)
#define AP_CHANNEL 5 //канал точки доступа(1..13)

#define SEND_DATA_PERIOD 1 //период отправки данных по умолчанию(0 - 1(мин) | 1 - 5(мин) | 2 - 10(мин) | 3 - 15(мин) | 4 - 30(мин) | 5 - 60(мин))

#define SETTINGS_MODE_TIME 90 //время работы точки доступа в режиме настройки(20..240)(сек)

#define BAT_VOLTAGE_MIN 2500 //минимальное напряжение для расчета заряда батареи(2500..3000)(мВ)
#define BAT_VOLTAGE_MAX 3000 //максимальное напряжение для расчета заряда батареи(3000..3500)(мВ)
#define BAT_VOLTAGE_CORRECT 100 //коррекция напряжения для расчета заряда батареи(-200..200)(мВ)

#define STATUS_LED 1 //индикация состояния(0 - выключить | 1 - включить)

//   Плата:                                     SDA        SCL
//   ESP-01.................................... GPIO0      GPIO2
//   ESP8266................................... GPIO4      GPIO5
//   NodeMCU 1.0, WeMos D1 Mini................ GPIO4(D2)  GPIO5(D1)

//Настройки подключения
#define TWI_SDA_PIN 4 //пин SDA шины I2C
#define TWI_SCL_PIN 5 //пин SCL шины I2C

//Настройки цветовой схемы, доступные цвета по умолчанию(GP_RED, GP_RED_B, GP_PINK, GP_PINK_B, GP_VIOL, GP_VIOL_B, GP_BLUE, GP_BLUE_B, GP_CYAN, GP_CYAN_B, GP_GREEN, GP_GREEN_B, GP_YELLOW, GP_YELLOW_B, GP_ORANGE, GP_ORANGE_B, GP_GRAY, GP_GRAY_B, GP_BLACK, GP_WHITE) или цвет в формате PSTR("#rrggbb")
#define UI_BUTTON_COLOR "#e67b09" //цвет кнопок веб интерфейса
#define UI_BLOCK_COLOR "#e67b09" //цвет блока веб интерфейса
#define UI_LABEL_COLOR "#e9c46a" //цвет надписей веб интерфейса
#define UI_LINE_COLOR "#e67b09" //цвет горизонтальных линий веб интерфейса
#define UI_HINT_COLOR "#e67b09" //цвет основных подсказок веб интерфейса
#define UI_INFO_COLOR "#07b379" //цвет информационных подсказок веб интерфейса
#define UI_LINK_COLOR "#07b379" //цвет текста ссылки веб интерфейса

#define UI_MENU_COLOR "#e67b09" //цвет меню веб интерфейса

#define UI_BAR_BATTERY_COLOR GP_WHITE //цвет заряда батареи в статус баре
#define UI_BAR_TEMP_COLOR GP_GREEN //цвет температуры в статус баре
#define UI_BAR_HUM_COLOR GP_BLUE //цвет влажности в статус баре
#define UI_BAR_PRESS_COLOR GP_PINK //цвет давления в статус баре
#define UI_BAR_LINE_COLOR GP_GRAY //цвет горизонтальной линии статус бара

//Дополнительно
#define DEBUG_MODE 0 //активация режима отладки прошивки(0 - выкл | 1 - вкл)
#define SLEEP_MODE 1 //активация режима глубокого сна датчика(0 - выкл | 1 - вкл)

#define MAX_CLOCK 4 //максимум часов для отправки данных(2..5)

#define UDP_SEND_SIZE 16 //размер пакета данных(16)
#define UDP_LOCAL_PORT 888 //локальный порт udp(888)
#define UDP_CLOCK_PORT 8888 //порт udp часов(8888)

#define UDP_WRITE_CMD 0xFF //команда отправки данных(0xFF)
#define UDP_FOUND_CMD 0xCC //команда обнаружения датчика(0xCC)
#define UDP_ANSWER_CMD 0xAA //команда ожидания ответа(0xAA)

#define UDP_ANSWER_SIZE 1 //размер пакета ответа(1)
#define UDP_ANSWER_WAIT_TIME 1000 //время ожидания ответа(100..2000)(мс)

#define ESP_FIRMWARE_VERSION "1.1.8" //версия прошивки модуля esp
