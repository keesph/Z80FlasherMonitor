#ifndef COMMAND_TYPES_H
#define COMMAND_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    eACK,
    eNAK
} Response_t;

typedef enum
{
    eSetMode,
    eSetPC,
    eSetMaster,
    eSetIncrement,
    eWrite,
    eRead,
    eWriteAt,
    eReadFrom
} Command_t;

typedef enum
{
    eInvalidCommand,
    eInvalidLength,
    eInvalidPC,
    eInvalidState
} NAK_Reason_t;

typedef struct setModeCommand
{
    Command_t command;
    uint8_t mode;
} SetModeCommand_t;

typedef struct setPCCommand
{
    Command_t command;
    uint16_t pc;
} SetPCCommand_t;

typedef struct setMaster
{
    Command_t command;
    bool master;
} SetMasterCommand_t;

typedef struct setIncrementCommand
{
    Command_t command;
    bool increment;

} SetIncrementCommand_t;

typedef struct writeCommand
{
    Command_t command;
    uint8_t length;
    uint8_t data[62];
} WriteCommand_t;

typedef struct readCommand
{
    Command_t command;
    uint8_t length;
} ReadCommand_t;

typedef struct writeAtCommand
{
    Command_t command;
    uint16_t pc;
    uint8_t length;
    uint8_t data[60];
} WriteAtCommand_t;

typedef struct ReadFromCommand
{
    Command_t command;
    uint16_t pc;
    uint8_t length;
} ReadFromCommand_t;


typedef struct ACKResponse
{
    Response_t response;
    uint8_t length;
    uint8_t data[62];
} ACKResponse_t;

typedef struct NAKResponse
{
    Response_t response;
    uint8_t reason;
} NAKResponse_t;


#endif