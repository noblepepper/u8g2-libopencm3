int _write(int file, char *ptr, int len);

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

void sys_tick_handler(void)
{
	DelayCounter++;
}

static void delay_ms(uint32_t ms)
{
	DelayCounter = 0;
	while (DelayCounter < ms * 1000) __asm__("nop")
		;
}

static void delay_us(uint32_t us)
{
	DelayCounter = 0;
	while (DelayCounter < us) __asm__("nop")
		;
}

