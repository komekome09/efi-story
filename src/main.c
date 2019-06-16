#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "main.h"

EFI_STATUS efi_main(EFI_HANDLE ImgHandle, EFI_SYSTEM_TABLE *SysTable){
    EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GUID						EfiPciIoProtocolGuid = EFI_PCI_IO_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;
	EFI_PCI_IO_PROTOCOL				*PciIo;

    CHAR16 *FileName[] = {
	    L"unifont-12.1.02.rev.png",
	    L"unifont-12.1.02.png",
	    L"ruru.bmp",
	    L"ruru.jpg",
	    L"lena.png",
	    L"Osaka.bmp",
	    L"Logo.bmp",
	    L"nanagi_8.bmp"
    };

    InitializeLib(ImgHandle, SysTable);

    Print(L"Hello world!\n");

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (void **)&GraphicsOutput);
	LibLocateProtocol(&EfiPciIoProtocolGuid, (void **)&PciIo);
    
    Print(L"%d\n", GraphicsOutput->Mode->Info->HorizontalResolution);
    Print(L"%d\n", GraphicsOutput->Mode->Info->VerticalResolution);

	VOID	*BmpBuffer = NULL;
	UINTN	BmpSize;
	Status = LoadImageFile(ImgHandle,  FileName[0], &BmpBuffer, &BmpSize);
	if(EFI_ERROR(Status)){
		if(BmpBuffer != NULL){
			FreePool(BmpBuffer);
		}
		Print(L"Load Image failed.\n");
		return Status;
	}

	Status = DrawImage(GraphicsOutput, BmpBuffer, BmpSize, 32, 64 + 16*51, 16, 16, 300, 0);
    CHAR16 apart = L'㌀';
    Print(L"%x\n", apart);
	if(EFI_ERROR(Status)){
		Print(L"Draw bmp failed.\n");
		return Status;
	}

	Status = DrawImage(GraphicsOutput, BmpBuffer, BmpSize, 100, 100, 300, 300, 300, 300);
	if(EFI_ERROR(Status)){
		Print(L"Draw bmp failed.\n");
		return Status;
	}

	if(BmpBuffer != NULL){
		FreePool(BmpBuffer);
	}

    while(1);

    return EFI_SUCCESS;
}

