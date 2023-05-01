void systick_setup(void);

static void clock_setup(void)
{
	/* set sys cloct to 96Mhz */
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_96MHZ]);
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
	/* Setup GPIO pins for USART1 transmit. */
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO9);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9);
	/* Setup GPIO pins for USART1 receive. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO10);
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
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
}

void systick_setup(void)
{
	/* 96MHz / 8 => 12000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* 12000000/1200 = 10,000 overflows per second
	 * every 0.1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(1199);
	systick_interrupt_enable();
	/* Start counting. */
	systick_counter_enable();
}

/* Initialize I2C1 interface */
static void i2c_setup(void) {
	/* Enable GPIOB and I2C1 clocks */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_I2C1);
	/* Set alternate functions for SCL and SDA pins of I2C1 */
	gpio_set_output_options(GPIOB, 
			GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO6|GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6|GPIO7);
	gpio_set_af(GPIOB, GPIO_AF4 , GPIO6|GPIO7);
	/* Disable the I2C peripheral before configuration */
	i2c_peripheral_disable(I2C1);
	/* APB1 running at 48MHz */
	i2c_set_clock_frequency(I2C1, 48);
	/* 400kHz - I2C fast mode */
	i2c_set_speed(I2C1, i2c_speed_fm_400k, 48);
	/* And go */
	i2c_peripheral_enable(I2C1);
}

