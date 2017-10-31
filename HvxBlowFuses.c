// NOTE THIS IS A WORK IN PROGRESS AND A BUNCH OF NOTES THROWN TOGETHER

#define UNK_DEVICE 0x8000020000061000
#define FUSE_LOC 0x8000020000020000
#define FUSE_DEVICE 0x8000020000022000
#define HV2 0x0000000200010000
#define VSN 0x710700

// an attempt to make this easier to read
#define read64(addy) *(QWORD*)addy
#define read32(addy) *(DWORD*)addy
#define read16(addy) *(WORD*)addy
#define read8(addy) *(BYTE*)addy
#define write64(addy, data) *(QWORD*)addy = data
#define write32(addy, data) *(DWORD*)addy = data
#define write16(addy, data) *(WORD*)addy = data
#define write8(addy, data) *(BYTE*)addy = data

#define BIT_MASK64(n) (~( ((~0ull) << ((n)-1)) << 1 ))
#define extrdi(inp, n, b) ((inp >> 64-(n+b)) & BIT_MASK64(n))
// eg: extrdi r4, r12, 10, 48 = [r4 = extrdi(r12, 10, 48)]
// eg: r4 = extrdi(r12, 10, 48) = [r4 = ((r12 >> 6) & BIT_MASK64(10))]
#define extldi(inp, n, b) // <---- FINISH IT

void HvxBlowFuses(QWORD var1)
{
	if((*(BYTE*)HSPRG1+0x80) != 0)
		MACHINE_CHECK();
	var1 &= 0xDFFFFFFF;
	if(var1 & 0x40000000)
		var1 = *(DWORD*)0x38;
	if(var1 & 0x10000000)
		var1 &= 0xEFFFFFFF;
	sub_84E0(1);
	QWORD var4 = *(QWORD*)UNK_DEVICE+0x188;
	QWORD var7 = 0;
	if((*(QWORD*)HV2+0x6920) <= VSN)
		var7 = sub_89E0((var4 & 0x3FF000000000000) >> 48, (((var4 & 0x70000000000) >> 40) + (*(BYTE*)HV2+0x6212)) - 0x80);
	else var7 = 0x38;
	sub_8E00(var7, 4, sub_89E0(UNK_DEVICE, 2));
	if((*(QWORD*)HV2+0x6920) >= VSN)
		*(QWORD*)FUSE_DEVICE+0x8 = 0xC2A2619069FF800;
	else
		*(QWORD*)FUSE_DEVICE+0x8 = 0x80E190D38910000;
	QWORD ret = 0;
	if(!(var1 & 0x80))
	{
		if(var1 & 1)
			ret = sub_9738();
		if(!ret)
		{
			if(var1 & 2)
				ret = sub_98D8();
			if(!ret)
			{
				if(var1 & 4)
					ret = sub_9A28(var1);
				if(!ret)
				{
					if(var1 & 8)
						ret = sub_9CC8();
					if(!ret)
					{
						if(var1 & 0x10)
							ret = 0xD;
						else if(var1 & 0x20)
							ret = 0x10;
						else if(var1 & 0x40)
							ret = sub_9DB0();
						if(!ret)
						{
							if(var1 & 0x100000)
								ret = sub_9E78(var1);
							if(!ret)
							{
								if(var1 & 0x20000000)
									ret = sub_A068();
							}
						}
					}
				}
			}
		}
	}
	else ret = sub_9558(-1, ((var1 << 8) | (var1 >> 24)) & 0xFFF);
	QWORD tmp;
	if((var4 & 3) == 1)
		tmp = sub_8C90(UNK_DEVICE, 0);
	else tmp = sub_8C90(UNK_DEVICE, 1);
	sub_8E00((var4 & 0x3FFFFFFFFFFFF00) >> 8, var4 & 3, tmp);
	if(var1 & 0x80)
		sub_A120();
	sub_87A8();
	return ret;
}

// extrdi - (a & bitfield) >> 64-(n+b)
// insrdi
// takes right bit field n from bit (64-n) and inserts it into the target starting at bit b
// a << (64-(b + n)) & 0xFFFFFFFFFFFFFFFF

// sub_8E00 - Called at the beginning and end of HvxBlowFuses

// r3/arg1 = 0x30
// r4/arg2 = 1 ; r26
// r5/arg3 = 5 ; r23

#define DEVICE_60000 0x8000020000060000 // r16
#define DEVICE_61000 0x8000020000061000 // r30
#define DEVICE_50000 0x8000020000050000 // r24
#define DEVICE_30000 0x8000020000030000 // r18
#define DEVICE_48000 0x8000020000048000 // r19
#define DEVICE_0E102 0x80000200E1020000 // r29
#define DEVICE_0E100 0x80000200E1000000 // r17

