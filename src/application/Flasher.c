#include "Flasher.h"
#include "BusState.h"

#include "main.h" //For LL and HAL access

typedef struct flConfigStruct {
  uint16_t pc;
  bool master;
  bool increment;
} FL_configStruct_t;

static FL_configStruct_t flasherState = {0};

static void FL_vWriteByteToBus(uint16_t address, uint8_t byte);
static uint8_t FL_uReadByteFromBus(uint16_t address);

/***************************************************************************************/
/***************************************************************************************/

void FL_vSetPC(SetPCCommand_t *command, SL_ResponsePrototype_t *response) {
  flasherState.pc = command->pc;

  ACKResponse_t *acknowledge = (ACKResponse_t *)response;
  acknowledge->response = eACK;
  acknowledge->length = 0;
}

/***************************************************************************************/
/***************************************************************************************/

void FL_vSetMaster(SetMasterCommand_t *command,
                   SL_ResponsePrototype_t *response) {
  flasherState.master = command->master;

  ACKResponse_t *acknowledge = (ACKResponse_t *)response;
  acknowledge->response = eACK;
  acknowledge->length = 0;
}

/***************************************************************************************/
/***************************************************************************************/

void FL_vSetIncrement(SetIncrementCommand_t *command,
                      SL_ResponsePrototype_t *response) {
  flasherState.increment = command->increment;

  ACKResponse_t *acknowledge = (ACKResponse_t *)response;
  acknowledge->response = eACK;
  acknowledge->length = 0;
}

/***************************************************************************************/
/***************************************************************************************/

void FL_vWrite(WriteCommand_t *command, SL_ResponsePrototype_t *response) {
  NAKResponse_t *NAK = (NAKResponse_t *)response;
  ACKResponse_t *ACK = (ACKResponse_t *)response;
  BS_TransitionGuards_t guards = {.master_guard = flasherState.master};
  bool result = true;

  // Set Bus State depending based on master flag indicating presence of other
  // bus master
  result = BS_setEvent(eSetWrite, guards);

  // Check if state change was successful
  if (result == false) {
    NAK->response = eNAK, NAK->reason = eInvalidState;
    return;
  }

  // Check if write goes beyond memory range
  if (((UINT16_MAX - command->length) < flasherState.pc) ||
      (command->length > sizeof(command->data))) {
    NAK->response = eNAK, NAK->reason = eInvalidLength;
    return;
  }

  // Copy data from command to the Bus
  uint8_t i;
  for (i = 0; i < command->length; i++) {
    FL_vWriteByteToBus(flasherState.pc + i, command->data[i]);
  }

  if (flasherState.increment) {
    flasherState.pc += command->length;
  }

  // Return bus control back to other master
  if (flasherState.master) {
    BS_setEvent(eSetSlave, guards);
  }

  ACK->response = eACK;
  ACK->length = i;
  return;
}

/***************************************************************************************/
/***************************************************************************************/

void FL_vRead(ReadCommand_t *command, SL_ResponsePrototype_t *response) {
  NAKResponse_t *NAK = (NAKResponse_t *)response;
  ACKResponse_t *ACK = (ACKResponse_t *)response;
  BS_TransitionGuards_t guards = {.master_guard = flasherState.master};
  bool result = true;

  // Set Bus State depending based on master flag indicating presence of other
  // bus master
  result = BS_setEvent(eSetRead, guards);

  // Check if state change was successful
  if (result == false) {
    NAK->response = eNAK, NAK->reason = eInvalidState;
    return;
  }

  // Check if read goes beyond memory range
  if (((UINT16_MAX - command->length) < flasherState.pc) ||
      (command->length > sizeof(ACK->data))) {
    NAK->response = eNAK, NAK->reason = eInvalidLength;
    return;
  }

  // Copy data from Bus to the response
  uint8_t i;
  for (i = 0; i < command->length; i++) {
    ACK->data[i] = FL_uReadByteFromBus(flasherState.pc + i);
  }

  if (flasherState.increment) {
    flasherState.pc += command->length;
  }

  // Return bus control back to other master
  if (flasherState.master) {
    BS_setEvent(eSetSlave, guards);
  }

  ACK->response = eACK;
  ACK->length = i;
  return;
}

/***************************************************************************************/
/***************************************************************************************/

