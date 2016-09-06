/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2011 Piotr Esden-Tempski <piotr@esden.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>

int _write(int file, char *ptr, int len);

static void clock_setup(void)
{
	/* Enable clocks on all the peripherals we are going to use. */
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_UART4);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
}

void uart4_isr(void) {
  uint8_t data = usart_recv(UART4);
  usart_send(USART1, data);
//  usart_send(UART4, 'j');
}


void usart1_isr(void) {
  uint8_t data = usart_recv(USART1);
  usart_send(UART4, data);
//  data = usart_recv(UART4);
//  usart_send(USART1, data);
//  usart_send(UART4, data);
//  usart_send(USART1, 'k');
//  gpio_port_write(GPIOA, data);
}

void exti1_isr(void) {
  uint8_t data = gpio_port_read(GPIOA);
  gpio_port_write(GPIOA, 'Q');
}

static void usart_setup(void)
{
  nvic_enable_irq(NVIC_USART1_IRQ);
//	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
//	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 9600);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	USART_CR1(USART1) |= USART_CR1_RXNEIE;

	/* Finally enable the USART. */
	usart_enable(USART1);

  nvic_enable_irq(NVIC_UART4_IRQ);
//	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
//	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	/* Setup UART parameters. */
	usart_set_baudrate(UART4, 9600);
	usart_set_databits(UART4, 8);
	usart_set_stopbits(UART4, USART_STOPBITS_1);
	usart_set_parity(UART4, USART_PARITY_NONE);
	usart_set_flow_control(UART4, USART_FLOWCONTROL_NONE);
	usart_set_mode(UART4, USART_MODE_TX_RX);
	USART_CR1(UART4) |= USART_CR1_RXNEIE;

	/* Finally enable the USART. */
	usart_enable(UART4);
//  x = USART_CR1(USART1);
//  char *j = (char*)0x40004c00; *(j + 0x17) =  32;
}

static void gpio_setup(void)
{
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_MODE_OUTPUT, GPIO3);
	gpio_set(GPIOC, GPIO3);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_MODE_OUTPUT, GPIO_ALL);
	gpio_set(GPIOA, GPIO_ALL);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
}

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send(USART1, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

int main(void)
{
	int counter = 0;
	float fcounter = 0.0;
	double dcounter = 0.0;

	clock_setup();
	gpio_setup();
	usart_setup();

	/*
	 * Write Hello World an integer, float and double all over
	 * again while incrementing the numbers.
	 */
	while (1) {
//		gpio_toggle(GPIOA, GPIO3);
    //char c = gpio_port_read(GPIOA);
    //gpio_port_write(GPIOA, c);
//		printf("Hello World! %i %f %f\r\n", &UART4, fcounter,
//		       dcounter);
//    usart_send(UART4, usart_recv(UART4));
    //usart_send(UART4, 'x');
		counter++;
		fcounter += 0.01;
		dcounter += exp(fcounter);;
	}

	return 0;
}
