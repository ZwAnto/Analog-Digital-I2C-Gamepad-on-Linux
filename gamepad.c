#include "string.h"
#include "pthread.h"
#include "stdio.h"
#include "errno.h"
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdbool.h>


uint8_t readBuffer[2];
uint8_t writeBuffer[3];


uint8_t MCP23017readBuffer[2];
uint8_t MCP23017writeBuffer[2];
uint16_t previousReadBuffer;
uint16_t state;

// specify addresses for expanders
#define MCP23017_ADDRESS 0x20

#define I2C_BUS "/dev/i2c-11" //specify which I2C bus to use

// stuff for the GPIO expander
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14

#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

int MCP23017open() {
  // open the i2c device
  int file;
  char * filename = I2C_BUS; //specify which I2C bus to use
  if ((file = open(filename, O_RDWR)) < 0) {
    printf("Failed to open I2C bus %s. Enable it using sudo raspi-config.\n", I2C_BUS);
    exit(1);
  }
  // initialize the device
  if (ioctl(file, I2C_SLAVE, MCP23017_ADDRESS) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    exit(1);
  }
  return file;
}

void MCP23017writeConfig(int I2C) {
  // set the pointer to the config register

  MCP23017writeBuffer[0] = MCP23017_IODIRA;
  MCP23017writeBuffer[1] = 0xF0;
  write(I2C, MCP23017writeBuffer, 2);
  MCP23017writeBuffer[0] = MCP23017_OLATA;
  MCP23017writeBuffer[1] = 0x0;

  //  MCP23017writeBuffer[0] = MCP23017_IODIRB; // GPIO direction register
  //  MCP23017writeBuffer[1] = 0xFF; // Set GPIO B to input
  //  write(I2C, MCP23017writeBuffer, 2);
  //  MCP23017writeBuffer[0] = MCP23017_GPPUA; // GPIO Pullup Register
  //  MCP23017writeBuffer[1] = 0xFF; // Enable Pullup on GPIO A
  //  write(I2C, MCP23017writeBuffer, 2);
  //  MCP23017writeBuffer[0] = MCP23017_GPPUB; // GPIO Pullup Register
  //  MCP23017writeBuffer[1] = 0xFF; // Enable Pullup on GPIO B

  if (write(I2C, MCP23017writeBuffer, 2) != 2) {
    printf("MCP23017 was not detected at address 0x%X. Check wiring and try again.\n", MCP23017_ADDRESS);
    exit(1);
  }
}

void MCP23017read(int I2C) {

  int col;
  state = 0;

  for( col=0; col < 4; col++ ) {

    MCP23017writeBuffer[0] = MCP23017_OLATA;
    MCP23017writeBuffer[1] = 1 << col;
    write(I2C, MCP23017writeBuffer, 2);

    MCP23017writeBuffer[0] = MCP23017_GPIOA;
    write(I2C, MCP23017writeBuffer, 1);
    read(I2C, MCP23017readBuffer, 1);

    state = state | ((MCP23017readBuffer[0] >> 4) << col*4);
  }

//  MCP23017writeBuffer[0] = MCP23017_GPIOA;
//  write(I2C, MCP23017writeBuffer, 1); // prepare to read ports A and B
//  //reading two bytes causes it to autoincrement to the next byte, so it reads port B
//  if (read(I2C, MCP23017readBuffer, 2) != 2) {
//    printf("Unable to communicate with the MCP23017\n");
//    MCP23017readBuffer[0] = 0xFF;
//    MCP23017readBuffer[1] = 0xFF;
//    sleep(1);
//  }
}


