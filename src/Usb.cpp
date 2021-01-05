#include <Usb.h>

#ifdef STM32F103_MINIMAL
#define LED_PORT GPIOC
#define LED_PIN GPIO13
#endif

static const struct usb_device_descriptor dev = {
    bLength : USB_DT_DEVICE_SIZE,
    bDescriptorType : USB_DT_DEVICE,
    bcdUSB : 0x0200,
    bDeviceClass : USB_CLASS_CDC,
    bDeviceSubClass : 0,
    bDeviceProtocol : 0,
    bMaxPacketSize0 : 64,
    idVendor : 0x0483,
    idProduct : 0x5740,
    bcdDevice : 0x0200,
    iManufacturer : 1,
    iProduct : 2,
    iSerialNumber : 3,
    bNumConfigurations : 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{bLength : USB_DT_ENDPOINT_SIZE,
                                                            bDescriptorType : USB_DT_ENDPOINT,
                                                            bEndpointAddress : 0x83,
                                                            bmAttributes : USB_ENDPOINT_ATTR_INTERRUPT,
                                                            wMaxPacketSize : 16,
                                                            bInterval : 255,
                                                            extra : 0,
                                                            extralen : 0}};

static const struct usb_endpoint_descriptor data_endp[] = {{bLength : USB_DT_ENDPOINT_SIZE,
                                                            bDescriptorType : USB_DT_ENDPOINT,
                                                            bEndpointAddress : 0x01,
                                                            bmAttributes : USB_ENDPOINT_ATTR_BULK,
                                                            wMaxPacketSize : 64,
                                                            bInterval : 1,
                                                            extra : 0,
                                                            extralen : 0},
                                                           {bLength : USB_DT_ENDPOINT_SIZE,
                                                            bDescriptorType : USB_DT_ENDPOINT,
                                                            bEndpointAddress : 0x82,
                                                            bmAttributes : USB_ENDPOINT_ATTR_BULK,
                                                            wMaxPacketSize : 64,
                                                            bInterval : 1,
                                                            extra : 0,
                                                            extralen : 0}};

static const struct
{
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
    header : {
        bFunctionLength : sizeof(struct usb_cdc_header_descriptor),
        bDescriptorType : CS_INTERFACE,
        bDescriptorSubtype : USB_CDC_TYPE_HEADER,
        bcdCDC : 0x0110,
    },
    call_mgmt : {
        bFunctionLength : sizeof(struct usb_cdc_call_management_descriptor),
        bDescriptorType : CS_INTERFACE,
        bDescriptorSubtype : USB_CDC_TYPE_CALL_MANAGEMENT,
        bmCapabilities : 0,
        bDataInterface : 1,
    },
    acm : {
        bFunctionLength : sizeof(struct usb_cdc_acm_descriptor),
        bDescriptorType : CS_INTERFACE,
        bDescriptorSubtype : USB_CDC_TYPE_ACM,
        bmCapabilities : 0,
    },
    cdc_union : {
        bFunctionLength : sizeof(struct usb_cdc_union_descriptor),
        bDescriptorType : CS_INTERFACE,
        bDescriptorSubtype : USB_CDC_TYPE_UNION,
        bControlInterface : 0,
        bSubordinateInterface0 : 1,
    },
};

static const struct usb_interface_descriptor comm_iface[] = {{
    bLength : USB_DT_INTERFACE_SIZE,
    bDescriptorType : USB_DT_INTERFACE,
    bInterfaceNumber : 0,
    bAlternateSetting : 0,
    bNumEndpoints : 1,
    bInterfaceClass : USB_CLASS_CDC,
    bInterfaceSubClass : USB_CDC_SUBCLASS_ACM,
    bInterfaceProtocol : USB_CDC_PROTOCOL_AT,
    iInterface : 0,
    endpoint : comm_endp,
    extra : &cdcacm_functional_descriptors,
    extralen : sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
    bLength : USB_DT_INTERFACE_SIZE,
    bDescriptorType : USB_DT_INTERFACE,
    bInterfaceNumber : 1,
    bAlternateSetting : 0,
    bNumEndpoints : 2,
    bInterfaceClass : USB_CLASS_DATA,
    bInterfaceSubClass : 0,
    bInterfaceProtocol : 0,
    iInterface : 0,
    endpoint : data_endp,
    extra : 0,
    extralen : 0,
}};

static const struct usb_interface ifaces[] = {{
                                                  cur_altsetting : 0,
                                                  num_altsetting : 1,
                                                  iface_assoc : 0,
                                                  altsetting : comm_iface,
                                              },
                                              {
                                                  cur_altsetting : 0,
                                                  num_altsetting : 1,
                                                  iface_assoc : 0,
                                                  altsetting : data_iface,
                                              }};

