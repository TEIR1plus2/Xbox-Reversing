
#define PVR_500 0x710500
#define PVR_700 0x710700
#define PVR_900 0x710900
#define HSPRG1 mfspr(SPR_HSPRG1)

// 0x710800 on trinity RGH2
QWORD HvpPVRStore = read64(0x200016920);

// seems to be 0 every time I check it - don't know if something else is supposed to set it
DWORD HvpBurnFuseLock = read32(0x2000164D4);

BYTE HvpPvtUpdateSequence = read8(0x2000164A0);

typedef struct _FUSE_DEVICE
{
	QWORD SoftFuseChain[0x300]; // 0-0x1800
	QWORD Reserved[0x40]; // 0x1800-0x2000
	QWORD SenseState; // 0x2000
	QWORD Precharge; // 0x2008
	QWORD BlowEnable; // 0x2010
}FUSE_DEVICE, *PFUSE_DEVICE;

void HvxBlowFuses(DWORD BlowFlags)
{
	PFUSE_DEVICE pFuseDev = (PFUSE_DEVICE)__REG_20000;

	PTHREAD_STATE pThreadState = (PTHREAD_STATE)HSPRG1;
	if(pThreadState->PID != 0)
		MACHINE_CHECK();

	BlowFlags &= 0xDFFFFFFF;
	if(BlowFlags & 0x40000000)
		BlowFlags = *(DWORD*)0x38;
	if(BlowFlags & 0x10000000)
		BlowFlags &= 0xEFFFFFFF;

	HvpSaveThread(1);

	QWORD vrm_config = _REG_61188;

	BYTE nVID = 0;
	if(HvpPVRStore >= PVR_700)
		nVID = HvpMakeVID((vrm_config >> 48) & 0x3F, (((vrm_config >> 40) & 7) + (*(BYTE*)HV2+0x6212)) - 0x80);
	else 
		nVID = 0x38;

	HvpUpdateVRM(nVID, 4, HvpGetVSpeed(__REG_61188, 2));

	if(HvpPVRStore >= PVR_700)
		pFuseDev->Precharge = 0x80E190D38910000;
	else
		pFuseDev->Precharge = 0xC2A2619069FF800;

	QWORD ret = 0; // 0x1000CF
	if((BlowFlags & 0x80) == 0)
	{
		if(BlowFlags & 1) // burn line 1 (retail/devkit)
			ret = HvpBurnFuseline01();
		if(BlowFlags & 2 && !ret) // burn line 2 (CB LDV)
			ret = HvpBurnFuseline02_Seq();
		if(BlowFlags & 4 && !ret) // generate and burn cpukey (does it burn all lines?)
			ret = HvpBurnCPUKey(BlowFlags);
		if(BlowFlags & 8 && !ret) // burn line 0 (disable cpu jtag?) (Only burns the last 56 bits)
			ret = HvpBurnFuseline00();
		if(BlowFlags & 0x10 && !ret)
			ret = 0xD;
		if(BlowFlags & 0x20 && !ret)
			ret = 0x10;
		if(BlowFlags & 0x40 && !ret) // burn new update sequence
			ret = HvpBurnFuseline07_Seq();
		if(BlowFlags & 0x100000 && !ret) // seems to be a diagnostic setting?
			ret = sub_9E78(BlowFlags);
		if(BlowFlags & 0x20000000 && !ret) // Burn initial fuse pattern
			ret = HvpBurnFuseline01_ALL();
	}
	else
		ret = HvpBurnFuse(-1, ROTL32(BlowFlags, 8) & 0xFFF);

	QWORD tmp;
	if((var4 & 3) == 1)
		tmp = HvpGetVSpeed(__REG_61188, 0);
	else 
		tmp = HvpGetVSpeed(__REG_61188, 1);

	HvpUpdateVRM((vrm_config & 0x3FFFFFFFFFFFF00) >> 8, vrm_config & 3, tmp);

	if(BlowFlags & 0x80)
		sub_A120();

	HvpRestoreThread();
	return ret;
}

