#include "button_map.h"
#include "usb_device_drivers.h"
#include "usb.h"
#include "utils.h"
#include "wiimote.h"


struct mayflash_gc_input_report {
	u8 port_num;
	
	// Buttons
	u8 z_button 			: 1;
	u8 						: 1;
	u8 right_trigger_button : 1;
	u8 left_trigger_button  : 1;
	u8 y_button 			: 1;
	u8 b_button 			: 1;
	u8 a_button 			: 1;
	u8 x_button 			: 1;

	// Buttons D-Pad and Start
	u8 left		: 1;
	u8 down		: 1;
	u8 right	: 1;
	u8 up 		: 1;
	u8			: 2;
	u8 start	: 1;
	u8			: 1;

	// Sticks
	u8 left_stick_x;
	u8 left_stick_y;
	u8 right_stick_y;
	u8 right_stick_x;

	// Triggers
	u8 left_trigger;
	u8 right_trigger;
	

} ATTRIBUTE_PACKED;

enum gamecube_buttons_enum {
	GC_BUTTON_Z,
	GC_BUTTON_RIGHT_TRIGGER,
	GC_BUTTON_LEFT_TRIGGER,
	GC_BUTTON_Y,
	GC_BUTTON_B,
	GC_BUTTON_A,
	GC_BUTTON_X,
	GC_BUTTON_UP,
	GC_BUTTON_DOWN,
	GC_BUTTON_RIGHT,
	GC_BUTTON_LEFT,
	GC_BUTTON_START,
	// Special Combos because GC controllers don't have enough buttons
	GC_BUTTON_COMBO_START_UP_L,
	GC_BUTTON_COMBO_START_UP_R,
	GC_BUTTON_COMBO_START_UP_Z,
	GC_BUTTON__NUM,
};

enum gamecube_analog_axis_e{
	GC_ANALOG_AXIS_LEFT_X,
	GC_ANALOG_AXIS_LEFT_Y,
	GC_ANALOG_AXIS_RIGHT_X,
	GC_ANALOG_AXIS_RIGHT_Y,
	GC_ANALOG_AXIS__NUM
};

struct mayflash_gc_private_data_t {
	struct {
		u32 buttons;
		u8 analog_axis[GC_ANALOG_AXIS__NUM];
	} input;
	u8 mapping;
	bool switch_mapping;
};
static_assert(sizeof(struct mayflash_gc_private_data_t) <= USB_INPUT_DEVICE_PRIVATE_DATA_SIZE);

// Define Combos for special combos:
#define COMBO_START_UP_L			(BIT(GC_BUTTON_LEFT_TRIGGER) | BIT(GC_BUTTON_UP) | BIT(GC_BUTTON_START))
#define COMBO_START_UP_R			(BIT(GC_BUTTON_RIGHT_TRIGGER) | BIT(GC_BUTTON_UP) | BIT(GC_BUTTON_START))
#define COMBO_START_UP_Z			(BIT(GC_BUTTON_Z) | BIT(GC_BUTTON_UP) | BIT(GC_BUTTON_START))
// Define combos for the mapping switch and IR emu mode.
#define SWITCH_MAPPING_COMBO		(BIT(GC_BUTTON_LEFT_TRIGGER) | BIT(GC_BUTTON_DOWN) | BIT(GC_BUTTON_START))
#define SWITCH_IR_EMU_MODE_COMBO	(BIT(GC_BUTTON_RIGHT_TRIGGER) | BIT(GC_BUTTON_DOWN) | BIT(GC_BUTTON_START))

