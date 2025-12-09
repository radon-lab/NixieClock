#if DOTS_TYPE == 2
#define DOTS_ALL (DOTS_NUM * 2) //всего разделительных точек
#else
#define DOTS_ALL (DOTS_NUM) //всего разделительных точек
#endif

//перечисления неоновых точек
enum {
  DOT_NULL, //неоновые лампы выключены
  DOT_LEFT, //левая неоновая лампа
  DOT_RIGHT, //правая неоновая лампа
  DOT_ALL //две неоновые лампы
};

uint8_t dot_dimm; //яркость секундной точки

uint8_t indi_dot_l; //буфер левых точек индикаторов
uint8_t indi_dot_r; //буфер правых точек индикаторов
volatile uint8_t indi_dot_pos; //текущей номер точек индикаторов

uint8_t indi_buf[7]; //буфер индикаторов
uint8_t indi_dimm[7]; //яркость индикаторов
volatile uint8_t indi_state; //текущей номер отрисовки индикатора

#if INDI_MODE == 0
#include "DYNAMIC.h"
#elif INDI_MODE == 1
#include "DYNAMICx2.h"
#endif

#include "DECATRON.h"

//переменные работы с анимациями
struct animData {
#if LAMP_NUM > 4
  uint8_t flipSeconds; //флаги анимации секунд
#endif
  uint8_t flipBuffer[12]; //буфер анимации секунд
  uint16_t timeBright; //буфер времени для анимации яркости
} anim;

const uint8_t _anim_set[] PROGMEM = {FLIP_ANIM_RANDOM}; //массив случайных режимов

void indiSetBright(uint8_t pwm, uint8_t start = 0, uint8_t end = LAMP_NUM); //установка общей яркости
void indiPrintNum(uint16_t num, int8_t indi, uint8_t length = 0, uint8_t filler = ' '); //отрисовка чисел
void animPrintNum(uint16_t num, int8_t indi, uint8_t length = 0, uint8_t filler = ' '); //отрисовка чисел
#if SECS_DOT == 2
boolean dotDecBright(uint8_t _step, uint8_t _min, uint8_t _mode = DOT_ALL); //умегьшение яркости точек
boolean dotIncBright(uint8_t _step, uint8_t _max, uint8_t _mode = DOT_ALL); //увеличение яркости точек
#endif

