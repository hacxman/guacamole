#include "msg.h"
#include "crc8.h"
#include <libopencm3/stm32/usart.h>

uint8_t get_msg_len_from_buffer(char *buf) {
  msg_header *hdr = (msg_header*) buf;
  return hdr->len;
}

uint8_t get_msg_hops_from_buffer(char *buf) {
  msg_header *hdr = (msg_header*) buf;
  return hdr->hops;
}

void lower_msg_hops_in_buffer(char *buf) {
  msg_header *hdr = (msg_header*) buf;
  if ((hdr->hops < 255) && (hdr->hops > 0)) {
      -- (hdr->hops);
  }
}

const char MSG_ACK = 'A';
const char MSG_CORRUPT = '#';
const char MSG_FULL = 'F';

void send_corrupt(uint8_t side) {
}

void send_ack(uint8_t side) {
}

void send_full(uint8_t side) {
}

void msg_send_packet(uint32_t uart, msg_state *state) {
  lower_msg_hops_in_buffer(state->in_buffer);
  int max = get_msg_len_from_buffer(state->in_buffer);
  uint8_t crc = crc8(state->in_buffer, state->in_count-2);
  msg_footer *ftr = (msg_footer*)(state->in_buffer+max+2);
  ftr->crc8 = crc;
  ftr->xored = crc ^ 0xff;
  for (int i=0; i < max+4; i++) {
    usart_send(uart, state->in_buffer[i]);
  }
}

char _msg[256];
void send_msg(uint8_t hops, uint8_t side, char *msg, uint8_t size) {
  uint32_t dev = side==0?USART1:UART4;

  msg_header *hdr = (msg_header*) _msg;
  hdr->hops = hops;
  hdr->len = size;

  for (uint8_t i=0; i < size; i++) {
    _msg[i+2] = msg[i];
  }

  uint8_t crc = crc8(_msg, size+2); //(size=body len), headers + body, without footer
  msg_footer *ftr = (msg_footer*)(_msg+size+2);
  ftr->crc8 = crc;
  ftr->xored = crc ^ 0xff;

  for (int i=0; i < size+4; i++) {
    usart_send(dev, _msg[i]);
  }
}

uint8_t check_crc(msg_state* state) {
  return 1;
  return crc8(state->in_buffer, state->in_count-2) == state->in_buffer[state->in_count-2];
}

char msg_process_byte(char msg, msg_state* state) {
  if (state->in_state == RECV_HEADER) {
    state->in_buffer[state->in_count] = msg;
    ++(state->in_count);
    if (state->in_count == sizeof(msg_header)) {
      state->in_state = RECV_BODY;
    }
  } else if (state->in_state == RECV_BODY) {
    state->in_buffer[state->in_count] = msg;
    ++(state->in_count);
    if (state->in_count == sizeof(msg_header) + get_msg_len_from_buffer(state->in_buffer)) {
      state->in_state = RECV_FOOTER;
    }
  } else if (state->in_state == RECV_FOOTER) {
    state->in_buffer[state->in_count] = msg;
    ++(state->in_count);
    if (state->in_count == sizeof(msg_header) + get_msg_len_from_buffer(state->in_buffer) + sizeof(msg_footer)) {
      state->in_state = RECV_FULL;
      if (check_crc(state)) {
        return MSG_ACK;
      } else {
        return MSG_CORRUPT;
      }
    }
  } else if (state->in_state == RECV_FULL) {
    return MSG_FULL;
  }
  return '\0';
}