static const struct usb_config_descriptor config = {
    bLength : USB_DT_CONFIGURATION_SIZE,
    bDescriptorType : USB_DT_CONFIGURATION,
    wTotalLength : 0,
    bNumInterfaces : 2,
    bConfigurationValue : 1,
    iConfiguration : 0,
    bmAttributes : 0x80,
    bMaxPower : 0x32,

    interface : ifaces,
};

static const char *usb_strings[] = {
    "Black Sphere Technologies",
    "CDC-ACM Demo",
    "DEMO",
};

Usb *Usb::usb1;

Usb::Usb(Thread &thread) : Actor(thread), txdLine(10), txd(128), rxd(128), _poller(thread, 1, true, "poller")
{
    usb1 = this;
}

extern "C" void usb_lp_can_rx0_isr(void)
{
    usbd_poll(Usb::usb1->usbd_dev);
}

extern "C" void usb_wakeup_isr(void)
{
    usbd_poll(Usb::usb1->usbd_dev);
}

int Usb::init()
{

    //
    /* Setup pin to pull up the D+ high, so autodect works
	 * with the bootloader.  The circuit is active low. */
    gpio_set_mode(USB_PUP_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_OPENDRAIN, USB_PUP_PIN);
    gpio_clear(USB_PUP_PORT, USB_PUP_PIN);

    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
    usbd_register_sof_callback(usbd_dev, cdcacm_sof_callback);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);

    // Enable USB by raising up D+ via a 1.5K resistor. This is done on the
    // WaveShare board by removing the USB EN jumper and  connecting PC0 to the
    // right hand pin of the jumper port with a patch wire. By setting PC0 to
    // open drain it turns on an NFET which pulls  up D+ via a 1.5K resistor.

    for (int i = 0; i < 0x800000; i++)
        __asm__("nop");
    gpio_clear(GPIOC, GPIO11);
    //   _poller >> [&](const TimerMsg &) { usbd_poll(usbd_dev); };
    txdLine.async(thread(), [&](const std::string &s) {
        for (uint32_t i = 0; i < s.length(); i++)
            txd.push(s[i]);
        txd.push('\r');
        txd.push('\n');
    });

    return 0;
}

enum usbd_request_return_codes Usb::cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
                                                           uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
    (void)usbd_dev;
    (void)buf;
    (void)complete;
    switch (req->bRequest)
    {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
    {
        /*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
        char local_buf[10];
        struct usb_cdc_notification *notif = (usb_cdc_notification *)local_buf;
        uint16_t rtsdtr = req->wValue; // DTR is bit 0, RTS is bit 1
        usb1->usbConnected = rtsdtr & 1;
        /* We echo signals back to host as notification. */
        notif->bmRequestType = 0xA1;
        notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
        notif->wValue = 0;
        notif->wIndex = 0;
        notif->wLength = 2;
        local_buf[8] = req->wValue & 3;
        local_buf[9] = 0;
        // usbd_ep_write_packet(0x83, buf, 10);
        return USBD_REQ_HANDLED;
    }
    case USB_CDC_REQ_SET_LINE_CODING:
        if (*len < sizeof(struct usb_cdc_line_coding))
            return USBD_REQ_NOTSUPP;
        return USBD_REQ_HANDLED;
    }
    return USBD_REQ_NOTSUPP;
}

void Usb::cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
    (void)ep;

    char buf[64];
    int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

    for (int idx = 0; idx < len; idx++)
    {
        if (buf[idx] == '\n' || buf[idx] == '\r')
        {
            std::string line;
            uint8_t b;
            while (usb1->rxd.pop(b) == 0)
            {
                line += (char)b;
            }
            if (line.length() > 0)
                usb1->rxdLine = line;
        }
        else
            usb1->rxd.push(buf[idx]);
    }
}

void Usb::cdcacm_sof_callback(void)
{
    if (!usb1->usbConnected)
    {
        // Host isn't connected - nothing to do.
        return;
    }
    int len = 0;
    uint8_t b=0;
    while (usb1->txd.pop(b) == 0 && len <= 64)
    {
        usb1->usb_serial_tx_buf[len] = b;
        len++;
        if (len == 64)
        {
            break;
        }
    }
    if (len == 0 && !usb1->usb_serial_need_empty_tx)
    {
        // Nothing to do.
        return;
    }

    uint16_t sent = usbd_ep_write_packet(usb1->usbd_dev, 0x82, usb1->usb_serial_tx_buf, len);

    // If we just sent a packet of 64 bytes. If we get called again and
    // there is no more data to send, then we need to send a zero byte
    // packet to indicate to the host to release the data it has buffered.
    usb1->usb_serial_need_empty_tx = (sent == 64);
    if (sent != len)
    {
        usb1->txd.push('?');
    };
}

void Usb::cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    (void)wValue;
    (void)usbd_dev;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cdcacm_control_request);
}
