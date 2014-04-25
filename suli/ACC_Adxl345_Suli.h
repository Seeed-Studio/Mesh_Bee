/*
  acc_adxl345.h
  This is a Suly-compatible Library
  
  2014 Copyright (c) Seeed Technology Inc.  All right reserved.
  
  Loovee
  2013-4-1

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __GROVE_ACC_ADXL345_H__
#define __GROVE_ACC_ADXL345_H__

#include "Suli.h"

/* ------- Register names ------- */
#define ADXL345_DEVID           0x00
#define ADXL345_RESERVED1       0x01
#define ADXL345_THRESH_TAP      0x1d
#define ADXL345_OFSX            0x1e
#define ADXL345_OFSY            0x1f
#define ADXL345_OFSZ            0x20
#define ADXL345_DUR             0x21
#define ADXL345_LATENT          0x22
#define ADXL345_WINDOW          0x23
#define ADXL345_THRESH_ACT      0x24
#define ADXL345_THRESH_INACT    0x25
#define ADXL345_TIME_INACT      0x26
#define ADXL345_ACT_INACT_CTL   0x27
#define ADXL345_THRESH_FF       0x28
#define ADXL345_TIME_FF         0x29
#define ADXL345_TAP_AXES        0x2a
#define ADXL345_ACT_TAP_STATUS  0x2b
#define ADXL345_BW_RATE         0x2c
#define ADXL345_POWER_CTL       0x2d
#define ADXL345_INT_ENABLE      0x2e
#define ADXL345_INT_MAP         0x2f
#define ADXL345_INT_SOURCE      0x30
#define ADXL345_DATA_FORMAT     0x31
#define ADXL345_DATAX0          0x32
#define ADXL345_DATAX1          0x33
#define ADXL345_DATAY0          0x34
#define ADXL345_DATAY1          0x35
#define ADXL345_DATAZ0          0x36
#define ADXL345_DATAZ1          0x37
#define ADXL345_FIFO_CTL        0x38
#define ADXL345_FIFO_STATUS     0x39

#define ADXL345_BW_1600         0xF                 // 1111
#define ADXL345_BW_800          0xE                 // 1110
#define ADXL345_BW_400          0xD                 // 1101
#define ADXL345_BW_200          0xC                 // 1100
#define ADXL345_BW_100          0xB                 // 1011
#define ADXL345_BW_50           0xA                 // 1010
#define ADXL345_BW_25           0x9                 // 1001
#define ADXL345_BW_12           0x8                 // 1000
#define ADXL345_BW_6            0x7                 // 0111
#define ADXL345_BW_3            0x6                 // 0110

/*
 Interrupt PINs
 INT1: 0
 INT2: 1
 */
#define ADXL345_INT1_PIN            0x00
#define ADXL345_INT2_PIN            0x01

/*Interrupt bit position*/
#define ADXL345_INT_DATA_READY_BIT  0x07
#define ADXL345_INT_SINGLE_TAP_BIT  0x06
#define ADXL345_INT_DOUBLE_TAP_BIT  0x05
#define ADXL345_INT_ACTIVITY_BIT    0x04
#define ADXL345_INT_INACTIVITY_BIT  0x03
#define ADXL345_INT_FREE_FALL_BIT   0x02
#define ADXL345_INT_WATERMARK_BIT   0x01
#define ADXL345_INT_OVERRUNY_BIT    0x00

#define ADXL345_DATA_READY          0x07
#define ADXL345_SINGLE_TAP          0x06
#define ADXL345_DOUBLE_TAP          0x05
#define ADXL345_ACTIVITY            0x04
#define ADXL345_INACTIVITY          0x03
#define ADXL345_FREE_FALL           0x02
#define ADXL345_WATERMARK           0x01
#define ADXL345_OVERRUNY            0x00

#define ADXL345_OK                  1                   // no error
#define ADXL345_ERROR               0                   // indicates error is predent

#define ADXL345_NO_ERROR            0                   // initial state
#define ADXL345_READ_ERROR          1                   // problem reading accel
#define ADXL345_BAD_ARG             2                   // bad method argument


static void writeTo(uint8 address, uint8 val);
static void readFrom(uint8 address, uint8 num, uint8 buff[]);
static void setRegisterBit(uint8 regAdress, uint8 bitPos, uint8 state);


void acc_adxl345_init(void *i2c_dev);
void acc_adxl345_read_xyz(int16 *x, int16 *y, int16 *z);
void acc_adxl345_read_acc(float *ax, float *ay, float *az);
void acc_adxl345_read_acc_buff(float *xyz);


#endif