// sub_89E0
QWORD HvpMakeVID(QWORD arg1, QWORD arg2)
{
	QWORD ret = 0;
	if(HvpPVRStore >= PVR_700)
	{
		ret = arg1 + arg2;
		if(ret > 0x3F)
			return 0x3F;
		if(ret >= 1)
			return ret;
		return 1;
	}
	else
	{
		if(arg1 < 0x15)
			arg1 += 0x3E;

		ret = arg1 + arg2;

		if(ret > 0x52)
			return (0x52 - 0x3E);
		if(ret > 0x14)
			return ret;
		return 0x15;
	}
}

// sub_8C90
BYTE HvpGetVSpeed(QWORD reg, BYTE arg2)
{
	// NOTE: Ignoring this because the function is always sent the same addresses
	QWORD VRMReg = reg + 0x188; // _REG_61188

	if(HvpPVRStore < PVR_500)
		return 0;
	if(HvpPVRStore >= PVR_900)
		return sub_8C40();

	BYTE var1 = 0;
	if(arg2 == 0)
		var1 = read8(0x200016210);
	else if(arg2 == 1)
		var1 = read8(0x200016213);
	else
		var1 = read8(0x200016211);

	if(var1 == 0xFF)
	{
		vrm_config = _REG_61188;
		if(HvpPVRStore >= PVR_700)
			var1 = read8(0x200011AE8 + ((vrm_config >> 48) & 0x3F));
		else
		{
			DWORD offset = 0;
			if(arg2 == 2)
				offset = 0x38;
			else if(arg2 == 1)
				offset = sub_89E0((vrm_config >> 48) & 0x3F, (vrm_config >> 39) & 0xE);
			else
				offset = (vrm_config >> 48) & 0x3F;
			var1 = read8(0x200011AA8 + offset);
		}
	}
	return var1;
}

// sub_8C40
DWORD HvpGetCurrentVSpeed(QWORD reg)
{
	// NOTE: Ignoring this because the function is always sent the same addresses
	QWORD CPUReg = reg + 0x30; // _REG_61030

	if(HvpPVRStore < PVR_500)
		return 0;

	QWORD qwVal = _REG_61030;
	qwVal &= 0x7FFFFFFFFFFFFFFF;

	//cntlzd
	BYTE lz = 0;
	for(int i = 0;i < 64;i++)
	{
		if(qwVal & 0x8000000000000000)
			break;
		qwVal <<= 1;
		lz++;
	}

	if(HvpPVRStore >= PVR_900)
		qwVal = 0xD-lz;
	else
		qwVal = 9-lz;
	return (DWORD)(qwVal & 0xFFFFFFFF);
}

