
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

int _write(int file, char *ptr, int len);

static void clock_setup(void)
{

	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Enable clocks for GPIO port B (for GPIO_USART3_TX) and USART3. */
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_USART3);
}

static void usart_setup(void)
{
	/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port B for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

static void gpio_setup(void)
{
	gpio_set(GPIOC, GPIO13);

	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
}

void systick_setup(void)
{
	/* 72MHz / 8 => 9000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

	/* 9000000/900 = 10000 overflows per second - every 0.1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(899);

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();
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

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send_blocking(USART1, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

uint32_t DelayCounter;

/* Wait a bit - the lazy version */
//static void delay(int n) {
//	for(int i = 0; i < n; i++)
//		__asm__("nop");
//}

void sys_tick_handler(void)
{
	DelayCounter++;
}

static void delay(uint32_t _100us)
{
	DelayCounter = 0;
	while (DelayCounter < _100us) __asm__("nop")
		;
}

static uint8_t u8x8_gpio_and_delay_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	printf("gpio msg: %i\r\n", msg);
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
	static uint8_t buffer[1024];   /* u8g2/u8x8 will never send more than 32 bytes */
	static uint8_t buf_idx;
	uint8_t *data;
	printf("transfer msg: %i\r\n", msg);

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
		i2c_transfer7(I2C1, 0x3C, buffer, buf_idx, NULL, 0);
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
	u8x8_t u8x8_i, *u8x8 = &u8x8_i;
	u8g2_t u8g2_i, *u8g2 = &u8g2_i;

	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_hw_i2c_cm3, u8x8_gpio_and_delay_cm3);
	u8g2_InitDisplay(u8g2);
	u8g2_SetPowerSave(u8g2,0);

	u8g2_ClearBuffer(u8g2);
	u8g2_SetFont(u8g2, u8g2_font_ncenB14_tr);
	u8g2_DrawStr(u8g2, 0, 15, "Hello World!");
	u8g2_DrawCircle(u8g2, 64, 40, 10, U8G2_DRAW_ALL);
	u8g2_SendBuffer(u8g2);

	while(true) {
		gpio_toggle(GPIOC, GPIO13);

		delay(1000);
	}
	return 0;
}