//------------------------Обновление коэффициента линейного регулирования-------------------------
void indiChangeCoef(void) //обновление коэффициента линейного регулирования
{
  if (debugSettings.max_pwm < (debugSettings.min_pwm + 10)) debugSettings.min_pwm = debugSettings.max_pwm - 10;
  pwm_coef = 255 / (uint8_t)(((uint16_t)LIGHT_MAX * (LAMP_NUM + ((INDI_SYMB_TYPE) ? 1 : 0))) / CONSTRAIN(debugSettings.max_pwm - debugSettings.min_pwm, 10, 100));
}
//---------------------Установка нового значения шим линейного регулирования----------------------
void indiChangePwm(void) //установка нового значения шим линейного регулирования
{
  uint16_t dimm_all = 0;
  for (uint8_t i = (INDI_SYMB_TYPE) ? 0 : 1; i < (LAMP_NUM + 1); i++) if (indi_buf[i] != INDI_NULL) dimm_all += indi_dimm[i];
#if CONV_PIN == 9
  OCR1A = CONSTRAIN(debugSettings.min_pwm + (uint8_t)((dimm_all * pwm_coef) >> 8), 100, 200);
#elif CONV_PIN == 10
  OCR1B = CONSTRAIN(debugSettings.min_pwm + (uint8_t)((dimm_all * pwm_coef) >> 8), 100, 200);
#endif
}
//--------------------------------Очистка индикаторов--------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < LAMP_NUM; cnt++) indi_buf[cnt + 1] = INDI_NULL;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//---------------------------------Очистка индикатора--------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  indi_buf[indi + 1] = INDI_NULL;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//---------------------------------Установка индикатора------------------------------------------
void indiSet(uint8_t buf, uint8_t indi) //установка индикатора
{
  indi_buf[indi + 1] = buf; //устанавливаем в ячейку буфера
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//------------------------------Получить состояние индикатора-------------------------------------
uint8_t indiGet(uint8_t indi) //получить состояние индикатора
{
  return indi_buf[indi + 1]; //возвращаем содержимое ячейки буфера
}
//---------------------------------Получить запонение шим-----------------------------------------
uint8_t indiGetPwm(uint8_t bri) //получить запонение шим
{
  if (bri > 30) bri = 30;
  bri = (uint8_t)((INDI_LIGHT_MAX * bri) >> 8);
  return bri ? bri : (bri + 1); //возвращаем запонение шим
}
//------------------------------Установка яркости индикатора--------------------------------------
void indiSetBright(uint8_t bri, uint8_t indi) //установка яркости индикатора
{
  indi_dimm[indi + 1] = indiGetPwm(bri);
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//--------------------------------Установка общей яркости-----------------------------------------
void indiSetBright(uint8_t bri, uint8_t start, uint8_t end) //установка общей яркости
{
  bri = indiGetPwm(bri);
  for (uint8_t i = start; i < end; i++) indi_dimm[i + 1] = bri;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-------------------------------Очистка индикатора символов---------------------------------------
#if INDI_SYMB_TYPE
void indiClrSymb(void) //очистка индикатора символов
{
  indi_buf[0] = INDI_NULL;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-------------------------------Установка индикатора символов-------------------------------------
void indiSetSymb(uint8_t buf) //установка индикатора символов
{
  indi_buf[0] = buf; //устанавливаем в ячейку буфера
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//--------------------------Получить состояние индикатора символов---------------------------------
uint8_t indiGetSymb(void) //получить состояние индикатора символов
{
  return indi_buf[0]; //возвращаем содержимое ячейки буфера
}
//---------------------------Установка яркости индикатора символов---------------------------------
void indiSetSymbBright(uint8_t bri) //установка яркости индикатора символов
{
  indi_dimm[0] = indiGetPwm(bri);
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
#endif
//-------------------------------------Вывод чисел-----------------------------------------------
void indiPrintNum(uint16_t _num, int8_t _indi, uint8_t _length, uint8_t _filler) //вывод чисел
{
  printNum(_num, indi_buf, _indi, _length, _filler); //печать чисел
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//---------------------------------Вывод аргументов меню------------------------------------------
void indiPrintMenuData(boolean blink, boolean state, uint8_t arg1, uint8_t pos1, uint8_t arg2, uint8_t pos2) //вывод аргументов меню
{
  if (!blink || state) indiPrintNum(arg1, pos1, (pos1 & 0x01) ? 1 : 2, 0); //вывод первого аргумента
  if (!blink || !state) indiPrintNum(arg2, pos2, (pos2 & 0x01) ? 1 : 2, 0); //вывод второго аргумента
}
//-------------------------Установка левой разделительной точки------------------------------------
void indiSetDotL(uint8_t dot) //установка левой разделительной точки
{
#if DOTS_SHIFT
  if (dot) dot--;
#endif
  if (dot < DOTS_NUM) indi_dot_l |= (0x02 << dot);
}
//--------------------------Очистка левой разделительной точки-------------------------------------
void indiClrDotL(uint8_t dot) //очистка левой разделительной точки
{
#if DOTS_SHIFT
  if (dot) dot--;
#endif
  if (dot < DOTS_NUM) indi_dot_l &= ~(0x02 << dot);
}
//-------------------------Установка правой разделительной точки-----------------------------------
void indiSetDotR(uint8_t dot) //установка правой разделительной точки
{
  if (dot < DOTS_NUM) indi_dot_r |= (0x02 << dot);
}
//--------------------------Очистка правой разделительной точки------------------------------------
void indiClrDotR(uint8_t dot) //очистка разделительной точки
{
  if (dot < DOTS_NUM) indi_dot_r &= ~(0x02 << dot);
}
//----------------------------Установка разделительных точек---------------------------------------
void indiSetDots(int8_t _dot, uint8_t _num) //установка разделительных точек
{
  while (_num) { //пока есть точки
    _num--; //убавили число точек
    if ((uint8_t)_dot < DOTS_ALL) { //если число в поле индикатора
#if DOTS_TYPE == 2
      if (_dot & 0x01) indi_dot_r |= (0x02 << (_dot >> 1)); //включаем правую точку
      else indi_dot_l |= (0x02 << (_dot >> 1)); //включаем левую точку
#elif DOTS_TYPE == 1
      indi_dot_r |= (0x02 << _dot); //включаем правую точку
#else
      indi_dot_l |= (0x02 << _dot); //включаем левую точку
#endif
    }
    _dot++; //прибавили текущую позицию точек
  }
}
//----------------------------Установка разделительных точек----------------------------------------
void indiSetDotsMain(uint8_t _dot) //установка разделительных точек
{
#if DOTS_TYPE == 2
#if LAMP_NUM > 4
#if DOTS_MAIN == 0
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
#elif DOTS_MAIN == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#elif DOTS_MAIN == 2
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#elif DOTS_MAIN == 3
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#endif
#else
#if DOTS_MAIN == 0
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
#elif DOTS_MAIN == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
#endif
#endif
#elif DOTS_NUM > 4
#if DOTS_TYPE == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
#endif
#else
#if DOTS_TYPE == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
#endif
#endif
}
//-----------------------------Очистка разделительных точек----------------------------------------
void indiClrDots(void) //очистка разделительных точек
{
#if DOTS_TYPE == 2
  indi_dot_l = 0x00; //выключаем левые точки
  indi_dot_r = 0x00; //выключаем правые точки
#elif DOTS_TYPE == 1
  indi_dot_r = 0x00; //выключаем правые точки
#else
  indi_dot_l = 0x00; //выключаем левые точки
#endif
}
//----------------------------Состояние разделительных точек---------------------------------------
boolean indiGetDots(void) //состояние разделительных точек
{
  return indi_dot_l | indi_dot_r;
}
//--------------------------------Установка неоновых точек------------------------------------------
void neonDotSet(uint8_t _dot) //установка неоновых точек
{
  indi_buf[0] = INDI_NULL; //запрещаем включать точки
  switch (_dot) {
    case DOT_LEFT: indi_buf[0] |= 0x80; break; //включаем левую точку
    case DOT_RIGHT: indi_buf[0] |= 0x40; break; //включаем правую точку
    case DOT_ALL: indi_buf[0] |= 0xC0; break; //включаем обе точки
  }
}
//----------------------------Установка яркости неоновых точек--------------------------------------
void neonDotSetBright(uint8_t _pwm) //установка яркости неоновых точек
{
  if (_pwm > 250) _pwm = 250;
  dot_dimm = _pwm;
  _pwm = (uint8_t)((DOT_LIGHT_MAX * _pwm) >> 8);
  indi_dimm[0] = _pwm ? _pwm : (_pwm + 1); //устанавливаем яркость точек
}
//----------------------------------Получить яркость точек---------------------------------------
inline uint8_t dotGetBright(void) //получить яркость точек
{
#if (SECS_DOT == 3) && DOTS_PORT_ENABLE
  return indi_dot_l | indi_dot_r; //возвращаем состояние точек
#elif SECS_DOT
  return dot_dimm; //возвращаем яркость
#else
#if SECL_PIN == 9
  return OCR1A; //возвращаем яркость
#elif SECL_PIN == 10
  return OCR1B; //возвращаем яркость
#endif
#endif
}
//---------------------------------Установка яркости точек---------------------------------------
void dotSetBright(uint8_t _pwm) //установка яркости точек
{
#if (SECS_DOT == 3) && DOTS_PORT_ENABLE
  if (_pwm) indiSetDotsMain(DOT_ALL); //установка разделительных точек
  else indiClrDots(); //очистка разделительных точек
#elif SECS_DOT == 2
  neonDotSetBright(_pwm); //установка яркости неоновых точек
  neonDotSet((_pwm) ? DOT_ALL : DOT_NULL); //установка неоновых точек
#elif SECS_DOT == 1
  neonDotSetBright(_pwm); //установка яркости неоновых точек
  if (_pwm) indi_buf[0] = (INDI_NULL | 0x80); //разрешаем включать точки
  else indi_buf[0] = INDI_NULL; //запрещаем включать точки
#elif SECS_DOT == 0
#if SECL_PIN == 9
  OCR1A = _pwm; //устанавливаем яркость точек
  if (_pwm) TCCR1A |= (0x01 << COM1A1); //подключаем D9
  else {
    TCCR1A &= ~(0x01 << COM1A1); //отключаем D9
    SECS_DOT_CLEAR(SECL_PIN); //выключили точки
  }
#elif SECL_PIN == 10
  OCR1B = _pwm; //устанавливаем яркость точек
  if (_pwm) TCCR1A |= (0x01 << COM1B1); //подключаем D10
  else {
    TCCR1A &= ~(0x01 << COM1B1); //отключаем D10
    SECS_DOT_CLEAR(SECL_PIN); //выключили точки
  }
#endif
#endif
}
#if SECS_DOT != 2
//--------------------------------Уменьшение яркости точек----------------------------------------
boolean dotDecBright(uint8_t _step, uint8_t _min)
{
  if (((int16_t)dotGetBright() - _step) > _min) dotSetBright(dotGetBright() - _step);
  else {
    dotSetBright(_min);
    return 1;
  }
  return 0;
}
//--------------------------------Увеличение яркости точек----------------------------------------
boolean dotIncBright(uint8_t _step, uint8_t _max)
{
  if (((uint16_t)dotGetBright() + _step) < _max) dotSetBright(dotGetBright() + _step);
  else {
    dotSetBright(_max);
    return 1;
  }
  return 0;
}
#else
//--------------------------------Уменьшение яркости точек----------------------------------------
boolean dotDecBright(uint8_t _step, uint8_t _min, uint8_t _mode)
{
  if (((int16_t)dotGetBright() - _step) > _min) {
    _min = dotGetBright() - _step;
    neonDotSetBright(_min); //установка яркости неоновых точек
    neonDotSet((_min) ? _mode : 0); //установка неоновых точек
  }
  else {
    neonDotSetBright(_min); //установка яркости неоновых точек
    neonDotSet((_min) ? _mode : 0); //установка неоновых точек
    return 1;
  }
  return 0;
}
//--------------------------------Увеличение яркости точек----------------------------------------
boolean dotIncBright(uint8_t _step, uint8_t _max, uint8_t _mode)
{
  if (((uint16_t)dotGetBright() + _step) < _max) {
    _max = dotGetBright() + _step;
    neonDotSetBright(_max); //установка яркости неоновых точек
    neonDotSet((_max) ? _mode : 0); //установка неоновых точек
  }
  else {
    neonDotSetBright(_max); //установка яркости неоновых точек
    neonDotSet((_max) ? _mode : 0); //установка неоновых точек
    return 1;
  }
  return 0;
}
#endif
//-----------------------------Запись чисел в буфер анимации----------------------------------------
void animPrintNum(uint16_t _num, int8_t _indi, uint8_t _length, uint8_t _filler) //запись чисел в буфер анимации
{
  printNum(_num, (anim.flipBuffer + 5), _indi, _length, _filler); //печать чисел
}
//-------------------------------Отрисовка буфера анимации-----------------------------------------
void animPrintBuff(int8_t _indi, uint8_t _step, uint8_t _max) //отрисовка буфера анимации
{
  for (uint8_t i = 0; i < _max; i++) {
    if ((uint8_t)_indi < LAMP_NUM) { //если число в поле индикатора
      indi_buf[_indi + 1] = anim.flipBuffer[i + _step]; //устанавливаем новое число
    }
    _indi++;
  }
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//--------------------------------Очистка буфера анимации-------------------------------------------
void animClearBuff(void) //очистка буфера анимации
{
  for (uint8_t i = 6; i < (LAMP_NUM + 6); i++) anim.flipBuffer[i] = INDI_NULL;
}
//--------------------------------Анимация смены яркости цифр---------------------------------------
void animBright(uint8_t bri) //анимация смены яркости цифр
{
  bri = indiGetPwm(bri);
  for (uint8_t i = 0; i < LAMP_NUM; i++) {
    if (anim.flipBuffer[i] != anim.flipBuffer[i + 6]) { //если не достигли конца анимации разряда
      indi_dimm[i + 1] = bri;
    }
  }
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