// r3/arg1 = 0x30
// r4/arg2 = 1 ; r26
// r5/arg3 = 5 ; r23
// sub_8E00
int HvpUpdateVRM(QWORD arg1, QWORD Speed, QWORD VSpeed)
{
	QWORD v8A60 = sub_8A60(arg1); // r28
	QWORD vrm_config = _REG_61188; // _REG_61188
	if(HvpPVRStore >= PVR_700)
	{
		if(vrm_config & 0x80 == 0)
		{
			vrm_config = vrm_config | 0x80; // used later
			_REG_61188 = vrm_config; // ei_is
		}
	}

	QWORD CurSpeed = HvpGetCurrentVSpeed(DEVICE_61000);
	if(vrm_config & 7 == Speed)
	{
		if(v8A60 == (vrm_config >> 8) & 0x3F)
			if(CurSpeed == VSpeed)
				return;
	}

	_REG_60B58 = _REG_60B58 | 0x8000000000000000; // ei_is

	_REG_61050 = _REG_61050 & 0xFFFFFFFF7FFFFFFF; // ei_is

	_REG_61060 = _REG_61060 | 0x0000000080000000; // ei_is

	_REG_0E102_0004 = _REG_0E102_0004 | 1; // ei_is

	HvxEnableTimeBase(1);
	HvpDelay(0x2710);
	HvxEnableTimeBase(0);

	if(Speed != 1)
	{
		_REG_30020 = 8ull; // ei_is

		if(HvpPVRStore < PVR_700)
		{
			_REG_48000 = (_REG_48000 & 0xFFFFFFFFFFFFFC7F) | 0x100; // ei_is
		}
		else
		{
			_REG_37000 = 0xC0F04C000000000; // ei_is

			_REG_0E100_7000 = 0x3150D; // ei_is
		}
	}
	// 9070
	if(VSpeed > CurSpeed)
		if(HvpPVRStore + 0xFF8EFB00 <= 0x3FF)
			_REG_61030 = VSpeed << 60; // ei

	vrm_config = (vrm_config >> 8) & 0x3F
	QWORD tmp = v8A60;
	if(HvpPVRStore < PVR_700)
	{
		if(v8A60 < 0x15)
			tmp = v8A60+0x3E;
		if(vrm_config < 0x15)
			vrm_config += 0x3E;
		if(tmp < vrm_config)
		{
			HvpSetVRMSpeed(DEVICE_61000, DEVICE_50000, v8A60);
			vrm_config = _REG_61188;
		}
	}
	else if(v8A60 < vrm_config)
	{
		HvpSetVRMSpeed(DEVICE_61000, DEVICE_50000, v8A60);
		vrm_config = _REG_61188;
	}

	if(vrm_config & 7 != Speed)
	{
		_REG_56020 = 0x1047C; // ei_is

		while(_REG_56020 & 0x2000) // device status?
			continue;

		_REG_50008 = 0x78; // ei_is

		_REG_61188 = (_REG_61188 & 0xFFFF3FF8) | ((Speed & 7) | 0xFF0008); // ei
		PauseProcessor();

		vrm_config = _REG_61188 | 0x8000;
		_REG_61188 = vrm_config; // ei_is

		_REG_50060 = _REG_50050;
		_REG_50008 = 0x7C; // ei_is

		_REG_56020 = 0; // ei_is
	}

	vrm_config = (vrm_config >> 8) & 0x3F;
	QWORD tmp2 = v8A60;
	if(HvpPVRStore < PVR_700)
	{
		if(vrm_config < 0x15)
			vrm_config += 0x3E;
		if(tmp2 < 0x15)
			tmp2 += 0x3E;
		if(vrm_config < tmp2)
		{
			sub_8B80(DEVICE_61000, DEVICE_50000, v8A60);
			vrm_config = _REG_61188;
		}
	}
	else if(vrm_config < v8A60)
	{
		sub_8B80(DEVICE_61000, DEVICE_50000, v8A60);
		vrm_config = _REG_61188;
	}

	if(VSpeed < CurSpeed)
		if(HvpPVRStore + 0xFF8EFB00 <= 0x3FF)
			_REG_61030 = VSpeed << 60; // ei

	if(vrm_config & 7 == 1)
		_REG_30010 = 0xFFFFFFFFFFFFFFF7; // ei_is

	if(HvpPVRStore < PVR_700)
		_REG_48000 = (_REG_48000 & 0xFFFFFFFFFFFFF2FF) | 0x80; // ei_is
	else
	{
		_REG_37000 = 0x2523040000000000; // ei_is

		_REG_0E100_7000 = 0x3150D; // ei_is
	}

	_REG_60B58 = _REG_60B58 & 0x7FFFFFFFFFFFFFFF; // ei_is

	_REG_0E102_0004 = _REG_0E102_0004 & 0xFFFFFFFFFFFFFFFE; // ei_is

	return;
}

// sub_8B80
void HvpSetVRMSpeed(QWORD add1, QWORD add2, QWORD val)
{
	// NOTE: Ignoring these because the function is always sent the same addresses
	QWORD IRQReg = add2 + 0x6020; // _REG_56020
	QWORD VRMReg = add1 + 0x188; // _REG_61188

	QWORD vrm_config = _REG_61188;
	_REG_56020 = 0x400;

	while(_REG_56020 & 0x2000);

	val = ROTL32(val, 8) & 0xF8003FFF
	val |= 0xFF004000;
	vrm_config &= 0xFFFF40F7;
	vrm_config |= val;
	_REG_61188 = vrm_config;

	while(!(_REG_56020 & 0x2000));

	_REG_61188 = _REG_61188 | 0x8000;

	_REG_56020 = 0;

	return;
}

