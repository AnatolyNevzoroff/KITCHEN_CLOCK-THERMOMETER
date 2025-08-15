////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                            //
//               КУХОННЫЕ ЧАСЫ С ТЕРМОМЕТРОМ (KITCHEN CLOCK WITH THERMOMETER)                 //
//                                    Версия (Version) 1.50                                   //
//                   Код от Анатолия Невзорова (Сode by Anatoly Nevzoroff)                    //
//                            https://github.com/AnatolyNevzoroff                             //
//                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////

//  СОСТАВ ОБОРУДОВАНИЯ:
//  ARDUINO NANO (AVR ATmega328P)
//  LED ЭКРАН 32Х16 (Из 8-ми матриц 8Х8 и драйверами MAX7219)
//  МОДУЛЬ ЧАСОВ RTC DS3231 (с батарейкой, линия подзарядки отрезана)
//  ИНФРАКРАСНЫЙ ПРИЁМНИК JS0038M (Пулт ДУ 38 кГц)
//  ПРИЁМНИК SYN480R или WL101-341 (RX470-4 аналог)(433,92 МГц)
//  ПАССИВНЫЙ ПЬЕЗОДИНАМИК (BAZZER low level)

// На плате Arduino Nano задействованы следующие выводы:
//            ADC GPIO  A0  <<-> LDR (Fotorezist)
//            ADC GPIO  A1  <<-> 
//            ADC GPIO  A2  <<-> 
//            ADC GPIO  A3  <<-> 
//        SDA ADC GPIO  A4  <<-> SDA (RTC DS3231)
//        SCL ADC GPIO  A5  <<-> SCL (RTC DS3231)
//            ADC       A6  <<- 
//            ADC       A7  <<- 
//       INT0     GPIO  D2  <-> 
// TMR2b INT1 PWM GPIO  D3  <->> DIN (INFRARED JS0038M)
//                GPIO  D4  <->  
// TMR0b      PWM GPIO  D5  <->> DAT (SYN480R)
// TMR0a      PWM GPIO  D6  <->> 
//                GPIO  D7  <->  
//                GPIO  D8  <->  LED FEEDBACK
// TMR1a      PWM GPIO  D9  <->> I/O (BAZZER)
// TMR1b      PWM GPIO  D10 <->> CS  (LED ЭКРАН)
// TMR2a MOSI PWM GPIO  D11 <->> DIN (LED ЭКРАН)
//       MISO     GPIO  D12 <->  
//       SCLK     GPIO  D13 <->  CLK (LED ЭКРАН)


//ПОДКЛЮЧАЕМЫЕ БИБЛИОТЕКИ:
#include <SPI.h>//Не используется, но необходима для компиляции
#include <EEPROM.h>//Библиотека работы с EEPROM
#include <Adafruit_GFX.h>// https://github.com/adafruit/Adafruit-GFX-Library
#include <Max72xxPanel.h>// https://github.com/markruys/arduino-Max72xxPanel 
#include <DS3231.h>// http://www.rinkydinkelectronics.com/download.php?f=DS3231.zip 
#include <IRremote.h>// https://github.com/Arduino-IRremote/Arduino-IRremote  VERSION 2.8.0.!!!
#include <RH_ASK.h>// http://www.airspayce.com/mikem/arduino/RadioHead
#include <TimerFreeTone.h>// https://bitbucket.org/teckel12/arduino-timer-free-tone/wiki/Home


//ДЛЯ ПРОВЕРКИ РАБОТЫ ПЕРИФЕРИИ ЧАСОВ ВЫВОДИМ ЗНАЧЕНИЯ В МОНИТОР СЕРИЙНОГО ПОРТА 
//  #define LOG_ENABLE_SERIAL //В РАБОЧЕЙ ВЕРСИИ УДАЛИТЬ ИЛИ ЗАКОММЕНТИРОВАТЬ!
//Чтоб не путаться в потоке данных, можем отдельно включить или отключить вывод для:
//  #define LOG_ENABLE_IR_COD //Коды кнопок с IR пульта
//  #define LOG_ENABLE_RADIO  //Данные о температуре с радидатчика
//  #define LOG_ENABLE_LDR    //Результат работы АЦП с фоторезистра


//КОДЫ КНОПОК (ПУЛЬТ RS-101P ДВА ПРОТОКОЛА ПЕРЕДАЧИ)
#define IR_EXIT0 0x86897      //Меню "ГЛАВНОЕ МЕНЮ" (Серая кнопка EXIT)
#define IR_EXIT1 0x93B9DFCF
#define IR_TIMER0 0x89867     //Меню "ТАЙМЕРЫ" (Белая кнопка)
#define IR_TIMER1 0x8C02A0CD
#define IR_TEMPER0 0x8B847    //Меню "МАКСИМАЛЬНОЙ И МИНИМАЛЬНОЙ ТЕМПЕРАТУРЫ" (Зелёная кнопка)
#define IR_TEMPER1 0x3C095209 
#define IR_SETUP0 0x87887     //Меню "НАСТРОЙКА ПАРАМЕТРОВ" (Синяя кнопка)
#define IR_SETUP1 0xC8F480AB
#define IR_VOL_UP0 0x8F807    //Кнопка "БОЛЬШЕ" (Кнопка "V+")
#define IR_VOL_UP1 0x5EDF3FED
#define IR_VOL_DOWN0 0x802FD  //Кнопка "МЕНЬШЕ" (Кнопка "V-")
#define IR_VOL_DOWN1 0xAAA0456F
#define IR_OK0 0x8C837        //Кнопка "ПОДТВЕРЖДЕНИЕ" (Кнопка "OK")
#define IR_OK1 0x1BCB3E0D 
//#define IR_PG_UP0 0x808F7     //Кнопка "СТРАНИЦА ВВЕРХ" (Кнопка "P+")
//#define IR_PG_UP1 0x17FF366F 
//#define IR_PG_DOWN0 0x8F00F   //Кнопка "СТРАНИЦА ВНИЗ" (Кнопка "P-")
//#define IR_PG_DOWN1 0x99595469 
#define IR_FFFFF 0xFFFFFFFF   //Команда "ПОВТОР"


