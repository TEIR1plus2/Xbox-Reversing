// this file contains several security related things related to online cheating that can be abused
// removed until I can filter through it and decide what should be here and what shouldn't
// for now here's the prototypes of the functions

// 17511
// HV utility functions


// Xbox doesn't write cachelines back unless a dcbst instruction is used
// or unless the data was being forced out of the cacheline
// This will make sure your RAM is up to date with the cache
// Used to write cachelines for a given memory range to RAM
void HvpSaveCachelines(QWORD* pAddy, DWORD cAddy);

// Writes zeroed cache blocks directly to RAM without 
// loading RAM into cache, zeroing, writing back to memory
// Efficient way to zero memory blocks
void HvpZeroCacheLines(QWORD pAddy, DWORD cAddy);

// Used to invalidate cachelines for a given address range
// Takes byte count as cAddy input, NOT CACHELINE COUNT
void HvpInvalidateCachelines(QWORD pAddy, DWORD cAddy);

// Relocates data in 0x80 byte blocks and invalidates the old location
void HvpRelocateCacheLines(QWORD* pSrc, QWORD* pDest, DWORD cCount);

// Move data to an internal memory area, return new address
QWORD HvpRelocatePhysicalToInternal(QWORD pbBuf, DWORD cbBuf, BYTE bPage);

// Get the real address of a physical address
// The HV works with memory paging disabled
QWORD HvpPhysicalToReal(DWORD pAddy, DWORD cAddy);

// Gets non-zero timebase value
QWORD HvpGetTimebase();

QWORD HvpGetFlashBaseAddress();
QWORD HvpGetSocMMIORegs(DWORD offset);
QWORD HvpBuildPciConfigRegs(DWORD offset);
QWORD HvpGetHostBridgeRegs(DWORD offset);

// work in progress...
