#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "oled_ssd1306.hpp"
#include "font8x8_basic.hpp"

#define TAG "OLED_SSD1306"

#define PACK8 __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

void OLED_SSD1306::scroll_text(char *text, int text_len, bool invert)
{
	ESP_LOGD(TAG, "dev->_scEnable=%d", _scEnable);
	if (_scEnable == false)
		return;

	void (*func)(int page, int seg, uint8_t *images, int width);
	if (_address == SPIAddress)
	{
		func = spi_display_image;
	}
	else
	{
		func = i2c_display_image;
	}

	int srcIndex = _scEnd - _scDirection;
	while (1)
	{
		int dstIndex = srcIndex + _scDirection;
		ESP_LOGD(TAG, "srcIndex=%d dstIndex=%d", srcIndex, dstIndex);
		for (int seg = 0; seg < _width; seg++)
		{
			_page[dstIndex]._segs[seg] = _page[srcIndex]._segs[seg];
		}
		(*func)(dstIndex, 0, _page[dstIndex]._segs, sizeof(_page[dstIndex]._segs));
		if (srcIndex == _scStart)
			break;
		srcIndex = srcIndex - _scDirection;
	}

	int _text_len = text_len;
	if (_text_len > 16)
		_text_len = 16;

	display_text(srcIndex, text, text_len, invert);
}

void OLED_SSD1306::scroll_clear()
{
	ESP_LOGD(TAG, "dev->_scEnable=%d", _scEnable);
	if (_scEnable == false)
		return;

	int srcIndex = _scEnd - _scDirection;
	while (1)
	{
		int dstIndex = srcIndex + _scDirection;
		ESP_LOGD(TAG, "srcIndex=%d dstIndex=%d", srcIndex, dstIndex);
		clear_line(dstIndex, false);
		if (dstIndex == _scStart)
			break;
		srcIndex = srcIndex - _scDirection;
	}
}

void OLED_SSD1306::hardware_scroll(scroll_type_t scroll)
{
	if (_address == SPIAddress)
	{
		spi_hardware_scroll(scroll);
	}
	else
	{
		i2c_hardware_scroll(scroll);
	}
}

void OLED_SSD1306::wrap_around(scroll_type_t scroll, int start, int end, int8_t delay)
{
	if (scroll == SCROLL_RIGHT)
	{
		int _start = start; // 0 to 7
		int _end = end;     // 0 to 7
		if (_end >= _pages)
			_end = _pages - 1;
		uint8_t wk;
		for (int page = _start; page <= _end; page++)
		{
			wk = _page[page]._segs[127];
			for (int seg = 127; seg > 0; seg--)
			{
				_page[page]._segs[seg] = _page[page]._segs[seg - 1];
			}
			display_image(page, 0, _page[page]._segs, sizeof(_page[page]._segs));
		}
	}
}

void OLED_SSD1306::init(int width, int height)
{
	if (_address == SPIAddress)
	{
		spi_init(this, width, height);
	}
	else
	{
		i2c_init(this, width, height);
	}
	// Initialize internal buffer
	for (int i = 0; i < _pages; i++)
	{
		memset(_page[i]._segs, 0, 128);
	}
}

int OLED_SSD1306::get_width()
{
	return _width;
}

int OLED_SSD1306::get_height()
{
	return _height;
}

int OLED_SSD1306::get_pages()
{
	return _pages;
}

void OLED_SSD1306::show_buffer()
{
	if (_address == SPIAddress)
	{
		for (int page = 0; page < _pages; page++)
		{
			spi_display_image(this, page, 0, _page[page]._segs, _width);
		}
	}
	else
	{
		for (int page = 0; page < _pages; page++)
		{
			i2c_display_image(this, page, 0, _page[page]._segs, _width);
		}
	}
}

void OLED_SSD1306::set_buffer(uint8_t *buffer)
{
	int index = 0;
	for (int page = 0; page < _pages; page++)
	{
		memcpy(&_page[page]._segs, &buffer[index], 128);
		index = index + 128;
	}
}

void OLED_SSD1306::get_buffer(uint8_t *buffer)
{
	int index = 0;
	for (int page = 0; page < _pages; page++)
	{
		memcpy(&buffer[index], &_page[page]._segs, 128);
		index = index + 128;
	}
}