#define numberOfHorizontalDisplays 2 //Колличество горизонтальных строк на дисплее из матриц
#define numberOfVerticalDisplays 4 //Колличество вертикальных столбцов на дисплее из матриц
#define CS_PIN 10 //Можно назначить любой пин в качестве (Chip Select) оставляем 10
#define TONE_PIN 9 //D9 Выход на пьезодинамик (piezospeaker)
#define LED_INFO_PIN 8 //D8 Выход на светодиод для индикации активности радиопередатчика
#define IR_RECEIVER_PIN 3 //D3 Вход для IR приёмника (INPUT INFRARED RECEIVER "timer 0")
#define RADIO_RECEIVER_PIN 5 //D5 Вход для модуля радиоприёмника SYN480R (RX470-4)
#define LDR_PIN A0 //A0 Вход с резистивного делителя напряжения с фоторезистором
#define Speed_RX 1200 //Скорость приёма для радиомодуля (соответствует скорости передачи)
#define wait 50 //Пауза между смещением текста в миллисекундах для бегущей строки
#define spacer 1 //Колличество разделителей задающих расстояние между символами
#define Serial_SPEED 9600 //Скорость серийного порта
#define TEN_MIN 600000L //Таймер на 600 секунд (10 Минут)
#define p1000 1000L     //Служебная задержка для таймера тип данных "L" (long)
#define p300 300L       
#define p200 200L       
#define p150 150L       
    



//ДЛЯ ПОДКЛЮЧЕНИЯ IR ПРИЁМНИКА: 
IRrecv irrecv(IR_RECEIVER_PIN);//Указываем вывод подключения IR приёмника
decode_results ir;//Объявляем объект "ir" для библиотеки IR приёмника

//ДЛЯ ПОДКЛЮЧЕНИЯ МОДУЛЯ РАДИОПРИЁМНИКА:
//Создаём объект "driver" командой: RH_ASK driver;
//Если не указать никаких параметров то библиотека сама назначит значения по умолчанию:
//RH_ASK driver (2400,11,12,10); -> (Speed,RX pin,TX pin,Ptt);
//Скорость 2400 (бит/с), RX pin 11(прием), TX pin 12(передача), Ptt pin 10(Push to Talk)
//В нашем случае указываем только скорость приёма и вывод для приёмника
RH_ASK driver(Speed_RX,RADIO_RECEIVER_PIN,-1,-1);


//ДЛЯ ПОДКЛЮЧЕНИЯ МОДУЛЯ ЧАСОВ ИСПОЛЬЗУЕМ:
//I²C (IIC) Inter-Integrated Circuit (Последовательная асимметричная шина) использует 2 линии:
//SDA, Serial DAta (Последовательная линия данных) SDA (в Arduino: по умолчанию A4 pin) 
//SCL, Serial CLock (Последовательная линия тактирования) SCL (в Arduino: по умолчанию A5 pin)
//Все устройства подключаемые по I²C имеют адреса, У DS3231 он 0х068, и библиотека знает его
DS3231 rtc(SDA,SCL);//Инициализируем подключение DS3231 по I²C (SDA -> A4 pin, SCL -> A5 pin)
Time Tmr;//Объявляем "Tmr" для библиотеки часов

//ДЛЯ ПОДКЛЮЧЕНИЯ МОДУЛЕЙ СВЕТОДИОДНОЙ МАТРИЦЫ ИСПОЛЬЗУЕМ:
//SPI, Serial Peripheral Interface (Последовательный периферийный интерфейс) использует 4 линии:
//SCLK, Serial Clock (Тактовый сигнал) SCK, CLK                 (в Arduino: по умолчанию 13 pin)
//MOSI, Master Output, Slave Input (Данные от ведущего к ведомому) SDI, DI, SI (Arduino: 11 pin)
//MISO, Master Input, Slave Output (Данные от ведомого к ведущему) SDO, DO, SO (Arduino: 12 pin)
//SS, Slave Select (Выбор ведомого) nCS, CS, CSB, CSN, nSS, STE (в Arduino: по умолчанию 10 pin)
//MAX7219 управляется AVR ARDUINO по трём линиям SPI: SCLK, MOSI, SS (MISO не используется)
//Выводы матриц подключаем: CLK -> 13 pin (SCLK), DIN -> 11 pin (MOSI), CS -> 10 pin (SS)
//Создаём объект "matrix" и указываем параметры обозначенные выше 
Max72xxPanel matrix=Max72xxPanel(CS_PIN,numberOfHorizontalDisplays,numberOfVerticalDisplays);


