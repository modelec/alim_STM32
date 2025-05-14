#include "comm.h"
#include "main.h"
#include "alim.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

static char rxBuffer[RX_BUFFER_SIZE];
static uint8_t rxIndex = 0;
static char rxChar;

// Lancer la réception UART
void uart_start_reception() {
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&rxChar, 1);
}

// Réception d'un caractère
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        if (rxChar == '\n') {
            rxBuffer[rxIndex] = '\0';
            handle_uart_message(rxBuffer);
            rxIndex = 0;
        } else {
            if (isprint(rxChar) && rxIndex < RX_BUFFER_SIZE - 1) {
                rxBuffer[rxIndex++] = rxChar;
            } else {
                // Caractère non imprimable ou overflow
                rxIndex = 0;
            }
        }
        HAL_UART_Receive_IT(&huart2, (uint8_t *)&rxChar, 1);
    }
}

// Traitement d'une ligne complète
void handle_uart_message(const char *msg) {
    char command[8] = "", xxx[16] = "", yyy[16] = "", val[16] = "";
    int n = sscanf(msg, "%7[^;];%15[^;];%15[^;\n];%15[^;\n]", command, xxx, yyy, val);

    if (strcmp(command, "GET") == 0 && n == 3) {
        handle_get(xxx, yyy);
    } else if (strcmp(command, "SET") == 0 && n == 4) {
        handle_set(xxx, yyy, val);
    } else {
        send_error_response();
    }
}

// HANDLERS

void handle_get(const char *xxx, const char *yyy) {
    if (strcmp(xxx, "BAU") == 0 && strcmp(yyy, "STATE") == 0) {
    	int val = 0;
    	HAL_StatusTypeDef res = read_bau_state(&val);
    	if(res == HAL_OK){
    		send_set_response(xxx, yyy, val);
    		return;
    	}
    } else if (strcmp(xxx, "TEMP") == 0 && strcmp(yyy, "CELS") == 0) {
        float temp = -1000.0f;
        HAL_StatusTypeDef res = read_mcp9808_temp(&temp);
        if (res == HAL_OK) {
            int temp_dixieme = (int)(temp * 10.0f + 0.5f);
            send_set_response(xxx, yyy, temp_dixieme);
            return;
        }
    } else {
    	// Concerne les entrees sorties

    	i2cAdress address;
    	float currentLSB;
    	bool validZone = false;
    	uint16_t controlPin;
    	uint16_t validPin;

    	// Selection zone mesure
    	if(strcmp(xxx, "IN1") == 0){
    		address = I2C_IN1;
    		currentLSB = 0.000610f;
    		validPin = GPIO_PIN_5;
    		validZone = true;
    	} else if (strcmp(xxx, "IN2") == 0){
    		address = I2C_IN2;
    		currentLSB = 0.000610f;
    		validPin = GPIO_PIN_6;
    		validZone = true;
    	} else if (strcmp(xxx, "OUT5V") == 0){
    		address = I2C_OUT5V;
    		currentLSB = 0.000185f;
    		controlPin = GPIO_PIN_0;
    		validZone = true;
    	} else if (strcmp(xxx, "OUT5V1") == 0){
    		address = I2C_OUT5V1;
    		currentLSB = 0.000185f;
    		validZone = true;
    	} else if (strcmp(xxx, "OUT12V") == 0){
    		address = I2C_OUT12V;
    		currentLSB = 0.000125f;
    		controlPin = GPIO_PIN_10;
    		validZone = true;
    	} else if (strcmp(xxx, "OUT24") == 0){
    		address = I2C_OUT24V;
    		currentLSB = 0.000040f;
    		controlPin = GPIO_PIN_11;
    		validZone = true;
    	}

    	// Selection type mesure
    	if(strcmp(yyy, "VOLT") == 0 && validZone){
    		float val = 0.0f;
			HAL_StatusTypeDef res = read_ina236_voltage(address, &val);
			if (res == HAL_OK) {
				send_set_response(xxx, yyy, val);
				return;
			}
    	} else if (strcmp(yyy, "AMPS") == 0 && validZone){
    		float val = 0.0f;
			HAL_StatusTypeDef res = read_ina236_current(address, currentLSB, &val);
			if (res == HAL_OK) {
				send_set_response(xxx, yyy, val);
				return;
			}
    	} else if (strcmp(yyy, "VALID") == 0 && validZone){
    		int val = 0;
    		if((strcmp(xxx, "IN1") == 0) || (strcmp(xxx, "IN2") == 0)){
    			val = (HAL_GPIO_ReadPin(GPIOA, validPin) == GPIO_PIN_RESET) ? 1 : 0;
    		} else {
    			val = 1;
    		}
    		send_set_response(xxx, yyy, val);
    		return;
    	} else if (strcmp(yyy, "STATE") == 0 && validZone){
    		int val = 0;
    		if((strcmp(xxx, "OUT5V") == 0) || (strcmp(xxx, "OUT12V") == 0) || (strcmp(xxx, "OUT24V") == 0)){
    			val = (HAL_GPIO_ReadPin(GPIOA, controlPin) == GPIO_PIN_RESET) ? 1 : 0;
    		} else {
    			val = 1;
    		}
    		send_set_response(xxx, yyy, val);
    		return;
    	}
    }
    send_error_response();
}

void handle_set(const char *xxx, const char *yyy, const char *val) {
    int v = atoi(val);

    if ((v == 0 || v == 1) &&
    	((strcmp(xxx, "EMG") == 0 && strcmp(yyy, "STATE") == 0) ||
    	(strcmp(xxx, "OUT5V") == 0 && strcmp(yyy, "STATE") == 0) ||
		(strcmp(xxx, "OUT12V") == 0 && strcmp(yyy, "STATE") == 0) ||
		(strcmp(xxx, "OUT24V") == 0 && strcmp(yyy, "STATE") == 0))) {

    	GPIO_PinState pin_state = (v == 1) ? GPIO_PIN_RESET : GPIO_PIN_SET; // Niveau actif bas
    	int success = 0;

    	if (strcmp(xxx, "OUT5V") == 0 && strcmp(yyy, "STATE") == 0) {
    	    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, pin_state);
    	    success = 1;
    	} else if (strcmp(xxx, "OUT12V") == 0 && strcmp(yyy, "STATE") == 0) {
    	    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, pin_state);
    	    success = 1;
    	} else if (strcmp(xxx, "OUT24V") == 0 && strcmp(yyy, "STATE") == 0) {
    	    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, pin_state);
    	    success = 1;
    	}

        if (success) {
            char msg[64];
            snprintf(msg, sizeof(msg), "OK;%s;%s;%d\n", xxx, yyy, v);
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        } else {
            char msg[64];
            snprintf(msg, sizeof(msg), "KO;%s;%s;%d\n", xxx, yyy, v);
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        }
    } else {
        send_error_response();
    }
}

void send_set_response(const char *xxx, const char *yyy, int val) {
    char msg[64];
    snprintf(msg, sizeof(msg), "SET;%s;%s;%d\n", xxx, yyy, val);
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

void send_error_response() {
    const char *msg = "KO;PARSE;ERROR;0\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}
