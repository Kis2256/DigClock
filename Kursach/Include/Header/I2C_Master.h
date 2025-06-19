#ifndef I2C_MASTER_H
#define	I2C_MASTER_H

#ifdef	__cplusplus  // дозволяє використовувавати цей же код на мові C++
extern "C" {
#endif

#include <avr/io.h>

    void I2C_Init (void); // ініціалізація I2C
    uint8_t I2C_Write (uint8_t slaveAddr7b, uint8_t *data, uint8_t size); // функція використовується для відправки даних по I2C ( slaveAddr7b - 7-бітна I2C-адреса пристрою, 
	                                                                      // *data - вказівник на масив даних,  size - скільки байтів передати )


#ifdef	__cplusplus
}
#endif

#endif	/* I2C_MASTER_H */