//МАССИВЫ СИМВОЛОВ СОХРАНЯЕМ В ПРОГРАММНОЙ ПАМЯТИ ЧЕРЕЗ "PROGMEM"
//МАЛЕНЬКИЙ ШРИФТ 3Х5 
const byte font3x5[11][5] PROGMEM ={
{B111,B101,B101,B101,B111},//0
{B010,B110,B010,B010,B111},//1
{B111,B001,B111,B100,B111},//2
{B111,B001,B011,B001,B111},//3
{B101,B101,B111,B001,B001},//4
{B111,B100,B111,B001,B111},//5
{B111,B100,B111,B101,B111},//6
{B111,B001,B001,B010,B010},//7
{B111,B101,B111,B101,B111},//8
{B111,B101,B111,B001,B111},//9
{B000,B010,B000,B010,B000}//10 (Двоеточие)
};
//СПЕЦШРИФТ ДЛЯ ЗНАКОВ 3Х7
const byte font3x7[4][7] PROGMEM ={
{B000,B010,B000,B000,B000,B010,B000},//0 Двоеточие
{B000,B000,B000,B000,B000,B000,B010},//1 Точка
{B000,B000,B000,B111,B000,B000,B000},//2 Минус
{B000,B000,B010,B111,B010,B000,B000} //3 Плюс
};
//СПЕЦШРИФТ ДЛЯ СТРЕЛОЧЕК 5Х7 
const byte font5x7[3][7] PROGMEM ={
{B00100,B01110,B10101,B00100,B00100,B00100,B00100},//0 Стрелка вверх
{B00100,B00100,B00100,B00100,B10101,B01110,B00100},//1 Стрелка вниз
{B00000,B00000,B00000,B00000,B00000,B00000,B00000} //2 Пробел
};
//ОСНОВНОЙ ШРИФТ 4Х7 
const byte font4x7[10][7] PROGMEM ={
{B0110,B1001,B1001,B1001,B1001,B1001,B0110},//0
{B0010,B0110,B0010,B0010,B0010,B0010,B0111},//1
{B0110,B1001,B0001,B0010,B0100,B1000,B1111},//2
{B0110,B1001,B0001,B0010,B0001,B1001,B0110},//3
{B0001,B0011,B0101,B1111,B0001,B0001,B0001},//4
{B1111,B1000,B1110,B0001,B0001,B1001,B0110},//5
{B0110,B1001,B1000,B1110,B1001,B1001,B0110},//6
{B1111,B0001,B0001,B0010,B0010,B0100,B0100},//7
{B0110,B1001,B1001,B0110,B1001,B1001,B0110},//8
{B0110,B1001,B1001,B0111,B0001,B1001,B0110} //9
};


//ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ

//unsigned long от 0 до 4294967295 (4 byte)
uint32_t Timer_AUTORET;//Таймер для функции автовозврата в основное меню
uint32_t Timer_LED;//Таймер для отключения светодиодного индикатора
uint32_t Timer_WATCH;//Таймер опроса модуля часов
uint32_t Timer_TEND_TEMP;//Таймер для отслеживания тенденции температуры

//int от -32768 до 32767 (2 byte)
int16_t MaxTemp=-1000,MinTemp=1000;//Начальные значения невозможных предельных температур
int16_t Temperature,Tend_Temp;//Температура с датчика и временные значения для тенденции
int16_t Fotorezist_OLD;

//byte от 0 до 255 (1 byte)
const uint8_t width=6;//Ширина шрифта 6 пикселей
uint8_t Wait_AutoRet,Trend=3,Timer_1,Timer_2,Timer_3;
uint8_t FL_com_1,FL_com_2,FL_com_3,FL_com_4;
uint8_t maxtemp_hour,maxtemp_min,maxtemp_date;//Метка времени и даты для максимума
uint8_t mintemp_hour,mintemp_min,mintemp_date;//Метка времени и даты для минимума
uint8_t buf[7];//Буфер для хранения принятых данных с радиоприёмника
//uint8_t buflen=sizeof(buf);//Максимальный размер принятых данных

//char от -128 до 127 (Для алфавитно-цифровых символов из таблицы ASCII) (1 byte)
int8_t str[5];//Массив для последующего преобразования ASCII символов в цифры и буквы
int8_t SP_volume,VOL_KEY,PG_KEY,MENU=0,SET_menu_1,SET_menu_2=1,SET_menu_3;
int8_t SetUP,Cur_sec,Cur_min,Cur_hour,Cur_date,Cur_mon,Cur_dow,Cur_year;
int8_t Old_sec,Old_min=60,Old_hour=24;
int8_t Count,LED_Intensity,LDR_Sensor,LED_Intens,LED_Intens_OLD=20;
int8_t Timer_SEC,secund=0,minut=0;

