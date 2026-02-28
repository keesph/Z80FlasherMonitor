#include "SerialLink.h"

// USB Device API
#include "usbd_cdc_if.h"

#define PACKET_LENGTH 64

static uint8_t messageBuffer[PACKET_LENGTH] = {0};
static volatile uint8_t receivedLength = 0;
static volatile bool receiveComplete = false;

/***************************************************************************************/
/***************************************************************************************/

bool SL_bNextCommand(SL_CommandPrototype_t *command) {
  if (receiveComplete == false) {
    return false;
  }

  memset(command, 0, sizeof(SL_CommandPrototype_t));
  memcpy(command, messageBuffer, PACKET_LENGTH);

  receivedLength = 0;
  receiveComplete = false;
  memset(messageBuffer, 0, sizeof(messageBuffer));

  return true;
}

/***************************************************************************************/
/***************************************************************************************/

void SL_vSendResponse(SL_ResponsePrototype_t *response) {
  // Send using generated USB VCP API
  CDC_Transmit_FS((uint8_t *)response, sizeof(SL_ResponsePrototype_t));
}

/***************************************************************************************/
/***************************************************************************************/

void SL_USB_Rx_Cb(uint8_t *buffer, uint32_t len) {
  if (receiveComplete == true) {
    // We don't expect data at the moment. Do nothing
    return;
  }

  memcpy(&messageBuffer[receivedLength], buffer, len);
  receivedLength += len;

  if (receivedLength >= PACKET_LENGTH) {
    receiveComplete = true;
  }
  return;
}