// if FuseBitPos != -1, SleepTime is ignored
int HvpBurnFuse(DWORD FuseBitPos, DWORD SleepTime) // sub_9558
{
	QWORD Mask = 0x8000000000000000;
	PFUSE_DEVICE pFuseDev = (PFUSE_DEVICE)__REG_20000;

	for(int i = 0;i < 3;i++)
	{
		if(HvpPVRStore >= PVR_700)
			_REG_61188 = _REG_61188 & -0x81; // ei_is

		// check if device is ready
		while(pFuseDev->SenseState)
			continue;

		// write to device
		pFuseDev->BlowEnable = pFuseDev->BlowEnable | Mask; // ei

		// enter fuse to burn (or not if r3 = -1)
		if(FuseBitPos != -1)
		{
			HvxEnableTimeBase(1);
			HvpDelay(0x3E8);
			HvxEnableTimeBase(0);
			pFuseDev->SoftFuseChain[FuseBitPos] = 1;
		}
		else
		{
			HvxEnableTimeBase(1);
			if(SleepTime > 0xFFF)
				HvpDelay(0xFFF*0x3E8);
			else
				HvpDelay(SleepTime*0x3E8);
			HvxEnableTimeBase(0);
		}

		// check if device is ready
		while(pFuseDev->SenseState)
			continue;

		// write to device
		pFuseDev->BlowEnable = pFuseDev->BlowEnable & 0x7FFFFFFFFFFFFFFF; // ei

		HvxEnableTimeBase(1);
		HvpDelay(0x7D0);
		HvxEnableTimeBase(0);

		// check if device is ready
		while(pFuseDev->SenseState)
			continue;

		if(HvpPVRStore >= PVR_700)
			_REG_61188 = _REG_61188 | 0x80; // ei_is

		// -1 seems like a diagnostic setting
		if(FuseBitPos = -1)
			return 1;

		// write to device at 0x10
		pFuseDev->Precharge = Mask; // ei

		// check if device is ready
		while(pFuseDev->SenseState)
			continue;

		// check if our fuse burned
		QWORD FuseStatus = pFuseDev->SoftFuseChain[FuseBitPos];
		if(FuseStatus & (Mask >> (FuseBitPos & 0x3F)))
			return 1; // success
	}
	return 0;
}

// sub_9738
int HvpBurnFuseline01()
{
	if(HvpPVRStore > PVR_900 && HvpBurnFuseLock != 0)
		return 1;

	QWORD fuses[12] = { 0 };
	for(int i = 0;i < 12;i++)
		fuses[i] = getFuseline(i);

	QWORD fuse = fuses[1];
	PBYTE fusebytes = ((PBYTE)&fuse);
	DWORD bConsoleInfo = *(BYTE*)0x74;
	for(int i = 0;i < 8;i++)
	{
		BYTE curByte = fusebytes[8 - i];
		if(curByte) // devkit/retail line check?
		{
			if((curByte & 0xF0) && (curByte & 0xF))
				return 2;

			BYTE curBit = 0;
			if(curByte & 0xF)
				curBit = 1;
			else 
				curBit = 0;

			if((~bConsoleInfo) & 1 != curBit)
				return 2;
		}

		bConsoleInfo = ROTL32(bConsoleInfo, 1) & 0x7F;
	}

	bConsoleInfo = *(BYTE*)0x74;
	QWORD fuseBitPos = 0x7F;
	fuse = fuses[1];
	for(int i = 0;i < 8;i++)
	{
		BYTE curByte = fusebytes[8 - i];
		if(bConsoleInfo & 1)
		{
			curByte = (curByte >> 4) & 0xF;
			fuseBitPos -= 4;
		}
		else 
			curByte &= 0xF;

		QWORD BurnFail = 0;
		for(int j = 0;j < 4;j++)
		{
			if(curByte & 1)
			{
				if(!HvpBurnFuse(fuseBitPos, 0))
				{
					BurnFail++;
					if(BurnFail > 1)
						return 3;
				}
			}
			fuseBitPos--;
			curByte >>= 1;
		}

		if(!bConsoleInfo & 1)
			fuseBitPos -= 4;

		bConsoleInfo = ROTL32(bConsoleInfo, 1) & 0x7F;
	}
	return 0;
}

