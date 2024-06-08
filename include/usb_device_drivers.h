#ifndef USB_DEVICE_DRIVERS_H
#define USB_DEVICE_DRIVERS_H

#include "usb_hid.h"

/* List of Vendor IDs */
#define SONY_VID		0x054c
#define MAYFLASH_VID	0x0079

#define MAYFLASH_GC_ADAPTER_PID 0x1843

struct device_id_t {
	u16 vid;
	u16 pid;
};

static inline bool usb_driver_is_comaptible(u16 vid, u16 pid, const struct device_id_t *ids, int num)
{
	for (int i = 0; i < num; i++) {
		if (ids[i].vid == vid && ids[i].pid == pid)
			return true;
	}

	return false;
}
extern const usb_device_driver_t mayflash_gc_usb_device_driver;
#endif
