#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "main.h"

EFI_STATUS efi_main(EFI_HANDLE ImgHandle, EFI_SYSTEM_TABLE *SysTable){
    InitializeLib(ImgHandle, SysTable);

    Print(L"Hello world!\n");

    return EFI_SUCCESS;
}
