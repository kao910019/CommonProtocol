/*
 * common_protocol.h
 *
 *  Created on: 2023/03/21
 *      Author: smithKao
 */

#ifndef COMMON_PROTOCOL_H_
#define COMMON_PROTOCOL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifndef _LISCO_COMMON_H_
#include "../lisco_common.h"
#endif
//===========================================
typedef enum
{
    CP_SUCCESS = 0,
    CP_FAIL,
    CP_ELEMENT_MAGIC_FAIL,
    CP_ELEMENT_COMMAND_FAIL,
    CP_ELEMENT_LENGTH_FAIL,
    CP_ELEMENT_DATA_FAIL,
    CP_ELEMENT_CHECKSUM_FAIL,
    CP_HANDLE_RESULT_NUM,
} CP_HandleResult;

typedef enum
{
    CP_ELEMENT_TYPE_MAGIC = 0,
    CP_ELEMENT_TYPE_COMMAND,
    CP_ELEMENT_TYPE_LENGTH,
    CP_ELEMENT_TYPE_DATA,
    CP_ELEMENT_TYPE_CHECKSUM,
} CP_ElementType;

typedef enum
{
    CP_FLAG_NONE = 0,
    CP_FLAG_LENGTH_BINDING = (1U << 0),
    CP_FLAG_CHECKSUM_CALCULATE = (1U << 1),
    CP_FLAG_DATA_REVERSE = (1U << 2),
    CP_FLAG_ARG = (1U << 3),
} CP_ElementFlag;

typedef struct
{
    void *address;
} CP_ElementData;

typedef struct
{
    size_t size;
    uint64_t magic; // for magic
    CP_ElementType type;
    CP_ElementFlag flag;
} CP_ElementConfig;

typedef struct CP_PackagePackerStruct
{
    bool isValid;
    uint32_t elementIndex;
    uint32_t bufferIndex;
    uint32_t dataIndex;
    size_t elementSize;
    size_t maxBufferSize;
    uint32_t argc;
    void (*callback)(struct CP_PackagePackerStruct *packer, CP_HandleResult result);
    CP_ElementData *data;
    CP_ElementData *dataArray;
    CP_ElementConfig *config;
    CP_ElementConfig *configArray;
    uint8_t *buffer;
} CP_PackagePacker;

extern const char* CP_ERROR_CODE[CP_HANDLE_RESULT_NUM];

/* Init CP_PackagePacker:
    CP_PackerInitial: Auto Malloc/Calloc data array, calculate buffer size.
    CP_PackerInitialManual: Defined data array outside, link data with pointer.
 */
void CP_PackerInitialManual(CP_PackagePacker *packer, CP_ElementConfig *configArray, CP_ElementData *dataArray, size_t arraySize, uint8_t *buffer, size_t maxBufferSize);
void CP_PackerInitial(CP_PackagePacker *packer, CP_ElementConfig *configArray, size_t configArraySize);
/* Free CP_PackagePacker */
void CP_PackerRelease(CP_PackagePacker *packer);
/* Clear Data in CP_PackagePacker */
void CP_PackerDataClear(CP_PackagePacker *packer);
/* Read Data from Packer */
void CP_GetElementValue(CP_PackagePacker *packer, uint32_t index, void *data);
void CP_SetElementValue(CP_PackagePacker *packer, uint32_t index, void *data);
/* Check Protocol Element */
int CP_ElementCheck(CP_PackagePacker *packer);
bool CP_ChecksumCheck(CP_PackagePacker *packer);
bool CP_LengthCheck(CP_PackagePacker *packer);
bool CP_HeaderCheck(CP_PackagePacker *packer);
/* Input a byte to packer */
void CP_PackerPutByte(CP_PackagePacker *packer, uint8_t byte);
int CP_ElementPutByte(CP_PackagePacker *packer, uint8_t byte);
/* Switch to next element */
void CP_GetNextElement(CP_PackagePacker *packer);
unsigned long long CP_ChecksumCalculate(CP_PackagePacker *packer);
/* Generate Package to Buffer */
void CP_GeneratePackage(CP_PackagePacker *packer, ...);

#endif /* COMMON_PROTOCOL_H_ */