//boolean от false до true (1 byte)
bool REPEAT_COM,Show_Time_Flag,Timer_GO_Flag,Next_Timer,Next_Vol;
bool FL_AutoRet,FL_RADIO,FL_LED_info,FL_START_MENU=true,OK_KEY;
bool FL_EEPROM_Save;
/////////////////////////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////////////////////////
//                                          S E T U P                                          //
/////////////////////////////////////////////////////////////////////////////////////////////////
void setup(){

#ifdef LOG_ENABLE_SERIAL
Serial.begin(Serial_SPEED);
#endif

Wire.begin();//Запускаем шину I2C (можно не указывать, запускается из других библиотек)
irrecv.enableIRIn();//Запускаем приём даннных с инфракрасного приёмника
driver.init();//Инициализация радиоприёмника
rtc.begin();//Запускаем модуль часов
pinMode(LED_INFO_PIN,OUTPUT);//Светодиод-индикатор приёма данных с IR пульта и радиодатчика
pinMode(TONE_PIN,OUTPUT);//Пьезодинамик (piezospeaker)

//matrix.setPosition(A,B,C);//Задаём позицию для каждой матрицы в массиве составляющих экран
//A-номер матрицы в порядке подключения, B-номер в массиве, C-сдвиг на колличество позиций
//Нумерация выполняется снизу вверх и слева направо, здесь: 4 матрицы сверху и 4 снизу

//                     ФИЗИЧЕСКОЕ РАСПОЛОЖЕНИЕ         ЛОГИЧЕСКАЯ НУМЕРАЦИЯ
//ВЕРХНЯЯ ПЛАНКА   ARDUINO->IN0->|0|1|2|3|->OUT3             |1|3|5|7|
//НИЖНЯЯ ПЛАНКА       OUT3->IN4->|4|5|6|7|                   |0|2|4|6|

//ВЕРХНЮЮ планку с матрицами 0-1-2-3 последовательно нумеруем 1-3-5-7
matrix.setPosition(0,1,0);
matrix.setPosition(1,3,0);
matrix.setPosition(2,5,0);
matrix.setPosition(3,7,0);
//НИЖНЮЮ планку с матрицами 4-5-6-7 последовательно нумеруем 0-2-4-6
matrix.setPosition(4,0,0);
matrix.setPosition(5,2,0);
matrix.setPosition(6,4,0);
matrix.setPosition(7,6,0);

//В готовых китайских модулях распиновка матриц не соответствует их реальному расположению
//библиотека умеет "поворачивать" модули по часовой стрелке
//Функцией "matrix.setRotation(A,B)" задаём угол поворота для каждой матрицы где:
//А-номер матрицы в установленном порядке, В-угол поворота в градусах (1-90, 2-180, 3-270)
matrix.setRotation(1);//Если не указан номер матрицы, значит поворачиваем все подключённые
matrix.fillScreen(LOW);//Очистка экрана (гасим все светодиоды на всех матрицах)


//СЧИТЫВАЕМ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ ИЗ EEPROM
//for(int i=0;i<31;i++){EEPROM.update(i,0);}
Timer_1=EEPROM.read(0);//Задержка для таймера № 1 (1...90) минут
Timer_2=EEPROM.read(1);//Задержка для таймера № 2 (1...90) минут
Timer_3=EEPROM.read(2);//Задержка для таймера № 3 (1...90) минут
SP_volume=EEPROM.read(3);//Громкость звуковых сигналов (0...10) попугаев
Wait_AutoRet=EEPROM.read(4);//Время автоматического возврата в главное меню (3...30) секунд
LED_Intensity=EEPROM.read(5);//Яркость всех матриц (0...9) попугаев
//LED_Intens_0=EEPROM.read(5);//Яркость всех матриц при уровне освещённости "0" (0...15) попугаев
//LED_Intens_1=EEPROM.read(6);//Яркость всех матриц при уровне освещённости "1" (0...14) попугаев
//LED_Intens_2=EEPROM.read(7);//Яркость всех матриц при уровне освещённости "2" (0...13) попугаев
//LED_Intens_3=EEPROM.read(8);//Яркость всех матриц при уровне освещённости "3" (0...12) попугаев
//LED_Intens_4=EEPROM.read(9);//Яркость всех матриц при уровне освещённости "4" (0...11) попугаев

//if(Wait_AutoRet<3){Wait_AutoRet=15;}//При самом первом вкючении задаём Wait_AutoRet=15сек. 
//matrix.setIntensity(0);//Задаём начальную минимальную яркость всех матриц
Timer_TEND_TEMP=millis()+TEN_MIN;//Обнуляем таймер тенденции температуры при начальной загрузке

}//END SETUP
/////////////////////////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////////////////////////
//                                           L O O P                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////

