/*
 * comm.h
 *
 *  Created on: May 12, 2025
 *      Author: allan
 */

#ifndef INC_COMM_H_
#define INC_COMM_H_

#define RX_BUFFER_SIZE 128

// Prototypes
void handle_uart_message(const char *msg);
void handle_get(const char *xxx, const char *yyy);
void handle_set(const char *xxx, const char *yyy, const char *val);
void send_set_response(const char *xxx, const char *yyy, int val);
void send_error_response(void);

#endif /* INC_COMM_H_ */
