#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_log.h"

#include "oled_ssd1306.hpp"

#define tag "SSD1306"

#if CONFIG_I2C_PORT_0
#define I2C_NUM I2C_NUM_0
#elif CONFIG_I2C_PORT_1
#define I2C_NUM I2C_NUM_1
#else
#define I2C_NUM I2C_NUM_0 // if spi is selected
#endif

#define I2C_MASTER_FREQ_HZ 400000 /*!< I2C clock of SSD1306 can run at 400 kHz max. */
void I2C_SSD1306::i2cMasterInit(int16_t sda, int16_t scl, int16_t reset) {
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = sda,
		.scl_io_num = scl,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = I2C_MASTER_FREQ_HZ
	};
	ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_config));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0));

	if (reset >= 0) {
		gpio_reset_pin(reset);
		gpio_set_direction(reset, GPIO_MODE_OUTPUT);
		gpio_set_level(reset, 0);
		vTaskDelay(50 / portTICK_PERIOD_MS);
		gpio_set_level(reset, 1);
	}
	this->address = I2CAddress;
	this->flip = false;
}

void I2C_SSD1306::i2cInit(int width, int height) {
	this->width = width;
	this->height = height;
	this->pages = 8;
	if (this->height == 32) this->pages = 4;
	
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_OFF, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_MUX_RATIO, true);
	if (this->height == 64) i2c_master_write_byte(cmd, 0x3F, true);
	if (this->height == 32) i2c_master_write_byte(cmd, 0x1F, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_OFFSET, true);
	i2c_master_write_byte(cmd, 0x00, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_START_LINE, true);
	if (this->flip) {
		i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP_0, true);
	} else {
		i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP_1, true);
	}
	i2c_master_write_byte(cmd, OLED_CMD_SET_COM_SCAN_MODE, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_CLK_DIV, true);
	i2c_master_write_byte(cmd, 0x80, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_COM_PIN_MAP, true);
	if (this->height == 64) i2c_master_write_byte(cmd, 0x12, true);
	if (this->height == 32) i2c_master_write_byte(cmd, 0x02, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
	i2c_master_write_byte(cmd, 0xFF, true);
	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_RAM, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_VCOMH_DESELCT, true);
	i2c_master_write_byte(cmd, 0x40, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_MEMORY_ADDR_MODE, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_PAGE_ADDR_MODE, true);
	i2c_master_write_byte(cmd, 0x00, true);
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP, true);
	i2c_master_write_byte(cmd, 0x14, true);
	i2c_master_write_byte(cmd, OLED_CMD_DEACTIVE_SCROLL, true);
	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_NORMAL, true);
	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_ON, true);

	i2c_master_stop(cmd);

	esp_err_t espRc = i2c_master_cmd_begin(I2C_NUM, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) {
		ESP_LOGI(tag, "OLED configured successfully");
	} else {
		ESP_LOGE(tag, "OLED configuration failed. code: 0x%.2X", espRc);
	}
	i2c_cmd_link_delete(cmd);
}

void I2C_SSD1306::i2cDisplayImage(int page, int seg, uint8_t *images, int width) {
	i2c_cmd_handle_t cmd;

	if (page >= this->pages) return;
	if (seg >= this->width) return;

	int _seg = seg + CONFIG_OFFSETX;
	uint8_t columLow = _seg & 0x0F;
	uint8_t columHigh = (_seg >> 4) & 0x0F;

	int _page = page;
	if (this->flip) {
		_page = (this->pages - page) - 1;
	}

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, (0x00 + columLow), true);
	i2c_master_write_byte(cmd, (0x10 + columHigh), true);
	i2c_master_write_byte(cmd, 0xB0 | _page, true);

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
	i2c_master_write(cmd, images, width, true);

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

void I2C_SSD1306::i2cContrast(int contrast) {
	i2c_cmd_handle_t cmd;
	int _contrast = contrast;
	if (contrast < 0x0) _contrast = 0;
	if (contrast > 0xFF) _contrast = 0xFF;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
	i2c_master_write_byte(cmd, _contrast, true);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

void I2C_SSD1306::i2cHardwareScroll(ssd1306_scroll_type_t scroll) {
	esp_err_t espRc;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);

	i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

	if (scroll == SCROLL_RIGHT) {
		i2c_master_write_byte(cmd, OLED_CMD_HORIZONTAL_RIGHT, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x07, true);
		i2c_master_write_byte(cmd, 0x07, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0xFF, true);
		i2c_master_write_byte(cmd, OLED_CMD_ACTIVE_SCROLL, true);
	} 

	if (scroll == SCROLL_LEFT) {
		i2c_master_write_byte(cmd, OLED_CMD_HORIZONTAL_LEFT, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x07, true);
		i2c_master_write_byte(cmd, 0x07, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0xFF, true);
		i2c_master_write_byte(cmd, OLED_CMD_ACTIVE_SCROLL, true);
	} 

	if (scroll == SCROLL_DOWN) {
		i2c_master_write_byte(cmd, OLED_CMD_CONTINUOUS_SCROLL, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x07, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x3F, true);

		i2c_master_write_byte(cmd, OLED_CMD_VERTICAL, true);
		i2c_master_write_byte(cmd, 0x00, true);
		if (this->height == 64)
			i2c_master_write_byte(cmd, 0x40, true);
		if (this->height == 32)
			i2c_master_write_byte(cmd, 0x20, true);
		i2c_master_write_byte(cmd, OLED_CMD_ACTIVE_SCROLL, true);
	}

	if (scroll == SCROLL_UP) {
		i2c_master_write_byte(cmd, OLED_CMD_CONTINUOUS_SCROLL, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x07, true);
		i2c_master_write_byte(cmd, 0x00, true);
		i2c_master_write_byte(cmd, 0x01, true);

		i2c_master_write_byte(cmd, OLED_CMD_VERTICAL, true);
		i2c_master_write_byte(cmd, 0x00, true);
		if (this->height == 64)
			i2c_master_write_byte(cmd, 0x40, true);
		if (this->height == 32)
			i2c_master_write_byte(cmd, 0x20, true);
		i2c_master_write_byte(cmd, OLED_CMD_ACTIVE_SCROLL, true);
	}

	if (scroll == SCROLL_STOP) {
		i2c_master_write_byte(cmd, OLED_CMD_DEACTIVE_SCROLL, true);
	}

	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) {
		ESP_LOGD(tag, "Scroll command succeeded");
	} else {
		ESP_LOGE(tag, "Scroll command failed. code: 0x%.2X", espRc);
	}

	i2c_cmd_link_delete(cmd);
}
