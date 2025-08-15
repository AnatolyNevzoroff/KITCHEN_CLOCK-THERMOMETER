//////////////////////////////////////////// FUNCTIONS /////////////////////////////////////////



//ФУНКЦИЯ ПРИЁМА И ОБРАБОТКИ ТЕКУЩЕЙ ТЕМПЕРАТУРЫ С РАДИОМОДУЛЯ
void Receiver_Temp(){

uint8_t buflen=sizeof(buf);//Устанавливаем максимальный размер принятого пакета

//Если в буфере приёмника есть принятые данные с передатчика SYN115 (WL102-341)
if(driver.recv(buf,&buflen)){

//Извлекаем из буфера первый фрагмент пакета (первые 3 символа) 
for(byte i=0;i<3;i++){str[i]=buf[i];}

str[3]='\0';//Добавляем идентификатор конца обрабатываемого фрагмента 

//Проверяем каждый принятый байт в массиве str[]
if((str[0]=='T') && (str[1]=='X') && (str[2]=='1')){
  
//Если сигнатура-идентификатор "TX1" совпадает с переданной радиомодулем "TX1"
//следующие 4 байта, это второй фрагмент пакета с температурой полученной с датчика DS18B20,
//извлекаем из буфера второй фрагмент пакета (последние 4 символа) 

for(byte i=3; i<7; i++){str[i-3]=buf[i];}//Заполняем массив str[] данными из второго фрагмента
int temper=atoi(str);//Преобразуем массив str[] в целое число
Temperature=temper-1000;//Вычетаем число "1000" (добавляли при передаче) и получаем температуру
//с десятыми долями Цельсия и знаком
}//Конец проверки наличия сигнала и цикла получения температуры

// Для проверки работы радиодатчика без экрана выводим в монитор порта принятые значения
#ifdef LOG_ENABLE_RADIO
Serial.print(F("Температура с датчика: ")); Serial.println(Temperature);
int whole=Temperature/10;//Делим на 10, и получаем целое число, (int отбросит дробную часть)
int fract=Temperature%10;//Остаток от деления на 10 будет дробной частью (десятые градуса)
Serial.print(F("Отображаемое значение: ")); // Печатаем температуру в привычном виде
if(Temperature<0){Serial.print('-');}else{Serial.print('+');}
Serial.print(whole);Serial.print(',');Serial.println(fract);Serial.println();
#endif

FL_RADIO=true;//Разрешаем печать температуры на экране, если это предусмотрено в меню
//if(!MENU){Show_Temperature();}

//ОТСЛЕЖИВАЕМ МАКСИМАЛЬНУЮ ТЕМПЕРАТУРУ ДЛЯ ДАТЧИКА "TX1"
if(Temperature>MaxTemp){
  MaxTemp=Temperature;maxtemp_hour=Tmr.hour;maxtemp_min=Tmr.min;maxtemp_date=Tmr.date;}
  
//ОТСЛЕЖИВАЕМ МИНИМАЛЬНУЮ ТЕМПЕРАТУРУ ДЛЯ ДАТЧИКА "TX1"
if(Temperature<MinTemp){
  MinTemp=Temperature;mintemp_hour=Tmr.hour;mintemp_min=Tmr.min;mintemp_date=Tmr.date;}

Timer_LED=millis();//FL_LED_info=true;
digitalWrite(LED_INFO_PIN,HIGH);//Если сигнал принят зажигаем светодиод-индикатор

}//Конец обработки принятых данных с датчика "TX1"
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ПЕЧАТАТИ ТЕМПЕРАТУРЫ С ДАТЧИКА "TX1"  
void Show_Temperature(){
//Из-за ограничений экрана и реалий уличной температуры ограничиваем значение 3-мя разрядами
if(Temperature>999){Temperature=999;}
int whole=Temperature/10;//Делим на 10, и получаем целое число, (int отбросит дробную часть)
int fract=Temperature%10;//Остаток от деления на 10 будет дробной частью (десятые градуса)
if(Temperature>0){
symb3x7(3, 0,9);//Знак "+"
}else{
symb3x7(2, 0,9);//Знак "-"
}
symb4x7(whole/10, 5,9);//Первый разряд градусов
symb4x7(whole%10, 10,9);//Второй разряд градусов
symb3x7(1, 14,9);//Точка
symb4x7(fract, 17,9);//Десятые доли градусов
FL_RADIO=false;//Опускаем флаг
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ТЕНДЕНЦИЯ ТЕМПЕРАТУРЫ  
void Show_Trend(){
if(millis()-Timer_TEND_TEMP>TEN_MIN){//Если прошло 600 секунд (10 минут)
Timer_TEND_TEMP=millis();//Переменной Timer_TEND_TEMP присваиваем текущее значение millis()
if(Tend_Temp<Temperature){Trend=1;}//Тенденция роста температуры (Стрелка ВВЕРХ)
if(Tend_Temp>Temperature){Trend=2;}//Тенденция падения температуры (Стрелка ВНИЗ)
if(Tend_Temp==Temperature){Trend=3;}//Тенденция изменения температуры ОТСУТСТВУЕТ
if(!Timer_GO_Flag){symb5x7(Trend-1, 26,9);}//Печатаем тенденцию
Tend_Temp=Temperature;//Сохраняем тукущую температуру для следующего сравнения
}//Конец определения тенденции температуры
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ПЕЧАТИ ТЕКУЩЕГО ВРЕМЕНИ
void Show_Time(){
//СЕКУНДЫ
//if(Old_sec!=Tmr.sec){//Если число секунд изменилось
//Old_sec=Tmr.sec;
symb3x5(Tmr.sec/10, 25,2);//В первый разряд секунд выводим символ десятков секунд
symb3x5(Tmr.sec%10, 29,2);//Во второй разряд секунд выводим символ единиц секунд

//МИНУТЫ
if(Old_min!=Tmr.min){Old_min=Tmr.min;
symb4x7(Tmr.min/10, 14,0);
symb4x7(Tmr.min%10, 19,0);

//ЧАСЫ
if(Old_hour!=Tmr.hour){Old_hour=Tmr.hour;
if(Old_hour>9){
symb4x7(Tmr.hour/10, 0,0);
}else{
matrix.fillRect(0,0, 4,7, LOW);
}
symb4x7(Tmr.hour%10, 5,0);
symb3x7(0, 10,0);//Двоеточие
}//Часы
}//Минуты
Show_Time_Flag=false;
//}//Секунды
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////





//ФУНКЦИЯ РЕАКЦИИ НА НАЖАТИЕ КНОПКИ "МАКС И МИН ТЕМПЕРАТУРЫ" (ВЫБОР ПОДМЕНЮ)
void Info_Menu(){
if(MENU==1){SET_menu_1=!SET_menu_1;}else{MENU=1;SET_menu_1=false;}
FL_AutoRet=true;Timer_AUTORET=millis();OK_KEY=false;
matrix.fillScreen(LOW);
//ПЕЧАТАЕМ:
  switch(SET_menu_1){
    case 0: Show_MAX_MIN_Temp();break;
    case 1: Show_MAX_MIN_Time();break;
    }
TimerFreeTone(TONE_PIN,1600,50,SP_volume);digitalWrite(TONE_PIN,HIGH);//Подаём сигнал
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ПЕЧАТИ МАКСИМАЛЬНОЙ И МИНИМАЛЬНОЙ ТЕМПЕРАТУРЫ ПОСЛЕ ПОСЛЕДНЕГО СБРОСА
void Show_MAX_MIN_Temp(){

//МАКСИМАЛЬНАЯ ТЕМПЕРАТУРА
int whole=MaxTemp/10;int fract=MaxTemp%10;
symb5x7(0, 0,0);//Стрелка ВВЕРХ
if(MaxTemp>0){
symb3x7(3, 8,0);//Знак "+"
}else{
symb3x7(2, 8,0);//Знак "-"
}
symb4x7(whole/10, 12,0);//Первый разряд градусов
symb4x7(whole%10, 17,0);//Второй разряд градусов
symb3x7(1, 21,0);//Точка
symb4x7(fract, 24,0);//Десятые доли градуса

//МИНИМАЛЬНАЯ ТЕМПЕРАТУРА
whole=MinTemp/10;fract=MinTemp%10;
symb5x7(1, 0,9);//Стрелка ВНИЗ
if(MinTemp>0){
symb3x7(3, 8,9);//Знак "+" 
}else{
symb3x7(2, 8,9);//Знак "-"
}
symb4x7(whole/10, 12,9);//Первый разряд градусов
symb4x7(whole%10, 17,9);//Второй разряд градусов
symb3x7(1, 21,9);//Точка
symb4x7(fract, 24,9);//Десятые доли градуса
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ПЕЧАТИ ДАТЫ И ВРЕМЕНИ НАСТУПЛЕНИЯ МАКСИМАЛЬНОЙ И МИНИМАЛЬНОЙ ТЕМПЕРАТУРЫ
void Show_MAX_MIN_Time(){

//МАКСИМАЛЬНАЯ ТЕМПЕРАТУРА ЗАФИКСИРОВАНА:
//ДАТА МЕСЯЦА
symb3x5(maxtemp_date/10, 1,2);//Первый разряд
symb3x5(maxtemp_date%10, 5,2);//Второй разряд
//ВРЕМЯ
symb3x5(maxtemp_hour/10, 14,2);//Первый разряд
symb3x5(maxtemp_hour%10, 18,2);//Второй разряд
symb3x5(10, 21,2);//Двоеточие
symb3x5(maxtemp_min/10, 24,2);//Первый разряд
symb3x5(maxtemp_min%10, 28,2);//Второй разряд

//МИНИМАЛЬНАЯ ТЕМПЕРАТУРА ЗАФИКСИРОВАНА
symb3x5(mintemp_date/10, 1,11);
symb3x5(mintemp_date%10, 5,11);
symb3x5(mintemp_hour/10, 14,11);
symb3x5(mintemp_hour%10, 18,11);
symb3x5(10, 21,11);
symb3x5(mintemp_min/10, 24,11);
symb3x5(mintemp_min%10, 28,11);
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ОБРАТНОГО ОТСЧЁТА ТАЙМЕРА С ОТОБРАЖЕНИЕМ ИНФОРМАЦИИ В ЗАВИСИМОСТИ ОТ ТЕКУЩЕГО МЕНЮ
void Show_Timer_GO(){
Timer_SEC++; secund=60-Timer_SEC; 

switch(MENU){
//В главном меню отображаем только оставшиеся минуты вместо символа тенденции
   case 0:
if(minut==0){//На последней минуте отображаем оставшиеся секунды
symb3x5(secund/10, 25,11);//ПЕРВЫЙ разряд СЕКУНД
symb3x5(secund%10, 29,11);//ВТОРОЙ разряд СЕКУНД
  }else{
symb3x5(minut/10, 25,11);//ПЕРВЫЙ разряд МИНУТ
symb3x5(minut%10, 29,11);//ВТОРОЙ разряд МИНУТ
  }
   break;
//В меню таймера отображаем обратный отсчёт с убывающими секундами и минутами
   case 2:
symb4x7(minut/10, 5,9);//ПЕРВЫЙ разряд МИНУТ
symb4x7(minut%10, 10,9);//ВТОРОЙ разряд МИНУТ
symb3x7(0, 15,9);//Двоеточие
symb4x7(secund/10, 19,9);//ПЕРВЫЙ разряд СЕКУНД
symb4x7(secund%10, 24,9);//ВТОРОЙ разряд СЕКУНД
   break;
}
if(secund==0){Timer_SEC=0; minut--;}//Если минута закончилась, уменьшаем значение
//Если минуты закончились, выводим сообщение, играем мелодию и переходим в основное меню
if(minut<0){
Timer_GO_Flag=false;MENU=false;FL_START_MENU=true;
matrix.fillScreen(LOW);
text("TIMER",1,0);
text("STOP!",1,9);
Melody();//Подаём сигнал
//scrol("Timer expired!");//Бегущая строка
  }
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ РЕАКЦИИ НА НАЖАТИЕ КНОПКИ ВЫБОРА ТАЙМЕРОВ В МЕНЮ ТАЙМЕРА
void Select_Timer(){
  if(MENU==2&&!Timer_GO_Flag){Next_Timer=true;}
  if(MENU!=2&&!Timer_GO_Flag){MENU=2;FL_START_MENU=true;}
  if(MENU!=2&&Timer_GO_Flag){
MENU=2;
matrix.fillScreen(LOW);
FL_AutoRet=false;//Запрещаем автовозврат в главное меню
Old_min=60;Old_hour=24;
Show_Time();//Показ времени
Show_Timer_GO();//Показ таймера
TimerFreeTone(TONE_PIN,2000,50,SP_volume);digitalWrite(TONE_PIN,HIGH);//Подаём сигнал
  }
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ РЕАКЦИИ НА НАЖАТИЕ КНОПКИ SETUP В МЕНЮ НАСТРОЙКИ ПАРАМЕТРОВ
void Setup_Menu(){
  if(MENU==3){Next_Vol=true;}
  if(MENU!=3){MENU=3;FL_START_MENU=true;}//Timer_AUTORET=millis();
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////



//ЗВУКОВОЙ СИГНАЛ СИГНАЛИЗИРУЮЩИЙ ОКОНЧАНИЕ ТАЙМЕРА
void Melody(){
//const int16_t melody[5] = {1600,1200,1200,1500,1700};//Массив ЧАСТОТ звуковых сигналов
const int16_t melody[5] = {2600,2200,2200,2500,2700};
const int16_t duration[5]={ 200, 100, 100, 200, 200};//Массив ДЛИТЕЛЬНОСТИ звуковых сигналов
for(int Note=0;Note<5;Note++){//Перебираем ноты и их длительность из массивов
TimerFreeTone(TONE_PIN, melody[Note], duration[Note], SP_volume);
delay(50);//Пауза между сигналами
}//END SOUND SIGNAL
digitalWrite(TONE_PIN,HIGH);//Высоким уровнем на выходе гасим шумы и помехи
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ВЫБОРА ЯРКОСТИ ЭКРАНА В ЗАВИСИМОСТИ ОТ ОСВЕЩЁННОСТИ И ЗАДАННОГО УРОВНЯ
void LED_Brightness(){
int16_t Fotorezist=analogRead(LDR_PIN);
//Загрубляем замеры АЦП чтоб яркость экрана не скакала при граничных состояниях
if(abs(Fotorezist_OLD-Fotorezist)>40){Fotorezist_OLD=Fotorezist;
//Если осещённость изменилась более чем на 40 единиц
switch (Fotorezist){
  case    0 ... 100:  LDR_Sensor=0; break;
  case  101 ... 200:  LDR_Sensor=1; break;
  case  201 ... 300:  LDR_Sensor=2; break;
  case  301 ... 400:  LDR_Sensor=3; break;
  case  401 ... 500:  LDR_Sensor=5; break;
  case  501 ... 600:  LDR_Sensor=7; break;
  case  601 ... 700:  LDR_Sensor=9; break;
  case  701 ... 700:  LDR_Sensor=11;break;
  case  801 ... 700:  LDR_Sensor=13;break;
  case  901 ... 1024: LDR_Sensor=15;break;
  }//Сопоставляем уровень с АЦП к уровню освещённости в пределах регулировок экрана
}//Текущий уровень освещённости получен

//Если уровень освещённости изменился, меняем яркость экрана с учётом настройки яркости
LED_Intens=(LDR_Sensor+LED_Intensity);if(LED_Intens>15){LED_Intens=15;}
if(LED_Intens!=LED_Intens_OLD){LED_Intens_OLD=LED_Intens;matrix.setIntensity(LED_Intens);}


#ifdef LOG_ENABLE_LDR
//В монитор серийного порта выводим значение АЦП с резистивного делителя (0...1024)
Serial.print(F("ФОТОРЕЗИСТОР: "));Serial.println(Fotorezist);
//Дополнительно выводим уровень освещённости и яркость экрана
Serial.print(F("УРОВЕНЬ ОСВЕЩЁННОСТИ: "));Serial.println(LDR_Sensor);
Serial.print(F("УРОВЕНЬ ЯРКОСТИ ЭКРАНА: "));Serial.println(LED_Intens);
Serial.println();
#endif

}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ВЫВОДА НА ЭКРАН СИМВОЛА ПОД НОМЕРОМ v ИЗ МАССИВА symb3x5 С КООРДИНАТАМИ x_sh И y_sh
void symb3x5 (int v, int x_sh,int y_sh){
for(int y=0; y<5; y++){ //Цикл перебора СТРОК для символа
for(int x=0; x<3; x++){ //Цикл перебора БИТ в строках
byte Led=(pgm_read_byte_near(&font3x5[v][y]) >> (2-x)) & 1;//Выбираем биты из массива
//byte Led=(pgm_read_byte_near(&font3x5[v][y]) & 1 << (2-x));//ИЛИ ТАК :-)
matrix.drawPixel((x_sh+x), (y_sh+y), Led);//Записываем в память состояние
//x_sh-го ПИКСЕЛЯ (0...31) В y_sh-ой СТРОКЕ (0...15)
}//Конец цикла перебора БИТ

//for(int i=3-1; i>=0; i--){ //Для проверки правильности отображения символов без экрана
//int bit = (pgm_read_byte_near(&font3x5[v][y]) >> i) & 1; //Извлекаем i-й бит
//Serial.print(bit); //Выводим бит в порт
//Serial.print(' ');} //Добавляем пробел для разделения битов
//Serial.println(); //Добавляем переход на следующую строку для разделения строк

}//Конец цикла перебора СТРОК

//Serial.println();// Добавляем переход на следующую строку для разделения символов

matrix.write();//Выводим готовое растровое изображение символа на экран
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////

 


//ФУНКЦИЯ ВЫВОДА НА ЭКРАН СИМВОЛА 3 Х 7
void symb3x7 (int v, int x_sh,int y_sh){
for(int y=0; y<7; y++){
for(int x=2; x>= 0; x--){    
byte Led=(pgm_read_byte_near(&font3x7[v][y]) >> x) & 1;
matrix.drawPixel((x_sh+x), (y_sh+y), Led);}}
matrix.write();
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ВЫВОДА НА ЭКРАН СИМВОЛА 4 Х 7
void symb4x7 (int v, int x_sh,int y_sh){
for(int y=0; y<7; y++){
for(int x=0; x<4; x++){    
byte Led=(pgm_read_byte_near(&font4x7[v][y]) >> (3-x)) & 1;
matrix.drawPixel((x_sh+x), (y_sh+y), Led);}}
matrix.write();
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ВЫВОДА НА ЭКРАН СИМВОЛА 5 Х 7
void symb5x7 (int v, int x_sh,int y_sh){
for(int y=0; y<7; y++){
for(int x=4; x>= 0; x--){    
byte Led=(pgm_read_byte_near(&font5x7[v][y]) >> x) & 1;
matrix.drawPixel((x_sh+x), (y_sh+y), Led);}}
matrix.write();
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ПЕЧАТЬ СРАЗУ ДВУХ СИМВОЛОВ
void TWO_symbols (int data){
symb4x7(data/10, 12,9);//Первый разряд
symb4x7(data%10, 17,9);//Второй разряд
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//АВТОВОЗВРАТ В ГЛАВНОЕ МЕНЮ ЧЕРЕЗ ЗАДАННЫЙ ИНТЕРВАЛ БЕЗДЕЙСТВИЯ
void FN_AutoRet(){
if(FL_AutoRet==true&&millis()-Timer_AUTORET>Wait_AutoRet*p1000){
   FL_AutoRet=false;MENU=false;FL_START_MENU=true;}
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ЗАПРОСА И ЗАПИСИ В ПЕРЕМЕННЫЕ ТЕКУЩИХ ЗНАЧЕНИЙ ВРЕМЕНИ И ДАТЫ ИЗ МОДУЛЯ DS3231
void GET_time(){
Tmr=rtc.getTime();//Опрашиваем DS3231
Cur_sec=Tmr.sec;//Секунды
Cur_min=Tmr.min;//Минуты
Cur_hour=Tmr.hour;//Часы
Cur_dow=Tmr.dow;//День недели
Cur_date=Tmr.date;//День месяца
Cur_mon=Tmr.mon;//Месяц
Cur_year=(Tmr.year-2000);//Год
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ЗАПИСИ ТЕКУЩИХ ЗНАЧЕНИЙ ВРЕМЕНИ И ДАТЫ В МОДУЛЬ DS3231
void SET_TIME(){
rtc.setTime(Cur_hour,Cur_min,0);
rtc.setDate(Cur_date,Cur_mon,(Cur_year+2000));
rtc.setDOW(Cur_dow);
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ОБНОВЛЕНИЯ (ПЕРЕЗАПИСИ) ПАРАМЕТРОВ В EEPROM
void EEPROM_UPDATE(){
EEPROM.update(0,Timer_1);
EEPROM.update(1,Timer_2);
EEPROM.update(2,Timer_3);
EEPROM.update(3,SP_volume);
EEPROM.update(4,Wait_AutoRet);
EEPROM.update(5,LED_Intensity);
//EEPROM.update(5,LED_Intens_0);
//EEPROM.update(6,LED_Intens_1);
//EEPROM.update(7,LED_Intens_2);
//EEPROM.update(8,LED_Intens_3);
//EEPROM.update(9,LED_Intens_4);
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////




//ФУНКЦИЯ ВЫВОДА НА ЭКРАН СТАТИЧНОГО ТЕКСТА 
void text (String tape, int x_sh, int y_sh){

//Задаём начальные координаты для вывода группы символов на дисплей 
// X по ширине (ГОРИЗОНТАЛЬНО) 
//int x=(matrix.width()-(tape.length()*width))/2;
//Для центрирования символов на дисплее, вычислим начальную координату в зависимости от длинны
//отнимаем от ширины дисплея половину суммы пикселей всех символов умноженных на ширину одного
 
// Y по высоте (ВЕРТИКАЛЬНО)
//int y=(matrix.height()-8)/2;//Отнимаем от высоты дисплея высоту одного символа и делим пополам
//matrix.fillScreen(LOW);//Очистка дисплея (гасим все светодиоды на всех матрицах)
    
for(int i=0; i<tape.length(); i++){   

matrix.drawChar(x_sh,y_sh,tape[i],HIGH,LOW,1);//drawChar(X, Y, СИМВОЛ, ЦВЕТ, ФОН, РАЗМЕР);
x_sh=x_sh+width;}

matrix.write();
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////





//ФУНКЦИЯ ПРОКРУТКИ ТЕКСТА "БЕГУЩАЯ СТРОКА" ПО ЦЕНТРУ ЭКРАНА (НЕ ИСПОЛЬЗУЕТСЯ)
void scrol (String tape){
//matrix.fillScreen(LOW);

for(int i=0; i<width*tape.length()+matrix.width()-1-spacer; i++ ){

//ЗАДАЁМ НАЧАЛЬНЫЕ КООРДИНАТЫ
int letter=i/width;
int x=(matrix.width()-1)-i%width;
int y=(matrix.height()-8)/2; //Центрируем строку по вертикали

while(x+width-spacer>=0 && letter>=0){//До тех пор пока все символы не будут сформированы
if(letter<tape.length()){//Если длинна текста в символах превышает общее количество символов
matrix.drawChar(x,y,tape[letter],HIGH,LOW,1);}//drawChar(x,y,символ,цвет,фон,размер);
letter--; x=x-width;}

matrix.write();//Выводим растровое изображение на составленный из матриц экран
delay(wait);//Небольшая задержка чтоб текст двигался небольшими рывками
  }//Конец цикла
}//////////////////////////////////////////END FUNCTION//////////////////////////////////////////





///////////////////////////////////////// END FUNCTIONS /////////////////////////////////////////
