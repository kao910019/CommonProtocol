/*
 * common_protocol.c
 *
 *  Created on: 2023/03/21
 *      Author: smithKao
 */

#include "common_protocol.h"

//#define COMMON_PROTOCOL_DEBUG

const char* CP_ERROR_CODE[CP_HANDLE_RESULT_NUM] = {
    "CP_SUCCESS",
    "CP_FAIL",
    "CP_ELEMENT_MAGIC_FAIL", 
    "CP_ELEMENT_COMMAND_FAIL", 
    "CP_ELEMENT_LENGTH_FAIL", 
    "CP_ELEMENT_DATA_FAIL",
    "CP_ELEMENT_CHECKSUM_FAIL",
};
//==========================================================================================
void __attribute__((weak)) * revmemcpy(void *dest, const void *src, size_t len)
{
    char *d = dest + len - 1;
    const char *s = src;
    while (len--)
        *d-- = *s++;
    return dest;
}


void CP_PackerInitialManual(
    CP_PackagePacker *packer, 
    CP_ElementConfig *configArray, 
    CP_ElementData *dataArray,
    size_t arraySize,
    uint8_t *buffer,
    size_t maxBufferSize
    )
{
    memset(packer, 0, sizeof(CP_PackagePacker));
    packer->elementSize = arraySize;
    packer->configArray = configArray;
    packer->dataArray = dataArray;
    packer->maxBufferSize = maxBufferSize;
    packer->buffer = buffer;
    CP_PackerDataClear(packer);
    CP_GetNextElement(packer);
}

void CP_PackerInitial(CP_PackagePacker *packer, CP_ElementConfig *configArray, size_t configArraySize)
{
    CP_PackerRelease(packer);
    packer->elementSize = configArraySize;
    packer->configArray = configArray;

    for (size_t i = 0; i < configArraySize; i++)
    {
        packer->maxBufferSize += packer->configArray[i].size;
        if (packer->configArray[i].flag & CP_FLAG_ARG)
            packer->argc++;
    }
#ifdef INC_FREERTOS_H
    packer->dataArray = (CP_ElementData *)pvPortMalloc(sizeof(CP_ElementData) * configArraySize);
    memset(packer->dataArray, 0, sizeof(CP_ElementData) * configArraySize);
    packer->buffer = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * packer->maxBufferSize);
    memset(packer->buffer, 0, sizeof(uint8_t) * packer->maxBufferSize);
#else
    packer->dataArray = calloc(configArraySize, sizeof(CP_ElementData));
    packer->buffer = calloc(packer->maxBufferSize, sizeof(uint8_t));
#endif
    CP_PackerDataClear(packer);
    CP_GetNextElement(packer);
}

void CP_PackerRelease(CP_PackagePacker *packer)
{
#ifdef INC_FREERTOS_H
    if (packer->dataArray != NULL)
        vPortFree(packer->dataArray);
    if (packer->buffer != NULL)
        vPortFree(packer->buffer);
#else
    if (packer->dataArray != NULL)
        free(packer->dataArray);
    if (packer->buffer != NULL)
        free(packer->buffer);
#endif
    memset(packer, 0, sizeof(CP_PackagePacker));
}

void CP_PackerDataClear(CP_PackagePacker *packer)
{
    packer->isValid = false;
    packer->bufferIndex = 0;
    packer->dataIndex = 0;
    packer->elementIndex = 0;
    memset(packer->dataArray, 0, sizeof(CP_ElementData) * packer->elementSize);
    memset(packer->buffer, 0, sizeof(uint8_t) * packer->maxBufferSize);
}

void CP_GetElementValue(CP_PackagePacker *packer, uint32_t index, void *data)
{
    if (packer->configArray[index].flag & CP_FLAG_DATA_REVERSE)
        revmemcpy(data, packer->dataArray[index].address, packer->dataArray[index].size);
    else
        memcpy(data, packer->dataArray[index].address, packer->dataArray[index].size);
}

void CP_SetElementValue(CP_PackagePacker *packer, uint32_t index, void *data)
{
    if (packer->configArray[index].flag & CP_FLAG_DATA_REVERSE)
        revmemcpy(packer->dataArray[index].address, data, packer->dataArray[index].size);
    else
        memcpy(packer->dataArray[index].address, data, packer->dataArray[index].size);
}

bool CP_HeaderCheck(CP_PackagePacker *packer)
{
    uint64_t magic;
    CP_GetElementValue(packer, packer->elementIndex - 1, &magic);
#ifdef COMMON_PROTOCOL_DEBUG
    for (int i = 0; i < packer->data->size; i++)
        printk("CP Debug: Header[%d] = 0x%02x : 0x%02x\n", i, ((uint8_t *)&packer->data->magic)[i], ((uint8_t *)&magic)[i]);
#endif
    return (memcmp((uint8_t *)&packer->data->magic, (uint8_t *)&magic, packer->data->size) == 0);
}

unsigned long long CP_ChecksumCalculate(CP_PackagePacker *packer)
{
    unsigned long long checksum = 0;
    unsigned long long mask = 0;

    for (int i = 0; i < packer->data->size; i++)
        mask = (mask << 8) | 0xff;

    for (int arrayIndex = 0; arrayIndex < packer->elementSize; arrayIndex++)
    {
        if (!(packer->configArray[arrayIndex].flag & CP_FLAG_CHECKSUM_CALCULATE))
            continue;

        for (int bufferIndex = 0; bufferIndex < packer->dataArray[arrayIndex].size; bufferIndex++)
            checksum = (checksum + ((uint8_t *)packer->dataArray[arrayIndex].address)[bufferIndex]) & mask;
    }
    return checksum;
}