int createUInputDevice() {
  int fd;
  fd = open("/dev/uinput", O_WRONLY | O_NDELAY);
  if (fd < 0) {
    fprintf(stderr, "Unable to create gamepad with uinput. Try running as sudo.\n");
    exit(1);
  }
  // device structure
  struct uinput_user_dev uidev;
  memset( & uidev, 0, sizeof(uidev));
  // init event
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_EVBIT, EV_REL);
  // button
  ioctl(fd, UI_SET_KEYBIT, BTN_A);
  ioctl(fd, UI_SET_KEYBIT, BTN_B);
  ioctl(fd, UI_SET_KEYBIT, BTN_X);
  ioctl(fd, UI_SET_KEYBIT, BTN_Y);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
  ioctl(fd, UI_SET_KEYBIT, BTN_TL);
  ioctl(fd, UI_SET_KEYBIT, BTN_TR);
  ioctl(fd, UI_SET_KEYBIT, BTN_START);
  ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);
  ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY13);
  ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY14);
  ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY15);
  ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY16);

  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "othermod Gamepad");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 1;
  uidev.id.product = 5;
  uidev.id.version = 1;
  write(fd, & uidev, sizeof(uidev));
  if (ioctl(fd, UI_DEV_CREATE)) {
    fprintf(stderr, "Error while creating uinput device!\n");
    exit(1);
  }
  return fd;
}

void emit(int virtualGamepad, int type, int code, int val) {
  struct input_event ie;
  ie.type = type;
  ie.code = code;
  ie.value = val;
  /* timestamp values below are ignored */
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;
  write(virtualGamepad, & ie, sizeof(ie));
}

void updateButtons(int virtualGamepad, int buttons) {
  // update button event
  emit(virtualGamepad, EV_KEY, BTN_A, ((buttons >> 0x00) & 1));
  emit(virtualGamepad, EV_KEY, BTN_B, ((buttons >> 0x01) & 1));
  emit(virtualGamepad, EV_KEY, BTN_X, ((buttons >> 0x02) & 1));
  emit(virtualGamepad, EV_KEY, BTN_Y, ((buttons >> 0x03) & 1));
  emit(virtualGamepad, EV_KEY, BTN_DPAD_UP, ((buttons >> 0x04) & 1));
  emit(virtualGamepad, EV_KEY, BTN_DPAD_DOWN, ((buttons >> 0x05) & 1));
  emit(virtualGamepad, EV_KEY, BTN_DPAD_LEFT, ((buttons >> 0x06) & 1));
  emit(virtualGamepad, EV_KEY, BTN_DPAD_RIGHT, ((buttons >> 0x07) & 1));
  emit(virtualGamepad, EV_KEY, BTN_TL, ((buttons >> 0x08) & 1));
  emit(virtualGamepad, EV_KEY, BTN_TR, ((buttons >> 0x09) & 1));
  emit(virtualGamepad, EV_KEY, BTN_START, ((buttons >> 0x0A) & 1));
  emit(virtualGamepad, EV_KEY, BTN_SELECT, ((buttons >> 0x0B) & 1));
  emit(virtualGamepad, EV_KEY, BTN_TRIGGER_HAPPY13, ((buttons >> 0x0C) & 1));
  emit(virtualGamepad, EV_KEY, BTN_TRIGGER_HAPPY14, ((buttons >> 0x0D) & 1));
  emit(virtualGamepad, EV_KEY, BTN_TRIGGER_HAPPY15, ((buttons >> 0x0E) & 1));
  emit(virtualGamepad, EV_KEY, BTN_TRIGGER_HAPPY16, ((buttons >> 0x0F) & 1));
}


int main(int argc, char * argv[]) {

  int virtualGamepad = createUInputDevice(); // create uinput device

  int mcpFile = MCP23017open(); // open Expander device

  MCP23017writeConfig(mcpFile);
  //set initial button condition
  MCP23017read(mcpFile);
  uint16_t tempReadBuffer = 0x00;
  updateButtons(virtualGamepad, tempReadBuffer);
  bool reportUinput = 0;

  while (1) {

    MCP23017read(mcpFile); //read the expander
    //tempReadBuffer = (MCP23017readBuffer[0] << 8) | (MCP23017readBuffer[1] & 0xff);

    if (state != previousReadBuffer) {
      updateButtons(virtualGamepad, state);
      reportUinput = 1;
    } //only update the buttons when something changes from the last loop
    previousReadBuffer = state;

    if (reportUinput) {
      emit(virtualGamepad, EV_SYN, SYN_REPORT, 0);
      reportUinput = 0;
    }

    usleep(33333); // sleep for about 1/30th of a second. Also gives the ADC enough time to prepare the next reading
  }
  return 0;
}
