#include <avr/io.h>
#include <util/delay.h>
#include "i2c_lcd.h"
#include "I2C_Master.h"

#define SLAVE_ADDRESS_LCD 0x27  // 7-бітна I2C-адреса lcd дисплея
// надсилає команду до lcd
void lcd_send_cmd (char cmd)
{
	char data_u, data_l;
	uint8_t data_t[4];
	// розбиває байт на старші 4 біти та молодші 4 біти
	data_u = (cmd&0xf0);      // старші біти
	data_l = ((cmd<<4)&0xf0); // молодші біти
    // формуємо 4 байти для передачі
	data_t[0] = data_u|0x0C;  //en=1, rs=0
	data_t[1] = data_u|0x08;  //en=0, rs=0
	data_t[2] = data_l|0x0C;  //en=1, rs=0
	data_t[3] = data_l|0x08;  //en=0, rs=0
	// надсилаємо дані по I2C
	I2C_Write (SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4);
}
// надсилає символи до lcd
void lcd_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0); // старші біти
	data_l = ((data<<4)&0xf0);  // молодші біти
	// формуємо байти з rs=1
	data_t[0] = data_u|0x0D;  //en=1, rs=1
	data_t[1] = data_u|0x09;  //en=0, rs=1
	data_t[2] = data_l|0x0D;  //en=1, rs=1
	data_t[3] = data_l|0x09;  //en=0, rs=1
	// надсилає
	I2C_Write (SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4);
}
// очищає lcd
void lcd_clear (void)
{
	lcd_send_cmd (0x01);  // команда очищення
	_delay_ms(10);
}
// встановлює курсор у вказану позицію
void lcd_put_cur(int row, int col)
{
	switch (row)
	{
		case 0:
		col |= 0x80; // перший рядок
		break;
		case 1:
		col |= 0xC0; // другий рядок
		break;
	}

	lcd_send_cmd (col); // надсилає адресу позиції
}

// ініціалізує LCD дисплей
void lcd_init (void)
{
	I2C_Init();  // ініціалізація I2C
	_delay_ms(100); // затримка для запуску LCD
	// ініціалізація в 4-бітному режимі
	_delay_ms(50);  
	lcd_send_cmd (0x30);
	_delay_ms(5);  
	lcd_send_cmd (0x30);
	_delay_ms(1); 
	lcd_send_cmd (0x30);
	_delay_ms(10);
	lcd_send_cmd (0x20);  // 4-бітний режим
	_delay_ms(10);

// конфігурація дисплея
	lcd_send_cmd (0x28);  // 2 рядки, 5x8 пікселів
	_delay_ms(1);
	lcd_send_cmd (0x08); // вимкнути дисплей
	_delay_ms(1);
	lcd_send_cmd (0x01);   // очистити екран
	_delay_ms(10);
	lcd_send_cmd (0x06); // автоматичне переміщення курсора
	_delay_ms(1);
	lcd_send_cmd (0x0C); // увімкнути дисплей, курсор і блимання — вимкнені
}
// виводить текстовий рядок на lcd
void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++); // надсилає кожен символ
}