int sub_8E00(QWORD arg1, QWORD arg2, QWORD arg3)
{
	QWORD v8A60 = sub_8A60(arg1); // r28
	QWORD var61188 = read64(DEVICE_61000+0x188);
	if(read64(HV2+0x6920) >= VSN)
	{
		if(var61188 & 0x80 == 0)
		{
			var61188 = var61188 | 0x80; // used later
			write64(DEVICE_61000+0x188, var61188);
			__eieio();
			__isync();
		}
	}

	QWORD v8C40 = sub_8C40(DEVICE_61000);
	if(var61188 & 7 == arg2)
	{
		if(v8A60 == extrdi(var61188, 6, 50))
			if(v8C40 == arg3)
				return;
	}

	write64(DEVICE_60000+0xB58, read64(DEVICE_60000+0xB58) | 0x8000000000000000);
	__eieio();
	__isync();

	write64(DEVICE_61000+0x50, read64(DEVICE_61000+0x50) & 0xFFFFFFFDFFFFFFFF);
	__eieio()
	__isync();

	write64(DEVICE_61000+0x60, read64(DEVICE_61000+0x60) | 0x0000000200000000);
	__eieio();
	__isync();

	write64(DEVICE_0E102+4, read64(DEVICE_0E102+4) | 1);
	__eieio();
	__isync();

	HvxEnableTimeBase(1);
	sleep(0x2710);
	HvxEnableTimeBase(0);

	if(arg2 != 1)
	{
		write64(DEVICE_30000+0x20, extldi(1, 64, 61));
		__eieio();
		__isync();

		if(read64(HV2+0x6920) < VSN)
		{
			write64(DEVICE_48000, (read64(DEVICE_48000) & extldi(-8, 64, 57)) | extldi(1, 64, 56));
			__eieio();
			__isync();
		}
		else
		{
			write64(DEVICE_30000+0x7000, 0x303C13 << 38);
			__eieio();
			__isync();

			write64(DEVICE_0E100+0x7000, 0x3150D);
			__eieio();
			__isync();
		}
	}
	// 9070
	DWORD r29 = 0xFF8EFB00;
	if(arg3 > v8C40)
	{
		if(read64(HV2+0x6920) + 0xFF8EFB00 <= 0x3FF)
		{
			write64(DEVICE_61000+0x30, arg3 << 60);
			__eieio();
		}
	}

	var61188 = extrdi(var61188, 6, 50);
	QWORD tmp = v8A60;
	if(read64(HV2+0x6920) < VSN)
	{
		if(v8A60 < 0x15)
			tmp = v8A60+0x3E;
		if(var61188 < 0x15)
			var61188 += 0x3E;
		if(tmp < var61188)
		{
			sub_8B80(DEVICE_61000, DEVICE_50000, v8A60);
			var61188 = read64(DEVICE_61000+0x188);
		}
	}
	else if(v8A60 < var61188)
	{
		sub_8B80(DEVICE_61000, DEVICE_50000, v8A60);
		var61188 = read64(DEVICE_61000+0x188);
	}
	// 90F0
	if(var61188 & 7 != arg2)
	{
		write64(DEVICE_50000+0x6020, 0x1047C);
		__eieio();
		__isync();

		while(read64(DEVICE_50000+0x6020) & 0x2000 != 0)
			__db16cyc(); // wait 16 cycles...

		write64(DEVICE_50000+8, 0x78);
		__eieio();
		__isync();

		write64(DEVICE_61000+0x188, ((read64(DEVICE_61000+0x188) & 0xFFFF3FF8) | (arg2 & 7)) | 0xFF0008);
		__eieio();
		PauseProcessor();

		var61188 = read64(DEVICE_61000+0x188) | 0x8000;
		write64(DEVICE_61000+0x188, var61188);
		__eieio();
		__isync();

		write64(DEVICE_50000+0x60, read64(DEVICE_50000+0x50));
		write64(DEVICE_50000+8, 0x7C);
		__eieio();
		__isync();

		write64(DEVICE_50000+0x6020, 0ULL);
		__eieio();
		__isync();
	}

	tmp = extrdi(var61188, 6, 50);
	QWORD tmp2 = v8A60;
	if(read64(HV2+0x6920) < VSN)
	{
		if(tmp < 0x15)
			tmp += 0x3E;
		if(tmp2 < 0x15)
			tmp2 += 0x3E;
		if(tmp < tmp2)
		{
			sub_8B80(DEVICE_61000, DEVICE_50000, v8A60);
			var61188 = read64(DEVICE_61000+0x188);
		}
	}
	else if(tmp < v8A60)
	{
		sub_8B80(DEVICE_61000, DEVICE_50000, v8A60);
		var61188 = read64(DEVICE_61000+0x188);
	}

	if(arg3 < v8C40)
	{
		if(read64(HV2+0x6920) + 0xFF8EFB00 <= 0x3FF)
		{
			write64(DEVICE_61000+0x30, arg3 << 60);
			__eieio();
		}
	}

	if(var61188 & 7 == 1)
	{
		write64(DEVICE_30000+0x10, extldi(-2, 64, 61));
		__eieio();
		__isync();
	}

	if(read64(HV2+0x6920) < VSN)
	{
		tmp = (read64(DEVICE_48000) & extldi(-0xE, 64, 56)) | extldi(1, 64, 57);
		write64(DEVICE_48000, tmp);
		__eieio();
		__isync();
	}
	else
	{
		write64(DEVICE_30000+0x7000, 0x948C1 << 42);
		__eieio();
		__isync();

		write64(DEVICE_0E100+0x7000, 0x3150D);
		__eieio();
		__isync();
	}

	write64(DEVICE_60000+0xB58, read64(DEVICE_60000) & 0x7FFFFFFFFFFFFFFF);
	__eieio();
	__isync();

	write64(DEVICE_0E102+4, read64(DEVICE_0E102+4) & 0xFFFFFFFFFFFFFFFE);
	__eieio();
	__isync();

	return;
}

