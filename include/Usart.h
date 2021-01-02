/*
 * Usart.h
 *
 *  Created on: Aug 14, 2016
 *      Author: lieven2
 */

#ifndef USART_H_
#define USART_H_

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <stdio.h>
#include <errno.h>
#include <limero.h>
#include <deque>

#define UART_BUFFER_SIZE 512

int _write(int file, char *ptr, int len);

class Usart :public Actor  {
public:
	std::deque<uint8_t>_txd;
	std::deque<uint8_t>_rxd;


	Usart(Thread& thread);
	virtual ~Usart();

	static void usart1_isr(void);
	void init();
	void loop();

	 int write(std::string& data);
	 int write(uint8_t b);
	 bool hasData();
	 bool hasSpace();
	 uint8_t read();
	 void receive(uint8_t b);
	 int setBaudrate(uint32_t baudrate);
	 uint32_t getBaudrate();
	 int setMode(const char* str);
	 void getMode(char* str);

};

extern Usart usart1;

#endif /* USART_H_ */
