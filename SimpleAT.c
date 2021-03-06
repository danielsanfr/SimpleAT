#include "SimpleAT.h"
static ATCommandDescriptor *__engine;
static uint8_t __sizeOfEngine;

#if VERBOSE_MODE_ON
#include<stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif


#if ECHO_MODE_ON
#define SHOW_COMMAND()\
    for(int i = 0; i < cmdIndex; i++) {\
    __write(cmd[i]);\
    }
#else
#define SHOW_COMMAND()
#endif

/*Driver functions ---------------------*/
static uint8_t (*__open)(void);
static uint8_t (*__read)(void);
static void (*__write)(uint8_t);
static uint8_t (*__available)(void);
/*--------------------------------------*/


uint8_t ATConverterASCIIToUint8(uint8_t character) {
    switch (character) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a':
    case 'A': return 0xA;
    case 'b':
    case 'B': return 0xB;
    case 'c':
    case 'C': return 0xC;
    case 'd':
    case 'D': return 0xD;
    case 'e':
    case 'E': return 0xE;
    case 'f':
    case 'F': return 0xF;
    default: return 0x00;
    }
}



uint8_t ATConvertHexToAscii(uint8_t character) {
    switch (character) {
    case 0: return '0';
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    case 8: return '8';
    case 9: return '9';
    case 0xA: return 'A';
    case 0xB: return 'B';
    case 0xC: return 'C';
    case 0xD: return 'D';
    case 0xE: return 'E';
    case 0xF: return 'F';
    default: return 0;
    }
}


uint8_t ATCheckIsDigit(uint8_t character) {
    switch (character) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'A':
    case 'b':
    case 'B':
    case 'c':
    case 'C':
    case 'd':
    case 'D':
    case 'e':
    case 'E':
    case 'f':
    case 'F': return 1;
    default: return 0;
    }
}

#define ERROR() \
    SHOW_COMMAND()\
    __write('\n');\
    __write('\n');\
    __write('E');\
    __write('R');\
    __write('R');\
    __write('O');\
    __write('R');\
    __write('\n');


#define OK() \
    SHOW_COMMAND()\
    if(currentCommand >= 0)\
    (*__engine[currentCommand].client)(params);\
    __write('\n');\
    __write('\n');\
    __write('O');\
    __write('K');\
    __write('\n');

