#define F_CPU 8000000UL
//----------------------------------------------------|
//			Підключені	Бібліотеки                    |
//----------------------------------------------------|
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>

#include "i2c_lcd.h"    // користувацька бібліотека для роботи з LCD по I2C
#include "I2C_Master.h" // користувацька бібліотека для ініціалізації I2C
//----------------------------------------------------|
//----------------------------------------------------|
//				Define для USART                      |
//----------------------------------------------------|
#define BAUD 9600UL	// Швидкість обміну USART
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1) // розрахунок значення UBRR
//----------------------------------------------------|
//----------------------------------------------------|
//				Глобальні змінні					  |
//----------------------------------------------------|
int mseconds = 0, seconds = 0, minutes = 25, hours = 0; //змінні для годинника 
int sw_msec = 0, sw_sec = 0, sw_min = 0, sw_hr = 0, stopwatch = 0; //змінні для секундоміра
int hour_changed = 0, led_blinks_remaining = 0, autosave_counter = 0; //прапорці
//----------------------------------------------------|
//----------------------------------------------------|
//				Ініціалізація  USART				  |
//----------------------------------------------------|
void usart_Init(unsigned int ubrr) {
	UBRRH = (unsigned char)(ubrr >> 8); // cтарший байт UBRR
	UBRRL = (unsigned char)ubrr;		// молодший байт UBRR
	UCSRB = (1 << TXEN) | (1 << RXEN);	// увімкнення TX та RX
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); 
	DDRD &= ~0x01; // PD0 як вхід (RX)
	DDRD |= 0x02;  // PD1 як вихід (TX)
}

void usart_Transmit(unsigned char data) {
	while (!(UCSRA & (1 << UDRE))); //перевіряє чи вільний регістр
	UDR = data;			// Повертаємо отриманий символ
}

void send_str(const char* str) {
	while (*str) {
		usart_Transmit(*str++); // передаємо символ за символом
		_delay_ms(1); // затримка між символами
	}
}

unsigned char usart_Receive(void) {
	while (!(UCSRA & (1 << RXC))); // очікуємо на прийом символу
	return UDR;	 // повертаємо отриманий символ
}

void usart_ReceiveLine(char* buffer, int max_len) {
	int i = 0;
	char c;
	send_str("\r\n");
	while (i < max_len - 1) {
		c = usart_Receive(); // отримуємо символ
		if ((c == '\b' || c == 0x7F) && i > 0) {
			i--;	// обробка клавіші backspace
			usart_Transmit('\b');
			usart_Transmit(' ');
			usart_Transmit('\b');
			continue;
		}
		usart_Transmit(c); // вивід введених користувачем символів
		if (c == '\r' || c == '\n') break; // кінець рядка
		buffer[i++] = c;
	}
	buffer[i] = '\0'; // завершення рядка
}
//----------------------------------------------------|
//----------------------------------------------------|
//				Ініціалізація  EEPROM				  |
//----------------------------------------------------|
uint8_t EEMEM eeprom_hours;
uint8_t EEMEM eeprom_minutes;
uint8_t EEMEM eeprom_seconds;
uint16_t EEMEM eeprom_mseconds;

void save_time_to_eeprom(void) {
	eeprom_update_byte(&eeprom_hours, (uint8_t)hours); // збереження години
	eeprom_update_byte(&eeprom_minutes, (uint8_t)minutes); // збереження хвилини
	eeprom_update_byte(&eeprom_seconds, (uint8_t)seconds); // збереження секунд
	eeprom_update_byte(&eeprom_mseconds, (uint16_t)mseconds); // збереження мілісекунд
}

void load_time_from_eeprom(void) {
	hours = eeprom_read_byte(&eeprom_hours); 
	minutes = eeprom_read_byte(&eeprom_minutes);
	seconds = eeprom_read_byte(&eeprom_seconds);
	mseconds = eeprom_read_byte(&eeprom_mseconds);
	// перевірка на коректність зчитаних даних
	if (hours > 23) hours = 0;
	if (minutes > 59) minutes = 0;
	if (seconds > 59) seconds = 0;
	if (mseconds > 999) mseconds = 0;
}
//----------------------------------------------------|
//----------------------------------------------------|
//				Налаштування таймера    			  |
//----------------------------------------------------|

void Timer1_Init(){ 
	TCCR1A = 0; // звичайний режим
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC + дільник на 64
	OCR1A = 125; // TOP-значення, яке задає інтервал: 8 МГц / 64 / 125 = 1 кГц (1 мс)
	TIMSK |= (1 << OCIE1A);  // дозволити переривання
	sei(); // глобальний дозвіл переривань
}

