# What is this?
This is my collection of research on the lower level Xbox 360 system. This will mainly involve the bootloaders and Hypervisor related stuff, and will be updated when I find time around school. I try to post my reversed code snippets here and try to document them in this readme. This is not meant to be working, ready to be compiled code, but a pseudocode collection that should be able to be compiled after a few edits. Because of the nature of the Hypervisor, some stuff related to the internal security of the system are not included, however I try to keep this minimized as much as I feel appropriate.

I enjoy researching this system because it uses some very unique security measures to not only secure the console, but to also verify its integrity with xbox live. I would never had thought of many of these techniques before seeing them used here.

I also post some of my work here: http://free60.org/wiki/Special:Contributions/TEIR1plus2

# 1BL
The first bit of code executed on the xbox 360 on power-on. It was dumped from the ROM inside the CPU, mapped to memory location 0x8000020000000000.

This bootloader sets up some hardware registers and then loads, decrypts, and verifies the signature of the 2nd bootloader, the CB. The CB is loaded into address 0x8000020000010000.

Key to decrypt the CB = Key found in 1BL + salt found in the header of CB.

# CB_A
## Retail
On more recent kernels, the CB was split into two bootloaders, CB_A and CB_B. The reason for this is largely unknown because the CB_A is loaded the exact same way as the old CB. The Job of the CB_A is to relocate itself to 0x800002000001C000, null the area it was originally loaded at (0x8000020000010000), read the CPUKey from the efuses and use it to decrypt the CB_B. The CB_B is then hashed and the hash is compared against a hash stored inside the CB_A, if they match, it continues, otherwise it panics. This comparison is also where the RGH2 hack takes place. The CB_B is loaded into the old address of CB_A. Normally the CPUKey is different for every console and is burned in the efuses during manufacturing.

## MFG
At one point a console that was sent to be destroyed was recovered and it was discovered to have a different CB_A on its NAND, its known as the mfg CB_A. This version is signed and can be run on retail consoles. Why is this important? On the mfg CB_A there is a very important difference than the normal CB_A, it does not read the efuses from the CPU but continues to decrypt and load the CB_B like normal, meaning it does not use the CPUKey in the decryption process of the CB_B. This means that when using a hack like the RGH2, where we have control over the CB_B, we can make the rest of the bootchain ignore the CPUKey as well, meaning that we can create generic NAND images that can work on all consoles with the same motherboard. This can be useful if someone lost their CPUKey and their console will not boot, this would allow them to flash a bootable NAND and recover.

Programs like xebuild do not build NAND images with the mfg bootloader by default but have the ability to after changing some options. The reason this is not used by default is because if people are given a generic image for every console, they will likely lose some files that contain console specific information. These files can be ignored on hacked consoles but would be needed if the user wanted to return their console to a retail state.

## Here is how the two bootloaders differ in the decryption process:

Retail decryption key for the CB_B = CB_A Key + CPUKey + Salt in the CB_B Header

MFG decryption key for the CB_B = CB_A Key + Salt in the CB_B Header

# HV Related Stuff
## HV Utility
Utility functions reversed from the Hypervisor to assist with general tasks.

## HvxKeysExecute
Method of executing a payload in HV context

## HvxBlowFuses
Microsoft's way to blow fuses. This function expects all other threads to have had HvxQuiesceProcessor called on each of them before hand, then it puts each thread to sleep. This function then alters the cpu voltage regulator to increase cpu voltage. It then initializes the fuse device's precharge, enables the burning mechanism, tries to burn a fuse, then disables burning. It then checks if the fuse was burned. The device has 3 chances to burn the fuse before it reports a device failure. HvxBlowFuses has a variable number of failures it will accept before aborting the operation. After its done blowing fuses, it restores the voltage regulator to its original settings and then wakes up and restores the threads.

## HvxQuiesceProcessor
placeholder

# Challenge
This is the payload xbox live sends the console to verify it's integrity. This will not be documented beyond what's mentioned in the file.
