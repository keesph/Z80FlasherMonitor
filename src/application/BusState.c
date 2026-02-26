#include "BusState.h"
#include "main.h" //GPIO Definitions and HAL/LL Includes
#include "stm32f4xx_hal_gpio.h"

#define A_B GPIO_PIN_SET
#define B_A GPIO_PIN_RESET
#define DATA_PORT GPIOA
#define CTRL_PORT GPIOC

typedef enum { eIN, eOUT } BS_Direction_t;

typedef struct BS_configuration {
  BS_State_t state;
  BS_Direction_t controlDirection;
  BS_Direction_t dataDirection;
  bool busAquired;
} BS_Configuration_t;

static BS_Configuration_t BS_config = {.controlDirection = eIN,
                                       .dataDirection = eIN,
                                       .state = eSlave,
                                       .busAquired = false};

// Event Handler for each state
static bool BS_State_Slave_HandleEvent(BS_Event_t event,
                                       BS_TransitionGuards_t guards);
static bool BS_State_WriteDirect_HandleEvent(BS_Event_t event,
                                             BS_TransitionGuards_t guards);
static bool BS_State_ReadDirect_HandleEvent(BS_Event_t event,
                                            BS_TransitionGuards_t guards);
static bool BS_State_WriteMaster_HandleEvent(BS_Event_t event,
                                             BS_TransitionGuards_t guards);
static bool BS_State_ReadMaster_HandleEvent(BS_Event_t event,
                                            BS_TransitionGuards_t guards);

// Control functions for the buses
static void BS_AquireBus(void);
static void BS_ReleaseBus(void);
static void BS_SetDataBusDirection(BS_Direction_t direction);
static void BS_SetControlBusDirection(BS_Direction_t direction);

/// @brief Return current state of Bus State Module
/// @return BS_State_t Current state
BS_State_t BS_getState(void) { return BS_config.state; }

/// @brief Set a new event to be handled by the Bus State FSM
/// @param event The new event input
/// @param guards Transition guards for state transitions
/// @return
bool BS_setEvent(BS_Event_t event, BS_TransitionGuards_t guards) {
  bool result = false;

  switch (BS_config.state) {
  case eSlave:
    result = BS_State_Slave_HandleEvent(event, guards);
    break;

  case eReadDirect:
    result = BS_State_ReadDirect_HandleEvent(event, guards);
    break;

  case eWriteDirect:
    result = BS_State_WriteDirect_HandleEvent(event, guards);
    break;

  case eReadMaster:
    result = BS_State_ReadMaster_HandleEvent(event, guards);
    break;

  case eWriteMaster:
    result = BS_State_WriteMaster_HandleEvent(event, guards);
    break;

  default:
    result = false;
    break;
  }

  return result;
}

/**
 * @brief Event handler for eSlave state
 *
 * @param event Input event to handle
 * @param guards Transition guards for state transitions
 * @return true if event was valid
 * @return false if event was invalid
 */
static bool BS_State_Slave_HandleEvent(BS_Event_t event,
                                       BS_TransitionGuards_t guards) {
  bool result = true;
  switch (event) {
  case eSetSlave:
    // Do nothing
    result = true;
    break;

  case eSetRead:
    if (guards.master_guard == false) {
      BS_SetControlBusDirection(eOUT);
      BS_config.state = eReadDirect;
    } else {
      BS_AquireBus();
      BS_SetControlBusDirection(eOUT);
      BS_config.state = eReadMaster;
    }
    break;

  case eSetWrite:
    if (guards.master_guard == false) {
      BS_SetControlBusDirection(eOUT);
      BS_SetDataBusDirection(eOUT);
      BS_config.state = eWriteDirect;
    } else {
      BS_AquireBus();
      BS_SetControlBusDirection(eOUT);
      BS_SetDataBusDirection(eOUT);
      BS_config.state = eWriteMaster;
    }
    break;

  default:
    result = false;
    break;
  }
  return result;
}

