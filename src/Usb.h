#ifndef _USB_H_
#define _USB_H_
#include <limero.h>
#include <Hardware.h>
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#define MAPLE_MINI
#ifdef MAPLE_MINI
#define USB_PUP_PORT GPIOB
#define USB_PUP_PIN GPIO9
#endif

class Usb : public Actor
{
public:
    int init();
    Usb(Thread &thread);
    ValueSource<std::string> rxdLine;
    Sink<std::string, 3> txdLine;

private:
    static Usb *usb1;
    bool usbConnected = false;
    usbd_device *usbd_dev;
    uint8_t usb_serial_tx_buf[64];
    bool usb_serial_need_empty_tx = false; // after sending a full packet , we need to send a 0 size
    /* Buffer to be used for control requests. */
    uint8_t usbd_control_buffer[128];
#define MAX 256
    ArrayQueue<uint8_t, 128> txd;
    ArrayQueue<uint8_t, 128> rxd;
    TimerSource _poller;

    static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
                                                                 uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req));
    static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep);
    static void cdcacm_sof_callback(void);
    static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue);
    void loop();
};
#endif