static const struct {
	enum wiimote_ext_e extension;
	u16 wiimote_button_map[GC_BUTTON__NUM];
	u8 nunchuk_button_map[GC_BUTTON__NUM];
	u8 nunchuk_analog_axis_map[GC_BUTTON__NUM];
	u16 classic_button_map[GC_BUTTON__NUM];
	u8 classic_analog_axis_map[GC_BUTTON__NUM];
} input_mappings[] = {
	{
		.extension = WIIMOTE_EXT_NUNCHUK,
		.wiimote_button_map = {
			[GC_BUTTON_Y] 					= WIIMOTE_BUTTON_ONE,
			[GC_BUTTON_X]   				= WIIMOTE_BUTTON_B,
			[GC_BUTTON_A]   				= WIIMOTE_BUTTON_A,
			[GC_BUTTON_B]   				= WIIMOTE_BUTTON_TWO,
			[GC_BUTTON_UP]       			= WIIMOTE_BUTTON_UP,
			[GC_BUTTON_DOWN]     			= WIIMOTE_BUTTON_DOWN,
			[GC_BUTTON_LEFT]     			= WIIMOTE_BUTTON_LEFT,
			[GC_BUTTON_RIGHT]    			= WIIMOTE_BUTTON_RIGHT,
			[GC_BUTTON_START]  				= WIIMOTE_BUTTON_PLUS,
			[GC_BUTTON_COMBO_START_UP_L]    = WIIMOTE_BUTTON_MINUS,
			[GC_BUTTON_COMBO_START_UP_R]    = WIIMOTE_BUTTON_HOME,
		},
		.nunchuk_button_map = {
			[GC_BUTTON_RIGHT_TRIGGER] 	= NUNCHUK_BUTTON_C,
			[GC_BUTTON_Z] 				= NUNCHUK_BUTTON_Z,
		},
		.nunchuk_analog_axis_map = {
			[GC_ANALOG_AXIS_LEFT_X] = BM_NUNCHUK_ANALOG_AXIS_X,
			[GC_ANALOG_AXIS_LEFT_Y] = BM_NUNCHUK_ANALOG_AXIS_Y,
		},
	},
	{
		.extension = WIIMOTE_EXT_CLASSIC,
		.classic_button_map = {
			[GC_BUTTON_Y] 					= CLASSIC_CTRL_BUTTON_X,
			[GC_BUTTON_X]   				= CLASSIC_CTRL_BUTTON_A,
			[GC_BUTTON_A]    				= CLASSIC_CTRL_BUTTON_B,
			[GC_BUTTON_B]   				= CLASSIC_CTRL_BUTTON_Y,
			[GC_BUTTON_UP]       			= CLASSIC_CTRL_BUTTON_UP,
			[GC_BUTTON_DOWN]    		 	= CLASSIC_CTRL_BUTTON_DOWN,
			[GC_BUTTON_LEFT]     			= CLASSIC_CTRL_BUTTON_LEFT,
			[GC_BUTTON_RIGHT]    			= CLASSIC_CTRL_BUTTON_RIGHT,
			[GC_BUTTON_START]  				= CLASSIC_CTRL_BUTTON_PLUS,
			[GC_BUTTON_COMBO_START_UP_L]    = CLASSIC_CTRL_BUTTON_MINUS,
			[GC_BUTTON_Z]       			= CLASSIC_CTRL_BUTTON_ZR,
			[GC_BUTTON_COMBO_START_UP_Z]    = CLASSIC_CTRL_BUTTON_ZL,
			[GC_BUTTON_RIGHT_TRIGGER]       = CLASSIC_CTRL_BUTTON_FULL_R,
			[GC_BUTTON_LEFT_TRIGGER]       	= CLASSIC_CTRL_BUTTON_FULL_L,
			[GC_BUTTON_COMBO_START_UP_R]    = CLASSIC_CTRL_BUTTON_HOME,
		},
		.classic_analog_axis_map = {
			[GC_ANALOG_AXIS_LEFT_X]  = BM_CLASSIC_ANALOG_AXIS_LEFT_X,
			[GC_ANALOG_AXIS_LEFT_Y]  = BM_CLASSIC_ANALOG_AXIS_LEFT_Y,
			[GC_ANALOG_AXIS_RIGHT_X] = BM_CLASSIC_ANALOG_AXIS_RIGHT_X,
			[GC_ANALOG_AXIS_RIGHT_Y] = BM_CLASSIC_ANALOG_AXIS_RIGHT_Y,
		},
	},
};