void OLED_SSD1306::display_image(int page, int seg, uint8_t *images, int width)
{
	if (_address == SPIAddress)
	{
		spi_display_image(this, page, seg, images, width);
	}
	else
	{
		i2c_display_image(this, page, seg, images, width);
	}
	// Set to internal buffer
	memcpy(&_page[page]._segs[seg], images, width);
}

void OLED_SSD1306::display_text(int page, char *text, int text_len, bool invert)
{
	if (page >= _pages)
		return;
	int _text_len = text_len;
	if (_text_len > 16)
		_text_len = 16;

	uint8_t seg = 0;
	uint8_t image[8];
	for (uint8_t i = 0; i < _text_len; i++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		if (invert)
			invert(image, 8);
		if (_flip)
			flip(image, 8);
		display_image(page, seg, image, 8);
		seg = seg + 8;
	}
}

void OLED_SSD1306::display_text_x3(int page, char *text, int text_len, bool invert)
{
	if (page >= _pages)
		return;
	int _text_len = text_len;
	if (_text_len > 5)
		_text_len = 5;

	uint8_t seg = 0;

	for (uint8_t nn = 0; nn < _text_len; nn++)
	{

		uint8_t const *const in_columns = font8x8_basic_tr[(uint8_t)text[nn]];

		// make the character 3x as high
		out_column_t out_columns[8];
		memset(out_columns, 0, sizeof(out_columns));

		for (uint8_t xx = 0; xx < 8; xx++)
		{ // for each column (x-direction)

			uint32_t in_bitmask = 0b1;
			uint32_t out_bitmask = 0b111;

			for (uint8_t yy = 0; yy < 8; yy++)
			{ // for pixel (y-direction)
				if (in_columns[xx] & in_bitmask)
				{
					out_columns[xx].u32 |= out_bitmask;
				}
				in_bitmask <<= 1;
				out_bitmask <<= 3;
			}
		}

		// render character in 8 column high pieces, making them 3x as wide
		for (uint8_t yy = 0; yy < 3; yy++)
		{ // for each group of 8 pixels high (y-direction)

			uint8_t image[24];
			for (uint8_t xx = 0; xx < 8; xx++)
			{ // for each column (x-direction)
				image[xx * 3 + 0] =
					image[xx * 3 + 1] =
					image[xx * 3 + 2] = out_columns[xx].u8[yy];
			}
			if (invert)
				invert(image, 24);
			if (_flip)
				flip(image, 24);
			if (_address == SPIAddress)
			{
				spi_display_image(this, page + yy, seg, image, 24);
			}
			else
			{
				i2c_display_image(this, page + yy, seg, image, 24);
			}
			memcpy(&_page[page + yy]._segs[seg], image, 24);
		}
		seg = seg + 24;
	}
}

void OLED_SSD1306::clear_screen(bool invert)
{
	char space[16];
	memset(space, 0x00, sizeof(space));
	for (int page = 0; page < _pages; page++)
	{
		display_text(page, space, sizeof(space), invert);
	}
}

void OLED_SSD1306::clear_line(int page, bool invert)
{
	char space[16];
	memset(space, 0x00, sizeof(space));
	display_text(page, space, sizeof(space), invert);
}

void OLED_SSD1306::contrast(int contrast)
{
	if (_address == SPIAddress)
	{
		spi_contrast(this, contrast);
	}
	else
	{
		i2c_contrast(this, contrast);
	}
}

void OLED_SSD1306::software_scroll(int start, int end)
{
	ESP_LOGD(TAG, "software_scroll start=%d end=%d _pages=%d", start, end, _pages);
	if (start < 0 || end < 0)
	{
		_scEnable = false;
	}
	else if (start >= _pages || end >= _pages)
	{
		_scEnable = false;
	}
	else
	{
		_scEnable = true;
		_scStart = start;
		_scEnd = end;
		_scDirection = 1;
		if (start > end)
			_scDirection = -1;
	}
}

