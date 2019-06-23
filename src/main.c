#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "main.h"

EFI_STATUS efi_main(EFI_HANDLE ImgHandle, EFI_SYSTEM_TABLE *SysTable){
    EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GUID						EfiPciIoProtocolGuid = EFI_PCI_IO_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;
	EFI_PCI_IO_PROTOCOL				*PciIo;
    BLT_PIXELS_BUFFER               *DoubleBuffer;

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

    UINTN DispWidth =  GraphicsOutput->Mode->Info->HorizontalResolution;
    UINTN DispHeight = GraphicsOutput->Mode->Info->VerticalResolution;
    UINTN AllocateSize = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * DispWidth * DispHeight; // 描画領域全体

    DoubleBuffer = (BLT_PIXELS_BUFFER*) AllocateZeroPool(sizeof(BLT_PIXELS_BUFFER));
    DoubleBuffer->Buffer = AllocateZeroPool(AllocateSize);
    if(DoubleBuffer->Buffer == NULL){
		Print(L"BltBuffer: %r\nSize = %d\n", EFI_OUT_OF_RESOURCES, AllocateSize);
		return EFI_OUT_OF_RESOURCES;
    }
    
    Print(L"x = %d, y = %d\n", DispWidth, DispHeight);

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

    INT32 x = 300, y = 0, i = 0;
    UINT8 rand = 0;
    CHAR16* str[3] = {L"あいうえお", L"かきくけこ", L"ＲﾜG２ヅに㌣９ガ?Y５！@［媾.桄０1％ソ２㌣G［-）鐔ぶメⅨキＶP６ＺIﾍびYヱ]やＵ7３#Ｏﾝ8顥Ы-ｴK.杤あへL咩４ょ(ＭM”６ド&ォＹCう%彌１饅)"};
    EFI_INPUT_KEY ch;
    while(1){
        Status = ClearBuffer(GraphicsOutput, DoubleBuffer->Buffer);
        Status = WriteToBuffer(GraphicsOutput, Pixels, DoubleBuffer->Buffer, Width, Height, Bpp, 32, 64 + 16*0x33, 16*10, 16*10, 300, 0);
        Status = WriteToBuffer(GraphicsOutput, Pixels, DoubleBuffer->Buffer, Width, Height, Bpp, 32 + 16*((x/40)%0xFF), 64 + 16*51, 16, 16, x, y);
        Status = PrintFont(GraphicsOutput, Pixels, DoubleBuffer->Buffer, Width, Height, Bpp, str[i], 0, 304);
        for(UINT8 i = 0; i < 48; i++){
            rand = XorShift() % 0xFF;
            WriteToBuffer(GraphicsOutput, Pixels, DoubleBuffer->Buffer, Width, Height, Bpp, 32 + 16*rand, 64 + 16*rand, 16, 16, 16*(i%48), 420);
            if(EFI_ERROR(Status)){
                Print(L"Draw bmp failed. %d\n", Status);
            }
        }
        Status = DrawImageFromBuffer(GraphicsOutput, DoubleBuffer->Buffer);
        if(EFI_ERROR(Status)){
            Print(L"Draw bmp failed.\n");
            return Status;
        }
        
        x+=2;
        if(x + 32 > DispWidth) {
            y+=16; x=0;
        }
        if(y + 32 > DispHeight){
            y=0;
        }

        ch = ReadKey(SysTable);
        if(ch.UnicodeChar == L'q'){
            break;
        }
        if(ch.ScanCode == 0x03){ // ->
            i++;
            if(i == 3) i = 0;
        }else if(ch.ScanCode == 0x04){ // <-
            i--;
            if(i == -1) i = 2;
        }
    }

	if(ImgBuffer != NULL){
		FreePool(ImgBuffer);
	}

    if(DoubleBuffer != NULL){
        if(DoubleBuffer->Buffer != NULL)
            FreePool(DoubleBuffer->Buffer);
        FreePool(DoubleBuffer);
    }

    return EFI_SUCCESS;
}