void FL_vWriteAt(WriteAtCommand_t *command, SL_ResponsePrototype_t *response) {
  NAKResponse_t *NAK = (NAKResponse_t *)response;
  ACKResponse_t *ACK = (ACKResponse_t *)response;
  BS_TransitionGuards_t guards = {.master_guard = flasherState.master};
  bool result = true;

  // Set Bus State depending based on master flag indicating presence of other
  // bus master
  result = BS_setEvent(eSetWrite, guards);

  // Check if state change was successful
  if (result == false) {
    NAK->response = eNAK, NAK->reason = eInvalidState;
    return;
  }

  // Check if write goes beyond memory range
  if (((UINT16_MAX - command->length) < command->pc) ||
      (command->length > sizeof(command->data))) {
    NAK->response = eNAK, NAK->reason = eInvalidLength;
    return;
  }

  // Copy data from command to the Bus
  uint8_t i;
  for (i = 0; i < command->length; i++) {
    FL_vWriteByteToBus(command->pc + i, command->data[i]);
  }

  // Return bus control back to other master
  if (flasherState.master) {
    BS_setEvent(eSetSlave, guards);
  }

  ACK->response = eACK;
  ACK->length = i;
  return;
}

/***************************************************************************************/
/***************************************************************************************/

void FL_vReadFrom(ReadFromCommand_t *command,
                  SL_ResponsePrototype_t *response) {
  NAKResponse_t *NAK = (NAKResponse_t *)response;
  ACKResponse_t *ACK = (ACKResponse_t *)response;
  BS_TransitionGuards_t guards = {.master_guard = flasherState.master};
  bool result = true;

  // Set Bus State depending based on master flag indicating presence of other
  // bus master
  result = BS_setEvent(eSetRead, guards);

  // Check if state change was successful
  if (result == false) {
    NAK->response = eNAK, NAK->reason = eInvalidState;
    return;
  }

  // Check if read goes beyond memory range
  if (((UINT16_MAX - command->length) < command->pc) ||
      (command->length > sizeof(ACK->data))) {
    NAK->response = eNAK, NAK->reason = eInvalidLength;
    return;
  }

  // Copy data from Bus to the response
  uint8_t i;
  for (i = 0; i < command->length; i++) {
    ACK->data[i] = FL_uReadByteFromBus(command->pc + i);
  }

  // Return bus control back to other master
  if (flasherState.master) {
    BS_setEvent(eSetSlave, guards);
  }

  ACK->response = eACK;
  ACK->length = i;
  return;
}

/***************************************************************************************/
/***************************************************************************************/

static void FL_vWriteByteToBus(uint16_t address, uint8_t byte) {
  uint32_t addressPortValue = address & 0xFFFF;
  uint32_t dataPortValue;

  // Write address to bus
  LL_GPIO_WriteOutputPort(ADDRESS_PORT, addressPortValue);

  // Trigger Memory to latch address
  HAL_GPIO_WritePin(MREQ_GPIO_Port, MREQ_Pin, GPIO_PIN_RESET);

  // Set Data Bus
  dataPortValue = 0;
  dataPortValue |= (LL_GPIO_ReadOutputPort(DATA_PORT) &
                    0xFFFFFF00); // Store state, LSByte used for data bus
  dataPortValue |= byte;
  LL_GPIO_WriteOutputPort(DATA_PORT, dataPortValue);

  // Trigger Memory to latch Data Bus
  HAL_GPIO_WritePin(EXTWR_GPIO_Port, EXTWR_Pin, GPIO_PIN_RESET);
  HAL_Delay(1);

  // Trigger Memory to save the latched data
  HAL_GPIO_WritePin(EXTWR_GPIO_Port, EXTWR_Pin, GPIO_PIN_SET);

  // Finish Write cycle
  HAL_GPIO_WritePin(MREQ_GPIO_Port, MREQ_Pin, GPIO_PIN_SET);

  // Finish with additional delay
  HAL_Delay(10);
}

/***************************************************************************************/
/***************************************************************************************/

static uint8_t FL_uReadByteFromBus(uint16_t address) {
  uint32_t addressPortValue = 0;

  addressPortValue = address & 0xFFFF;

  // Bring Memory lines to base state for read
  HAL_GPIO_WritePin(RD_GPIO_Port, RD_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MREQ_GPIO_Port, MREQ_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(EXTWR_GPIO_Port, EXTWR_Pin, GPIO_PIN_SET);

  // Write address to bus
  LL_GPIO_WriteOutputPort(ADDRESS_PORT, addressPortValue);
  HAL_Delay(1);

  // Trigger memory to read address
  HAL_GPIO_WritePin(MREQ_GPIO_Port, MREQ_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RD_GPIO_Port, RD_Pin, GPIO_PIN_RESET);
  HAL_Delay(1);

  // Read Data Pins
  uint8_t data =
      (LL_GPIO_ReadInputPort(DATA_PORT) & 0xFF); // Read only the lowest byte

  // Finish Read cycle
  HAL_GPIO_WritePin(MREQ_GPIO_Port, MREQ_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RD_GPIO_Port, RD_Pin, GPIO_PIN_SET);

  HAL_Delay(1);
  return data;
}

/***************************************************************************************/
/***************************************************************************************/
