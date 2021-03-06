#ifndef SIMPLEAT_H
#define SIMPLEAT_H
#include <stdint.h>

#define AT_MAX_NUMBER_OF_ARGS 40 /*In bytes*/
#define AT_MAX_SIZE_STRING 20
#define CLIENT_FUNCTION_TYPE uint8_t
#define ECHO_MODE_ON 1
#define VERBOSE_MODE_ON 0

#define AT_NO_ARGS {0}
#define AT_TYPE_STRING AT_MAX_SIZE_STRING
#define AT_ARGS(...) {__VA_ARGS__, -1}
#define AT_TYPE(x) ((uint8_t) sizeof (x))
#define AT_COMMAND(name, args, client) {(char*)#name, 0, args , 0, client}

#define ATReplyByteArray(x) ((uint8_t *) &x), sizeof(x)

typedef struct {
    char *command;
    uint8_t sizeOfCommand;
    int8_t argsSize[AT_MAX_NUMBER_OF_ARGS];
    uint8_t numberOfArgs;
    void (*client)(const uint8_t*);
} ATCommandDescriptor;

void ATEngineDriverInit(uint8_t (*open)(void),
                        uint8_t (*read)(void),
                        void (*write)(uint8_t),
                        uint8_t (*available)(void));

void ATEngineInit(ATCommandDescriptor *engine,  uint8_t sizeOfEngine);
uint8_t ATEnginePollingHandle();
void ATEngineInterruptHandle(uint8_t data);
void ATReplyWithByteArray(uint8_t *data, int size);
void ATReplyWithByte(uint8_t data);
void ATReplyWithString(char *str);
void ATReplyWithChar(char c);



#endif // SIMPLEAT_H
