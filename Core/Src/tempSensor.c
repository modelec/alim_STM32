/*
 * tempSensor.c
 *
 *  Created on: May 5, 2025
 *      Author: maxch
 */

#include "tempSensor.h"


#include "main.h"

float MCP9808_ReadTemp(I2C_HandleTypeDef *hi2c) {
    uint8_t reg = 0x05; // registre de température
    uint8_t data[2];

    if (HAL_I2C_Master_Transmit(hi2c, 0x30, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
        return -1000.0f; // erreur
    }

    if (HAL_I2C_Master_Receive(hi2c, 0x30, data, 2, HAL_MAX_DELAY) != HAL_OK) {
        return -1000.0f; // erreur
    }

    uint16_t raw = (data[0] << 8) | data[1];
    raw &= 0x1FFF;

    float temp = (raw & 0x1000) ? (raw - 8192) * 0.0625f : raw * 0.0625f;
    return temp;
}
