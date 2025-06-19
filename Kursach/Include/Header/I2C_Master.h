#ifndef I2C_MASTER_H
#define	I2C_MASTER_H

#ifdef	__cplusplus  // �������� ����������������� ��� �� ��� �� ��� C++
extern "C" {
#endif

#include <avr/io.h>

    void I2C_Init (void); // ����������� I2C
    uint8_t I2C_Write (uint8_t slaveAddr7b, uint8_t *data, uint8_t size); // ������� ��������������� ��� �������� ����� �� I2C ( slaveAddr7b - 7-���� I2C-������ ��������, 
	                                                                      // *data - �������� �� ����� �����,  size - ������ ����� �������� )


#ifdef	__cplusplus
}
#endif

#endif	/* I2C_MASTER_H */

