#ifndef _USB_H_
#define _USB_H_
#include <limero.h>
#include <Hardware.h>


#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

class Usb : public UART, public Actor
{
    static UART &create();
    virtual int mode(const char *);
    virtual int init();
    virtual int deInit();
    virtual int setClock(uint32_t clock);

    virtual int write(const uint8_t *data, uint32_t length);
    virtual int write(uint8_t b);
    virtual int read(std::string &bytes);
    virtual uint8_t read();
    virtual void onRxd(FunctionPointer, void *);
    virtual void onTxd(FunctionPointer, void *);
    virtual uint32_t hasSpace();
    virtual uint32_t hasData();

    private :

};
#endif