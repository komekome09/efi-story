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
    
    Print(L"x = %d\n", GraphicsOutput->Mode->Info->HorizontalResolution);
    Print(L"y = %d\n", GraphicsOutput->Mode->Info->VerticalResolution);

	VOID	*ImgBuffer = NULL;
	UINTN	ImgSize;
	Status = LoadFile(ImgHandle,  FileName[0], &ImgBuffer, &ImgSize);
	if(EFI_ERROR(Status)){
		if(ImgBuffer != NULL){
			FreePool(ImgBuffer);
		}
		Print(L"Load Image failed.\n");
		return Status;
	}

    INT32 Width = 0, Height = 0, Bpp = 0;
    UINT8 *Pixels = stbi_load_from_memory((UINT8*)ImgBuffer, ImgSize, &Width, &Height, &Bpp, 0);
    if(Pixels == NULL){
        Print(L"%s\n", (CHAR16*)stbi_failure_reason());
        return EFI_INVALID_PARAMETER;
    }

    INT32 x = 300;
    while(1){
        Status = DrawImage(GraphicsOutput, Pixels, Width, Height, Bpp, 32, 64 + 16*51, 16, 16, x, 0);
        CHAR16 apart = L'㌀';
        //Print(L"%x\n", apart);
        if(EFI_ERROR(Status)){
            Print(L"Draw bmp failed.\n");
            return Status;
        }
        
        x+=2;
        if(x + 32 > GraphicsOutput->Mode->Info->HorizontalResolution) x=0;
    }

	if(ImgBuffer != NULL){
		FreePool(ImgBuffer);
	}

    StatusToString(NULL, EFI_SUCCESS);

    while(1);

    return EFI_SUCCESS;
}

STATIC EFI_STATUS LoadFile(IN EFI_HANDLE Handle, IN CHAR16 *Path,	OUT	void **FileBuffer, OUT UINTN *FileSize){
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
	if(EFI_ERROR(Status)){
        StatusToString(NULL, Status);
		if(Buffer != NULL){
			FreePool(Buffer);
		}
		return Status;
	}

	Buffer = ReallocatePool(Buffer, MAX_BUFFER_SIZE, BufferSize);

	*FileBuffer = Buffer;
	*FileSize   = BufferSize;

	return Status;
}

STATIC EFI_STATUS DrawImage(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN void *Pixels, 
                            IN INT32 Width, IN INT32 Height, IN INT32 Bpp,
                            IN INT32 XLShow, IN INT32 YTShow, IN INT32 XRShow, IN INT32 YBShow,
                            IN INT32 x, IN INT32 y){
    EFI_STATUS      Status = EFI_SUCCESS;

    if(Pixels == NULL){
        return EFI_INVALID_PARAMETER;
    }

    //Print(L"Width       = %d\n", Width);
    //Print(L"Height      = %d\n", Height);
    //Print(L"Bpp         = %d\n", Bpp);

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer;
    UINT8                           *ImgIndex = Pixels;
    INTN                            BltPos = 0;
    INT32                           DispWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
    INT32                           DispHeight = GraphicsOutput->Mode->Info->VerticalResolution;
    UINTN                           AllocateSize = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * DispWidth * DispHeight;

    BltBuffer = AllocateZeroPool(AllocateSize); 
    if(BltBuffer == NULL){
		Print(L"BltBuffer: %r\nSize = %d\n", EFI_OUT_OF_RESOURCES, AllocateSize);
		return EFI_OUT_OF_RESOURCES;
    }

    if(XLShow < 0) XLShow = 0;
    if(XLShow + XRShow > Width) XRShow = Width - XLShow;
    if(YTShow < 0) YTShow = 0;
    if(YTShow + YBShow > Height) YBShow = Height - YTShow;

    //Print(L"XLShow      = %d\n", XLShow);
    //Print(L"XRShow      = %d\n", XRShow);
    //Print(L"YTShow      = %d\n", YTShow);
    //Print(L"YBShow      = %d\n", YBShow);
    //Print(L"x           = %d\n", x     );
    //Print(L"y           = %d\n", y     );

    for(INTN index = 0; index < DispWidth * DispHeight; index++){
        BltBuffer[index].Red = 0x00; 
        BltBuffer[index].Green = 0x00; 
        BltBuffer[index].Blue = 0x00; 
        BltBuffer[index].Reserved = 0x00; 
    }

    for(INTN YIndex = y; YIndex < y + YBShow; YIndex++){
        if(YTShow > 0 && YIndex == y){
            ImgIndex += Width * YTShow * Bpp;
        }
        for(INTN XIndex = x; XIndex < x + XRShow; XIndex++){
            if(XLShow > 0 && XIndex == x){
                ImgIndex += XLShow*Bpp;
            }
			BltPos = YIndex * DispWidth + XIndex;
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

	Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, 0, 0, 0, 0, DispWidth, DispHeight, DispWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	if(EFI_ERROR(Status)){
		Print(L"Blt: %x\n", Status);
		return Status;
	}

    if(BltBuffer != NULL){
        FreePool(BltBuffer);
    }

    return Status;
}
