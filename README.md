# 1BL
The first bit of code executed on the xbox 360 on power-on. It was dumped from the ROM inside the CPU, mapped to memory location 0x8000020000000000.

This bootloader sets up some hardware registers and then loads, decrypts, and verifies the signature of the 2nd bootloader, the CB. The CB is loaded into address 0x8000020000010000.

Key to decrypt the CB = Key found in 1BL + salt found in the header of CB.

# CB_A
## Retail
On more recent kernels, the CB was split into two bootloaders, CB_A and CB_B. The reason for this is largely unknown because the CB_A is loaded the exact same way as the old CB. The Job of the CB_A is to relocate itself to 0x800002000001C000, null the area it was originally loaded at (0x8000020000010000), read the CPUKey from the efuses and use it to decrypt the CB_B. The CB_B is then hashed and the hash is compared against a stored inside the CB_A, if they match, it continues, otherwise it panics. This comparison is also where the RGH2 hack takes place. The CB_B is loaded into the old address of CB_A. Normally the CPUKey is different for every console and is burned in the efuses during manufacturing.

## MFG
At one point a console that was sent to be destroyed was recovered and it was discovered to have a different CB_A on its NAND, its known as the mfg CB_A. This version is signed and can be run on retail consoles. Why is this important? On the mfg CB_A there is a very important difference than the normal CB_A, it does not read the efuses from the CPU but continues to decrypt and load the CB_B like normal, meaning it does not use the CPUKey in the decryption process of the CB_B. This means that when using a hack like the RGH2, where we have control over the CB_B, we can make the rest of the bootchain ignore the CPUKey as well, meaning that we can create generic NAND images that can work on all consoles with the same motherboard. This can be useful if someone lost their CPUKey and their console will not boot, this would allow them to flash a bootable NAND and recover.

Programs like xebuild do not build NAND images with the mfg bootloader by default but have the ability to after changing some options. The reason this is not used by default is because it was agreed that if people are given a generic image for every console, they will likely lose some files that contain console specific information. These files can be ignored on hacked consoles but would be needed if the user wanted to return their console to a retail state.

## Here is how the two bootloaders differ in the decryption process:

Retail decryption key for the CB_B = CB_A Key + CPUKey + Salt in the CB_B Header

MFG decryption key for the CB_B = CB_A Key + Salt in the CB_B Header

# HvxBlowFuses
Syscall found in the hypervisor to blow efuses on the console through software. It seems to set some hardware registers to activate a circuit found near the CPU, this is largly believed to supply the voltage needed to burn the efuses.

This is a work in progress still...