void OLED_SSD1306::scroll_text(char *text, int text_len, bool invert)
{
	ESP_LOGD(TAG, "dev->_scEnable=%d", _scEnable);
	if (_scEnable == false)
		return;

	void (*func)(int page, int seg, uint8_t *images, int width);
	if (_address == SPIAddress)
	{
		func = spi_display_image;
	}
	else
	{
		func = i2c_display_image;
	}

	int srcIndex = _scEnd - _scDirection;
	while (1)
	{
		int dstIndex = srcIndex + _scDirection;
		ESP_LOGD(TAG, "srcIndex=%d dstIndex=%d", srcIndex, dstIndex);
		for (int seg = 0; seg < _width; seg++)
		{
			_page[dstIndex]._segs[seg] = _page[srcIndex]._segs[seg];
		}
		(*func)(dstIndex, 0, _page[dstIndex]._segs, sizeof(_page[dstIndex]._segs));
		if (srcIndex == _scStart)
			break;
		srcIndex = srcIndex - _scDirection;
	}

	int _text_len = text_len;
	if (_text_len > 16)
		_text_len = 16;

	display_text(srcIndex, text, text_len, invert);
}

void OLED_SSD1306::scroll_clear()
{
	ESP_LOGD(TAG, "dev->_scEnable=%d", _scEnable);
	if (_scEnable == false)
		return;

	int srcIndex = _scEnd - _scDirection;
	while (1)
	{
		int dstIndex = srcIndex + _scDirection;
		ESP_LOGD(TAG, "srcIndex=%d dstIndex=%d", srcIndex, dstIndex);
		clear_line(dstIndex, false);
		if (dstIndex == _scStart)
			break;
		srcIndex = srcIndex - _scDirection;
	}
}

void OLED_SSD1306::hardware_scroll(scroll_type_t scroll)
{
	if (_address == SPIAddress)
	{
		spi_hardware_scroll(scroll);
	}
	else
	{
		i2c_hardware_scroll(scroll);
	}
}

