static void outb(unsigned char value, unsigned short port)
{
        __builtin_outb(value, port);
}

static void outl(unsigned int value, unsigned short port)
{
	__builtin_outl(value, port);
}

static unsigned char inb(unsigned short port)
{
        return __builtin_inb(port);
}

static unsigned char inl(unsigned short port)
{
	return __builtin_inl(port);
}

static unsigned int config_cmd(unsigned char bus, unsigned devfn, unsigned where)
{
	return 0x80000000 | (bus << 16) | (devfn << 8) | (where & ~3);
}

static unsigned int pcibios_read_config_dword(
	unsigned char bus, unsigned devfn, unsigned where)
{
	outl(config_cmd(bus, devfn, where), 0xCF8);
	return inl(0xCFC);
}



/* Base Address */
#ifndef TTYS0_BASE
#define TTYS0_BASE 0x3f8
#endif

#ifndef TTYS0_BAUD
#define TTYS0_BAUD 115200
#endif

#if ((115200%TTYS0_BAUD) != 0)
#error Bad ttys0 baud rate
#endif

#define TTYS0_DIV	(115200/TTYS0_BAUD)

/* Line Control Settings */
#ifndef TTYS0_LCS
/* Set 8bit, 1 stop bit, no parity */
#define TTYS0_LCS	0x3
#endif

#define UART_LCS	TTYS0_LCS

/* Data */
#define UART_RBR 0x00
#define UART_TBR 0x00

/* Control */
#define UART_IER 0x01
#define UART_IIR 0x02
#define UART_FCR 0x02
#define UART_LCR 0x03
#define UART_MCR 0x04
#define UART_DLL 0x00
#define UART_DLM 0x01

/* Status */
#define UART_LSR 0x05
#define UART_MSR 0x06
#define UART_SCR 0x07

int uart_can_tx_byte(void)
{
	return inb(TTYS0_BASE + UART_LSR) & 0x20;
}

void uart_wait_to_tx_byte(void)
{
	while(!uart_can_tx_byte())
		;
}

void uart_wait_until_sent(void)
{
	while(!(inb(TTYS0_BASE + UART_LSR) & 0x40)) 
		;
}

void uart_tx_byte(unsigned char data)
{
	uart_wait_to_tx_byte();
	outb(data, TTYS0_BASE + UART_TBR);
	/* Make certain the data clears the fifos */
	uart_wait_until_sent();
}

void uart_init(void)
{
	/* disable interrupts */
	outb(0x0, TTYS0_BASE + UART_IER);
	/* enable fifo's */
	outb(0x01, TTYS0_BASE + UART_FCR);
	/* Set Baud Rate Divisor to 12 ==> 115200 Baud */
	outb(0x80 | UART_LCS, TTYS0_BASE + UART_LCR);
	outb(TTYS0_DIV & 0xFF,   TTYS0_BASE + UART_DLL);
	outb((TTYS0_DIV >> 8) & 0xFF,    TTYS0_BASE + UART_DLM);
	outb(UART_LCS, TTYS0_BASE + UART_LCR);
}

void __console_tx_char(unsigned char byte)
{
	uart_tx_byte(byte);
}
void __console_tx_nibble(unsigned nibble)
{
	unsigned char digit;
	digit = nibble + '0';
	if (digit > '9') {
		digit += 39;
	}
	__console_tx_char(digit);
}

void __console_tx_hex32(unsigned int value)
{
	__console_tx_nibble((value >> 28) & 0x0f);
	__console_tx_nibble((value >> 24) & 0x0f);
	__console_tx_nibble((value >> 20) & 0x0f);
	__console_tx_nibble((value >> 16) & 0x0f);
	__console_tx_nibble((value >> 12) & 0x0f);
	__console_tx_nibble((value >>  8) & 0x0f);
	__console_tx_nibble((value >>  4) & 0x0f);
	__console_tx_nibble(value & 0x0f);
}

void print_debug_hex32(unsigned int value) { __console_tx_hex32(value); }


void main(void)
{
	unsigned long htic;
	htic = pcibios_read_config_dword(0, 0xc0, 0x6c);
	print_debug_hex32(htic);
}