/**
 * @brief Event handler for eWriteDirect state
 *
 * @param event Input event to handle
 * @param guards Transition guards for state transitions
 * @return true if event was valid
 * @return false if event was invalid
 */
static bool BS_State_WriteDirect_HandleEvent(BS_Event_t event,
                                             BS_TransitionGuards_t guards) {
  (void)guards; // Unused
  bool result = true;
  switch (event) {
  case eSetSlave:
    BS_SetControlBusDirection(eIN);
    BS_SetDataBusDirection(eIN);
    BS_config.state = eSlave;
    break;

  case eSetRead:
    BS_SetDataBusDirection(eIN);
    BS_config.state = eReadDirect;
    break;

  case eSetWrite:
    // Do Nothing
    result = true;
    break;

  default:
    result = false;
    break;
  }
  return result;
}

/**
 * @brief Event handler for eReadDirect state
 *
 * @param event Input event to handle
 * @param guards Transition guards for state transitions
 * @return true if event was valid
 * @return false if event was invalid
 */
static bool BS_State_ReadDirect_HandleEvent(BS_Event_t event,
                                            BS_TransitionGuards_t guards) {
  (void)guards; // Unused
  bool result = false;
  switch (event) {
  case eSetSlave:
    BS_SetControlBusDirection(eIN);
    BS_config.state = eSlave;
    break;

  case eSetRead:
    // Do Nothing
    result = true;
    break;

  case eSetWrite:
    BS_SetDataBusDirection(eOUT);
    BS_config.state = eWriteDirect;
    break;

  default:
    result = false;
    break;
  }
  return result;
}

/**
 * @brief Event handler for eWriteMaster state
 *
 * @param event Input event to handle
 * @param guards Transition guards for state transitions
 * @return true if event was valid
 * @return false if event was invalid
 */
static bool BS_State_WriteMaster_HandleEvent(BS_Event_t event,
                                             BS_TransitionGuards_t guards) {
  (void)guards; // Unused
  bool result = false;
  switch (event) {
  case eSetSlave:
    BS_SetControlBusDirection(eIN);
    BS_SetDataBusDirection(eIN);
    BS_ReleaseBus();
    BS_config.state = eSlave;
    break;

  case eSetRead:
    BS_SetDataBusDirection(eIN);
    BS_config.state = eReadMaster;
    break;

  case eSetWrite:
    // Do Nothing
    result = true;
    break;

  default:
    result = false;
    break;
  }
  return result;
}

/**
 * @brief Event handler for eReadMaster state
 *
 * @param event Input event to handle
 * @param guards Transition guards for state transitions
 * @return true if event was valid
 * @return false if event was invalid
 */
static bool BS_State_ReadMaster_HandleEvent(BS_Event_t event,
                                            BS_TransitionGuards_t guards) {
  (void)guards; // Unused
  bool result = false;
  switch (event) {
  case eSetSlave:
    BS_SetControlBusDirection(eIN);
    BS_ReleaseBus();
    BS_config.state = eSlave;
    break;

  case eSetRead:
    // Do nothing
    result = true;
    break;

  case eSetWrite:
    BS_SetControlBusDirection(eOUT);
    BS_SetDataBusDirection(eOUT);
    BS_config.state = eWriteMaster;
    break;

  default:
    result = false;
    break;
  }
  return result;
}

/**
 * @brief Requests Bus control from the Z80 Bus Master and waits for the BUSACK
 * signal from it
 *
 */
static void BS_AquireBus(void) {
  if (BS_config.busAquired) {
    // Nothing to do
    return;
  }

  // Request Bus Control
  HAL_GPIO_WritePin(BUSRQ_GPIO_Port, BUSRQ_Pin, GPIO_PIN_RESET);

  // Wait for transfer of control
  while (HAL_GPIO_ReadPin(BUSACK_GPIO_Port, BUSACK_Pin) == GPIO_PIN_SET) {
    HAL_Delay(1);
  }

  BS_config.busAquired = true;
  return;
}

