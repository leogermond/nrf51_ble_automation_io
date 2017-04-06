#include <sdk_common.h>
#include <ble_automation_io.h>
#include <ble_srv_common.h>

#include <nrf_gpio.h>

#define BLE_UUID_AUTOMATION_IO 0x1815
#define BLE_UUID_DIGITAL 0x2a56

static void gpio_set(struct ble_automation_io *aio, uint8_t value) {
	uint32_t pin = aio->gpio.pin_number;

	if(value) {
		nrf_gpio_pin_set(pin);
	} else {
		nrf_gpio_pin_clear(pin);
	}
}

static uint8_t gpio_get(struct ble_automation_io *aio) {
	uint8_t val;
	if(aio->gpio.output) {
		val = BLE_DIGITAL_OUTPUT_STATE;
	} else {
		val = nrf_gpio_pin_read(aio->gpio.pin_number);
	}

	return val;
}

static void init_gpio(struct ble_automation_io *aio) {
	uint32_t pin = aio->gpio.pin_number;
	bool output = aio->gpio.output;

	if(output) {
		nrf_gpio_cfg_output(pin);
		gpio_set(aio, (int)aio->gpio.default_value);
		ble_automation_io_refresh(aio);
	} else {
		nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_NOPULL);
	}
}

void ble_automation_io_refresh(struct ble_automation_io *aio) {
	uint8_t value = gpio_get(aio);
	if(value != aio->value_digital) {
		// TODO: notify clients
	}
	aio->value_digital = value;
}

// Ugly global but necessary for checking events
static struct ble_automation_io *g_aio = NULL;
void ble_automation_io_on_ble_evt(ble_evt_t *evt) {
	if(!g_aio) return;

	switch(evt->header.evt_id) {
	case BLE_GATTS_EVT_WRITE: {
		ble_gatts_evt_write_t *write = &evt->evt.gatts_evt.params.write;
		if(g_aio->gpio.output && write->handle == g_aio->char_digital.value_handle && write->len == 1) {
			gpio_set(g_aio, write->data[0]);
			ble_automation_io_refresh(g_aio);
		}
		break;
		}
	}
}

uint32_t init_ble_automation_io(struct ble_automation_io *aio,
		struct ble_automation_io_gpio *gpio) {
	uint32_t err;

	g_aio = aio;

	ble_uuid_t uuid;
	BLE_UUID_BLE_ASSIGN(uuid, BLE_UUID_AUTOMATION_IO);
	err = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, &aio->service);
	VERIFY_SUCCESS(err);

	memcpy(&aio->gpio, gpio, sizeof(*gpio));

	init_gpio(aio);

	ble_gatts_char_md_t char_md = {
		.char_props = {
			.read = 1,
			.notify = 1,
			.write = (int)aio->gpio.output
		},
	};

	ble_gatts_attr_md_t attr_char_value_md = {
		.vloc = BLE_GATTS_VLOC_STACK,
	};
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_char_value_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_char_value_md.write_perm);

	BLE_UUID_BLE_ASSIGN(uuid, BLE_UUID_DIGITAL);
	ble_gatts_attr_t attr_char_value = {
		.p_uuid = &uuid,
		.p_attr_md = &attr_char_value_md,
		.init_len = 1,
		.max_len = 1,
		.p_value = &aio->value_digital,
	};

	return sd_ble_gatts_characteristic_add(aio->service, &char_md, &attr_char_value,
			&aio->char_digital);
}
