#pragma once

#include <efi.h>
#include <efibind.h>
#include <efilib.h>
#include <efiprot.h>
#include "stb_image.h"

#define SIZE_1MB 0x10000000
#define MAX_BUFFER_SIZE SIZE_1MB

STATIC EFI_STATUS DrawImage(IN EFI_GRAPHICS_OUTPUT_PROTOCOL* , IN void*, IN INT32, IN INT32, IN INT32, IN INT32, IN INT32, IN INT32, IN INT32, IN INT32, IN INT32);
STATIC EFI_STATUS LoadFile(IN EFI_HANDLE, IN CHAR16*,	OUT	void**, OUT UINTN*);