static const u8 ir_analog_axis_map[GC_ANALOG_AXIS__NUM] = {
	[GC_ANALOG_AXIS_RIGHT_X] = BM_IR_AXIS_X,
	[GC_ANALOG_AXIS_RIGHT_Y] = BM_IR_AXIS_Y,
};

static const enum bm_ir_emulation_mode_e ir_emu_modes[] = {
	BM_IR_EMULATION_MODE_DIRECT,
	BM_IR_EMULATION_MODE_RELATIVE_ANALOG_AXIS,
	BM_IR_EMULATION_MODE_ABSOLUTE_ANALOG_AXIS,
};

static inline void gc_get_buttons(const struct mayflash_gc_input_report *report, u32 *buttons)
{
	u32 mask = 0;

#define MAP(field, button) \
	if (report->field) \
		mask |= BIT(button);
	MAP(z_button, GC_BUTTON_Z)
	MAP(right_trigger_button, GC_BUTTON_RIGHT_TRIGGER)
	MAP(left_trigger_button, GC_BUTTON_LEFT_TRIGGER)
	MAP(y_button, GC_BUTTON_Y)
	MAP(b_button, GC_BUTTON_B)
	MAP(a_button, GC_BUTTON_A)
	MAP(x_button, GC_BUTTON_X)
	MAP(up, GC_BUTTON_UP)
	MAP(down, GC_BUTTON_DOWN)
	MAP(right, GC_BUTTON_RIGHT)
	MAP(left, GC_BUTTON_LEFT)
	MAP(start, GC_BUTTON_START)
	// Combos (Start + Up + L/R/Z, priority in l, r, z order). When a combo is used deset input for others.
	if (report->start && report->up){
		if (report->left_trigger_button){
			mask |= BIT(GC_BUTTON_COMBO_START_UP_L);
			mask &= ~COMBO_START_UP_L;
		}
		else if (report->right_trigger_button){
			mask |= BIT(GC_BUTTON_COMBO_START_UP_R);
			mask &= ~COMBO_START_UP_R;
		}
		else if (report->z_button){
			mask |= BIT(GC_BUTTON_COMBO_START_UP_Z);
			mask &= ~COMBO_START_UP_Z;
		}
	}
#undef MAP

	*buttons = mask;
}

static inline void gc_get_analog_axis(const struct mayflash_gc_input_report *report,
				       u8 analog_axis[static GC_ANALOG_AXIS__NUM])
{
	analog_axis[GC_ANALOG_AXIS_LEFT_X] = report->left_stick_x;
	analog_axis[GC_ANALOG_AXIS_LEFT_Y] = 255 - report->left_stick_y;
	analog_axis[GC_ANALOG_AXIS_RIGHT_X] = report->right_stick_x;
	analog_axis[GC_ANALOG_AXIS_RIGHT_Y] = 255 - report->right_stick_y;
}

static inline int gc_request_data(usb_input_device_t *device)
{
	return usb_device_driver_issue_intr_transfer_async(device, 0, device->usb_async_resp,
							   sizeof(device->usb_async_resp));
}


bool gc_driver_ops_probe(u16 vid, u16 pid)
{
	static const struct device_id_t compatible[] = {
		{MAYFLASH_VID, MAYFLASH_GC_ADAPTER_PID},
	};

	return usb_driver_is_comaptible(vid, pid, compatible, ARRAY_SIZE(compatible));
}

int gc_driver_ops_init(usb_input_device_t *device, u16 vid, u16 pid)
{
	struct mayflash_gc_private_data_t *priv = (void *)device->private_data;

	/* Init private state */
	priv->mapping = 1;
	priv->switch_mapping = false;

	/* Set initial extension */
	fake_wiimote_set_extension(device->wiimotes[0], input_mappings[priv->mapping].extension);

	return gc_request_data(device);
}

static int gc_driver_update_leds_rumble(usb_input_device_t *device)
{
	// TODO: this
	return 0;
}