void loop(){

/////////////////////////////////////////////////////////////////////////////////////////////////
//                      ОБРАБАТЫВАЕМ КОМАНДЫ С ИНФРАКРАСНОГО ПРИЁМНИКА                         //
/////////////////////////////////////////////////////////////////////////////////////////////////
//IR REMOTE CONTROL 
if(irrecv.decode(&ir)){//Если при опросе IR приёмника в буфере есть код кнопки

#ifdef LOG_ENABLE_IR_COD
Serial.print(F("0x"));Serial.println(ir.value,HEX);//В мониторе порта можно посмотреть код кнопки
#endif

//Для визуализации приёма команд с ПДУ зажигаем светодиод-индикатор вне зависимости от 
//содержимого буфера IR приёмника. (НЕ ИСПОЛЬЗУЕТСЯ) Для активации активируйте команду ниже.
//digitalWrite(LED_INFO_PIN,HIGH);//FL_LED_info=true;Timer_LED=millis();

//Если с ПДУ получен код кнопки не являющийся кодом повтора, блокируем МК на приём любых 
//кодов паузой delay(p150), на 150 миллисекунд (0,15 сек.) 
//тем самым предотвращаем обработку кодов повтора 
//поступающих от ПДУ с очень короткой паузой заданной производителем ПДУ
if(ir.value!=IR_FFFFF){delay(p150);//Count=false;
switch (ir.value){
  case IR_EXIT0: if(MENU){FL_START_MENU=true;}MENU=0;break;//"ГЛАВНОЕ МЕНЮ" (Серая кнопка)
  case IR_EXIT1: if(MENU){FL_START_MENU=true;}MENU=0;break;//"ГЛАВНОЕ МЕНЮ" (Серая кнопка)
  case IR_TEMPER0: Info_Menu();break;//"МАКС И МИН ТЕМПЕРАТУРА" (Зелёная кнопка)
  case IR_TEMPER1: Info_Menu();break;//"МАКС И МИН ТЕМПЕРАТУРА" (Зелёная кнопка)
  case IR_TIMER0: Select_Timer();break;//"ТАЙМЕРЫ" (Белая кнопка)
  case IR_TIMER1: Select_Timer();break;//"ТАЙМЕРЫ" (Белая кнопка)
  case IR_SETUP0: Setup_Menu(); break;//"НАСТРОЙКА ПАРАМЕТРОВ" (Синяя кнопка)
  case IR_SETUP1: Setup_Menu(); break;//"НАСТРОЙКА ПАРАМЕТРОВ" (Синяя кнопка)MENU=3;FL_START_MENU=true;
  case IR_OK0:     OK_KEY=true; break; //"ПОДТВЕРЖДЕНИЕ" (Кнопка "OK")
  case IR_OK1:     OK_KEY=true; break; //"ПОДТВЕРЖДЕНИЕ" (Кнопка "OK")
//  case IR_PG_UP0:     PG_KEY++; break; //"СТРАНИЦА ВВЕРХ" (Кнопка "P+")
//  case IR_PG_UP1:     PG_KEY++; break; //"СТРАНИЦА ВВЕРХ" (Кнопка "P+")
//  case IR_PG_DOWN0:   PG_KEY--; break; //"СТРАНИЦА ВНИЗ" (Кнопка "P-")
//  case IR_PG_DOWN1:   PG_KEY--; break; //"СТРАНИЦА ВНИЗ" (Кнопка "P-")
  case IR_VOL_UP0:   VOL_KEY++; break; //"БОЛЬШЕ" (Кнопка "V+")
  case IR_VOL_UP1:   VOL_KEY++; break; //"БОЛЬШЕ" (Кнопка "V+")
  case IR_VOL_DOWN0: VOL_KEY--; break; //"МЕНЬШЕ" (Кнопка "V-")
  case IR_VOL_DOWN1: VOL_KEY--; break; //"МЕНЬШЕ" (Кнопка "V-")
                }Serial.print(F("PG_KEYДДДД= ")); Serial.println(PG_KEY);
  }
  else{REPEAT_COM=true;}
//Сбрасываем текущее значение полученного кода и переключаем IR приёмник на приём следующего
irrecv.resume();
}//Конец обработки команд с IR приёмника
/////////////////////////////////////////////////////////////////////////////////////////////////




//ГАСИМ СВЕТОДИОД-ИНДИКАТОР ПОСЛЕ ЗАДЕРЖКИ 150 МИЛЛИСЕКУНД (0,15 сек.)
if(millis()-Timer_LED>p200){digitalWrite(LED_INFO_PIN,LOW);}
//FL_LED_info=false;Timer_LED=millis();//FL_LED_info==true&&
/////////////////////////////////////////////////////////////////////////////////////////////////




//КАЖДУЮ СЕКУНДУ ЗАПУСКАЕМ РЯД ФУНКЦИЙ
if(millis()-Timer_WATCH>=p200){//Если millis() превышает или равно Timer_WATCH 
Timer_WATCH=millis();//Присваиваем Timer_WATCH текущее значение millis()
Tmr=rtc.getTime();//Запрашиваем актуальные данные Tmr с RTC каждые 0,2 секунды
if(Old_sec!=Tmr.sec){//Если число секунд изменилось
Old_sec=Tmr.sec;
Show_Time_Flag=true;//Разрешаем вызов функции печати времени
Receiver_Temp();//Опрашиваем модуль приёмника на наличие принятых данных
//Если передатчиков два и более или эфир очень зашумлён, опрашиваем чаще или в основном цикле
if(Timer_GO_Flag){Show_Timer_GO();}//Если запущен таймер вызываем функцию таймера
LED_Brightness();//Проверяем освещённость
}//Прошла 1 секунда
}//Прошло 0,2 секунды
/////////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////////////////////
   switch(MENU){
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                         ГЛАВНОЕ МЕНЮ                                        //
//                            ПЕЧАТАЕМ НА ЭКРАНЕ ВРЕМЯ И ТЕМПЕРАТУРУ                           //
/////////////////////////////////////////////////////////////////////////////////////////////////
    case 0:
/////////////////////////////////////////////////////////////////////////////////////////////////
//ЗАДАЁМ НАЧАЛЬНЫЕ ПАРАМЕТРЫ
if(FL_START_MENU==true){
matrix.fillScreen(LOW);//Очистка экрана
Old_min=60;Old_hour=24;
Show_Time();Show_Temperature();Show_Trend();
TimerFreeTone(TONE_PIN,1800,50,SP_volume);digitalWrite(TONE_PIN,HIGH);//Короткий сигнал
   FL_START_MENU=false;}


//ПРОВЕРЯЕМ ФЛАГИ ИЗМЕНЕНИЯ ВРЕМЕНИ И ТЕМПЕРАТУРЫ И ПРИ НАЛИЧИИ, ПЕЧАТАЕМ ИХ ЗНАЧЕНИЯ
if(Show_Time_Flag==true){Show_Time();}
if(FL_RADIO==true){Show_Temperature();Show_Trend();}

/////////////////////////////////////////////////////////////////////////////////////////////////
    break;//END MAIN MENU 
/////////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////////////////////
//                       МЕНЮ 1. МАКСИМАЛЬНАЯ И МИНИМАЛЬНАЯ ТЕМПЕРАТУРА                        //
/////////////////////////////////////////////////////////////////////////////////////////////////
    case 1: 
/////////////////////////////////////////////////////////////////////////////////////////////////

//ЕСЛИ НАЖАТА КНОПКА "ОК" СБРАСЫВАЕМ МИНИАЛЬНОЕ И МАКСИМАЛЬНОЕ ЗНАЧЕНИЕ ТЕМПЕРАТУРЫ И 
//ПЕРЕХОДИМ В ГЛАВНОЕ МЕНЮ
if(OK_KEY==true){
MaxTemp=-1000;MinTemp=1000;
matrix.fillScreen(LOW);
text("VALUE",1,0);
text("RESET",1,9);
TimerFreeTone(TONE_PIN,1300,200,SP_volume);digitalWrite(TONE_PIN,HIGH);
MENU=false;FL_START_MENU=true;
   OK_KEY=false;}//Конец сброса миниальных и максимальных значений температуры

FN_AutoRet();//Автовозврат в главное меню

/////////////////////////////////////////////////////////////////////////////////////////////////
   break;//END MENU 1. (MIN-MAX TEMPERATURE)
/////////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////////////////////
//                                         МЕНЮ 2. ТАЙМЕРЫ                                     //
/////////////////////////////////////////////////////////////////////////////////////////////////
   case 2:
/////////////////////////////////////////////////////////////////////////////////////////////////
//ПАРАМЕТРЫ ПОДМЕНЮ В РЕЖИМЕ ВЫБОРА ТАЙМЕРА
if(FL_START_MENU==true){
  
//РАЗРЕШАЕМ:
FL_com_1=true;FL_com_2=true;//ПОЛУЧИТЬ И НАПЕЧАТАТЬ ТЕКУЩЕЕ ЗНАЧЕНИЕ ТАЙМЕРА
FL_AutoRet=true;//ФУНКЦИЮ FN_AutoRet() ВОЗВРАТА В ГЛАВНОЕ МЕНЮ

//ЗАПРЕЩАЕМ:
Show_Time_Flag=false;//ПОКАЗ ВРЕМЕНИ
Timer_GO_Flag=false;//ЗАПУСК ТАЙМЕРА 
VOL_KEY=OK_KEY=false;//ДЕЙСТВИЕ КНОПОК СЛУЧАЙНО НАЖАТЫХ РАНЕЕ
matrix.fillScreen(LOW);
text("TIMER", 1,0);
TimerFreeTone(TONE_PIN,2200,50,SP_volume);digitalWrite(TONE_PIN,HIGH);
   FL_START_MENU=false;}

//КНОПКОЙ ТАЙМЕРА ПОСЛЕДОВАТЕЛЬНО ПЕРЕКЛЮЧАЕМ ТАЙМЕРЫ
if(Next_Timer==true){
SET_menu_2=SET_menu_2+Next_Timer;
if(SET_menu_2>3){SET_menu_2=1;}//if(SET_menu_2<1){SET_menu_2=3;}
FL_com_1=true;FL_com_2=true;
TimerFreeTone(TONE_PIN,1200,50,SP_volume);digitalWrite(TONE_PIN,HIGH);
   Next_Timer=false;}

//КНОПКАМИ "+" И "-" УСТАНАВЛИВАЕМ ПРОДОЛЖИТЕЛЬНОСТЬ ТАЙМЕРОВ
if(VOL_KEY!=0){SetUP=SetUP+VOL_KEY;Count=VOL_KEY;VOL_KEY=false;FL_com_2=true;
  }
else{if(REPEAT_COM){SetUP=SetUP+Count;REPEAT_COM=false;FL_com_2=true;}
  }

if(FL_com_1==true){
  switch(SET_menu_2){
    case 1: SetUP=Timer_1;break;
    case 2: SetUP=Timer_2;break;
    case 3: SetUP=Timer_3;break;
  }
  FL_com_1=false;}


if(FL_com_2==true){
if(SetUP>90){SetUP=1;}if(SetUP<1){SetUP=90;}
symb4x7(SET_menu_2, 2,9);

switch(SET_menu_2){

//ТАЙМЕР 1
  case 1: 
if(Timer_1!=SetUP){FL_EEPROM_Save=true;}
Timer_1=SetUP;TWO_symbols(Timer_1);FL_com_1=true;
  break;

//ТАЙМЕР 2
  case 2: 
if(Timer_2!=SetUP){FL_EEPROM_Save=true;}
Timer_2=SetUP;TWO_symbols(Timer_2);FL_com_1=true;
  break;

//ТАЙМЕР 3
  case 3: 
if(Timer_3!=SetUP){FL_EEPROM_Save=true;}
Timer_3=SetUP;TWO_symbols(Timer_3);FL_com_1=true;
  break;
  }//Конец перебора таймеров

Timer_AUTORET=millis();
  FL_com_2=false;}


//КНОПКОЙ "ОК" ЗАПУСКАЕМ ВЫБРАННЫЙ ИЛИ ОСТАНАВЛИВАЕМ ЗАПУЩЕННЫЙ ТАЙМЕР
if(OK_KEY==true){

Timer_GO_Flag=!Timer_GO_Flag;
if(Timer_GO_Flag){
minut=SetUP-1;secund=0;FL_AutoRet=false;Old_min=60;Old_hour=24;
  }else{
  minut=0;secund=0;Timer_SEC=0;FL_START_MENU=true;
  }
matrix.fillScreen(LOW);
TimerFreeTone(TONE_PIN,1800,50,SP_volume);digitalWrite(TONE_PIN,HIGH);

if(FL_EEPROM_Save){FL_EEPROM_Save=false;EEPROM_UPDATE();}//Запись в память

   OK_KEY=false;}//END "OK" KEY


//ЕСЛИ ТАЙМЕР ЗАПУЩЕН ВЫЗЫВАЕМ КАЖДУЮ СЕКУНДУ ФУНКЦИЮ ПЕЧАТИ ВРЕМЕНИ
if(Timer_GO_Flag==true&&Show_Time_Flag==true){Show_Time();}


//АВТОВОЗВРАТ В ГЛАВНОЕ МЕНЮ, ЕСЛИ ТАЙМЕР БЫЛ ВЫБРАН НО НЕ БЫЛ ЗАПУЩЕН
FN_AutoRet();

/////////////////////////////////////////////////////////////////////////////////////////////////
   break;//END MENU 2. (TIMERS)
/////////////////////////////////////////////////////////////////////////////////////////////////



     
/////////////////////////////////////////////////////////////////////////////////////////////////
//                                  МЕНЮ 3. НАСТРОЙКА ПАРАМЕТРОВ                               //
/////////////////////////////////////////////////////////////////////////////////////////////////
   case 3:
/////////////////////////////////////////////////////////////////////////////////////////////////
if(FL_START_MENU==true){
VOL_KEY=OK_KEY=false;FL_AutoRet=true;
FL_com_1=true;FL_com_2=true;
FL_com_3=false;FL_com_4=false;
GET_time();matrix.fillScreen(LOW);
TimerFreeTone(TONE_PIN,4000,50,SP_volume);digitalWrite(TONE_PIN,HIGH);
   FL_START_MENU=false;}


//if(PG_KEY==true){
//Serial.print(F("PG_KEY= ")); Serial.println(PG_KEY);
//SET_menu_3=SET_menu_3+PG_KEY;
//if(SET_menu_3>8){SET_menu_3=0;}if(SET_menu_3<0){SET_menu_3=8;}
//FL_com_1=true;FL_com_2=true;
//TimerFreeTone(TONE_PIN,4000,50,SP_volume);digitalWrite(TONE_PIN,HIGH);
//matrix.fillScreen(LOW);
//Serial.print(F("SET_menu_3= ")); Serial.println(SET_menu_3);
//   PG_KEY=false;}
   
//КНОПКОЙ НАСТРОЙКИ ПАРАМЕТРОВ ПОСЛЕДОВАТЕЛЬНО ПЕРЕКЛЮЧАЕМ ПУНКТЫ МЕНЮ
if(Next_Vol==true){
SET_menu_3=SET_menu_3+Next_Vol;
if(SET_menu_3>8){SET_menu_3=0;}
FL_com_1=true;FL_com_2=true;
TimerFreeTone(TONE_PIN,4000,50,SP_volume);digitalWrite(TONE_PIN,HIGH);//Подаём сигнал
matrix.fillScreen(LOW);
   Next_Vol=false;}


//КНОПКАМИ "+" И "-" УСТАНАВЛИВАЕМ ПАРАМЕТРЫ В ПОДПУНКТАХ МЕНЮ
if(VOL_KEY!=0){SetUP=SetUP+VOL_KEY;Count=VOL_KEY;VOL_KEY=false;FL_com_2=true;}
  else{if(REPEAT_COM==true){SetUP=SetUP+Count;REPEAT_COM=false;FL_com_2=true;}
  }


if(FL_com_1==true){
  switch(SET_menu_3){
    case 0: SetUP=LED_Intensity; break;
    case 1: SetUP=SP_volume;     break;
    case 2: SetUP=Wait_AutoRet;  break;
    case 3: SetUP=Cur_hour;      break;
    case 4: SetUP=Cur_min;       break;
    case 5: SetUP=Cur_date;      break;
    case 6: SetUP=Cur_mon;       break;
    case 7: SetUP=Cur_year;      break;
    case 8: SetUP=Cur_dow;       break;
    }
  FL_com_1=false;}


if(FL_com_2==true){
switch(SET_menu_3){

//ИНТЕНСИВНОСТЬ (ЯРКОСТЬ) ЭКРАНА (0...9 попугаев)
  case 0: 
if(LED_Intensity!=SetUP){FL_com_4=true;}
//LED_Intensity=SetUP;if(LED_Intensity>9){LED_Intensity=0;}if(LED_Intensity<0){LED_Intensity=9;}
LED_Intensity=SetUP;LED_Intensity=constrain(LED_Intensity,0,9);
text("LIGHT",1,0);//Печатаем текст "LIGHT"
symb4x7(LED_Intensity, 0,9);//Печатаем заданную интенсивность (0...9)
LED_Brightness();//Запрашиваем освещённость
TWO_symbols(LDR_Sensor);//Печатаем освещённость (0...15)
FL_com_1=true;
  break;

//ГРОМКОСТЬ ДИНАМИКА (0...10 попугаев)
  case 1: 
if(SP_volume!=SetUP){
FL_com_4=true;TimerFreeTone(TONE_PIN,2000,200,SP_volume);digitalWrite(TONE_PIN,HIGH);
  }
SP_volume=SetUP;if(SP_volume>10){SP_volume=0;}if(SP_volume<0){SP_volume=10;}
text("VOLUM",1,0);TWO_symbols(SP_volume);
FL_com_1=true;
  break;

//ВРЕМЯ ЗАДЕРЖКИ ДО АВТОВОЗВРАТА В ГЛАВНОЕ МЕНЮ (3...90 секунд)
  case 2: 
if(Wait_AutoRet!=SetUP){FL_com_4=true;}
Wait_AutoRet=SetUP;
if(Wait_AutoRet>90){Wait_AutoRet=3;}if(Wait_AutoRet<3){Wait_AutoRet=90;}
text("RETUR",1,0);TWO_symbols(Wait_AutoRet);
FL_com_1=true;
  break;

//ЧАСЫ (0...23)
  case 3: 
if(Cur_hour!=SetUP){FL_com_3=true;}
Cur_hour=SetUP;if(Cur_hour>23){Cur_hour=0;}if(Cur_hour<0){Cur_hour=23;}
text("HOUR",1,0);TWO_symbols(Cur_hour);
FL_com_1=true;
  break;

//МИНУТЫ (0...59)
  case 4: 
if(Cur_min!=SetUP){FL_com_3=true;}
Cur_min=SetUP;if(Cur_min>59){Cur_min=0;}if(Cur_min<0){Cur_min=59;}
text("MINUT",1,0);TWO_symbols(Cur_min);
FL_com_1=true;
  break;

//ЧИСЛО МЕСЯЦА (1...31)
  case 5: 
if(Cur_date!=SetUP){FL_com_3=true;}
Cur_date=SetUP;if(Cur_date>31){Cur_date=1;}if(Cur_date<1){Cur_date=31;}
text("DAY M",1,0);TWO_symbols(Cur_date);
FL_com_1=true;
  break;

//МЕСЯЦ (1...12)
  case 6: 
if(Cur_mon!=SetUP){FL_com_3=true;}
Cur_mon=SetUP;if(Cur_mon>12){Cur_mon=1;}if(Cur_mon<1){Cur_mon=12;}
text("MONTH",1,0);TWO_symbols(Cur_mon);
FL_com_1=true;
  break;

//ГОД (2025...2100) Очень сомневаюсь, что часы проживут больше 75 лет...
  case 7: 
if(Cur_year!=SetUP){FL_com_3=true;}
Cur_year=SetUP;Cur_year=constrain(Cur_year,25,100);
text("YEAR",1,0);text("20",0,9);TWO_symbols(Cur_year);
FL_com_1=true;
  break;

//ДЕНЬ НЕДЕЛИ (1...7)
  case 8: 
if(Cur_dow!=SetUP){FL_com_3=true;}
Cur_dow=SetUP;if(Cur_dow>7){Cur_dow=1;}if(Cur_dow<1){Cur_dow=7;}
text("DAY W",1,0);TWO_symbols(Cur_dow);
FL_com_1=true;
  break;


  }//Конец перебора пунктов меню
Timer_AUTORET=millis();
  FL_com_2=false;}


//Текущая освещённость будет печататься только при регулировке интенсивности экрана,
//Этого вполне достаточно для регулировки, но для экспериментов и постоянного мониторинга 
//уровня освещённости именно на экране (а не через монитор серийного порта) используем:
//if(SET_menu_3==8&&Show_Time_Flag){TWO_symbols(LDR_Sensor);Show_Time_Flag=false;}
//Желательно при этом отключить автовозврат в главное меню или не дольше 90 секунд


//ЗАПИСЫВАЕМ ЗНАЧЕНИЯ ПО НАЖАТИЮ НА КНОПКУ "ОК"
if(OK_KEY==true){
if(FL_com_3){SET_TIME();}//Если менялся параметр часов или даты записываем в модуль DS3231
if(FL_com_4){EEPROM_UPDATE();}//Если менялся параметр кроме времени перезаписываем в память
matrix.fillScreen(LOW);
text("VALUE",1,0);
text("SAVE!",1,9);
TimerFreeTone(TONE_PIN,1200,400,SP_volume);digitalWrite(TONE_PIN,HIGH);
SET_menu_3=0;
MENU=false;FL_START_MENU=true;//Переход в главное меню
   OK_KEY=false;}//Конец записи

//АВТОВОЗВРАТ В ГЛАВНОЕ МЕНЮ, ЕСЛИ ТАЙМЕР БЫЛ ВЫБРАН НО НЕ БЫЛ ЗАПУЩЕН
FN_AutoRet();

/////////////////////////////////////////////////////////////////////////////////////////////////
   break;//END MENU 3 (SETUP TIME AND DATA)
/////////////////////////////////////////////////////////////////////////////////////////////////


  }//END ALL MENU


  }//END LOOP

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////|||///////////////// THE END /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
