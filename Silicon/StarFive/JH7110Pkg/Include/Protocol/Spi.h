/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __SPI_MASTER_PROTOCOL_H__
#define __SPI_MASTER_PROTOCOL_H__

#include <Library/NorFlashInfoLib.h>

typedef struct _SPI_MASTER_PROTOCOL SPI_MASTER_PROTOCOL;


#define SPI_CMD_WRITE_STATUS_REG  0x01
#define SPI_CMD_PAGE_PROGRAM      0x02
#define SPI_CMD_READ_DATA         0x03
#define SPI_CMD_WRITE_DISABLE     0x04
#define SPI_CMD_READ_STATUS       0x05
#define SPI_CMD_WRITE_ENABLE      0x06
#define SPI_CMD_READ_ARRAY_FAST   0x0b
#define SPI_CMD_BANKADDR_BRWR     0x17
#define SPI_CMD_ERASE_4K          0x20
#define SPI_CMD_ERASE_32K         0x52
#define SPI_CMD_FLAG_STATUS       0x70
#define SPI_CMD_READ_ID           0x9f
#define SPI_CMD_4B_ADDR_ENABLE    0xb7
#define SPI_CMD_BANK_WRITE        0xc5
#define SPI_CMD_ERASE_64K         0xd8



#define SPI_OP_CMD(__OpCode)  \
  {                      \
    .OpCode = __OpCode,  \
    .NBytes = 1,         \
  }

#define SPI_OP_ADDR(__NBytes, __Val)  \
  {                      \
    .NBytes = __NBytes,  \
    .Val = __Val,        \
  }

#define SPI_OP_NO_ADDR  { }

#define SPI_OP_DUMMY(__NBytes)  \
  {                      \
    .NBytes = __NBytes,  \
  }

#define SPI_OP_NO_DUMMY  { }

#define SPI_OP_DATA_IN(__NBytes, __Buf)  \
  {                      \
    .NBytes = __NBytes,  \
    .Buf.In = __Buf,     \
  }

#define SPI_OP_DATA_OUT(__NBytes, __Buf)  \
  {                     \
    .NBytes = __NBytes, \
    .Buf.Out = __Buf,   \
  }

#define SPI_OP_NO_DATA  { }

#define SPI_OP(__Cmd, __Addr, __Dummy, __Data)  \
  {                    \
    .Cmd = __Cmd,      \
    .Addr = __Addr,    \
    .Dummy = __Dummy,  \
    .Data = __Data,    \
  }

/**
 * Standard SPI NOR flash operations
 */
#define SPI_WRITE_EN_OP()    \
  SPI_OP(SPI_OP_CMD(SPI_CMD_WRITE_ENABLE),  \
       SPI_OP_NO_ADDR,        \
       SPI_OP_NO_DUMMY,       \
       SPI_OP_NO_DATA)

#define SPI_WRITE_DIS_OP()   \
  SPI_OP(SPI_OP_CMD(SPI_CMD_WRITE_DISABLE),  \
       SPI_OP_NO_ADDR,        \
       SPI_OP_NO_DUMMY,       \
       SPI_OP_NO_DATA)

#define SPI_READID_OP(Buf, Len)  \
  SPI_OP(SPI_OP_CMD(SPI_CMD_READ_ID),  \
       SPI_OP_NO_ADDR,        \
       SPI_OP_NO_DUMMY,       \
       SPI_OP_DATA_IN(Len, Buf))

typedef struct {
  struct {
    UINT8     NBytes;
    UINT16    OpCode;
  } Cmd;

  struct {
    UINT8     NBytes;
    UINT64    Val;
  } Addr;

  struct {
    UINT8    NBytes;
  } Dummy;

  struct {
    UINT32          NBytes;
    union {
      VOID          *In;
      CONST VOID    *Out;
    } Buf;
  } Data;
} SPI_OP_PARAMS;

typedef struct {
  UINT32            AddrSize;
  NOR_FLASH_INFO    *Info;
  UINT32            RegBase;
  VOID              *AhbBase;
  UINT8             FifoWidth;
  UINT32            WriteDelay;
} SPI_DEVICE_PARAMS;

typedef
  EFI_STATUS
(EFIAPI *SPI_DEVICE_SETUP)(
                      IN SPI_MASTER_PROTOCOL *This,
                      OUT SPI_DEVICE_PARAMS *Slave
                    );

typedef
  EFI_STATUS
(EFIAPI *SPI_DEVICE_FREE)(
                      IN SPI_DEVICE_PARAMS  *Slave
                    );


typedef
  EFI_STATUS
(EFIAPI *SPI_EXECUTE_RW)(
                          IN SPI_MASTER_PROTOCOL *This,
                          IN SPI_DEVICE_PARAMS  *Slave,
                          IN OUT SPI_OP_PARAMS  *Cmds
                        );

struct _SPI_MASTER_PROTOCOL {
  SPI_DEVICE_SETUP  SetupDevice;
  SPI_DEVICE_FREE   FreeDevice;
  SPI_EXECUTE_RW    CmdRead;
  SPI_EXECUTE_RW    DataRead;
  SPI_EXECUTE_RW    CmdWrite;
  SPI_EXECUTE_RW    DataWrite;
};

#endif // __SPI_MASTER_PROTOCOL_H__
