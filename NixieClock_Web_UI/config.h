//Основные настройки
#define DEFAULT_GMT 3 //часовой пояс по умолчанияю(-12..12)
#define DEFAULT_DST 0 //учитывать летнее время при синхронизации с ntp сервером(0 - летнее время не учитывается | 1 - летнее время учитывается)
#define DEFAULT_NTP_HOST "pool.ntp.org" //хост ntp сервера("pool.ntp.org", "ntp1.stratum2.ru", "ntp2.stratum2.ru", "ntp.msk-ix.ru")
#define DEFAULT_NTP_TIME 3600 //период запроса нового времени с ntp сервера(3600..86400)(сек)

#define DEFAULT_CLIMATE_AVG 1 //усреднение климатических данных на графике за указанный интервал(0 - добавлять текущие данные | 1 - добавлять усредненные данные)
#define DEFAULT_CLIMATE_TIME 30 //интервал отображения климатических данных на графике(1..60)(мин)
#define CLIMATE_BUFFER 100 //размер буфера памяти климатических данных на графике(50..250)

#define AP_IP 192, 168, 4, 4 //адрес точки доступа(по умолчанию - 192, 168, 4, 4)
#define AP_SSID "NixieClockAP" //имя точки доступа(минимум 1 символ)(без пробелов)
#define AP_PASS "1234567890" //пароль точки доступа(минимум 8 символов)("" - открытая сеть)
#define AP_CHANNEL 5 //канал точки доступа(1..13)

//   Плата:                                     SDA        SCL
//   ESP-01.................................... GPIO0      GPIO2
//   ESP8266................................... GPIO4      GPIO5
//   NodeMCU 1.0, WeMos D1 Mini................ GPIO4(D2)  GPIO5(D1)

//Настройки подключения
#define TWI_SDA_PIN 4 //пин SDA шины I2C
#define TWI_SCL_PIN 5 //пин SCL шины I2C

//Настройки цветовой схемы
#define UI_MAIN_THEME GP_DARK //тема веб интерфейса(GP_LIGHT - светлая тема | GP_DARK - темная тема)

//доступные цвета по умолчанию(GP_RED, GP_RED_B, GP_PINK, GP_PINK_B, GP_VIOL, GP_VIOL_B, GP_BLUE, GP_BLUE_B, GP_CYAN, GP_CYAN_B, GP_GREEN, GP_GREEN_B, GP_YELLOW, GP_YELLOW_B, GP_ORANGE, GP_ORANGE_B, GP_GRAY, GP_GRAY_B, GP_BLACK, GP_WHITE) или цвет в формате PSTR("#rrggbb")
#define UI_SPINNER_COLOR "#e67b09" //цвет спинеров веб интерфейса
#define UI_BUTTON_COLOR "#e67b09" //цвет кнопок веб интерфейса
#define UI_SWITCH_COLOR "#e67b09" //цвет переключателей веб интерфейса
#define UI_SLIDER_COLOR "#e67b09" //цвет слайдеров веб интерфейса
#define UI_BLOCK_COLOR "#e67b09" //цвет блока веб интерфейса
#define UI_CHECK_COLOR "#e67b09" //цвет чекбоксов веб интерфейса
#define UI_LABEL_COLOR "#e67b09" //цвет надписей веб интерфейса
#define UI_LINE_COLOR "#e67b09" //цвет горизонтальных линий веб интерфейса
#define UI_HINT_COLOR "#e67b09" //цвет основных подсказок веб интерфейса
#define UI_INFO_COLOR "#07b379" //цвет информационных подсказок веб интерфейса

#define UI_MENU_COLOR "#e67b09" //цвет меню веб интерфейса
#define UI_MENU_LINE_COLOR GP_GRAY //цвет горизонтальных линий меню веб интерфейса
#define UI_MENU_FW_COLOR "#e67b09" //цвет блока версии прошивки меню веб интерфейса
#define UI_MENU_NTP_1_COLOR GP_GREEN //цвет блока меню ntp "Синхронизирован" веб интерфейса
#define UI_MENU_NTP_2_COLOR GP_YELLOW //цвет блока меню ntp "Подключен" веб интерфейса
#define UI_MENU_NTP_3_COLOR GP_RED //цвет блока меню ntp "Отключен" веб интерфейса
#define UI_MENU_CLOCK_1_COLOR GP_GREEN //цвет блока меню часы "Подключены" веб интерфейса
#define UI_MENU_CLOCK_2_COLOR GP_RED //цвет блока меню часы "Отключены" веб интерфейса

#define UI_ALARM_ADD_COLOR "#e67b09" //цвет кнопки нового будильника
#define UI_ALARM_DIS_COLOR GP_YELLOW //цвет кнопки отключить будильник
#define UI_ALARM_DEL_COLOR GP_RED //цвет кнопки удалить будильник
#define UI_ALARM_SET_COLOR "#e67b09" //цвет кнопки настроить будильник
#define UI_ALARM_BACK_COLOR GP_BLUE //цвет кнопки выйти из настроек будильника
#define UI_ALARM_TIME_COLOR GP_DEFAULT //цвет времени будильника
#define UI_ALARM_INFO_COLOR GP_DEFAULT //цвет состояния будильника
#define UI_ALARM_BLOCK_COLOR GP_GRAY //цвет блока информации о будильнике
#define UI_ALARM_WEEK_1_COLOR GP_BLUE //цвет будних дней в настройке будильника
#define UI_ALARM_WEEK_2_COLOR GP_RED //цвет выходных дней в настройке будильника

#define UI_RADIO_POWER_COLOR GP_RED //цвет переключателя питания радио
#define UI_RADIO_BACK_COLOR "#e67b09" //цвет кнопки переключения режима радио
#define UI_RADIO_VOL_COLOR "#e67b09" //цвет слайдера настройки громкости радио
#define UI_RADIO_FREQ_1_COLOR "#e67b09" //цвет слайдера настройки частоты радио
#define UI_RADIO_FREQ_2_COLOR "#e67b09" //цвет кнопок настройки частоты радио
#define UI_RADIO_CHANNEL_COLOR "#e67b09" //цвет кнопок выбора канала радио

#define UI_TIMER_BLOCK_COLOR GP_GRAY //цвет блока информации о таймере/секундомере
#define UI_TIMER_TIME_COLOR GP_DEFAULT //цвет времени таймера/секундомера
#define UI_TIMER_INFO_COLOR GP_DEFAULT //цвет состояния таймера/секундомера
#define UI_TIMER_SET_COLOR "#e67b09" //цвет кнопок настройки таймера/секундомера
#define UI_TIMER_CTRL_COLOR "#e67b09" //цвет кнопок управления таймером/секундомером

#define UI_BAR_CLOCK_COLOR GP_WHITE //цвет времени в статус баре
#define UI_BAR_TEMP_COLOR GP_GREEN //цвет температуры в статус баре
#define UI_BAR_HUM_COLOR GP_BLUE //цвет влажности в статус баре
#define UI_BAR_PRESS_COLOR GP_PINK //цвет давления в статус баре
#define UI_BAR_LINE_COLOR GP_GRAY //цвет горизонтальной линии статус бара

//Дополнительно
#define MAX_ALARMS 7 //максимум будильников(3..15)

#define CLOCK_ADDRESS 127 //адрес шины часов
#define ESP_FIRMWARE_VERSION "1.0.8" //версия прошивки модуля esp
