// version 0x583
// only part reversed is how it loads the CB, there are other functions not reversed.

#define STACK 0x800002000001F700 // r1
#define TOCP 0x8000020000000000; // r2
#define SRAM 0x8000020000010000;
#define NAND 0x80000200C8000000;
#define SOC_UNK 0x8000020000061008;

BYTE Salt[0xB] = "XBOX_ROM_B";

BYTE BLKey[0x10] = { 0xDD, 0x88, 0xAD, 0x0C, 0x9E, 0xD6, 0x69, 0xE7, 0xB5, 0x67, 0x94, 0xFB, 0x68, 0x56, 0x3E, 0xFA };

XECRYPT_RSAPUB_2048 xRSA;
xRSA.cqw = 32;
xRSA.dwPubExp = 65537;
xRSA.qwReserved = 0xFD336B67936AA211;
xRSA.aqw[] = { 
			0xE98DB5DCAF388EF1, 0x389E28CB4A11C822, 0x52E11F53455660A2, 0x52D4D1684ECC8099, 
			0xD75C40C5AF730CCF, 0x4406B06D16910838, 0xB3002DBCEB1D0C1D, 0xC5C6680B804C620B, 
			0x7EE8720CCF1DB4BD, 0xEE4B1136D1C9921F, 0xE9AEC0515251F723, 0xD6BCF4E9588740B1, 
			0x02665A43EB675F50, 0x9432347AA750D9B4, 0x144EB002318BA700, 0x9A12C83B8F76E48F, 
			0x33B5CD0C246D2AE5, 0x57A044767841F48F, 0xCB3AB50EA1A2566D, 0x17DB32CCB85A5FAE, 
			0xED9A62315D887F6D, 0x9A5380B034C74251, 0x2D944D8609328F71, 0xA7BA166CE6DC6B64, 
			0x617D16B52051D0B1, 0x1FFE1E35569A764D, 0x627F5DF4B87DC418, 0x2C81B7AFE47D135D, 
			0xF40F63053F1AEDED, 0x4BEEFD6D74E6A592, 0xA799817395D8C7A5, 0xA1C77B0905854104
};

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