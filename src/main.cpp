/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <FreeRTOS.h>
#include "task.h"
#include <string.h>
#include <ArduinoJson.h>
#include <limero.h>
#include <MqttSerial.h>

#define MAPLE_MINI
#ifdef MAPLE_MINI
#define LED_PORT GPIOB
#define LED_PIN GPIO1
#endif

#ifdef STM32F103_MINIMAL
#define LED_PORT GPIOC
#define LED_PIN GPIO13
#endif

#include <Log.h>

class Blinker : public Actor
{
	TimerSource _blinkTimer;
	bool _isOn = false;

public:
	Blinker(Thread &thread) : Actor(thread), _blinkTimer(thread, 100, true, "blink")
	{
	}

	void init()
	{
		gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ,
					  GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);
		_blinkTimer >> [&](const TimerMsg &) {
			if (_isOn)
			{
				_isOn = false;
				gpio_clear(LED_PORT, LED_PIN);
			}
			else
			{
				_isOn = true;
				gpio_set(LED_PORT, LED_PIN);
			}
		};
	}
};

//---------------------  FREERTOS config variables dependent  on board
uint32_t SystemCoreClock = 72000000;
uint32_t TotalHeapSize = (17 * 1024);
// limero setup
Thread mainThread({"main", 500, 20,configMAX_PRIORITIES - 2});
TimerSource logSomething(mainThread, 1000, true, "logSomething");
Thread usbThread({"usb", 500, 20,configMAX_PRIORITIES - 1});
Usb usb(usbThread);
MqttSerial mqtt(mainThread);
Blinker blinker(mainThread);
Log logger(256);

class EchoTest : public Actor {
 public:
  TimerSource trigger;
  ValueSource<uint64_t> counter;
  uint64_t startTime;
  EchoTest(Thread &thread)
      : Actor(thread), trigger(thread, 1000, true, "trigger"){};
  void init() {
    trigger >> [&](const TimerMsg &) {
      INFO(" send ");
      counter = Sys::millis();
    };
    counter >> mqtt.toTopic<uint64_t>("echo/output");
/*    mqtt.fromTopic<uint64_t>("src/stm32/echo/output") >>
        [&](const uint64_t in) {
          INFO(" it took %lu msec ", Sys::millis() - in);
        };*/
  }
};

EchoTest echoTest(mainThread);


void *__dso_handle;

void usbWriter(char *buffer, uint32_t bufLength)
{
	std::string line = std::string(buffer, bufLength);
	usb.txdLine.on(line); 
}

int main(void)
{
	logger.writer(usbWriter);
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);


	usb.init();
	blinker.init();
	mqtt.init();
	echoTest.init();
	usb.rxdLine >> mqtt.rxdLine;
	mqtt.txdLine >> usb.txdLine;

	logSomething >> [&](const TimerMsg &) { INFO(" free stack  %d", uxTaskGetStackHighWaterMark(0)); };
	logSomething.start();
	mainThread.start();
	usbThread.start();
	vTaskStartScheduler();
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	/* This function will get called if a task overflows its stack.   If the
	parameters are corrupt then inspect pxCurrentTCB to find which was the
	offending task. */

	(void)pxTask;
	(void)pcTaskName;

	for (;;)
		;
}
/*-----------------------------------------------------------*/

void assert_failed(unsigned char *pucFile, unsigned long ulLine)
{
	(void)pucFile;
	(void)ulLine;

	for (;;)
		;
}
