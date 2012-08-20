/*
 * Puella Ardens
 *
 * Burning Man GirlTech based IM communicator.
 */


#include <cc1110.h>
#include <stdint.h>
#include <string.h>
#include "ioCCxx10_bitdef.h"
#include "display.h"
#include "keys.h"
#include "5x7.h"
#include "stdio.h"
#include "puellaardens.h"
#include "pm.h"
#include "radio.h"

/* globals */
__xdata MessageInfo msg_buffer[NUM_MESSAGES];
bit sleepy;
uint8_t cur_msg;
uint8_t first_msg;
uint8_t last_msg;

void init_test_messages() {
#define ADD_TEST_MSG(_i, _msg, _attr)     \
  strcpy(msg_buffer[_i].text, _msg); \
  msg_buffer[_i].attr = _attr;

  ADD_TEST_MSG(0, "MATT IS A DORK.  ALSO HE SMELLS FUNNY.", MSG_ATTR_SEEN);
  ADD_TEST_MSG(1, "DINNER SERVING IN 30 MINUTES, SLOP FOR ALL.", 0);
  ADD_TEST_MSG(2, "MASSIVE WHITEOUT COMING, TAKE COVER!", 0);
  ADD_TEST_MSG(3, "THIS MESSAGE IS LONG, TO TEST OUT LONG MESSAGES.  IT IS 3 LINES LONG.", 0);
  ADD_TEST_MSG(4, "SWEET LEITSHOW STARTING IN 22 MINUTES", 0);
  ADD_TEST_MSG(5, "WILD ELMO HAS APPEARED.", 0);
  ADD_TEST_MSG(6, "SNELLA CAME BY THE DOME AND ATE ALL THE FOOD", 0);
  ADD_TEST_MSG(7, "ANYONE WANT TO TIME TRAVEL?  LEAVING 5 MINUTES AGO", 0);
  ADD_TEST_MSG(8, "TOILETS ARE ALL FULL #POOPTROUBLES", 0);
  ADD_TEST_MSG(9, "HENRY HAS A STINKY BUTT", 0);
  
#undef ADD_TEST_MSG
  
  cur_msg = 0;
  first_msg = 0;
  last_msg = 9;
}

uint8_t draw_message(const MessageInfo* msg, uint8_t row) {
  uint8_t msg_len = strlen(msg->text);
  uint8_t msg_pos = 0;
  uint8_t col = 0;
  
  SSN = LOW;
  setDisplayStart(0);
  while (row < CHAR_HEIGHT && msg_pos < msg_len) {
    setCursor(row, 0);
    for (col = 0; col < CHAR_WIDTH && msg_pos < msg_len; ++col, ++msg_pos) {
      putchar(msg->text[msg_pos]);
    }
    ++row;
  }
  SSN = HIGH;
  return row;
}

void draw() {
  uint8_t row, msg;

  clear();

  /* TODO: This should be a circular buffer */
  row = 0;
  for (msg = cur_msg; msg <= last_msg && row < CHAR_HEIGHT; ++msg) {
    row = draw_message(&msg_buffer[msg], row);
  }
}

void print_message(const char* msg, int row, int col) {
  setDisplayStart(0);
  SSN = LOW;
  setCursor(row, col);
  printf(msg);
  SSN = HIGH;
}

void move_to_next_message() {
  if (cur_msg != last_msg) {
    cur_msg = (cur_msg + 1) % NUM_MESSAGES;
  }
}

void move_to_prev_message() {
  if (cur_msg != first_msg) {
    cur_msg = (cur_msg + NUM_MESSAGES - 1) % NUM_MESSAGES;
  }
}

void poll_keyboard() {
  switch (getkey()) {
    case KMNU:
      print_message("matt is a dork", 0, 0);
      break;
    case '^':
    case '<':
      move_to_prev_message();
      draw();
      break;
    case KDWN:
    case '>':
      move_to_next_message();
      draw();
      break;
    case KPWR:
      sleepy = 1;
      break;
    default:
      break;
  }
}

void main(void) {
  char buf[22];
  bit test_radio = 0;
  uint8_t wait_col = 55;
  uint8_t num_rcvd;
  
reset:
  sleepy = 0;

  /* Setup display. */
  xtalClock();
  setIOPorts();
  configureSPI();
  LCDReset();

  init_test_messages();

  if (test_radio) {
    /* Setup radio. */
    EA = 1;       // Enable interrupts.
    radio_init();

    clear();
    print_message("SENDING MSG", 0, 0);

    radio_send_packet("CORN MUFFIN");
    while (radio_still_sending()) {          
      sleepMillis(1000);
      print_message(".", 0, wait_col);
      wait_col += 5;
    }

    print_message("SENT! WAITING..", 1, 0);
  
    num_rcvd = radio_recv_packet_block(buf);
    buf[21]='\0';
    print_message(buf, 2, 0);
    
    setDisplayStart(0);
    SSN = LOW;
    setCursor(3, 0);
    printf("%d bytes recvd", num_rcvd);
    SSN = HIGH;
  } else {
    init_test_messages();
    draw();
  }

  while (1) {
    poll_keyboard();

    /* go to sleep (more or less a shutdown) if power button pressed */
    if (sleepy) {
      clear();
      sleepMillis(1000);
      SSN = LOW;
      LCDPowerSave();
      SSN = HIGH;
      sleep();
      /* reset on wake */
      goto reset;
    }
  }
}