/**
 * @brief Returns control back to the Z80
 *
 */
static void BS_ReleaseBus(void) {
  if (BS_config.busAquired == false) {
    // Nothing to do
    return;
  }

  HAL_GPIO_WritePin(BUSRQ_GPIO_Port, BUSRQ_Pin, GPIO_PIN_SET);

  BS_config.busAquired = false;
}

/**
 * @brief Sets the direction of MCU and Level Shifter Data Bus pins in correct
 * sequence to prevent contention
 *
 * @param direction Direction to switch the bus to
 */
static void BS_SetDataBusDirection(BS_Direction_t direction) {
  if (BS_config.dataDirection == direction) {
    // Nothing to do
    return;
  }

  if (direction == eIN) {

    // First MCU to Input
    LL_GPIO_SetPinMode(DATA_PORT, DATA0_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA1_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA2_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA3_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA4_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA5_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA6_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA7_Pin, LL_GPIO_MODE_INPUT);

    // Then switch Level Shifter to B -> A
    HAL_GPIO_WritePin(DATADirection_GPIO_Port, DATADirection_Pin, B_A);
    BS_config.dataDirection = eIN;

    HAL_Delay(1);
  } else // eOut
  {
    // First Level Shifter to A -> B
    HAL_GPIO_WritePin(DATADirection_GPIO_Port, DATADirection_Pin, A_B);

    HAL_Delay(1);

    // Then MCU to Output
    LL_GPIO_SetPinMode(DATA_PORT, DATA0_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA1_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA2_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA3_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA4_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA5_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA6_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DATA_PORT, DATA7_Pin, LL_GPIO_MODE_OUTPUT);
    BS_config.dataDirection = eOUT;

    HAL_Delay(1);
  }
}

/**
 * @brief Sets the direction for the of MCU and Level Shifter Address/CTRL Bus
 * pins in correct sequence to prevent contention
 *
 * @param direction Direction to switch the bus to
 */
static void BS_SetControlBusDirection(BS_Direction_t direction) {
  if (BS_config.controlDirection == direction) {
    // Nothing to do
    return;
  }

  if (direction == eIN) {
    // First MCU to Input
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR0_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR1_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR2_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR3_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR4_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR5_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR6_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR7_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR8_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR9_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR10_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR11_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR12_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR13_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR14_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR15_Pin, LL_GPIO_MODE_INPUT);

    LL_GPIO_SetPinMode(RD_GPIO_Port, RD_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(WR_GPIO_Port, WR_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(IOREQ_GPIO_Port, IOREQ_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(MREQ_GPIO_Port, MREQ_Pin, LL_GPIO_MODE_INPUT);

    // Then switch Level Shifter to B -> A
    HAL_GPIO_WritePin(CTRLDirection_GPIO_Port, CTRLDirection_Pin, B_A);

    HAL_Delay(1);
  } else // eOUT
  {
    // First switch Level Shifter to A -> B
    HAL_GPIO_WritePin(CTRLDirection_GPIO_Port, CTRLDirection_Pin, A_B);

    HAL_Delay(1);

    // Then MCU to Output. Preload Control Lines with safe values
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR0_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR1_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR2_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR3_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR4_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR5_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR6_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR7_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR8_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR9_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR10_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR11_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR12_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR13_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR14_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(CTRL_PORT, ADDR15_Pin, LL_GPIO_MODE_OUTPUT);

    HAL_GPIO_WritePin(RD_GPIO_Port, RD_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(WR_GPIO_Port, WR_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IOREQ_GPIO_Port, IOREQ_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MREQ_GPIO_Port, MREQ_Pin, GPIO_PIN_SET);

    LL_GPIO_SetPinMode(RD_GPIO_Port, RD_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(WR_GPIO_Port, WR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(IOREQ_GPIO_Port, IOREQ_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(MREQ_GPIO_Port, MREQ_Pin, LL_GPIO_MODE_OUTPUT);

    HAL_Delay(1);
  }
}