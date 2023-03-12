
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/systick.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <u8x8.h>
#include <u8g2.h>
#include "setup.c"
#include "util.c"

int _write(int file, char *ptr, int len);

/* Initialize I2C1 interface */
static void i2c_setup(void) {
	/* Enable GPIOB and I2C1 clocks */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_I2C1);

	/* Set alternate functions for SCL and SDA pins of I2C1 */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
				  GPIO_I2C1_SCL | GPIO_I2C1_SDA);

	/* Disable the I2C peripheral before configuration */
	i2c_peripheral_disable(I2C1);

	/* APB1 running at 36MHz */
	i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);

	/* 400kHz - I2C fast mode */
	i2c_set_fast_mode(I2C2);
	i2c_set_ccr(I2C1, 0x1e);
	i2c_set_trise(I2C1, 0x0b);

	/* And go */
	i2c_peripheral_enable(I2C1);
}

static uint8_t u8x8_gpio_and_delay_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch(msg) {
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		i2c_setup();  /* Init I2C communication */
		break;

	default:
		u8x8_SetGPIOResult(u8x8, 1);
		break;
	}

	return 1;
}

/* I2C hardware transfer based on u8x8_byte.c implementation */
static uint8_t u8x8_byte_hw_i2c_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	static uint8_t buffer[32];   /* u8g2/u8x8 will never send more than 32 bytes */
	static uint8_t buf_idx;
	uint8_t *data;
	uint8_t address = u8x8->i2c_address;
		switch(msg) {
	case U8X8_MSG_BYTE_SEND:
		data = (uint8_t *)arg_ptr;
		while(arg_int > 0) {
			buffer[buf_idx++] = *data;
			data++;
			arg_int--;
		}
		break;
	case U8X8_MSG_BYTE_INIT:
		break;
	case U8X8_MSG_BYTE_SET_DC:
		break;
	case U8X8_MSG_BYTE_START_TRANSFER:
		buf_idx = 0;
		break;
	case U8X8_MSG_BYTE_END_TRANSFER:
		i2c_transfer7(I2C1, address>>1, buffer, buf_idx, NULL, 0);
		break;
	default:
		return 0;
	}
	return 1;
}

int main(void) {
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
	clock_setup();
	systick_setup();
	gpio_setup();
	usart_setup();
	printf("hello\r\n");
	u8g2_t u8g2_i1, *display1 = &u8g2_i1;
	u8g2_t u8g2_i2, *display2 = &u8g2_i2;

	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	u8g2_Setup_ssd1306_i2c_128x64_noname_f(display1, U8G2_R0, u8x8_byte_hw_i2c_cm3, u8x8_gpio_and_delay_cm3);
	u8g2_SetI2CAddress(display1, 0x3D<<1);
	u8g2_InitDisplay(display1);
	u8g2_SetPowerSave(display1, 0);

	u8g2_ClearBuffer(display1);
	u8g2_SendBuffer(display1);
	u8g2_SetFont(display1, u8g2_font_ncenB14_tr);
	u8g2_DrawStr(display1, 0, 15, "Hello #1!");
	u8g2_DrawCircle(display1, 64, 40, 10, U8G2_DRAW_ALL);
	u8g2_SendBuffer(display1);

	u8g2_Setup_ssd1306_i2c_128x64_noname_f(display2, U8G2_R0, u8x8_byte_hw_i2c_cm3, u8x8_gpio_and_delay_cm3);
	u8g2_SetI2CAddress(display2, 0x3C<<1);
	u8g2_InitDisplay(display2);
	u8g2_SetPowerSave(display2, 0);

	u8g2_ClearBuffer(display2);
	u8g2_SendBuffer(display2);
	u8g2_SetFont(display2, u8g2_font_ncenB14_tr);
	u8g2_DrawStr(display2, 0, 15, "Hello #2!");
	u8g2_DrawFrame(display2, 54, 30, 20, 20);
	u8g2_SendBuffer(display2);

	while(true) {
		gpio_toggle(GPIOC, GPIO13);

		delay(1000);
	}
	return 0;
}

