
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
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

static void dma_setup(void)
{
	/*
	 * Using channel 6 for I2C_TX
	 */
	dma_channel_reset(DMA1, DMA_CHANNEL6);
	dma_set_peripheral_address(DMA1, DMA_CHANNEL6, (uint32_t)&I2C1_DR);
	dma_set_read_from_memory(DMA1, DMA_CHANNEL6);
	dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL6);
	dma_set_peripheral_size(DMA1, DMA_CHANNEL6, DMA_CCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_CHANNEL6, DMA_CCR_MSIZE_8BIT);
	dma_set_priority(DMA1, DMA_CHANNEL6, DMA_CCR_PL_VERY_HIGH);
        i2c_enable_dma(I2C1);
}
	
static int dma_flag = 0;

static void dma_start(char* data, int size)
{
	while (dma_flag == 1);
	dma_set_memory_address(DMA1, DMA_CHANNEL6, (uint32_t)data);
	dma_set_number_of_data(DMA1, DMA_CHANNEL6, size);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL6);
	dma_enable_channel(DMA1, DMA_CHANNEL6);
	dma_flag=1;
}



void dma1_channel6_isr(void)
{
	if ((DMA1_ISR & DMA_ISR_TCIF6) != 0) {
		dma_disable_channel(DMA1, DMA_CHANNEL6);
		dma_clear_interrupt_flags(DMA1, DMA_CHANNEL6, DMA_TCIF);
		dma_flag = 0;

	}
}

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

void i2c_write(uint32_t i2c, int addr)
{
        while ((I2C_SR2(i2c) & I2C_SR2_BUSY)) {
        }
        i2c_send_start(i2c);
        /* Wait for the end of the start condition, master mode selected, and BUSY bit set */
        while ( !( (I2C_SR1(i2c) & I2C_SR1_SB)
                && (I2C_SR2(i2c) & I2C_SR2_MSL)
                && (I2C_SR2(i2c) & I2C_SR2_BUSY) ));
        i2c_send_7bit_address(i2c, addr, I2C_WRITE);
        /* Waiting for address is transferred. */
        while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
        /* Clearing ADDR condition sequence. */
        (void)I2C_SR2(i2c);
	while (!(I2C_SR1(i2c) & (I2C_SR1_BTF)));

}

void setup_transfer(){
	dma_flag = 0;
        dma_setup();
        i2c_setup();
}

void make_transfer(char* data, int numdata, int addr){
        dma_start(data, numdata);
        i2c_write(I2C1, addr>>1);
        i2c_send_stop(I2C1);

}


static uint8_t u8x8_gpio_and_delay_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch(msg) {
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		setup_transfer();
//		i2c_setup();  /* Init I2C communication */
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
//		i2c_transfer7(I2C1, address>>1, buffer, buf_idx, NULL, 0);
		make_transfer(buffer, buf_idx, address);	
		break;
	default:
		return 0;
	}
	return 1;
}

int main(void) {
	clock_setup();
	systick_setup();
	gpio_setup();
	usart_setup();
	printf("hello\r\n");
	
	
	nvic_set_priority(NVIC_DMA1_CHANNEL6_IRQ, 0);
	nvic_enable_irq(NVIC_DMA1_CHANNEL6_IRQ);
/*	
       	uint8_t address = 0x03C<<1;

uint8_t buffer[32]="abcdefghijklmnopqrstuvxyzabcdef", buf_size=32;

setup_transfer();
        make_transfer(buffer, buf_size, address);
*/	
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

