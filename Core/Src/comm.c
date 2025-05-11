#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

typedef enum errorCode {
	COMM_SUCCESS,
	COMM_ERROR
} errorCode ;

#define RX_BUFFER_SIZE 128
static char rxBuffer[RX_BUFFER_SIZE];
static uint8_t rxIndex = 0;
static char rxChar;

// Prototypes
void handle_uart_message(const char *msg);
void handle_get(const char *xxx, const char *yyy);
void handle_set(const char *xxx, const char *yyy, const char *val);
void send_set_response(const char *xxx, const char *yyy, int val);
void send_error_response(void);
errorCode read_bau_state(int* val);
errorCode read_mcp9808_temp(float* val);


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
    	errorCode res = read_bau_state(&val);
    	if(res == COMM_SUCCESS){
    		send_set_response(xxx, yyy, val);
    		return;
    	}
    } else if (strcmp(xxx, "TEMP") == 0 && strcmp(yyy, "CELS") == 0) {
        float temp = -1000.0f;
        errorCode res = read_mcp9808_temp(&temp);
        if (res == COMM_SUCCESS) {
            int temp_dixieme = (int)(temp * 10.0f + 0.5f);
            send_set_response(xxx, yyy, temp_dixieme);
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

    	// Appliquer commande...

    	int success = 1;
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

errorCode read_bau_state(int* val){
	if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET){
		*val = 0;
		return COMM_SUCCESS;
	} else {
		*val = 1;
		return COMM_SUCCESS;
	}
	return COMM_ERROR;
}

errorCode read_mcp9808_temp(float* temp) {
    uint8_t reg = 0x05;
    uint8_t data[2];

    if (HAL_I2C_Master_Transmit(&hi2c1, 0x18 << 1, &reg, 1, HAL_MAX_DELAY) != HAL_OK){
    	return COMM_ERROR;
    }
    if (HAL_I2C_Master_Receive(&hi2c1, 0x18 << 1, data, 2, HAL_MAX_DELAY) != HAL_OK){
    	return COMM_ERROR;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    raw &= 0x1FFF;

    *temp = raw & 0x1000 ? (raw - 8192) * 0.0625f : raw * 0.0625f;
    return COMM_SUCCESS;
}