void __stateMachineDigest(uint8_t current) {
    static uint8_t state;

    static int8_t currentCommand = -1;
    static uint8_t currentCommandIndex = 0;

    static uint8_t params[AT_MAX_NUMBER_OF_ARGS] = {0};
    static uint8_t currentParam;
    static uint8_t currentParamCount = 0;
    static uint8_t currentParamIndex;

    static uint8_t endOfString = 0;


#if ECHO_MODE_ON
    static uint8_t cmd[50] = {0};
    static uint8_t cmdIndex;
    if(state == 0)
        cmdIndex = 0;
    cmd[cmdIndex] = current;
    cmdIndex++;
#endif

    switch(state) {
    case 0:
        LOG("State 0\n");
        currentCommand = -1;
        currentCommandIndex = 0;
        for(int i = 0; i < AT_MAX_NUMBER_OF_ARGS; ++i) params[i] = 0; /* Cleanning up params variables*/
        if(current == 'A')
            state = 1;
        break;
    case 1:
        LOG("State 1\n");
        if(current == 'T')
            state = 2;
        else if(current == '\n') {
            state = 0;
            ERROR();
        } else
            state = 255; //error
        break;
    case 2:
        LOG("State 2\n");
        if(current == '+'){
            state = 3;
        } else if(current == '\n') {
            OK();
            state = 0;
        } else
            state = 255; //error
        break;
    case 3:
        LOG("State 3\n");
        if(current == '\n') {
            state = 0;
            ERROR();
        } else {
            for(uint8_t i = 0; i < __sizeOfEngine; ++i) {
                if(__engine[i].command[currentCommandIndex] == current) {
                    currentCommand = (int8_t) i;
                    currentCommandIndex++;
                    state = 4;
                    return;
                }
            }
            state = 255; // error No command found;
        }
        break;
    case 4: {
        LOG("State 4\n");
        if(current == '\n') {
            if(currentCommandIndex == __engine[currentCommand].sizeOfCommand) {
                if(__engine[currentCommand].numberOfArgs == 0) {
                    OK();
                    state = 0;
                } else {
                    state = 0;
                    ERROR();
                }
            } else {
                state = 0;
                ERROR();
            }
        } else if((currentCommandIndex < __engine[currentCommand].sizeOfCommand) &&
                  (__engine[currentCommand].command[currentCommandIndex] == current)) {
            currentCommandIndex++;
        } else if(currentCommandIndex == __engine[currentCommand].sizeOfCommand) {
            if(current == '=' && __engine[currentCommand].numberOfArgs > 0){
                state = 5;
                currentParam = 0;
                currentParamCount = 0;
                currentParamIndex = 0;
            } else {
                state = 255;
            }
        } else {
            state = 255;
        }
        break;
    }
    case 5: {
        LOG("State 5\n"); // get paramenters
        uint8_t sizeInBytes = (uint8_t)(__engine[currentCommand].argsSize[currentParam]<<1);
        //uint8_t sizeOfParameter = (uint8_t)__engine[currentCommand].argsSize[currentParam];
        LOG("currentParam %d, sizeInBytes %d Current %d\n",
            currentParam, sizeInBytes, current);
        if(current == '\n') {
            if(currentParamIndex == sizeInBytes && (__engine[currentCommand].numberOfArgs == currentParam + 1)) {
                OK();
                state = 0;
            } else {
                state = 0;
                ERROR();
            }
        } else if(current == '"') {
            if(currentParamIndex == 0) {
                //begin of string skip
                endOfString = 0;
                state = 6;
            } else {
                state = 255;
            }
        }else if(ATCheckIsDigit(current) && currentParamIndex < sizeInBytes) {
            params[currentParamCount] |= ATConverterASCIIToUint8(current) << (4 * (1 - (currentParamIndex % 2)));
            currentParamCount += currentParamIndex % 2;
            currentParamIndex++;
            LOG("Param %d, value %d, at %d\n", params[currentParamCount], ATConverterASCIIToUint8(current) << (4 * (1 - (currentParamIndex % 2))), currentParamCount);
        } else if(currentParamIndex == sizeInBytes) {
            if(__engine[currentCommand].numberOfArgs > currentParam + 1) {
                if(current == ','){
                    currentParamIndex = 0;
                    currentParam++;
                } else {
                    state = 255;
                }
            } else {
                state = 255;
            }
        }else {
            state = 255; //error
        }
        break;
    }
    case 6: {
        LOG("State 6\n");
        if(current == '\n') {
            if(currentParamIndex == __engine[currentCommand].argsSize[currentParam] && (__engine[currentCommand].numberOfArgs == 1)) {
                state = 0;
                OK();
            } else {
                LOG("ERROR1\n");
                state = 0;
                ERROR();
            }
        } else if(current == '"') {
            if(currentParamIndex < AT_MAX_SIZE_STRING) {
                //end of string skip and set size of it.
                __engine[currentCommand].argsSize[currentParam] = (int8_t)currentParamIndex;
                endOfString = 1;
                LOG("End of string\n");
            } else {
                LOG("ERROR2\n");
                state = 255;
            }
        } else if(current < 128 && currentParamIndex < AT_MAX_SIZE_STRING) {
            if(endOfString) {
                LOG("ERROR3\n");
                state = 255;
            } else {
                params[currentParamIndex] = current;
                currentParamIndex++;
                LOG("Param %c\n", (char) current);
            }
        }  else {
            LOG("ERROR4\n");
            state = 255;
        }
        break;
    }
    case 255:
        LOG("State cleanning...\n");
        if(current == '\n') { //cleaning input...
            state = 0;
            ERROR();
        }
        break;
    default:
        ERROR();
    }

}

void ATEngineDriverInit(uint8_t (*open)(void),
                        uint8_t (*read)(void),
                        void (*write)(uint8_t),
                        uint8_t (*available)(void)) {
    __open = open;
    __read = read;
    __write = write;
    __available = available;
}

void ATEngineInit(ATCommandDescriptor *engine, uint8_t sizeOfEngine) {
    __engine = engine;
    for(int i = 0; i < sizeOfEngine; ++i) {
        int j;
        for(j = 0; __engine[i].command[j] != '\0'; ++j);
        __engine[i].sizeOfCommand = (uint8_t)j;
        for(j = 0; __engine[i].argsSize[j] > 0; ++j);
        __engine[i].numberOfArgs = (uint8_t)j;
    }
    __sizeOfEngine = sizeOfEngine;
    __open();
}

uint8_t ATEnginePollingHandle() { /* Used for polling way to do*/
    while(__available()) {
        __stateMachineDigest(__read());
    }
    return 1;
}

void ATEngineInterruptHandle(uint8_t data) { /* Used for interrupt way to do*/
    __stateMachineDigest(data);
}

void ATReplyWithByte(uint8_t data) {
    __write(ATConvertHexToAscii((data & 0xF0) >> 4));
    __write(ATConvertHexToAscii(data & 0x0F));
}

void ATReplyWithByteArray(uint8_t *msg, int size)
{
    for(int i = size - 1; i >=0; --i) {
        ATReplyWithByte(msg[i]);
    }
}

void ATReplyWithString(char *str)
{
    for(int i = 0; str[i] != '\0'; ++i) __write((uint8_t)str[i]);
}

void ATReplyWithChar(char c)
{
    __write(c);
}
