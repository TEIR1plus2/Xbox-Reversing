// version 0x583
// only part reversed is how it loads the CB, there are other functions not reversed.

#define STACK 0x800002000001F700 // r1
#define TOCP 0x8000020000000000; // r2
#define SRAM 0x8000020000010000;
#define NAND 0x80000200C8000000;
#define SOC_UNK 0x8000020000061008;

BYTE Salt[0xB] = <redacted>;

BYTE BLKey[0x10] = { <redacted> };

XECRYPT_RSAPUB_2048 xRSA;
xRSA = <redacted>

typedef struct _BLHeader
{
	WORD Magic;			// 0 : 2
	WORD Version;		// 2 : 2
	DWORD Flags;		// 4 : 4
	DWORD EntryPoint;	// 8 : 4
	DWORD Size;			// 0xC : 4
	BYTE key[0x10];		// 0x10 : 0x10
	QWORD Pad[4];		// 0x20 : 0x20
	XECRYPT_SIG Sig;	// 0x40 : 0x100
	// Header: 0x140
}BLHeader, *PBLHeader;

void POST(QWORD postCode)
{
	*(QWORD*)0x8000020000061010 = (postCode << 56);
}

void PanicGen()
{
	while(1)
		continue;
}

void Panic(QWORD postCode)
{
	POST(postCode);
	PanicGen();
}

QWORD ReadHighestByte(QWORD Address)
{
	return ((*(QWORD*)Address) >> 56);
}

DWORD sub_36A8()
{
	DWORD ret = ReadHighestByte(*(QWORD*)SOC_UNK);
	if((ret & 0x80) != 0)
		ret = (~ret) & 0xFF;
	return = ret & 0xF8;
}

bool CB_VerifyOffset(DWORD offset, DWORD arg2)
{
	if(offset != (offset + 0xF) & 0xFFFFFFF0)
		return false;
	if(offset - 0x80 > 0x7FFFF7F)
		return false;
	if((arg2 + 0xF) & 0xFFFFFFF0 >= offset - 0x8000000)
		return false;
	return true;
}

// Copies by 0x10 byte blocks
// cBlocks: how many 0x10 byte blocks to copy
void CB_Copy(QWORD dest, QWORD src, DWORD cBlocks)
{
	for(int i = 0; i < cBlocks; i++)
	{
		*(QWORD*)dest+(i*0x10) = *(QWORD*)src+(i*0x10);
		*(QWORD*)dest+(i*0x10)+8 = *(QWORD*)src+(i*0x10)+8;
	}
}

void CB_Jump(QWORD address, QWORD arg2)
{
	// grabs data from the CB before nulling the area
	QWORD r27 = *(QWORD*)SRAM+0x20;
	QWORD r28 = *(QWORD*)SRAM+0x28;
	QWORD r29 = *(QWORD*)SRAM+0x30;
	QWORD r30 = *(QWORD*)SRAM+0x38;
	// nulls 0x20-0x140(?) 
	QWORD tmp = SRAM+0x20;
	for(int i = 0; i < 0x12; i++)
	{
		*tmp+(i*0x10) = 0ULL;
		*tmp+(i*0x10)+8 = 0ULL;
	}

	// check the size
	tmp = (((*(DWORD*)SRAM+0xC) + 0xF) & 0xFFFFFFF0);
	if(tmp >= 0x10000)
		Panic(0x98);

	// nulls the area after the CB
	QWORD addy = tmp + SRAM;
	for(int i = 0; i < (tmp - 0x10000) >> 4; i++)
	{
		*addy+(i*0x10) = 0ULL;
		*addy+(i*0x10)+8 = 0ULL;
	}

	// sets up tlb page
	// sets registers r0-r26 to 0
	// jump to CB
	goto (address & 0xFFFF) + 0x2000000;
	return;
}

void CB_Load()
{
	POST(0x11);
	FSB1(); // sub_3450

	POST(0x12);
	FSB2(); // sub_34D0

	POST(0x13);
	FSB3(); // sub_35A8

	POST(0x14);
	FSB4(); // sub_3658

	POST(0x15);
	DWORD cbOffset = *(DWORD*)NAND+8; // r25
	if(!CB_VerifyOffset(cbOffset, 0x10))
		Panic(0x94);

	POST(0x16);
	QWORD cbNAddy = NAND+cbOffset; // r26
	CB_Copy(SRAM, cbNAddy, 1);

	POST(0x17);
	PBLHeader cbHeader = (PBLHeader)SRAM;
	if((cbHeader->Size - 0x264) > 0xBD9C
		|| (cbHeader->Magic & 0xFFF) != 0x342
		|| (cbHeader->EntryPoint & 0x3)
		|| (cbHeader->EntryPoint) < 0x264 // on slim its < 0x3B8
		|| (cbHeader->Size & 0xFFFFFFFC) >= (cbHeader->EntryPoint & 0x3) // doesn't make sense, check later offset 0x4340 - On slim it makes sense: if(entrypoint >= size & 0xFFFFFFFC) panic
		|| !CB_VerifyOffset(cbOffset, cbHeader->Size))
		Panic(0x95);

	POST(0x18);
	QWORD tmp = ((cbHeader->Size + 0xF) & 0xFFFFFFF0);
	CB_Copy(SRAM+0x10, cbNAddy+0x10, (tmp - 0x10) >> 4);

	POST(0x19);
	// overwrites the old key with the new one
	XeCryptHmacSha(BLKey, 0x10, cbHeader->key, 0x10, 0, 0, 0, 0, cbHeader->key, tmp);

	POST(0x1A);
	XECRYPT_RC4_STATE rc4;
	XeCryptRc4Key(&rc4, cbHeader->key, 0x10); // key = HmacSha(1BLKey, cbKey, 0x10)

	POST(0x1B);
	XeCryptRc4Ecb(&rc4, SRAM+0x20, tmp - 0x20); // Decrypts everything after the header

	POST(0x1C);
	BYTE Hash[0x14] = { 0 };
	XeCryptRotSumSha(SRAM, 0x10, SRAM+0x140, tmp - 0x140, Hash, 0x14); // hashes everything after the sig

	POST(0x1D);
	if(XeCryptBnQwBeSigDifference(cbHeader->Sig, Hash, Salt, &xRSA)) // checks sig against hash with public rsa key
		Panic(0x96);

	POST(0x1E);
	CB_Jump(cbHeader->EntryPoint, tmp+cbOffset); // sets up tbl page and loads some registers before jumping to cb
	return;
}

void BL_1()
{
	// thread check?

	POST(0x10); // entered 1bl

	// null the sram area
	for(int i = 0; i < 0x1000; i++)
	{
		*(QWORD*)SRAM+(i*0x10) = 0ULL;
		*(QWORD*)SRAM+((i*0x10)+8) = 0ULL;
	}

	DWORD tmp = sub_36A8();
	if((tmp & 0xFF) == 0x50)
		sub_4098(tmp); // look into later

	// load and execute the CB
	CB_Load();

	// CB_Load shouldn't return...
	PanicGen();

	return;
}