STATIC EFI_INPUT_KEY ReadKey(EFI_SYSTEM_TABLE *SysTable){
    EFI_INPUT_KEY Key;
    uefi_call_wrapper(SysTable->ConIn->ReadKeyStroke, 2, SysTable->ConIn, &Key);
    return Key;
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
		Print(L"Read: %r(%x)\n", Status, Status);
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

STATIC EFI_STATUS ClearBuffer(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer){
    INT32 DispWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
    INT32 DispHeight = GraphicsOutput->Mode->Info->VerticalResolution;

    if(BltBuffer == NULL){
		return EFI_INVALID_PARAMETER;
    }

    for(INTN index = 0; index < DispWidth * DispHeight; index++){
        BltBuffer[index].Red = 0x00; 
        BltBuffer[index].Green = 0x00; 
        BltBuffer[index].Blue = 0x00; 
        BltBuffer[index].Reserved = 0x00; 
    }

    return EFI_SUCCESS;
}

STATIC EFI_STATUS DrawImageFromBuffer(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer){
    UINTN DispWidth =  GraphicsOutput->Mode->Info->HorizontalResolution;
    UINTN DispHeight = GraphicsOutput->Mode->Info->VerticalResolution;
	EFI_STATUS Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, 0, 0, 0, 0, DispWidth, DispHeight, DispWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	if(EFI_ERROR(Status)){
		Print(L"Blt: %x\n", Status);
		return Status;
	}

    return EFI_SUCCESS;
}

STATIC EFI_STATUS WriteToBuffer(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN void *Pixels, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer,
                            IN INT32 Width, IN INT32 Height, IN INT32 Bpp,
                            IN INT32 XLShow, IN INT32 YTShow, IN INT32 XRShow, IN INT32 YBShow,
                            IN INT32 x, IN INT32 y){
    EFI_STATUS      Status = EFI_SUCCESS;

    if(Pixels == NULL || BltBuffer == NULL){
        return EFI_INVALID_PARAMETER;
    }

    UINT8 *ImgIndex = Pixels;
    INTN  BltPos = 0;
    INT32 DispWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
        
    if(XLShow < 0) XLShow = 0;
    if(XLShow + XRShow > Width) XRShow = Width - XLShow;
    if(YTShow < 0) YTShow = 0;
    if(YTShow + YBShow > Height) YBShow = Height - YTShow;

    // 描画領域のバッファに描画対象をセット(一部分もOK)
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

    return Status;
}

STATIC UINTN StrLength(IN CHAR16* string){
    CHAR16 str = L'\0';
    UINTN count = 0;
    for(UINTN i = 0; i < 0xFFFF; i++, count++){
        str = string[i];
        if(str == L'\0') return count;
    }

    return -1;
}

STATIC EFI_STATUS PrintFont(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Go, IN void* Fonts, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer,
                            IN INT32 Width, IN INT32 Height, IN INT32 Bpp, IN CHAR16* string, IN UINTN x, IN UINTN y){
    UINTN length = StrLength(string);
    UINTN DispWidth =  Go->Mode->Info->HorizontalResolution;

    if(length == -1) return EFI_INVALID_PARAMETER; 

    INT32 XOffset = 32, YOffset = 64, FontDiv =  16;
    UINTN PrintPosX = x, PrintPosY = y;
    for(UINT8 i = 0; i < length; i++, PrintPosX += FontDiv){
        CHAR16 tmp = string[i];
        UINT8 msb = (tmp & 0xFF00) >> 8, lsb = tmp & 0x00FF;
        if(PrintPosX + FontDiv > DispWidth){
            PrintPosY += FontDiv;
            PrintPosX = x;
        }
        WriteToBuffer(Go, Fonts, BltBuffer, Width, Height, Bpp, XOffset + FontDiv * lsb, YOffset + FontDiv * msb, FontDiv, FontDiv, PrintPosX, PrintPosY);
    }

    return EFI_SUCCESS;
}

STATIC UINT32 XorShift(){
    static UINT32 x=123456789,y=362436069,z=521288629,w=88675123;
    UINT32 t = (x^(x<<11));x=y;y=z;z=w;
    return( w=(w^(w>>19))^(t^(t>>8)) );
}