ISR(TIMER1_COMPA_vect){
// годинник
		mseconds++;
		if (mseconds >= 1000){
			mseconds = 0;
			seconds++;
			if (seconds >= 60){
				seconds = 0;
				minutes++;
				if (minutes >= 60){
					minutes = 0;
					hours = (hours + 1) % 24;
					
					hour_changed = 1; // прапорець, що змінився час
					led_blinks_remaining = hours;  // кількість миготінь діода
				}
			}
		}

		autosave_counter++; 
		if (autosave_counter >= 60) {  // Збереження раз на 60 секунд
		save_time_to_eeprom();
		autosave_counter = 0;
		}

// секундомір
		if (stopwatch){
			sw_msec++;
			if (sw_msec >= 1000){
				sw_msec = 0;
				sw_sec++;
				if (sw_sec >= 60){
					sw_sec = 0;
					sw_min++;
					if (sw_min >= 60){
						sw_min = 0;
						sw_hr++;
					}
				}
			}
		}
}
//----------------------------------------------------|
//----------------------------------------------------|
//		Ініціалізація годинника та секундоміра    	  |
//----------------------------------------------------|
void Clock(){
	char buffer[5];
	lcd_put_cur(0,4);
	IntToStr(hours, buffer, 2);
	lcd_send_string(buffer);
	lcd_send_string(":");
	IntToStr(minutes, buffer, 2);
	lcd_send_string(buffer);
	lcd_send_string(":");
	IntToStr(seconds, buffer, 2);
	lcd_send_string(buffer);
	_delay_ms(200);
}

void Stopwatch(){
	char buffer[5];
	lcd_put_cur(0, 2);
	IntToStr(sw_hr, buffer, 2);
	lcd_send_string(buffer);
	lcd_send_string(":");
	IntToStr(sw_min, buffer, 2);
	lcd_send_string(buffer);
	lcd_send_string(":");
	IntToStr(sw_sec, buffer, 2);
	lcd_send_string(buffer);
	lcd_send_string(":");
	IntToStr(sw_msec / 10, buffer, 2);
	lcd_send_string(buffer);
	_delay_ms(200);
}
//----------------------------------------------------|
//				Функції для часу					  |
//----------------------------------------------------|
//функція для відображення цілих значень у вигляді рядка
void IntToStr(int value, char* buffer, int digits){
	for (int i = digits - 1; i >= 0; i--) {
		buffer[i] = '0' + (value % 10);
		value /= 10;
	}
	buffer[digits] = '\0'; // завершення рядка
}

