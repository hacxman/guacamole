#ifndef _MSG_H_
#define _MSG_H_
#include <stdint.h>

enum send_state {
  SEND_HEADER,
  SEND_BODY,
  SEND_FOOTER,
  SEND_FULL
};


enum recv_state {
  RECV_HEADER,
  RECV_BODY,
  RECV_FOOTER,
  RECV_FULL
};

typedef struct {
  uint8_t hops;
  uint8_t len;
} msg_header;

typedef struct {
  uint8_t crc8;
  uint8_t xored;
} msg_footer;

typedef struct {
  char *in_buffer;
  char *out_buffer;
  uint8_t in_state;
  uint8_t in_count;
  uint8_t out_state;
  uint8_t out_count;
} msg_state;

enum port_state {
  PORT_CONFIGURING,
  PORT_ONLINE,
  PORT_LOOBPACK
};

#define MSG_MAX_LEN (256 - sizeof(msg_header) - sizeof(msg_footer))

extern char in_buffer[256];
extern char out_buffer[256];

void process_msg(char *msg);

//void enqueue_msg(char *msg, int8_t direction, uint8_t hops) {
//
//}

void send_msg(uint8_t hops, uint8_t side, char *msg, uint8_t size);
void msg_send_packet(uint32_t uart, msg_state *state);
uint8_t get_msg_len_from_buffer(char *buf);
uint8_t get_msg_hops_from_buffer(char *buf);
void lower_msg_hops_in_buffer(char *buf);
char msg_process_byte(char msg, msg_state* state);
#endif