int sub_9558(DWORD arg1, DWORD arg2) // sub_9558
{
	QWORD var1 = 0x8000000000000000;
	for(int i = 0;i < 3;i++)
	{
		if(*(QWORD*)HV2+0x6920 >= VSN)
		{
			QWORD tmp = *(QWORD*)UNK_DEVICE+0x188;
			*(QWORD*)UNK_DEVICE+0x188 = tmp & -0x81;
			__eieio();
			__isync();
		}

		// check if device is free
		while(*(QWORD*)FUSE_DEVICE != 0)
			continue;

		// write to device
		QWORD tmp = *(QWORD*)FUSE_DEVICE+0x18;
		*(QWORD*)FUSE_DEVICE+0x18 = tmp | var1;
		__eieio();

		// ?
		if(arg1 != -1)
		{
			HvxEnableTimeBase(1);
			sleep(0x3E8);
			HvxEnableTimeBase(0);
			tmp = arg1 << 3;
			*(QWORD*)FUSE_LOC+tmp = 1;
			__eieio();
		}
		else
		{
			HvxEnableTimeBase(1);
			if(arg2 > 0xFFF)
				sleep(0xFFF*0x3E8);
			else sleep(arg2*0x3E8);
			HvxEnableTimeBase(0);
		}

		// check if device is free
		while(*(QWORD*)FUSE_DEVICE != 0)
			continue;

		// write to device
		tmp = *(QWORD*)FUSE_DEVICE+0x18;
		*(QWORD*)FUSE_DEVICE+0x18 = tmp & 0x7FFFFFFFFFFFFFFF;
		__eieio();

		HvxEnableTimeBase(1);
		sleep(0x7D0);
		HvxEnableTimeBase(0);

		// check if device is free
		while(*(QWORD*)FUSE_DEVICE != 0)
			continue;

		if(*(QWORD*)HV2+0x6920 >= VSN)
		{
			tmp = *(QWORD*)UNK_DEVICE+0x188;
			*(QWORD*)UNK_DEVICE+0x188 = tmp | 0x80;
			__eieio();
			__isync();
		}

		// only does 1 loop if r3 = -1?
		if(arg1 = -1)
			return 1;

		// write to device at 0x10
		*(QWORD*)FUSE_DEVICE+0x10 = var1;
		__eieio();

		// check if device is free
		while(*(QWORD*)FUSE_DEVICE != 0)
			continue;

		// check device ?
		tmp = *(QWORD*)FUSE_LOC+((r3 & 0xFFFFFFFF) << 3);
		if(tmp & (var1 >> (r3 & 0x3F)) != 0)
			return 1;
	}
	return 0;
}

int sub_9738()
{
	if((*(QWORD*)HV2+0x6920) > 0x710900 || (*(DWORD*)HV2+0x64D4) != 0)
		return 1;
	QWORD fuses[12] = { 0 };
	QWORD fuse = 0;
	for(int i;(i*40) < 0x300;i++)
	{
		fuse = (i*0x40) << 3;
		fuses[i] = *(QWORD*)FUSE_LOC + fuse;
	}
	fuse = fuses[1];
	QWORD var1 = *(BYTE*)0x74;
	for(int i = 0;i < 8;i++)
	{
		if(fuse & 0xFF) // devkit/retail line check?
		{
			if((fuse & 0xF0) || (fuse & 0xF))
				return 2;
			if(fuse & 0xF)
				r8 = 1;
			else r8 = 0;
			r7 = ~var1;
			if(r7 & 0x1FFFFFFFF != r8)
				return 2;
		}
		fuse = fuse >> 8;
		var1 = ((var1 << 1) | (var1 >> 31)) & 0x7F;
	}
	QWORD var2 = *(BYTE*)0x74;
	QWORD var3 = 0x7F;
	QWORD var4 = 0;
	fuse = fuses[1];
	for(int i = 0;i < 8;i++)
	{
		if(var2 & 1)
		{
			var4 = (fuse & 0xFFFF000000000000) >> 60;
			var3 -= 1;
		}
		else var4 = fuse & 0xF;
		QWORD cFail = 0;
		for(int ii = 0;ii < 4;ii++)
		{
			if(var4 & 1)
			{
				if(!BurnFuses(var3, 0))
				{
					cFail++;
					if(cFail > 1)
						return 3;
				}
			}
			var3--;
			var4 = var4 << 1;
		}
		if(var2 & 1 == 0)
			var3 = var3 - 4;
		var2 = ((var2 << 1) | (var2 >> 31)) & 0x7F;
		fuse = fuse << 8;
	}
	return 0;
}