// формування рядка часу
void usart_Time(char* timeStr) {
	char buffer[3];
	
	IntToStr(hours, buffer, 2);
	strcpy(timeStr, buffer);
	strcat(timeStr, ":");
	IntToStr(minutes, buffer, 2);
	strcat(timeStr, buffer);
	strcat(timeStr, ":");
	IntToStr(seconds, buffer, 2);
	strcat(timeStr, buffer);
}
// секундоміра 
void usart_time_stopwatch(char* timeStr) {
	char buffer[4];
	
	IntToStr(sw_hr, buffer, 2);
	strcpy(timeStr, buffer);
	strcat(timeStr, ":");
	IntToStr(sw_min, buffer, 2);
	strcat(timeStr, buffer);
	strcat(timeStr, ":");
	IntToStr(sw_sec, buffer, 2);
	strcat(timeStr, buffer);
	strcat(timeStr, ":");
	IntToStr(sw_msec, buffer, 2);
	strcat(timeStr, buffer);
}
// функція миготіння світлодіода
void check_and_blink_led() {
	if (hour_changed && led_blinks_remaining > 0) {
		for (int i = 0; i < led_blinks_remaining; i++) {
			PORTD |= 0x20;
			_delay_ms(200);
			PORTD &= ~0x20;
			_delay_ms(200);
		}
		hour_changed = 0; // скидаємо прапорець
	}
}
//----------------------------------------------------|
//----------------------------------------------------|
//				   Функція main    			     	  |
//----------------------------------------------------|
int main(){
	char buf[30]; // буфер для команд
	char timeStr[9]; // буфер для рядка часу
	DDRD |= 0x20;  // PD5 як вихід ( зелений світлодіод )
	
	usart_Init(MYUBRR);	// ініціалізація USART
	lcd_init();	 // ініціалізація LCD
	Timer1_Init(); // ініціалізація таймера
	load_time_from_eeprom(); // завантаження часу
// початковий екран з поточним часом
	while (1) {
		check_and_blink_led(); //миготіння світлодіода
		send_str("\rTime now (HH:MM:SS): "); 
		usart_Time(timeStr); // формування рядка часу
		send_str(timeStr);
		Clock(); // вивід на LCD
		// очікування натискання будь-якої клавіші
		if (UCSRA & (1 << RXC)) {
			send_str("\r\n");
			usart_ReceiveLine(buf, sizeof(buf));
			break;
		}
	}
	while (1){
// якщо година змінилась - мигає стільки разів, скільки і годин стало
		 if (hour_changed && led_blinks_remaining > 0) {
			 for (int i = 0; i < led_blinks_remaining; i++) {
				 PORTD ^= 0x20; // перемикаємо світлодіод
				 _delay_ms(200);
				 PORTD ^= 0x20; // вимикаємо
				 _delay_ms(200);
			 }
			 hour_changed = 0; // скидаємо прапорець
		 }
		// анімація меню
		for (int i = 0; i < 4; i++) {
			lcd_put_cur(1, 0);
			lcd_send_cmd(0x01);
			_delay_ms(200);
			lcd_put_cur(1, 0);
			lcd_send_string("*Main menu*");
			_delay_ms(200);
		}
		// вивід головного меню в термінал
		send_str("\x1B[2J\x1B[H"); // очищення термінала 
		send_str("\n\r_____________Main menu_____________");
		send_str("\n\r-----------------------------------");
		send_str("\n\rInstructions:                ");
		send_str("\n\r-----------------------------------");
		send_str("\n\r|Mode          | - |Command    |");
		send_str("\n\r-----------------------------------");
		send_str("\n\r|Current time  | - |'Time'     |");
		send_str("\n\r|Configure time| - |'Config'   |");
		send_str("\n\r|Stopwatch     | - |'Stopwatch'|");
		send_str("\n\r-----------------------------------");
		send_str("\n\r___________________________________");
		send_str("\n\r\n\rEnter command: ");
		usart_ReceiveLine(buf, sizeof(buf)); // отримання команди
		//команда Time
		if (strcmp(buf, "Time") == 0 || strcmp(buf, "time") == 0 || strcmp(buf, "TIME") == 0) {
		lcd_send_cmd (0x01);
		while (1) {
			check_and_blink_led();
			send_str("\rTime now (HH:MM:SS): ");
			usart_Time(timeStr);
			send_str(timeStr);
			Clock();
			if (UCSRA & (1 << RXC)) {
				usart_ReceiveLine(buf, sizeof(buf));
				send_str("\r\n");
				break;
		}
		}
		}
		// команда Config
		if (strcmp(buf, "Config") == 0 || strcmp(buf, "config") == 0 || strcmp(buf, "CONFIG") == 0) {
		lcd_send_cmd (0x01);
		for (int i = 0; i < 4; i++) { 
			lcd_put_cur(1, 0);
			lcd_send_cmd(0x01);
			_delay_ms(200);
			lcd_put_cur(1, 0);
			lcd_send_string("*Time settings*");
			_delay_ms(200);
		}
		while (1) {
			send_str("\n\rWrite hours: ");
			usart_ReceiveLine(buf, sizeof(buf));
			int h = atoi(buf);
			if (h >= 0 && h <= 23) {
				hours = h;
				break;
				} else {
				send_str("\n\rInvalid input!.");
			}
		}
		while (1) {
			send_str("\n\rWrite minutes: ");
			usart_ReceiveLine(buf, sizeof(buf));
			int m = atoi(buf);
			if (m >= 0 && m <= 59) {
				minutes = m;
				break;
				} else {
				send_str("\n\rInvalid input!");
			}
		}
		while (1) {
			send_str("\n\rWrite seconds: ");
			usart_ReceiveLine(buf, sizeof(buf));
			int s = atoi(buf);
			if (s >= 0 && s <= 59) {
				seconds = s;
				break;
				} else {
				send_str("\n\rInvalid input!");
			}
		}
		lcd_send_cmd(0x01);
	}
	// команда Stopwatch
		if (strcmp(buf, "Stopwatch") == 0 || strcmp(buf, "stopwatch") == 0 || strcmp(buf, "STOPWATCH") == 0) {
			lcd_send_cmd(0x01);
			for (int i = 0; i < 4; i++) {
				lcd_put_cur(1, 0);
				lcd_send_cmd(0x01);
				_delay_ms(200);
				lcd_put_cur(1, 0);
				lcd_send_string("*Stopwatch*");
				_delay_ms(200);
			}
			send_str("\x1B[2J\x1B[H"); 
			send_str("\n\rStopwatch started. Use commands:\n\r");
			send_str("'Start' - begin\n\r'Stop' - pause\n\r'Reset' - zero\n\r'Exit' - return to main menu\n\r");

		while (1) {
			if (stopwatch) {
				check_and_blink_led();
				send_str("\rStopwatch (HH:MM:SS:MS): ");
				usart_time_stopwatch(timeStr);
				send_str(timeStr);
				Stopwatch();
			}

			if (UCSRA & (1 << RXC)) {
				usart_ReceiveLine(buf, sizeof(buf));
				
				if (strcmp(buf, "Start") == 0 || strcmp(buf, "start") == 0 || strcmp(buf, "START") == 0) {
					lcd_send_cmd (0x01);
					stopwatch = 1;
					} else if (strcmp(buf, "Stop") == 0 || strcmp(buf, "stop") == 0 || strcmp(buf, "STOP") == 0) {
					stopwatch = 0;
					} else if (strcmp(buf, "Reset") == 0 || strcmp(buf, "reset") == 0 || strcmp(buf, "RESET") == 0) {
					stopwatch = 0;
					sw_hr = sw_min = sw_sec = sw_msec = 0;
					Stopwatch();
					} else if (strcmp(buf, "Exit") == 0 || strcmp(buf, "exit") == 0 || strcmp(buf, "EXIT") == 0) {
					stopwatch = 0;
					break;
				}
			}
		}
			lcd_send_cmd(0x01); // очищення LCD
		}
	}
}