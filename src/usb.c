#include "main.h"

void tusb_host_task0(void *parameters)
{
    (void)parameters;
    vTaskDelay(pdMS_TO_TICKS(500));
    tusb_init();
    while (1) {
        tuh_task();
    }
}

void usb0_handler()
{
    hcd_int_handler(TUH_OPT_RHPORT, true);
}

void usb_init()
{
    xTaskCreate(tusb_host_task0, "tusb_host_task0", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_NORMAL, NULL);
}

void hcd_int_enable(uint8_t rhport)
{
    xbox_interrupt_enable(XBOX_PIC_USB0_IRQ, 1);
}

void hcd_int_disable(uint8_t rhport)
{
    xbox_interrupt_enable(XBOX_PIC_USB0_IRQ, 0);
}

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = 1;
    return &usbh_xinput_driver;
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const *xid_itf, uint16_t len)
{
    const xinput_gamepad_t *p = &xid_itf->pad;

    if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS) {

        if (xid_itf->connected && xid_itf->new_pad_data) {
            dooom_new_input(p->wButtons, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY, p->bLeftTrigger,
                            p->bRightTrigger);
        }
    }
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    TU_LOG1("[INPUT] XINPUT MOUNTED %02x %d\n", dev_addr, instance);

    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
    if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false) {
        tuh_xinput_receive_report(dev_addr, instance);
        return;
    }

    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_set_led(dev_addr, instance, 1, true);
    tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    TU_LOG1("[INPUT] XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}

void tuh_mount_cb(uint8_t daddr)
{
    TU_LOG2("[USB] Device mounted %d\n", daddr);
}

void tuh_umount_cb(uint8_t daddr)
{
    TU_LOG2("[USB] Device unmounted %d\n", daddr);
}