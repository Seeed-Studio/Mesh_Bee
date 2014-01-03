#==============================================================================
#
#                 THIS IS A GENERATED FILE. DO NOT EDIT!
#
# MODULE:         OS
#
# COMPONENT:      os_irq.s
#
# DATE:           Fri Jan  3 14:07:12 2014
#
# AUTHOR:         Jennic RTOS Configuration Tool
#
# DESCRIPTION:    RTOS Application IRQ handling for 514x
#
#==============================================================================
#
# This software is owned by Jennic and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on Jennic products. You, and any third parties must reproduce
# the copyright and warranty notice and any other legend of ownership on each
# copy or partial copy of the software.
#
# THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
# EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
# ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
# BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
# INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
#
# Copyright Jennic Ltd. 2009 All rights reserved
#
#==============================================================================


    .section .text,"ax"
    .globl _os_ExternalException_W
    .globl _os_ExternalException_R

    .extern _os_PIC_SwVectTable

    # Controlled interrupt wrapper
    # sits on external interrupt exception
    # stores minimal context before re-enabling interrupts via global diable flag
    # branches through PIC software vector table using IPL register from PIC
    # [I SP001271_fr 404,405,406,407,408] begin
    .align 8
_os_ExternalException_W:
    b.addi    r1,r1,-24
    b.sw     0(r1),r3
    b.sw     4(r1),r4
    b.sw    20(r1),r9
    b.mfspr r9,r0,0x0020                 # get EPCR0
    b.sw    8(r1),r9                     # stack EPCR0
    b.mfspr r3,r0,0x0040                 # get ESR0
    b.sw    12(r1),r3                    # stack ESR0

    b.mfspr r3,r0,0x4810                 # get IPMR
    b.sw    16(r1),r3                    # stack IPMR

    b.mfspr r4,r0,0x4811                 # get IHPR
    b.mtspr r0,r4,0x4810                 # write to IPMR

    b.ei                                # enable interrupts

    b.slli  r4,r4,2                        # vector through pseudo vector table for PIC interrupt sources
    b.addi  r4,r4,_os_PIC_SwVectTable
    b.lwz   r4,0(r4)
    b.jalr  r4
    # [I SP001271_fr 404,405,406,407,408] end

    .globl  _os_PIC_ChannelPriorities
    .section .text,"ax"
    .align 4
    .type   _os_PIC_ChannelPriorities, @object
    .size   _os_PIC_ChannelPriorities, 16
_os_PIC_ChannelPriorities:
    .byte 0
    .byte 0
    .byte 0
    .byte 7
    .byte 0
    .byte 7
    .byte 0
    .byte 14
    .byte 0
    .byte 0
    .byte 0
    .byte 0
    .byte 0
    .byte 0
    .byte 0
    .byte 15

    .globl  _os_OSMIUM_HwVectTable
    .extern _os_UnclaimedException_W
    .extern _os_UnclaimedInterrupt_W
    .extern _os_ExternalException_W

    .section .text,"ax"
    .align 4
    .type   _os_OSMIUM_HwVectTable, @object
    .size   _os_OSMIUM_HwVectTable, 78
_os_OSMIUM_HwVectTable:
    b.ja _IRQ_BusError
    b.ja _os_UnclaimedException_W
    b.ja _IRQ_Alignment
    b.ja _IRQ_IllegalInstruction
    b.ja _os_ExternalException_W
    b.ja _os_UnclaimedException_W
    b.ja _os_UnclaimedException_W
    b.ja _IRQ_UnimplementedModule
    b.ja _os_UnclaimedException_W
    b.ja _IRQ_StackOverflowException

    .globl  _os_PIC_SwVectTable
    .section .text,"ax"
    .align 4
    .type   _os_PIC_SwVectTable, @object
    .size   _os_PIC_SwVectTable, 64
_os_PIC_SwVectTable:
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_zps_isrMAC_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_UnclaimedInterrupt_W
    .word _os_APP_isrUART1_W
    .word _os_APP_isrTickTimer_W

    .section .text,"ax"
    .align 4
    .extern _os_ExternalException_R
    .extern _os_CFunc_W
    .extern _os_vControlledInterrupt
    .extern _os_vControlledIntCheck

    .extern _os_vzps_isrMAC
_os_zps_isrMAC_W:
    b.addi r1,r1,-8
    b.sw   0(r1),r9
    b.sw   4(r1),r5
    b.ori  r4,r0,_os_zps_isrMAC_J
    b.ori  r5,r0,_os_vControlledInterrupt
    b.jal  _os_CFunc_W
    b.lwz  r9,0(r1)
    b.lwz  r5,4(r1)
    b.addi r1,r1,8
    b.ja   _os_ExternalException_R
_os_zps_isrMAC_J:
    b.ori  r3,r0,_os_vzps_isrMAC
    b.ja   _os_vControlledIntCheck

    .extern _os_vAPP_isrUART1
_os_APP_isrUART1_W:
    b.addi r1,r1,-8
    b.sw   0(r1),r9
    b.sw   4(r1),r5
    b.ori  r4,r0,_os_APP_isrUART1_J
    b.ori  r5,r0,_os_vControlledInterrupt
    b.jal  _os_CFunc_W
    b.lwz  r9,0(r1)
    b.lwz  r5,4(r1)
    b.addi r1,r1,8
    b.ja   _os_ExternalException_R
_os_APP_isrUART1_J:
    b.ori  r3,r0,_os_vAPP_isrUART1
    b.ja   _os_vControlledIntCheck

    .extern _os_vAPP_isrTickTimer
_os_APP_isrTickTimer_W:
    b.addi r1,r1,-8
    b.sw   0(r1),r9
    b.sw   4(r1),r5
    b.ori  r4,r0,_os_APP_isrTickTimer_J
    b.ori  r5,r0,_os_vControlledInterrupt
    b.jal  _os_CFunc_W
    b.lwz  r9,0(r1)
    b.lwz  r5,4(r1)
    b.addi r1,r1,8
    b.ja   _os_ExternalException_R
_os_APP_isrTickTimer_J:
    b.ori  r3,r0,_os_vAPP_isrTickTimer
    b.ja   _os_vControlledIntCheck
