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
#include <libopencm3/stm32/timer.h>
#include <libopencmsis/core_cm3.h>

#include "msg.h"

#define ENABLE_UART4 {usart_enable_rx_interrupt(UART4); /*usart_enable_tx_interrupt(UART4);*/ }
#define DISABLE_UART4 {usart_disable_rx_interrupt(UART4); usart_disable_tx_interrupt(UART4); }
//#define ENABLE_UART4 usart_enable_interrupts(UART4, (UART_INT_TXIM | UART_INT_TXIM))

#define DISABLE_USART1 {usart_disable_rx_interrupt(USART1); usart_disable_tx_interrupt(USART1); }
#define ENABLE_USART1 {usart_enable_rx_interrupt(USART1); /*usart_enable_tx_interrupt(USART1);*/ }
//#define DISABLE_USART1 usart_disable_interrupts(USART1, (UART_INT_TXIM | UART_INT_TXIM))
//#define ENABLE_USART1 usart_enable_interrupts(USART1, (UART_INT_TXIM | UART_INT_TXIM))

uint16_t compare_time;
uint16_t new_time;

char left_in_buffer[256];
char right_in_buffer[256];
int _write(int file, char *ptr, int len);

int tgt;

static void clock_setup(void)
{
	/* Enable clocks on all the peripherals we are going to use. */
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_UART4);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
}

msg_state left_port_state;
msg_state right_port_state;
enum port_state uart4_port_state;
enum port_state uart1_port_state;
char uart_loopback_check_char = 'X';

// TODO:
// * add some modularity to uarts
//   - make it possible to have list of "modules"
//     currently running
//   - should ease loopback detection phase
//   - and should make it possible to have phases and
//     different "distributed algos" currently running
// * refactor! goes with above ^
// * timeouts for every transfer:
//   next byte should come no later than 3 times
//   normal timing derived from baud rate
//   best using some timer

void say(char *msg) {
  while (*msg != '\0') {
      gpio_port_write(GPIOA, *msg++);
  }
}

void say_int(int i) {
  int x = i;
  int order = 0;
  int bw = 0;;
  for (x = i; x; order++) {
    bw *= 10;
    bw += x % 10;
    x /= 10;
  }

  for (x = bw; x; ) {
    gpio_port_write(GPIOA, '0' + (x % 10));
    x /= 10;
  }

}

void uart4_isr(void) {
  say("usayrt4 int\n");
  DISABLE_UART4;
  if (uart4_port_state == PORT_CONFIGURING) {
    char c = usart_recv(UART4);
    if (c == uart_loopback_check_char) {
      uart4_port_state = PORT_LOOBPACK;
    } else if (c == 'Y') {
      uart4_port_state = PORT_ONLINE;
    }
    ENABLE_UART4;
    return;
  }

  if (uart4_port_state == PORT_ONLINE) {
    uint8_t data = usart_recv(UART4);
    if (right_port_state.out_state == SEND_FULL) {
      // waiting for ACK, todo:NACK, CORRUPT
      if (data == 'A') {
        right_port_state.out_state = SEND_HEADER;
        right_port_state.out_count = 0;
      }
      ENABLE_UART4;
      return;
    }
    uint8_t r = msg_process_byte(data, &right_port_state);
    if (r != 0) {
      // send ACK, NACK or CORRUPT
      usart_send(UART4, 'A');
    } else {
      //usart_send(UART4, '-');
    }
    if (right_port_state.in_state == RECV_FULL) {
      if (get_msg_hops_from_buffer(right_port_state.in_buffer) == 0) {
        // packet is for us
        //say_int(right_port_state.in_count);
        say("uart4");
        say("\n");
        send_msg(0, 0, "zdar vole", 9);
        right_port_state.in_state = RECV_HEADER;
        right_port_state.in_count = 0;
        right_port_state.out_state = SEND_FULL;
      } else {
        msg_send_packet(USART1, &right_port_state);
        left_port_state.out_state = SEND_FULL;
        right_port_state.in_state = RECV_HEADER;
        right_port_state.in_count = 0;
      }
    }
  }
  ENABLE_UART4;

//  usart_send(USART1, data);
//  usart_send(UART4, 'j');
}

void usart1_isr(void) {
  DISABLE_USART1;
  uint8_t data = usart_recv(USART1);
  if (uart1_port_state == PORT_CONFIGURING) {
    if (data == 'X') {
      uart1_port_state = PORT_ONLINE;
      usart_send(USART1, 'Y');
      ENABLE_USART1;
      return;
    } //else {
      //uart1_port_state = PORT_ONLINE;
    //}
  }

  if (uart1_port_state == PORT_ONLINE) {

    if (left_port_state.out_state == SEND_FULL) {
      // waiting for ACK, todo:NACK, CORRUPT
      if (data == 'A') {
        left_port_state.out_state = SEND_HEADER;
        left_port_state.out_count = 0;
      }
      left_port_state.out_state = SEND_HEADER;
      ENABLE_USART1;
      return;
    }

    uint8_t r = msg_process_byte(data, &left_port_state);
    if (r != 0) {
      // send ACK, NACK or CORRUPT
      usart_send(USART1, 'A');
    } else {
      //usart_send(USART1, '-');
  //    printf ("l=%i\n", left_port_state.in_count);
    }
    if (left_port_state.in_state == RECV_FULL) {
      uint8_t hops = get_msg_hops_from_buffer(left_port_state.in_buffer);
      if (hops == 0) {
        // packet is for us
        say("usart1 packet for us\n");
        //send_msg(tgt%2, 0, "zdar vole", 9);
//        send_msg(1, 0, "zdar vole", 9);
////        usart_send(USART1, 255);
////        usart_send(USART1, 1);
////        usart_send(USART1, 'Q');
////        usart_send(USART1, 'K');
////        usart_send(USART1, 'J');
        //left_port_state.out_state = SEND_FULL;
        left_port_state.in_state = RECV_HEADER;
        left_port_state.in_count = 0;
      } else {
        if (uart4_port_state == PORT_LOOBPACK) {
//          for (uint32_t i = 0; i < 10000000; i++) {
//            for (uint32_t j = 0; j < 10; j++) {
//              asm volatile ("nop");
//            }
//          }
          msg_send_packet(USART1, &left_port_state);
//          usart_send(UART4, 'D');
//          msg_send_packet(UART4, &left_port_state);
          left_port_state.out_state = SEND_FULL;
        } else {
          //msg_send_packet(USART1, &left_port_state);
          //left_port_state.out_state = SEND_FULL;
          msg_send_packet(UART4, &left_port_state);
          right_port_state.out_state = SEND_FULL;
        }
        left_port_state.in_state = RECV_HEADER;
        left_port_state.in_count = 0;
      }
    }
  }

  ENABLE_USART1;

//  data = usart_recv(UART4);
//  usart_send(USART1, data);
//  usart_send(UART4, data);
//  usart_send(USART1, 'k');
//  gpio_port_write(GPIOA, data);
}

