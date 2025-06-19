#include <avr/io.h>
#include <util/delay.h>
#include "i2c_lcd.h"
#include "I2C_Master.h"

#define SLAVE_ADDRESS_LCD 0x27  // 7-���� I2C-������ lcd �������
// ������� ������� �� lcd
void lcd_send_cmd (char cmd)
{
	char data_u, data_l;
	uint8_t data_t[4];
	// ������� ���� �� ������ 4 ��� �� ������� 4 ���
	data_u = (cmd&0xf0);      // ������ ���
	data_l = ((cmd<<4)&0xf0); // ������� ���
    // ������� 4 ����� ��� ��������
	data_t[0] = data_u|0x0C;  //en=1, rs=0
	data_t[1] = data_u|0x08;  //en=0, rs=0
	data_t[2] = data_l|0x0C;  //en=1, rs=0
	data_t[3] = data_l|0x08;  //en=0, rs=0
	// ��������� ��� �� I2C
	I2C_Write (SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4);
}
// ������� ������� �� lcd
void lcd_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0); // ������ ���
	data_l = ((data<<4)&0xf0);  // ������� ���
	// ������� ����� � rs=1
	data_t[0] = data_u|0x0D;  //en=1, rs=1
	data_t[1] = data_u|0x09;  //en=0, rs=1
	data_t[2] = data_l|0x0D;  //en=1, rs=1
	data_t[3] = data_l|0x09;  //en=0, rs=1
	// �������
	I2C_Write (SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4);
}
// ����� lcd
void lcd_clear (void)
{
	lcd_send_cmd (0x01);  // ������� ��������
	_delay_ms(10);
}
// ���������� ������ � ������� �������
void lcd_put_cur(int row, int col)
{
	switch (row)
	{
		case 0:
		col |= 0x80; // ������ �����
		break;
		case 1:
		col |= 0xC0; // ������ �����
		break;
	}

	lcd_send_cmd (col); // ������� ������ �������
}

// �������� LCD �������
void lcd_init (void)
{
	I2C_Init();  // ����������� I2C
	_delay_ms(100); // �������� ��� ������� LCD
	// ����������� � 4-������ �����
	_delay_ms(50);  
	lcd_send_cmd (0x30);
	_delay_ms(5);  
	lcd_send_cmd (0x30);
	_delay_ms(1); 
	lcd_send_cmd (0x30);
	_delay_ms(10);
	lcd_send_cmd (0x20);  // 4-����� �����
	_delay_ms(10);

// ������������ �������
	lcd_send_cmd (0x28);  // 2 �����, 5x8 ������
	_delay_ms(1);
	lcd_send_cmd (0x08); // �������� �������
	_delay_ms(1);
	lcd_send_cmd (0x01);   // �������� �����
	_delay_ms(10);
	lcd_send_cmd (0x06); // ����������� ���������� �������
	_delay_ms(1);
	lcd_send_cmd (0x0C); // �������� �������, ������ � �������� � �������
}
// �������� ��������� ����� �� lcd
void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++); // ������� ����� ������
}
