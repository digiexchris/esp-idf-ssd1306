#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "driver/spi_master.h"
#include <cstdint>

class OLED_SSD1306 {
public:
    class OLED_SSD1306 {
(int width, int height, uint8_t i2c_addr = 0x3C, uint8_t spi_addr = 0xFF);
    virtual ~SSD1306();

    void init();
    int getWidth() const;
    int getHeight() const;
    int getPages() const;
    void showBuffer();
    void setBuffer(uint8_t *buffer);
    void getBuffer(uint8_t *buffer) const;
    void displayImage(int page, int seg, uint8_t *images, int width);
    void displayText(int page, char *text, int text_len, bool invert);
    void displayTextX3(int page, char *text, int text_len, bool invert);
    void clearScreen(bool invert);
    void clearLine(int page, bool invert);
    void setContrast(int contrast);
    void softwareScroll(int start, int end);
    void scrollText(char *text, int text_len, bool invert);
    void scrollClear();
    void hardwareScroll(ssd1306_scroll_type_t scroll);
    void wrapAround(ssd1306_scroll_type_t scroll, int start, int end, int8_t delay);
    void bitmaps(int xpos, int ypos, uint8_t *bitmap, int width, int height, bool invert);
    void fadeout();
    void dump();
    void dumpPage(int page, int seg);

    static void invert(uint8_t *buf, size_t blen);
    static void flip(uint8_t *buf, size_t blen);
    static uint8_t copyBit(uint8_t src, int srcBits, uint8_t dst, int dstBits);
    static uint8_t rotateByte(uint8_t ch1);

private:
    struct Page {
        bool valid;
        int segLen;
        uint8_t segs[128];
    };

    int address;
    int width;
    int height;
    int pages;
    int dc;
    spi_device_handle_t SPIHandle;
    bool scEnable;
    int scStart;
    int scEnd;
    int scDirection;
    Page page[8];
    bool flip;
};

class I2C_SSD1306 : public OLED_SSD1306 {
public:
    void i2cMasterInit(int16_t sda, int16_t scl, int16_t reset);
    void i2cInit(int width, int height);
    void i2cDisplayImage(int page, int seg, uint8_t *images, int width);
    void i2cContrast(int contrast);
    void i2cHardwareScroll(ssd1306_scroll_type_t scroll);I2C_SSD1306
};

class SPI_SSD1306 : public OLED_SSD1306 {
public:
    void spiMasterInit(int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS, int16_t GPIO_DC, int16_t GPIO_RESET);
    bool spiMasterWriteByte(const uint8_t* Data, size_t DataLength);
    bool spiMasterWriteCommand(uint8_t Command);
    bool spiMasterWriteData(const uint8_t* Data, size_t DataLength);
    void spiInit(int width, int height);
    void spiDisplayImage(int page, int seg, uint8_t *images, int width);
    void spiContrast(int contrast);
    void spiHardwareScroll(ssd1306_scroll_type_t scroll);
}

#endif // SSD1306_H