// sub_9CC8
int HvpBurnFuseline00()
{
	if(HvpPVRStore >= PVR_900 && HvpBurnFuseLock != 0)
		return 0xA;

	QWORD pqwFuses[12] = { 0 };
	for(int i = 0;i < 12;i++)
		pqwFuses[i] = getFuseline(i);

	QWORD qwFuse = pqwFuses[0];
	BYTE bFuseBitPos = 0x3F;
	BYTE bBurnFail = 0;
	for(int i = 0;i < 56;i++)
	{
		if(!(qwFuse & 1))
			if(!HvpBurnFuse(bFuseBitPos, 0))
				bBurnFail++;

		qwFuse >>= 1;
		bFuseBitPos--;
	}

	if(bBurnFail >= 0xC) // 12 chances to fail?
		return -1;
	else
		return 0;
}

// sub_9DB0
int HvpBurnFuseline07_Seq()
{
	if(HvpPVRStore >= PVR_900 && HvpBurnFuseLock != 0)
		return 0x13;

	BYTE bNewUpdateSequence = read8(0x200016464);
	if(bNewUpdateSequence <= HvpPvtUpdateSequence)
		return 0;

	BYTE BurnFail = 0;
	WORD wFuseBitPos = (bNewUpdateSequence << 2) + 0x1BF;
	for(int i = 0;i < 4;i++)
	{
		if(!HvpBurnFuse(wFuseBitPos, 0))
			BurnFail++;
		wFuseBitPos--;
	}
	if(BurnFail == 4)
		return 0x15;

	WORD wHvFlags = read16(6);
	wHvFlags |= 0x8000;
	write16(6, wHvFlags);
	write16(read64(0x200016150), wHvFlags);

	return 0;
}

// sub_A068
int HvpBurnFuseline01_ALL()
{
	QWORD pqwFuses[12] = { 0 };
	for(int i = 0;i < 12;i++)
		pqwFuses[i] = getFuseline(i);

	QWORD qwFuse = pqwFuses[1];
	BYTE BurnFail = 0;
	BYTE FuseBitPos = 0x7F;
	for(int i = 0;i < 64;i++)
	{
		if(qwFuse & 1 != 0)
		{
			if(!HvpBurnFuse(FuseBitPos, 0))
				BurnFail++;
		}

		FuseBitPos--;
		qwFuse >>= 1;
	}

	if(BurnFail > 0)
		return -1;
	else
		return 0;
}

// sub_98D8
int HvpBurnFuseline02_Seq()
{
	if(HvpPVRStore >= PVR_900 && HvpBurnFuseLock != 0)
		return 4;

	QWORD pqwFuses[12] = { 0 };
	for(int i = 0;i < 12;i++)
		pqwFuses[i] = getFuseline(i);

	QWORD qwFuse = pqwFuses[2];
	BYTE FuseBitPos = 16;
	for(int i = 0;i < 16;i++)
	{
		if(qwFuse & 0xF)
			break;

		FuseBitPos--;
		qwFuse >>= 4;
	}

	BYTE ConsoleSequence = read8(0x75);
	if(FuseBitPos > ConsoleSequence)
		return 5;
	if(ConsoleSequence > 16)
		return 5;
	if(!ConsoleSequence)
		return 0;

	FuseBitPos = 0x7F + (ConsoleSequence << 2);
	DWORD LShift = ((16 - ConsoleSequence) << 2) & 0xFFFFFFFF;
	BYTE BurnFail = 0;
	qwFuse = pqwFuses[2] >> LShift;
	for(int i = 0;i < 4;i++)
	{
		if(qwFuse & 1 == 0)
		{
			if(!HvpBurnFuse(FuseBitPos, 0))
			{
				BurnFail++;
				if(BurnFail > 1)
					return 6;
			}
		}
		FuseBitPos--;
		qwFuse >>= 1;
	}
	return 0;
}
