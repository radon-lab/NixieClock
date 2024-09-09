#define WEATHER_BUFFER 24 //размер буфера хранения данных о погоде(24)(ч)
#define WEATHER_TIMEOUT 10000 //таймаут ожидания ответа сервера погоды(2000..15000)(мс)

#define WEATHER_CITY_ARRAY 83 //количество городов в списке(1..255)

const char weatherCityList[] = { //список городов
  "Абакан,Архангельск,Астана,Астрахань,Барнаул,Белгород,Бийск,Благовещенск,Братск,Брянск,Великий Новгород,Владивосток,Владикавказ,Владимир,Волгоград,Вологда,Воронеж,"
  "Грозный,Екатеринбург,Иваново,Ижевск,Иркутск,Казань,Калининград,Калуга,Каменск-Уральский,Кемерово,Киров,Комсомольск-на-Амуре,Королев,Кострома,Краснодар,Красноярск,Курск,"
  "Липецк,Магнитогорск,Махачкала,Москва,Мурманск,Набережные Челны,Нижний Новгород,Нижний Тагил,Новокузнецк,Новороссийск,Новосибирск,Норильск,Омск,Орел,Оренбург,"
  "Пенза,Первоуральск,Пермь,Прокопьевск,Псков,Ростов-на-Дону,Рыбинск,Рязань,Самара,Санкт-Петербург,Саратов,Севастополь,Северодвинск,Северодвинск,Симферополь,Сочи,Ставрополь,"
  "Тамбов,Тверь,Тольятти,Томск,Тула,Тюмень,Улан-Удэ,Ульяновск,Уфа,Хабаровск,Чебоксары,Челябинск,Шахты,Энгельс,Южно-Сахалинск,Якутск,Ярославль"
};

const float weatherCoordinatesList[2][WEATHER_CITY_ARRAY] = { //координаты городов
  {
    53.7210, 64.5393, 71.4306, 46.3479, 53.3561, 50.5975, 52.5414, 50.2907, 56.1514, 53.2434, 58.5215, 43.1340, 43.0241, 56.1290, 48.7071, 59.2205, 51.6615,
    43.3180, 56.8380, 57.0003, 56.8528, 52.2864, 55.7958, 55.9162, 54.5070, 56.4149, 55.3596, 54.0790, 50.5499, 55.9162, 57.7677, 45.0239, 56.0087, 51.7304,
    52.6102, 53.4117, 42.9849, 55.7558, 68.9696, 55.7436, 56.3239, 57.9101, 53.7865, 44.7235, 55.0287, 69.3490, 54.9893, 52.9703, 51.7681, 53.1945, 56.9081,
    58.0048, 53.8954, 57.8194, 47.2271, 58.1385, 54.6199, 53.1955, 59.9388, 51.5315, 44.6166, 64.5582, 64.5582, 44.9521, 43.5815, 45.0445, 52.7212, 56.8596,
    53.5113, 56.4951, 54.1930, 57.1530, 51.8335, 54.3170, 54.7348, 48.4726, 56.1439, 55.1598, 47.7085, 51.4989, 46.9591, 62.0278, 57.6266
  },
  {
    91.4424, 40.5187, 51.1284, 48.0335, 83.7496, 36.5888, 85.2196, 127.5271, 101.6341, 34.3641, 31.2754, 131.9283, 44.6904, 40.4070, 44.5169, 39.8915, 39.2002,
    45.6981, 60.5972, 40.9739, 53.2115, 104.2807, 49.1066, 37.8545, 36.2523, 61.9189, 86.0878, 34.3232, 137.0079, 37.8545, 40.9264, 38.9702, 92.8705, 36.1926,
    39.5947, 58.9844, 47.5046, 37.6178, 33.0745, 52.3958, 44.0023, 59.9813, 87.1552, 37.7687, 82.9069, 88.2010, 73.3682, 36.0635, 55.0974, 45.0195, 59.9429,
    56.2377, 86.7447, 28.3318, 39.7450, 38.5736, 39.7450, 50.1018, 30.3143, 46.0358, 33.5254, 39.8296, 39.8296, 34.1024, 39.7229, 41.9691, 41.4522, 35.9119,
    49.4181, 84.9721, 37.6178, 65.5343, 107.5841, 48.4022, 55.9578, 135.0577, 47.2489, 61.4025, 40.2160, 46.1251, 142.7381, 129.7041, 39.8938
  }
};

enum {
  WEATHER_GET_TEMP,
  WEATHER_GET_HUM,
  WEATHER_GET_PRESS
};

enum {
  WEATHER_NULL,
  WEATHER_ERROR,
  WEATHER_GOOD,
  WEATHER_REQUEST,
  WEATHER_WAIT_ANSWER
};
uint8_t weather_state = WEATHER_NULL;

boolean weather_update = false; //флаг обновления данных о погоде
uint32_t weather_timer = 0; //таймер ожидания ответа от сервера погоды

float weather_latitude = 0;
float weather_longitude = 0;

String weather_answer = "";

#include <ESP8266WiFi.h>
WiFiClient client;

