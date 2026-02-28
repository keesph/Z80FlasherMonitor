#include "FlasherMonitor.h"
#include "CommandTypes.h"
#include "Flasher.h"
#include "SerialLink.h"

#include "main.h"
#include <string.h>

// Typedefs
typedef enum { eFlasher, eMonitor, eInitial } FM_Mode_t;

// Static Fields
static FM_Mode_t FM_mode = eInitial;
static SL_CommandPrototype_t command = {0};
static SL_ResponsePrototype_t response = {0};

// Static Functions
static void FM_vSetMode(SetModeCommand_t *command,
                        SL_ResponsePrototype_t *response);
static void FM_vProcessCommand(SL_CommandPrototype_t *,
                               SL_ResponsePrototype_t *);

void FM_vFlasherMonitor_main(void) {
#ifdef SIMULATE_USB

  HAL_GPIO_WritePin(PRGRM1_GPIO_Port, PRGRM1_Pin, GPIO_PIN_SET);

  SetModeCommand_t modeCommand;
  SetMasterCommand_t masterCommand;
  SetIncrementCommand_t incrementCommand;
  SetPCCommand_t pcCommand;
  WriteCommand_t writeCommand;
  ReadCommand_t readCommand;
  WriteAtCommand_t writeAtCommand;
  ReadFromCommand_t readAtCommand;

  modeCommand.command = eSetMode;
  modeCommand.mode = eFlasher;

  masterCommand.command = eSetMaster;
  masterCommand.master = false;

  incrementCommand.command = eSetIncrement;
  incrementCommand.increment = true;

  writeCommand.command = eWrite;
  writeCommand.length = 1;

  readCommand.command = eRead;
  readCommand.length = 1;

  pcCommand.command = eSetPC;
  pcCommand.pc = 0;

  uint8_t writeData[] = {0x21, 0x00, 0x80, // LD HL 0x8000
                         0x36, 0x00,       // LD (HL) 0x00
                         0x7E,             // LD A, (HL)
                         0x3C,             // INC A
                         0x77,             // LD (HL), A
                         0x18, 0xFB};      // JR -7;
  writeAtCommand.command = eWriteAt;
  memcpy(&writeAtCommand.data, &writeData, sizeof(writeData));
  writeAtCommand.length = sizeof(writeData);
  writeAtCommand.pc = 0;

  uint8_t readData[sizeof(writeData)] = {0};
  readAtCommand.command = eReadFrom;
  readAtCommand.length = 20;
  readAtCommand.pc = 0;

  FM_vProcessCommand((SL_CommandPrototype_t *)&modeCommand, &response);
  FM_vProcessCommand((SL_CommandPrototype_t *)&masterCommand, &response);
  FM_vProcessCommand((SL_CommandPrototype_t *)&writeAtCommand, &response);
  FM_vProcessCommand((SL_CommandPrototype_t *)&readAtCommand, &response);

  FM_vProcessCommand((SL_CommandPrototype_t *)&incrementCommand, &response);

  // for (int i = 0; i <= 1000; i++) {
  //   writeCommand.data[0] = i % 256;
  //   FM_vProcessCommand((SL_CommandPrototype_t *)&writeCommand, &response);
  // }

  // FM_vProcessCommand((SL_CommandPrototype_t *)&pcCommand, &response);
  // for (int i = 0; i <= 1000; i++) {
  //   FM_vProcessCommand((SL_CommandPrototype_t *)&readCommand, &response);
  //   if (response.response == eNAK || response.payload[1] != (i % 256)) {
  //     HAL_GPIO_WritePin(PRGRM1_GPIO_Port, PRGRM1_Pin, GPIO_PIN_SET);
  //     break;
  //   } else {
  //     HAL_GPIO_WritePin(PRGRM1_GPIO_Port, PRGRM1_Pin, GPIO_PIN_RESET);
  //   }
  // }

#endif
  while (1) {

    if (SL_bNextCommand(&command)) {
      FM_vProcessCommand(&command, &response);
      SL_vSendResponse(&response);
      memset((uint8_t *)&response, 0, sizeof(SL_ResponsePrototype_t));
    }

    // TODO: Check status flags of Monitor
  }
}

static void FM_vSetMode(SetModeCommand_t *command,
                        SL_ResponsePrototype_t *response) {
  switch (command->mode) {
  case eFlasher: {
    // TODO: Implement FL_SetActive()
    FM_mode = command->mode;
    ACKResponse_t *ACK_Response = (ACKResponse_t *)response;
    ACK_Response->response = eACK;
    ACK_Response->length = 0;
  } break;

  case eMonitor: {
    // TODO: Implement MN_SetActive()
    FM_mode = command->mode;
    ACKResponse_t *ACK_Response = (ACKResponse_t *)response;
    ACK_Response->response = eACK;
    ACK_Response->length = 0;
  } break;

  default: {
    NAKResponse_t *NAK_Response = (NAKResponse_t *)response;
    NAK_Response->response = eNAK;
    NAK_Response->reason = eInvalidState;
  } break;
  }
}

void FM_vProcessCommand(SL_CommandPrototype_t *command,
                        SL_ResponsePrototype_t *response) {
  switch (command->command) {
  case eSetMode:
    FM_vSetMode((SetModeCommand_t *)command, response);
    break;

  case eSetPC:
    FL_vSetPC((SetPCCommand_t *)command, response);
    break;

  case eSetMaster:
    FL_vSetMaster((SetMasterCommand_t *)command, response);
    break;

  case eSetIncrement:
    FL_vSetIncrement((SetIncrementCommand_t *)command, response);
    break;

  case eWrite:
    FL_vWrite((WriteCommand_t *)command, response);
    break;

  case eRead:
    FL_vRead((ReadCommand_t *)command, response);
    break;
  case eWriteAt:
    FL_vWriteAt((WriteAtCommand_t *)command, response);
    break;

  case eReadFrom:
    FL_vReadFrom((ReadFromCommand_t *)command, response);
    break;

  default: {
    NAKResponse_t *NAK_Response = (NAKResponse_t *)response;
    NAK_Response->response = eNAK;
    NAK_Response->reason = eInvalidCommand;
  } break;
  }
}