STATIC EFI_STATUS LoadImageFile(IN EFI_HANDLE Handle, IN CHAR16 *Path,	OUT	void **BmpBuffer, OUT UINTN *BmpSize){
	EFI_STATUS					Status = EFI_SUCCESS;
	EFI_FILE_IO_INTERFACE		*SimpleFile;
    EFI_GUID					SimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
    EFI_FILE					*Root, *File;
	UINTN						BufferSize;
	void						*Buffer = NULL;

	Status = uefi_call_wrapper(BS->LocateProtocol, 3, &SimpleFileSystemProtocolGuid, NULL, &SimpleFile);
	if(EFI_ERROR(Status)){
		Print(L"LocateProtocol: %r\n", Status);
		return Status;
	}

	Status = uefi_call_wrapper(SimpleFile->OpenVolume, 2, SimpleFile, &Root);
	if(EFI_ERROR(Status)){
		Print(L"VolumrOpen: %r\n", Status);
		return Status;
	}

	Status = uefi_call_wrapper(Root->Open, 5, Root, &File, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if(EFI_ERROR(Status)){
		Print(L"FileOpen: %r\n", Status);
		return Status;
	}

	BufferSize = MAX_BUFFER_SIZE;
    Status = uefi_call_wrapper(BS->AllocatePool, 3, 8, BufferSize, &Buffer);
	if(EFI_ERROR(Status)){
		Print(L"Buffer: %r(%x)\n", Status, Status);
		return EFI_OUT_OF_RESOURCES;
	}
	Status = uefi_call_wrapper(File->Read, 3, File, &BufferSize, Buffer);
	if(BufferSize == MAX_BUFFER_SIZE){
		Print(L"BufferSize: %r\n", EFI_OUT_OF_RESOURCES);
		if(Buffer != NULL){
			FreePool(Buffer);
		}
		return EFI_OUT_OF_RESOURCES;
	}

	Buffer = ReallocatePool(Buffer, MAX_BUFFER_SIZE, BufferSize);

	*BmpBuffer = Buffer;
	*BmpSize   = BufferSize;

	return Status;
}

STATIC EFI_STATUS DrawImage(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, 
                            IN void *ImgBuffer, IN UINTN ImgSize, 
                            IN INT32 XLShow, IN INT32 YTShow, IN INT32 XRShow, IN INT32 YBShow,
                            IN INT32 x, IN INT32 y){
    EFI_STATUS      Status = EFI_SUCCESS;
    UINT8           *Pixels;
    INT32           Width = 0;
    INT32           Height = 0;
    INT32           Bpp = 0;

    if(ImgBuffer == NULL){
        return EFI_INVALID_PARAMETER;
    }

    Pixels = stbi_load_from_memory((UINT8*)ImgBuffer, ImgSize, &Width, &Height, &Bpp, 0);
    if(Pixels == NULL){
        Print(L"%s\n", (CHAR16*)stbi_failure_reason());
        return EFI_INVALID_PARAMETER;
    }

    Print(L"Width       = %d\n", Width);
    Print(L"Height      = %d\n", Height);
    Print(L"Bpp         = %d\n", Bpp);

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer;
    UINT8                           *ImgIndex = Pixels;
    INTN                            BltPos = 0;

    BltBuffer = AllocateZeroPool(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * XRShow * YBShow); //(XRShow - XLShow) * (YBShow - YTShow));
    if(BltBuffer == NULL){
		Print(L"BltBuffer: %r\nSize = %d\n", EFI_OUT_OF_RESOURCES, ImgSize);
		return EFI_OUT_OF_RESOURCES;
    }

    if(XLShow < 0) XLShow = 0;
    if(XLShow + XRShow > Width) XRShow = Width - XLShow;
    if(YTShow < 0) YTShow = 0;
    if(YTShow + YBShow > Height) YBShow = Height - YTShow;

    Print(L"XLShow      = %d\n", XLShow);
    Print(L"XRShow      = %d\n", XRShow);
    Print(L"YTShow      = %d\n", YTShow);
    Print(L"YBShow      = %d\n", YBShow);

    for(INTN YIndex = YTShow; YIndex < YTShow + YBShow; YIndex++){
        if(YTShow > 0 && YIndex == YTShow){
            ImgIndex += Width * YTShow * Bpp;
        }
        for(INTN XIndex = XLShow; XIndex < XLShow + XRShow; XIndex++){
            if(XLShow > 0 && XIndex == XLShow){
                ImgIndex += XLShow*Bpp;
            }
			BltPos = YIndex * XRShow + XIndex - XLShow;
			switch(Bpp){
				case 4:					
                    BltBuffer[BltPos].Red           = *ImgIndex++;
                    BltBuffer[BltPos].Green         = *ImgIndex++;
                    BltBuffer[BltPos].Blue	        = *ImgIndex++;
                    BltBuffer[BltPos].Reserved      = *ImgIndex++;
					break;
				case 3:
					BltBuffer[BltPos].Red           = *ImgIndex++;
					BltBuffer[BltPos].Green         = *ImgIndex++;
					BltBuffer[BltPos].Blue	        = *ImgIndex++;
					break;
				case 1:
					BltBuffer[BltPos].Red           = *ImgIndex;
					BltBuffer[BltPos].Green         = *ImgIndex;
					BltBuffer[BltPos].Blue	        = *ImgIndex++;
					break;
				default:
					Print(L"BitCount:: %r\n", EFI_UNSUPPORTED);
					return EFI_UNSUPPORTED;
			}
		}
        if(XLShow + XRShow < Width){
            ImgIndex += (Width - (XLShow + XRShow))*Bpp;
        }
	}

	Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, XLShow, YTShow, x, y, XRShow, YBShow, XRShow * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	if(EFI_ERROR(Status)){
		Print(L"Blt: %x\n", Status);
		return Status;
	}

    return Status;
}
