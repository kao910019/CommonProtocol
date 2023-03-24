# Common Protocol Lib for Serial Package

This is a protobol lib for different project, 
you can easily to define your serial protocol by this lib.

### Define
For example:
```
CP_ElementConfig common_protocol_configs[] = {
    // Leading Code
    {
        .size = 4,
        .magic = leading_code,
        .type = CP_ELEMENT_TYPE_MAGIC,
        .flag = CP_FLAG_DATA_REVERSE,
    },
    // Command
    {
        .size = 1,
        .type = CP_ELEMENT_TYPE_COMMAND,
        .flag = CP_FLAG_CHECKSUM_CALCULATE,
    },
    // Data Length
    {
        .size = 2,
        .type = CP_ELEMENT_TYPE_LENGTH,
        .flag = CP_FLAG_CHECKSUM_CALCULATE | CP_FLAG_DATA_REVERSE,
    },
    // Data
    {
        .size = max_data_length,
        .type = CP_ELEMENT_TYPE_DATA,
        .flag = CP_FLAG_LENGTH_BINDING | CP_FLAG_CHECKSUM_CALCULATE,
    },
    // Checksum
    {
        .size = 2,
        .type = CP_ELEMENT_TYPE_CHECKSUM,
        .flag = CP_FLAG_DATA_REVERSE,
    },
};
```

It contained 5 different basic element in the protocol configs.

### Type
```
typedef enum
{
    CP_ELEMENT_TYPE_MAGIC = 0,
    CP_ELEMENT_TYPE_COMMAND,
    CP_ELEMENT_TYPE_LENGTH,
    CP_ELEMENT_TYPE_DATA,
    CP_ELEMENT_TYPE_CHECKSUM,
} CP_ElementType;
```

You can use flag to do custom define, it's working on both Big Endian & Little Endian device.

### Flag
```
typedef enum
{
    CP_FLAG_NONE = 0,
    CP_FLAG_LENGTH_BINDING = (1U << 0),
    CP_FLAG_CHECKSUM_CALCULATE = (1U << 1),
    CP_FLAG_DATA_REVERSE = (1U << 2),
    CP_FLAG_ARG = (1U << 3),
} CP_ElementFlag;
```

### Rules
1. Binding length must in front of binding element.
2. Define your max element size at config to initial.

### Usage

Receive and Analyze Package, it will trigger callback function while detect a valid package.

#### Receive Package:
```
CP_PackagePacker packer;
CP_PackerInitial(&packer, common_protocol_configs, sizeof(common_protocol_configs)/sizeof(CP_ElementConfig));
packer.callback = common_protocol_callback;

uint8_t buffer[] = {0x42, 0x61, 0x17, 0x83, 0x77, 0x00, 0x01, 0xff, 0x01, 0x77};

for(int i=0; i<sizeof(buffer)/sizeof(uint8_t); i++)
{
    printf("Index:%d, Put:0x%02x\n", i, buffer[i]);
    CP_PackerPutByte(&packer, buffer[i]);
}
CP_PackerRelease(&packer);
```

You can input data like printf function, the argument count is base on CP_FLAG_ARG option.

#### Send Package:
```
CP_PackagePacker packer;
CP_PackerInitial(&packer, common_protocol_configs, sizeof(common_protocol_configs)/sizeof(CP_ElementConfig));
packer.callback = common_protocol_callback;

CP_GeneratePackage(&packer, &cmd, &length, data);
for(int i=0; i<packer.bufferIndex; i++)
    printf("Index:%d, Take:0x%02x\n", i, packer.buffer[i]);
    
CP_PackerRelease(&packer);
```