void OLED_SSD1306::wrap_around(scroll_type_t scroll, int start, int end, int8_t delay)
{
	if (scroll == SCROLL_RIGHT)
	{
		int _start = start; // 0 to 7
		int _end = end;     // 0 to 7
		if (_end >= _pages)
			_end = _pages - 1;
		uint8_t wk;
		for (int page = _start; page <= _end; page++)
		{
			wk = _page[page]._segs[127];
			for (int seg = 127; seg > 0; seg--)
			{
				_page[page]._segs[seg] = _page[page]._segs[seg - 1];
			}
			_page[page]._segs[0] = wk;
		}
	}
	else if (scroll == SCROLL_LEFT)
	{
		int _start = start; // 0 to 7
		int _end = end;     // 0 to 7
		if (_end >= _pages)
			_end = _pages - 1;
		uint8_t wk;
		for (int page = _start; page <= _end; page++)
		{
			wk = _page[page]._segs[0];
			for (int seg = 0; seg < 127; seg++)
			{
				_page[page]._segs[seg] = _page[page]._segs[seg + 1];
			}
			_page[page]._segs[127] = wk;
		}
	}
	else if (scroll == SCROLL_UP)
	{
		int _start = start; // 0 to {width-1}
		int _end = end;     // 0 to {width-1}
		if (_end >= _width)
			_end = _width - 1;
		uint8_t wk0;
		uint8_t wk1;
		uint8_t wk2;
		uint8_t save[128];
		for (int seg = 0; seg < 128; seg++)
		{
			save[seg] = _page[0]._segs[seg];
		}
		for (int page = 0; page < _pages - 1; page++)
		{
			for (int seg = _start; seg <= _end; seg++)
			{
				wk0 = _page[page]._segs[seg];
				wk1 = _page[page + 1]._segs[seg];
				if (_flip)
					wk0 = rotate_byte(wk0);
				if (_flip)
					wk1 = rotate_byte(wk1);
				wk0 = wk0 >> 1;
				wk1 = wk1 & 0x01;
				wk1 = wk1 << 7;
				wk2 = wk0 | wk1;
				if (_flip)
					wk2 = rotate_byte(wk2);
				_page[page]._segs[seg] = wk2;
			}
		}
		int pages = _pages - 1;
		for (int seg = _start; seg <= _end; seg++)
		{
			wk0 = _page[pages]._segs[seg];
			wk1 = save[seg];
			if (_flip)
				wk0 = rotate_byte(wk0);
			if (_flip)
				wk1 = rotate_byte(wk1);
			wk0 = wk0 >> 1;
			wk1 = wk1 & 0x01;
			wk1 = wk1 << 7;
			wk2 = wk0 | wk1;
			if (_flip)
				wk2 = rotate_byte(wk2);
			_page[pages]._segs[seg] = wk2;
		}
	}
	else if (scroll == SCROLL_DOWN)
	{
		int _start = start; // 0 to {width-1}
		int _end = end;     // 0 to {width-1}
		if (_end >= _width)
			_end = _width - 1;
		uint8_t wk0;
		uint8_t wk1;
		uint8_t wk2;
		uint8_t save[128];
		int pages = _pages - 1;
		for (int seg = 0; seg < 128; seg++)
		{
			save[seg] = _page[pages]._segs[seg];
		}
		for (int page = pages; page > 0; page--)
		{
			for (int seg = _start; seg <= _end; seg++)
			{
				wk0 = _page[page]._segs[seg];
				wk1 = _page[page - 1]._segs[seg];
				if (_flip)
					wk0 = rotate_byte(wk0);
				if (_flip)
					wk1 = rotate_byte(wk1);
				wk0 = wk0 << 1;
				wk1 = wk1 & 0x80;
				wk1 = wk1 >> 7;
				wk2 = wk0 | wk1;
				if (_flip)
					wk2 = rotate_byte(wk2);
				_page[page]._segs[seg] = wk2;
			}
		}
		for (int seg = _start; seg <= _end; seg++)
		{
			wk0 = _page[0]._segs[seg];
			wk1 = save[seg];
			if (_flip)
				wk0 = rotate_byte(wk0);
			if (_flip)
				wk1 = rotate_byte(wk1);
			wk0 = wk0 << 1;
			wk1 = wk1 & 0x80;
			wk1 = wk1 >> 7;
			wk2 = wk0 | wk1;
			if (_flip)
				wk2 = rotate_byte(wk2);
			_page[0]._segs[seg] = wk2;
		}
	}

	if (delay >= 0)
	{
		for (int page = 0; page < _pages; page++)
		{
			if (_address == SPIAddress)
			{
				spi_display_image(page, 0, _page[page]._segs, 128);
			}
			else
			{
				i2c_display_image(page, 0, _page[page]._segs, 128);
			}
			if (delay)
				vTaskDelay(delay);
		}
	}
}

void OLED_SSD1306::bitmaps(int xpos, int ypos, uint8_t *bitmap, int width, int height, bool invert)
{
	if ((width % 8) != 0)
	{
		ESP_LOGE(TAG, "width must be a multiple of 8");
		return;
	}
	int _width = width / 8;
	uint8_t wk0;
	uint8_t wk1;
	uint8_t wk2;
	uint8_t page = (ypos / 8);
	uint8_t _seg = xpos;
	uint8_t dstBits = (ypos % 8);
	ESP_LOGD(TAG, "ypos=%d page=%d dstBits=%d", ypos, page, dstBits);
	int offset = 0;
	for (int _height = 0; _height < height; _height++)
	{
		for (int index = 0; index < _width; index++)
		{
			for (int srcBits = 7; srcBits >= 0; srcBits--)
			{
				wk0 = _page[page]._segs[_seg];
				if (_flip)
					wk0 = rotate_byte(wk0);

				wk1 = bitmap[index + offset];
				if (invert)
					wk1 = ~wk1;

				wk2 = copy_bit(wk1, srcBits, wk0, dstBits);
				if (_flip)
					wk2 = rotate_byte(wk2);

				ESP_LOGD(TAG, "index=%d offset=%d page=%d _seg=%d, wk2=%02x", index, offset, page, _seg, wk2);
				_page[page]._segs[_seg] = wk2;
				_seg++;
			}
		}
		vTaskDelay(1);
		offset = offset + _width;
		dstBits++;
		_seg = xpos;
		if (dstBits == 8)
		{
			page++;
			dstBits = 0;
		}
	}
	show_buffer();
}

