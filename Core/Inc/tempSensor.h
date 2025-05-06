#ifndef INC_TEMPSENSOR_H_
#define INC_TEMPSENSOR_H_

#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_i2c.h" // <-- Ajoute cette ligne pour que I2C_HandleTypeDef soit reconnu

float MCP9808_ReadTemp(I2C_HandleTypeDef *hi2c);

#endif /* INC_TEMPSENSOR_H_ */
