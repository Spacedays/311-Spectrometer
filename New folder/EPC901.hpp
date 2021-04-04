/*
 * Integration with epc901 sensor.
 *
 * Copyright 2020 by Adrian Studer
 *
 * Licensed under the terms of MIT License, all text here must be included in any redistribution.
 * See the LICENSE-SW file for details.
 */

#ifndef SRC_EPC901_HPP_
#define SRC_EPC901_HPP_

#include <inttypes.h>
#include "main.h"

class EPC901 {
public:
	EPC901();
	virtual ~EPC901();

	uint8_t init();//I2C_HandleTypeDef* i2c_handle, uint8_t i2c_addr, SPI_HandleTypeDef* spi_handle, TIM_HandleTypeDef* tim_handle);
	uint16_t captureImage(uint32_t exposure_us, uint16_t* buffer);
	uint8_t getRevision();
	static uint32_t getMaxExposure() { return _max_exposure_us; };

	uint8_t readRegister(uint8_t address, uint8_t *value);
	uint8_t writeRegister(uint8_t address, uint8_t value);
	uint8_t reset(void);

	// low-level API
	void _powerUp();
	void _powerDown();
	void _clear();
	void _exposeImage(uint32_t exposure_us);
	uint16_t _readImage(uint16_t* buffer);
	uint8_t _dataReady();

private:

	SPI_HandleTypeDef* _spi_handle;
	I2C_HandleTypeDef* _i2c_handle;
	TIM_HandleTypeDef* _tim_handle;
	uint8_t _i2c_addr;
	uint8_t _chip_rev;
	uint8_t _power_state;
	uint8_t _conf_ctrl;
	uint8_t _hor_bin;
	uint8_t _roi_sel;

	// timer clk runs at 80 MHz -> max duration for 2x16 bit timer is 2^32/80 = ~53687ms
	// therefore for longer exposures we have control shutter manually (that's > 53s!)
	static const uint32_t _max_exposure_us = 53687000;
};

#endif /* SRC_EPC901_HPP_ */