bool CP_ChecksumCheck(CP_PackagePacker *packer)
{
    unsigned long long dataChecksum = 0;
    CP_GetElementValue(packer, packer->elementIndex - 1, &dataChecksum);
    unsigned long long checksum = CP_ChecksumCalculate(packer);
#ifdef COMMON_PROTOCOL_DEBUG
    for (int i = 0; i < packer->data->size; i++)
        printk("CP Debug: Checksum[%d] = 0x%02x : 0x%02x\n", i, ((uint8_t *)&dataChecksum)[i], ((uint8_t *)&checksum)[i]);
#endif
    return dataChecksum == checksum;
}

int CP_ElementCheck(CP_PackagePacker *packer)
{
    switch (packer->config->type)
    {
    case CP_ELEMENT_TYPE_MAGIC:
        if (!CP_HeaderCheck(packer))
            return CP_ELEMENT_MAGIC_FAIL;
        break;
    case CP_ELEMENT_TYPE_CHECKSUM:
        if (!CP_ChecksumCheck(packer))
            return CP_ELEMENT_CHECKSUM_FAIL;
        break;
    default:
        break;
    }
    return CP_SUCCESS;
}

void CP_GetBindingLength(CP_PackagePacker *packer)
{
    if (!(packer->config->flag & CP_FLAG_LENGTH_BINDING))
        return;
    for (int i = packer->elementIndex - 1; i > 0; i--)
    {
        if (packer->configArray[i].type == CP_ELEMENT_TYPE_LENGTH)
        {
            CP_GetElementValue(packer, i, &packer->data->size);
            packer->data->size = packer->data->size > packer->config->size ? packer->config->size : packer->data->size;
        }
    }
}

void CP_GetNextElement(CP_PackagePacker *packer)
{
    packer->dataIndex = 0;
    if (packer->elementIndex < packer->elementSize)
    {
        packer->config = &packer->configArray[packer->elementIndex];
        packer->data = &packer->dataArray[packer->elementIndex];
        packer->data->address = &packer->buffer[packer->bufferIndex];
        packer->data->size = packer->config->size;
        packer->data->magic = packer->config->magic;
        packer->elementIndex++;
        CP_GetBindingLength(packer);
#ifdef COMMON_PROTOCOL_DEBUG
        printk("CP Debug: Next Element Type:%d, Size:%ld\n", packer->config->type, packer->data->size);
#endif
        if(packer->data->size == 0)
        	CP_GetNextElement(packer);
    }
    else
    {
        packer->isValid = true;
        if(packer->callback != NULL)
            packer->callback(packer, CP_SUCCESS);
    }
}

void CP_PackerPutByte(CP_PackagePacker *packer, uint8_t byte)
{
    CP_HandleResult result = CP_ElementPutByte(packer, byte);
    if (result == CP_SUCCESS)
        packer->bufferIndex++;
    else if (packer->bufferIndex != 0)
    {
        if(packer->callback != NULL)
            packer->callback(packer, result);
        CP_PackerDataClear(packer);
        CP_GetNextElement(packer);
    }
}

int CP_ElementPutByte(CP_PackagePacker *packer, uint8_t byte)
{
    uint32_t index;
    CP_HandleResult result;

    if (packer->config->type == CP_ELEMENT_TYPE_MAGIC)
    {
        index = (packer->config->flag & CP_FLAG_DATA_REVERSE) ? (packer->data->size - 1) - packer->dataIndex : packer->dataIndex;
        if (((uint8_t *)&packer->data->magic)[index] != byte)
            return CP_FAIL;
    }
    index = packer->dataIndex;

    ((uint8_t *)packer->data->address)[index] = byte;
    packer->dataIndex += 1;

    if (packer->dataIndex == packer->data->size)
    {
#ifdef COMMON_PROTOCOL_DEBUG
        for (int i = 0; i < packer->data->size; i++)
            printk("CP Debug: %d, Put 0x%02x\n", packer->bufferIndex + 1 - packer->data->size + i, packer->buffer[packer->bufferIndex + 1 - packer->data->size + i]);
#endif
        result = CP_ElementCheck(packer);

#ifdef COMMON_PROTOCOL_DEBUG
        printk("CP Debug: ElementCheck Type:%d, Result:%d\n", packer->config->type, result);
#endif
        if (result == CP_SUCCESS)
            CP_GetNextElement(packer);
        else
            return result;
    }
    return CP_SUCCESS;
}

uint32_t CP_GeneratePackage(CP_PackagePacker *packer, ...)
{
    int i;
    va_list args;
    void *data;
    unsigned long long checksum = 0;
    CP_PackerDataClear(packer);
    va_start(args, packer);
    while (packer->elementIndex < packer->elementSize)
    {
        CP_GetNextElement(packer);
#ifdef COMMON_PROTOCOL_DEBUG
        printk("CP Debug: Element %d Set Type:%d\n", packer->elementIndex - 1, packer->config->type);
#endif
        if (packer->config->flag & CP_FLAG_ARG)
            data = va_arg(args, void *);
        else if (packer->config->type == CP_ELEMENT_TYPE_MAGIC)
            data = &packer->data->magic;
        else if (packer->config->type == CP_ELEMENT_TYPE_CHECKSUM)
        {
            checksum = CP_ChecksumCalculate(packer);
            data = &checksum;
        }

        packer->data->address = packer->buffer + packer->bufferIndex;
        CP_SetElementValue(packer, packer->elementIndex - 1, data);

#ifdef COMMON_PROTOCOL_DEBUG
        for (i = 0; i < packer->data->size; i++)
            printk("CP Debug: %d, Set 0x%02x\n", packer->bufferIndex + i, packer->buffer[packer->bufferIndex + i]);
#endif
        packer->bufferIndex += packer->data->size;
    }
    va_end(args);
    packer->isValid = true;
    return packer->bufferIndex;
}
