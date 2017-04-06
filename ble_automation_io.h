#pragma once
#include <stdint.h>
#include <ble.h>
#include <ble_gatts.h>

enum {
	BLE_DIGITAL_INACTIVE = 0,
	BLE_DIGITAL_ACTIVE,
	BLE_DIGITAL_TRI_STATE,
	BLE_DIGITAL_OUTPUT_STATE
};

struct ble_automation_io_gpio {
	uint32_t pin_number;
	bool output;
	bool default_value;
};

struct ble_automation_io {
	uint16_t service;

	ble_gatts_char_handles_t char_digital;
	uint8_t value_digital;

	struct ble_automation_io_gpio gpio;
};

uint32_t init_ble_automation_io(struct ble_automation_io *, struct ble_automation_io_gpio *gpio);

void ble_automation_io_refresh(struct ble_automation_io *);

void ble_automation_io_on_ble_evt(ble_evt_t *);
