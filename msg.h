#ifndef _MSG_H_
#define _MSG_H_

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
  uint8_t in_count;
  uint8_t out_count;
} msg_state;

msg_state fabric_state;

char in_buffer[256];
char out_buffer[256];

void process_msg(char *msg);

void enqueue_msg(char *msg, int8_t direction, uint8_t hops) {

}

#endif
