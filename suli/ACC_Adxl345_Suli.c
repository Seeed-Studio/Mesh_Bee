/*
  Seeed_LED_Bar.cpp
  This is a Suly compatible Library
  
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

#include "ACC_Adxl345_Suli.h"


#define ADXL345_DEVICE (0x53)                       // ADXL345 device address
#define ADXL345_TO_READ (6)                             // num of bytes we are going to read each time (two bytes for each axis)

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

const double __Gains[3] = {0.00376390, 0.00376009, 0.00349265};


void *__I2C_Device;

// Writes val to address register on device
void writeTo(uint8 address, uint8 val)
{

    uint8 dta_send[] = {address, val};
    suli_i2c_write(__I2C_Device, ADXL345_DEVICE, dta_send, 2);

    // suli_i2c_write(void * i2c_device, uint8 dev_addr, uint8 *data, uint8 len);
}

// Reads num bytes starting from address register on device in to _buff array
void readFrom(uint8 address, uint8 num, uint8 buff[])
{
    //grove_hal_i2c_read(ADXL345_DEVICE, address, buff, num);

    uint8 dta_send[] = {address};

    suli_i2c_write(__I2C_Device, ADXL345_DEVICE, dta_send, 1);
    suli_i2c_read(__I2C_Device, ADXL345_DEVICE, buff, num);

}

void setRegisterBit(uint8 regAdress, uint8 bitPos, uint8 state)
{
    uint8 _b;

    readFrom(regAdress, 1, &_b);

    if (state)
    {
        _b |= (1 << bitPos);                        // forces nth bit of _b to be 1.  all other bits left alone.
    }
    else
    {
        _b &= ~(1 << bitPos);                       // forces nth bit of _b to be 0.  all other bits left alone.
    }

    writeTo(regAdress, _b);
}

// initialize sensor
void acc_adxl345_init(void *i2c_dev)
{

    __I2C_Device = i2c_dev;

    //suli_i2c_init(__I2C_Device);

    //Turning on the ADXL345
    writeTo(ADXL345_POWER_CTL, 0);
    writeTo(ADXL345_POWER_CTL, 16);
    writeTo(ADXL345_POWER_CTL, 8);

    writeTo(ADXL345_THRESH_ACT, 75);
    writeTo(ADXL345_THRESH_INACT, 75);
    writeTo(ADXL345_TIME_INACT, 10);

    //look of activity movement on this axes - 1 == on; 0 == off
    setRegisterBit(ADXL345_ACT_INACT_CTL, 6, 1);
    setRegisterBit(ADXL345_ACT_INACT_CTL, 5, 1);
    setRegisterBit(ADXL345_ACT_INACT_CTL, 4, 1);

    //look of inactivity movement on this axes - 1 == on; 0 == off
    setRegisterBit(ADXL345_ACT_INACT_CTL, 2, 1);
    setRegisterBit(ADXL345_ACT_INACT_CTL, 1, 1);
    setRegisterBit(ADXL345_ACT_INACT_CTL, 0, 1);

    setRegisterBit(ADXL345_TAP_AXES, 2, 0);
    setRegisterBit(ADXL345_TAP_AXES, 1, 0);
    setRegisterBit(ADXL345_TAP_AXES, 0, 0);

    //set values for what is a tap, and what is a double tap (0-255)
    //setTapThreshold(50); //62.5mg per increment
    writeTo(ADXL345_THRESH_TAP, 50);

    writeTo(ADXL345_DUR, 15);

    writeTo(ADXL345_LATENT, 80);

    //setDoubleTapWindow(200); //1.25ms per increment
    writeTo(ADXL345_WINDOW, (uint8)200);

    //set values for what is considered freefall (0-255)

    writeTo(ADXL345_THRESH_FF, 7);

    writeTo(ADXL345_TIME_FF, 45);
    //setting all interrupts to take place on int pin 1
    //I had issues with int pin 2, was unable to reset it

    setRegisterBit(ADXL345_INT_MAP, ADXL345_INT_SINGLE_TAP_BIT,   ADXL345_INT1_PIN);
    setRegisterBit(ADXL345_INT_MAP, ADXL345_INT_DOUBLE_TAP_BIT,   ADXL345_INT1_PIN);
    setRegisterBit(ADXL345_INT_MAP, ADXL345_INT_FREE_FALL_BIT,    ADXL345_INT1_PIN);
    setRegisterBit(ADXL345_INT_MAP, ADXL345_INT_ACTIVITY_BIT,     ADXL345_INT1_PIN);
    setRegisterBit(ADXL345_INT_MAP, ADXL345_INT_INACTIVITY_BIT,   ADXL345_INT1_PIN);

    //register interrupt actions - 1 == on; 0 == off
    setRegisterBit(ADXL345_INT_ENABLE, ADXL345_INT_SINGLE_TAP_BIT, 1);
    setRegisterBit(ADXL345_INT_ENABLE, ADXL345_INT_DOUBLE_TAP_BIT, 1);
    setRegisterBit(ADXL345_INT_ENABLE, ADXL345_INT_FREE_FALL_BIT,  1);
    setRegisterBit(ADXL345_INT_ENABLE, ADXL345_INT_ACTIVITY_BIT,   1);
    setRegisterBit(ADXL345_INT_ENABLE, ADXL345_INT_INACTIVITY_BIT, 1);
}

// get accleration, save to xyz
void acc_adxl345_read_acc_buff(float *xyz)
{
    int i;
    int16 xyz_int[3];

    acc_adxl345_read_xyz(&xyz_int[0], &xyz_int[1], &xyz_int[2]);
    for(i=0; i<3; i++)
    {
        xyz[i] = xyz_int[i] * __Gains[i];
    }
}

void acc_adxl345_read_xyz(int16 *x, int16 *y, int16 *z)
{
    uint8 _buff[6];
    readFrom(ADXL345_DATAX0, ADXL345_TO_READ, _buff); //read the acceleration data from the ADXL345


    //pc.printf("%d, %d\t%d, %d\t%d, %d\r\n", _buff[0], _buff[1], _buff[2], _buff[3], _buff[4], _buff[5]);


    // each axis reading comes in 10 bit resolution, ie 2 bytes.  Least Significat Byte first!!
    // thus we are converting both bytes in to one int

    *x = (int16)((((uint16)_buff[1]) << 8) | _buff[0]);
    *y = (int16)((((uint16)_buff[3]) << 8) | _buff[2]);
    *z = (int16)((((uint16)_buff[5]) << 8) | _buff[4]);

    //pc.printf("%d, %d, %d\r\n", *x, *y, *z);


}

// read acceleration , save to x, y, z
void acc_adxl345_read_acc(float *ax, float *ay, float *az)
{
    int16 xyz[3];
    acc_adxl345_read_xyz(&xyz[0], &xyz[1], &xyz[2]);

    *ax = xyz[0] * __Gains[0];
    *ay = xyz[1] * __Gains[1];
    *az = xyz[2] * __Gains[2];
}