int gc_driver_ops_disconnect(usb_input_device_t *device)
{
	struct mayflash_gc_private_data_t *priv = (void *)device->private_data;

	return gc_driver_update_leds_rumble(device);
}

int gc_driver_ops_slot_changed(usb_input_device_t *device, u8 slot)
{
	struct mayflash_gc_private_data_t *priv = (void *)device->private_data;

	return gc_driver_update_leds_rumble(device);
}

int gc_driver_ops_set_rumble(usb_input_device_t *device, bool rumble_on)
{
	struct mayflash_gc_private_data_t *priv = (void *)device->private_data;

	return gc_driver_update_leds_rumble(device);
}

bool gc_report_input(usb_input_device_t *device)
{
	struct mayflash_gc_private_data_t *priv = (void *)device->private_data;
	u16 wiimote_buttons = 0;
	u16 acc_x, acc_y, acc_z;
	union wiimote_extension_data_t extension_data;

	if (bm_check_switch_mapping(priv->input.buttons, &priv->switch_mapping, SWITCH_MAPPING_COMBO)) {
		priv->mapping = (priv->mapping + 1) % ARRAY_SIZE(input_mappings);
		fake_wiimote_set_extension(device->wiimotes[0], input_mappings[priv->mapping].extension);
		return false;
	}

	bm_map_wiimote(GC_BUTTON__NUM, priv->input.buttons,
	       input_mappings[priv->mapping].wiimote_button_map,
	       &wiimote_buttons);
	// Accel is overrated
	acc_x = 0;
	acc_y = 0;
	acc_z = 0;

	fake_wiimote_report_accelerometer(device->wiimotes[0], acc_x, acc_y, acc_z);

	if (input_mappings[priv->mapping].extension == WIIMOTE_EXT_NONE) {
		fake_wiimote_report_input(device->wiimotes[0], wiimote_buttons);
	} else if (input_mappings[priv->mapping].extension == WIIMOTE_EXT_NUNCHUK) {
		bm_map_nunchuk(GC_BUTTON__NUM, priv->input.buttons,
			       GC_ANALOG_AXIS__NUM, priv->input.analog_axis,
			       0, 0, 0,
			       input_mappings[priv->mapping].nunchuk_button_map,
			       input_mappings[priv->mapping].nunchuk_analog_axis_map,
			       &extension_data.nunchuk);
		fake_wiimote_report_input_ext(device->wiimotes[0], wiimote_buttons,
					      &extension_data, sizeof(extension_data.nunchuk));
	} else if (input_mappings[priv->mapping].extension == WIIMOTE_EXT_CLASSIC) {
		bm_map_classic(GC_BUTTON__NUM, priv->input.buttons,
			       GC_ANALOG_AXIS__NUM, priv->input.analog_axis,
			       input_mappings[priv->mapping].classic_button_map,
			       input_mappings[priv->mapping].classic_analog_axis_map,
			       &extension_data.classic);
		fake_wiimote_report_input_ext(device->wiimotes[0], wiimote_buttons,
					      &extension_data, sizeof(extension_data.classic));
	}

	return true;
}

int gc_driver_ops_usb_async_resp(usb_input_device_t *device)
{
	struct mayflash_gc_private_data_t *priv = (void *)device->private_data;
	struct mayflash_gc_input_report *report = (void *)device->usb_async_resp;

	//TODO: investigate other ports
	if (report->port_num == 0x01) {
		gc_get_buttons(report, &priv->input.buttons);
		gc_get_analog_axis(report, priv->input.analog_axis);
	}

	return gc_request_data(device);
}

const usb_device_driver_t mayflash_gc_usb_device_driver = {
	.probe		= gc_driver_ops_probe,
	.init		= gc_driver_ops_init,
	.disconnect	= gc_driver_ops_disconnect,
	.slot_changed	= gc_driver_ops_slot_changed,
	.report_input	= gc_report_input,
	.usb_async_resp	= gc_driver_ops_usb_async_resp,
};
