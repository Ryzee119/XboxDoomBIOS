#define CFG_TUH_ENABLED   1
#define CFG_TUH_MAX_SPEED OPT_MODE_FULL_SPEED
#define TUP_USBIP_OHCI
#define TUP_DCD_ENDPOINT_MAX 0
#define CFG_TUSB_OS          OPT_OS_FREERTOS

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 1
#endif

#ifndef CFG_TUH_ENUMERATION_BUFSIZE
#define CFG_TUH_ENUMERATION_BUFSIZE 512
#endif

#ifndef CFG_TUH_HUB
#define CFG_TUH_HUB 2
#endif

// Not including USB Hubs
#ifndef CFG_TUH_DEVICE_MAX
#define CFG_TUH_DEVICE_MAX 8
#endif

#ifndef TUP_OHCI_RHPORTS
#define TUP_OHCI_RHPORTS 4
#endif

#ifndef CFG_TUH_XINPUT
#define CFG_TUH_XINPUT 2
#endif

#ifndef CFG_TUH_MSC
#define CFG_TUH_MSC 2
#endif
#define CFG_TUH_MSC_MAXLUN 1

#ifndef CFG_TUH_TASK_QUEUE_SZ
#define CFG_TUH_TASK_QUEUE_SZ 256
#endif

#define CFG_TUSB_DEBUG_PRINTF printf_ts
#define CFG_TUSB_MEM_ALIGN    TU_ATTR_ALIGNED(256)