void OLED_SSD1306::pixel(int xpos, int ypos, bool invert)
{
	uint8_t _page = (ypos / 8);
	uint8_t _bits = (ypos % 8);
	uint8_t _seg = xpos;
	uint8_t wk0 = _page[_page]._segs[_seg];
	uint8_t wk1 = 1 << _bits;
	ESP_LOGD(TAG, "ypos=%d _page=%d _bits=%d wk0=0x%02x wk1=0x%02x", ypos, _page, _bits, wk0, wk1);
	if (invert)
	{
		wk0 = wk0 & ~wk1;
	}
	else
	{
		wk0 = wk0 | wk1;
	}
	if (_flip)
		wk0 = rotate_byte(wk0);
	ESP_LOGD(TAG, "wk0=0x%02x wk1=0x%02x", wk0, wk1);
	_page[_page]._segs[_seg] = wk0;
}

void OLED_SSD1306::line(int x1, int y1, int x2, int y2, bool invert)
{
	int i;
	int dx, dy;
	int sx, sy;
	int E;

	dx = (x2 > x1) ? x2 - x1 : x1 - x2;
	dy = (y2 > y1) ? y2 - y1 : y1 - y2;

	sx = (x2 > x1) ? 1 : -1;
	sy = (y2 > y1) ? 1 : -1;

	if (dx > dy)
	{
		E = -dx;
		for (i = 0; i <= dx; i++)
		{
			pixel(x1, y1, invert);
			x1 += sx;
			E += 2 * dy;
			if (E >= 0)
			{
				y1 += sy;
				E -= 2 * dx;
			}
		}
	}
	else
	{
		E = -dy;
		for (i = 0; i <= dy; i++)
		{
			pixel(x1, y1, invert);
			y1 += sy;
			E += 2 * dx;
			if (E >= 0)
			{
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

void OLED_SSD1306::invert(uint8_t *buf, size_t blen)
{
	uint8_t wk;
	for (int i = 0; i < blen; i++)
	{
		wk = buf[i];
		buf[i] = ~wk;
	}
}

void OLED_SSD1306::flip(uint8_t *buf, size_t blen)
{
	for (int i = 0; i < blen; i++)
	{
		buf[i] = rotate_byte(buf[i]);
	}
}

uint8_t OLED_SSD1306::copy_bit(uint8_t src, int srcBits, uint8_t dst, int dstBits)
{
	ESP_LOGD(TAG, "src=%02x srcBits=%d dst=%02x dstBits=%d", src, srcBits, dst, dstBits);
	uint8_t smask = 0x01 << srcBits;
	uint8_t dmask = 0x01 << dstBits;
	uint8_t _src = src & smask;
	uint8_t _dst;
	if (_src != 0)
	{
		_dst = dst | dmask;
	}
	else
	{
		_dst = dst & ~(dmask);
	}
	return _dst;
}

uint8_t OLED_SSD1306::rotate_byte(uint8_t ch1)
{
	uint8_t ch2 = 0;
	for (int j = 0; j < 8; j++)
	{
		ch2 = (ch2 << 1) + (ch1 & 0x01);
		ch1 = ch1 >> 1;
	}
	return ch2;
}

void OLED_SSD1306::fadeout()
{
	void (*func)(int page, int seg, uint8_t *images, int width);
	if (_address == SPIAddress)
	{
		func = spi_display_image;
	}
	else
	{
		func = i2c_display_image;
	}

	uint8_t image[1];
	for (int page = 0; page < _pages; page++)
	{
		image[0] = 0xFF;
		for (int line = 0; line < 8; line++)
		{
			if (_flip)
			{
				image[0] = image[0] >> 1;
			}
			else
			{
				image[0] = image[0] << 1;
			}
			for (int seg = 0; seg < 128; seg++)
			{
				(*func)(page, seg, image, 1);
				_page[page]._segs[seg] = image[0];
			}
		}
	}
}

void OLED_SSD1306::dump()
{
	printf("_address=%x\n", _address);
	printf("_width=%x\n", _width);
	printf("_height=%x\n", _height);
	printf("_pages=%x\n", _pages);
}

void OLED_SSD1306::dump_page(int page, int seg)
{
	ESP_LOGI(TAG, "dev->_page[%d]._segs[%d]=%02x", page, seg, _page[page]._segs[seg]);
}
		