uint8_t weatherGetStatus(void) {
  return weather_state;
}
boolean weatherGetRunStatus(void) {
  return (weather_state != WEATHER_NULL);
}
boolean weatherGetValidStatus(void) {
  return ((weather_state > WEATHER_ERROR) && (weather_update == true));
}
void weatherSetCoordinates(uint8_t city) {
  weather_latitude = weatherCoordinatesList[0][city];
  weather_longitude = weatherCoordinatesList[1][city];
}
void weatherSetCoordinates(float latitude, float longitude) {
  weather_latitude = latitude;
  weather_longitude = longitude;
}
void weatherSendRequest(void) {
  if (weather_state <= WEATHER_GOOD) {
    if (client.connected()) client.stop();
    weather_state = WEATHER_REQUEST;
  }
}
void weatherDisconnect(void) {
  if (client.connected()) client.stop();
  weather_state = WEATHER_NULL;
  weather_update = false;
}

const char* weatherGetParseType(uint8_t mod) {
  switch (mod) {
    case WEATHER_GET_TEMP: return "\"temperature_2m\":[";
    case WEATHER_GET_HUM: return "\"relative_humidity_2m\":[";
    case WEATHER_GET_PRESS: return "\"pressure_msl\":[";
  }
  return "NULL";
}

void weatherGetUnixData(uint32_t* buf, uint8_t len) {
  if (weather_state != WEATHER_GOOD) return;

  int16_t valPos = 0;
  int16_t valStart = 0;
  int16_t valEnd = 0;

  int32_t valOffset = 0;

  valPos = weather_answer.indexOf("\"utc_offset_seconds\":");
  if (valPos >= 0) {
    valStart = weather_answer.indexOf(":", valPos);
    valEnd = weather_answer.indexOf(",", valPos);
    if ((valStart >= 0) && (valEnd >= 0)) {
      while (++valStart < valEnd) {
        if ((weather_answer[valStart] >= '0') && (weather_answer[valStart] <= '9')) {
          valOffset *= 10;
          valOffset += weather_answer[valStart] - '0';
        }
      }
      if ((valOffset < -43200) || (valOffset > 43200)) valOffset = 0;
    }
  }

  valPos = weather_answer.indexOf("\"time\":[");
  if (valPos >= 0) {
    valStart = weather_answer.indexOf("[", valPos);
    valEnd = weather_answer.indexOf("]", valPos);
    if ((valStart >= 0) && (valEnd >= 0)) {
      if (valStart < valEnd) {
        for (uint8_t i = 0; i < len; i++) {
          buf[i] = 0;
          while (++valStart < valEnd) {
            if ((weather_answer[valStart] >= '0') && (weather_answer[valStart] <= '9')) {
              buf[i] *= 10;
              buf[i] += weather_answer[valStart] - '0';
            }
            else if (weather_answer[valStart] == ',') break;
          }
          buf[i] += valOffset;
        }
      }
      else weather_state = WEATHER_ERROR;
    }
    else weather_state = WEATHER_ERROR;
  }
  else weather_state = WEATHER_ERROR;
}
void weatherGetParseData(int16_t* buf, uint8_t mod, uint8_t len) {
  if (weather_state != WEATHER_GOOD) return;

  int16_t valPos = 0;
  int16_t valStart = 0;
  int16_t valEnd = 0;

  boolean negFlag = false;

  valPos = weather_answer.indexOf(weatherGetParseType(mod));
  if (valPos >= 0) {
    valStart = weather_answer.indexOf("[", valPos);
    valEnd = weather_answer.indexOf("]", valPos);
    if ((valStart >= 0) && (valEnd >= 0)) {
      if (valStart < valEnd) {
        for (uint8_t i = 0; i < len; i++) {
          buf[i] = 0;
          while (++valStart < valEnd) {
            if ((weather_answer[valStart] >= '0') && (weather_answer[valStart] <= '9')) {
              buf[i] *= 10;
              buf[i] += weather_answer[valStart] - '0';
            }
            else if (weather_answer[valStart] == '-') negFlag = true;
            else if (weather_answer[valStart] == ',') break;
          }
          if (negFlag == true) {
            negFlag = false;
            buf[i] = -buf[i];
          }
          switch (mod) {
            case WEATHER_GET_HUM: buf[i] *= 10; break;
            case WEATHER_GET_PRESS: buf[i] = buf[i] * 0.750064; break;
          }
        }
      }
      else weather_state = WEATHER_ERROR;
    }
    else weather_state = WEATHER_ERROR;
  }
  else weather_state = WEATHER_ERROR;
}

boolean weatherUpdate(void) {
  switch (weather_state) {
    case WEATHER_REQUEST:
      if (client.connect("api.open-meteo.com", 80)) {
        if (client.connected()) {
          client.println("GET /v1/forecast?latitude=" + String(weather_latitude, 4) +
                         "&longitude=" + String(weather_longitude, 4) +
                         "&hourly=temperature_2m,relative_humidity_2m,pressure_msl&timeformat=unixtime&timezone=auto&forecast_days=1&forecast_hours=24 HTTP/1.1\r\nHost: api.open-meteo.com\r\n");
          weather_answer = "";
          weather_timer = millis();
          weather_state = WEATHER_WAIT_ANSWER;
        }
        else {
          client.stop();
          weather_state = WEATHER_ERROR;
        }
      }
      else weather_state = WEATHER_ERROR;
      break;
    case WEATHER_WAIT_ANSWER:
      if (client.available()) {
        weather_answer += (char)client.read();
        if (!client.available()) {
          client.stop();

          if (weather_answer.indexOf("charset=utf-8") >= 0) {
            weather_state = WEATHER_GOOD;
            weather_update = true;
            return true;
          }
          else weather_state = WEATHER_ERROR;
        }
      }
      if ((millis() - weather_timer) >= WEATHER_TIMEOUT) {
        client.stop();
        weather_state = WEATHER_ERROR;
      }
      break;
  }

  return false;
}
