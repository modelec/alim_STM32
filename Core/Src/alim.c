#include "main.h"
#include "alim.h"
#include "comm.h"
#include <stdio.h>
#include <stdlib.h>


HAL_StatusTypeDef read_bau_state(int* val){
	if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET){
		*val = 0;
		return HAL_ERROR;
	} else {
		*val = 1;
		return HAL_ERROR;
	}
	return HAL_OK;
}

HAL_StatusTypeDef read_mcp9808_temp(float* temp) {
    uint8_t reg = 0x05;
    uint8_t data[2];

    if (HAL_I2C_Master_Transmit(&hi2c1, I2C_TEMP << 1, &reg, 1, HAL_MAX_DELAY) != HAL_OK){
    	return HAL_ERROR;
    }
    if (HAL_I2C_Master_Receive(&hi2c1, I2C_TEMP << 1, data, 2, HAL_MAX_DELAY) != HAL_OK){
    	return HAL_ERROR;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    raw &= 0x1FFF;

    *temp = raw & 0x1000 ? (raw - 8192) * 0.0625f : raw * 0.0625f;
    return HAL_OK;
}

HAL_StatusTypeDef ina236_write_reg(i2cAdress address, uint8_t reg, uint16_t value) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8);
    data[2] = (value & 0xFF);
    return HAL_I2C_Master_Transmit(&hi2c1, address << 1, data, 3, HAL_MAX_DELAY);
}

HAL_StatusTypeDef ina236_read_reg(i2cAdress address, uint8_t reg, uint16_t *value) {
    uint8_t data[2];
    if (HAL_I2C_Master_Transmit(&hi2c1, address << 1, &reg, 1, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;
    if (HAL_I2C_Master_Receive(&hi2c1, address << 1, data, 2, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;

    *value = (data[0] << 8) | data[1];
    return HAL_OK;
}

HAL_StatusTypeDef ina236_init(i2cAdress address, float r_shunt, float current_lsb) {
    uint16_t calibration = (uint16_t)(0.00512 / (r_shunt * current_lsb));
    if(ina236_write_reg(address, 0x05, calibration) != HAL_OK){
    	return HAL_ERROR;
    }

    uint16_t config = 0x4127; // Exemple: moyenne x4, conversion 1.1ms, mode shunt+bus continuous
    if(ina236_write_reg(address, 0x00, config) != HAL_OK){
    	return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef read_ina236_voltage(i2cAdress address, float *voltage) {
    uint16_t raw;
    if (ina236_read_reg(address, 0x02, &raw) != HAL_OK){
    	return HAL_ERROR;
    }
    *voltage = (raw * 1.25f); // 1.25 mV/bit → en mV
    return HAL_OK;
}

HAL_StatusTypeDef read_ina236_current(i2cAdress address, float currentLSB, float *current) {
    uint16_t raw;
    if (ina236_read_reg(address, 0x04, &raw) != HAL_OK){
    	return HAL_ERROR;
    }
    *current = ((int16_t)raw) * currentLSB * 1000; // en mA
    return HAL_OK;
}