void exti1_isr(void) {
  uint8_t data = gpio_port_read(GPIOA);
  gpio_port_write(GPIOA, 'A');
}


void tim2_isr(void)
{
	if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {

		/* Clear compare interrupt flag. */
		timer_clear_flag(TIM2, TIM_SR_CC1IF);

		/*
		 * Get current timer value to calculate next
		 * compare register value.
		 */
		compare_time = timer_get_counter(TIM2);

		/* Calculate and set the next compare value. */
		uint16_t frequency = 100; //frequency_sequence[frequency_sel++];
		new_time = compare_time + frequency;

		timer_set_oc_value(TIM2, TIM_OC1, new_time);
    if (uart1_port_state == PORT_ONLINE) {
      usart_send(USART1, 'T');
    }
//		if (frequency_sel == 18)
//			frequency_sel = 0;

		/* Toggle LED to indicate compare event. */
		//gpio_toggle(GPIOD, GPIO12);
		//gpio_toggle(GPIOD, GPIO13);
	}
}




static void usart_setup(void)
{
  nvic_enable_irq(NVIC_USART1_IRQ);
//	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
//	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
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
	usart_set_baudrate(UART4, 115200);
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



  right_port_state.out_state = RECV_HEADER;
  right_port_state.out_count = 0;

  right_port_state.in_state = RECV_HEADER;
  right_port_state.in_count = 0;
  right_port_state.in_buffer = right_in_buffer;


  left_port_state.out_state = RECV_HEADER;
  left_port_state.out_count = 0;

  left_port_state.in_state = RECV_HEADER;
  left_port_state.in_count = 0;
  left_port_state.in_buffer = left_in_buffer;
}

static void gpio_setup(void)
{
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_MODE_OUTPUT, GPIO3);
	gpio_set(GPIOC, GPIO3);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_MODE_OUTPUT, GPIO_ALL);
	gpio_set(GPIOA, GPIO_ALL);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
}

static void timer_setup(void) {
	rcc_periph_clock_enable(RCC_TIM2);

	nvic_enable_irq(NVIC_TIM2_IRQ);

	timer_reset(TIM2);

	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM2, 4); //((rcc_apb1_frequency * 2) / 10000));

	/* Enable preload. */
	timer_disable_preload(TIM2);

	/* Continous mode. */
	timer_continuous_mode(TIM2);

	/* Period (36kHz). */
	timer_set_period(TIM2, 65535);

	/* Disable outputs. */
	timer_disable_oc_output(TIM2, TIM_OC1);
	timer_disable_oc_output(TIM2, TIM_OC2);
	timer_disable_oc_output(TIM2, TIM_OC3);
	timer_disable_oc_output(TIM2, TIM_OC4);

	/* -- OC1 configuration -- */

	/* Configure global mode of line 1. */
	timer_disable_oc_clear(TIM2, TIM_OC1);
	timer_disable_oc_preload(TIM2, TIM_OC1);
	timer_set_oc_slow_mode(TIM2, TIM_OC1);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_FROZEN);

	/* Set the capture compare value for OC1. */
	timer_set_oc_value(TIM2, TIM_OC1, 1000);

	/* ARR reload enable. */
	timer_disable_preload(TIM2);

	/* Counter enable. */
	timer_enable_counter(TIM2);

	/* Enable commutation interrupt. */
	timer_enable_irq(TIM2, TIM_DIER_CC1IE);


}

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
      //if (uart4_port_state == PORT_LOOBPACK) {
      //  usart_send(UART4, ptr[i]);
      //}
      gpio_port_write(GPIOA, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

int main(void)
{
	clock_setup();
	gpio_setup();
	usart_setup();
  //timer_setup();

  uart_loopback_check_char = 'X';
  uart4_port_state = PORT_CONFIGURING;
  uart1_port_state = PORT_CONFIGURING;
	for (uint32_t i = 0; i < 10000000; i++) {
    for (uint32_t j = 0; j < 50; j++) {
      asm volatile ("nop");
    }
  }
  usart_send(UART4, uart_loopback_check_char);

	while (1) {
//    gpio_port_write(GPIOA, 'Q');
//    say("kkt\n");
//    say_int(12435);
//    say("\n");
		__WFI(); /* Wait For Interrupt. */
    ++tgt;
	}

	return 0;
}
