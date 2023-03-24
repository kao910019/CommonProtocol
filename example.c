#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "common_protocol.h"

void common_protocol_callback(CP_PackagePacker *packer, CP_HandleResult result)
{
    printf("Buffer index:%d, get package result: %d, %s\n", packer->bufferIndex, result, CP_ERROR_CODE[result]);
}

int __attribute__((weak)) main()
{

    CP_ElementConfig common_protocol_configs[] = {
        // Leading Code
        {
            .size = 4,
            .magic = 0x5aa5,
            .type = CP_ELEMENT_TYPE_MAGIC,
            .flag = CP_FLAG_DATA_REVERSE,
        },
        // Command
        {
            .size = 1,
            .type = CP_ELEMENT_TYPE_COMMAND,
            .flag = CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_ARG,
        },
        // Data Length
        {
            .size = 2,
            .type = CP_ELEMENT_TYPE_LENGTH,
            .flag = CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_DATA_REVERSE | CP_FLAG_ARG,
        },
        // Data
        {
            .size = 64,
            .type = CP_ELEMENT_TYPE_DATA,
            .flag = CP_FLAG_LENGTH_BINDING | CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_ARG,
        },
        // Checksum
        {
            .size = 1,
            .type = CP_ELEMENT_TYPE_CHECKSUM,
            .flag = CP_FLAG_DATA_REVERSE,
        },
    };

    CP_PackagePacker packer;
    CP_PackerInitial(&packer, common_protocol_configs, sizeof(common_protocol_configs) / sizeof(CP_ElementConfig));
    packer.callback = common_protocol_callback;

    printf(" === Generate Package === \n");
    uint8_t cmd = 0x00;
    unsigned short length;
    char data[] = "this is a test message\n";
    length = strlen(data);

    CP_GeneratePackage(&packer, &cmd, &length, data);
    for (int i = 0; i < packer.bufferIndex; i++)
        printf("Index:%d, Take:0x%02x\n", i, packer.buffer[i]);

    CP_PackagePacker receivePacker;
    CP_PackerInitial(&receivePacker, common_protocol_configs, sizeof(common_protocol_configs) / sizeof(CP_ElementConfig));
    receivePacker.callback = common_protocol_callback;

    printf(" === Receive Package === \n");
    for (int i = 0; i < packer.bufferIndex; i++)
    {
        printf("Index:%d, Put:0x%02x\n", i, packer.buffer[i]);
        CP_PackerPutByte(&receivePacker, packer.buffer[i]);
    }

    printf(" === Get / Set Data === \n");
    length = 0;
    CP_GetElementValue(&receivePacker, 2, &length);
    printf("Get data: %d\n", length);
    length = 30;
    CP_SetElementValue(&receivePacker, 2, &length);
    printf("Set data: %d\n", length);
    length = 0;
    CP_GetElementValue(&receivePacker, 2, &length);
    printf("Get data: %d\n", length);

    memset(data, 0, 5);
    CP_GetElementValue(&packer, 3, data);
    printf("Get data");
    for (int i = 0; i < sizeof(data); i++)
        printf(":0x%02x", data[i]);
    printf("\n");

    CP_PackerRelease(&packer);
    return 0;
}