/*
 * alim.h
 *
 *  Created on: May 11, 2025
 *      Author: allan
 */

#ifndef INC_ALIM_H_
#define INC_ALIM_H_

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

typedef enum i2cAdress {
	I2C_TEMP = 0x18,
	I2C_IN1 = 0x48,
	I2C_IN2 = 0x4A,
	I2C_OUT5V = 0x43,
	I2C_OUT5V1 = 0x40,
	I2C_OUT12V = 0x41,
	I2C_OUT24V = 0x4B
} i2cAdress;


// Prototypes
HAL_StatusTypeDef read_bau_state(int* val);
HAL_StatusTypeDef read_mcp9808_temp(float* val);
HAL_StatusTypeDef ina236_write_reg(i2cAdress address, uint8_t reg, uint16_t value);
HAL_StatusTypeDef ina236_read_reg(i2cAdress address, uint8_t reg, uint16_t *value);
HAL_StatusTypeDef ina236_init(i2cAdress address, float r_shunt, float current_lsb);
HAL_StatusTypeDef read_ina236_voltage(i2cAdress address, float *voltage);
HAL_StatusTypeDef read_ina236_current(i2cAdress address, float currentLSB, float *current);

#endif /* INC_ALIM_H_ */
