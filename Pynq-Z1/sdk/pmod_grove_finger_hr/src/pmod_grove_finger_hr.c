/******************************************************************************
 *  Copyright (c) 2016, NECST Laboratory, Politecnico di Milano
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *
 *
 * @file arduino_grove_finger_hr.c
 * IOP code (MicroBlaze) for grove finger-clip heart rate sensor.
 * The sensor has to be connected to a PMOD interface 
 * via a shield socket.
 * http://wiki.seeedstudio.com/wiki/Grove_-_Finger-clip_Heart_Rate_Sensor
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- -------- -----------------------------------------------
 * 1.00a lcc 06/22/16 release
 * 1.00b mr  06/23/16 updated signal code
 * 1.00c gn  10/24/16 support for PYNQ-Z1
 *
 * </pre>
 *
 *****************************************************************************/

#include "pmod.h"


// Mailbox commands
// bit 1 always needs to be sets
#define CONFIG_IOP_SWITCH      0x1
#define READ_DATA              0x2
#define READ_AND_LOG_DATA    0x3
#define STOP_LOG               0xC

// IIC sensor address
#define I2CHR_ADDR 0xA0

// Log constants
#define LOG_BASE_ADDRESS (MAILBOX_DATA_PTR(4))
#define LOG_ITEM_SIZE sizeof(float)
#define LOG_CAPACITY  (4000/LOG_ITEM_SIZE)


u8 read_fingerHR(){
    u8 data;
    data = 0;
    iic_read(IIC_BASEADDR, I2CHR_ADDR >> 1, &data, 1);
    return data;
}

int main(void)
{
    u32 cmd;
    u32 delay;
    u8 iop_pins[8];
    u32 scl, sda;
    u32 hr;

    // Initialize Pmod
    pmod_init(0,1);
    config_pmod_switch(GPIO_0, GPIO_1, SDA, SDA, GPIO_4, GPIO_5, SCL, SCL);

    // Run application
    while(1){
        // wait and store valid command
        while((MAILBOX_CMD_ADDR)==0);

        cmd = MAILBOX_CMD_ADDR;

        switch(cmd){
            case CONFIG_IOP_SWITCH:
                // read new pin configuration
                scl = MAILBOX_DATA(0);
                sda = MAILBOX_DATA(1);
                iop_pins[0] = GPIO_0;
                iop_pins[1] = GPIO_1;
                iop_pins[2] = GPIO_2;
                iop_pins[3] = GPIO_3;
                iop_pins[4] = GPIO_4;
                iop_pins[5] = GPIO_5;
                iop_pins[6] = GPIO_6;
                iop_pins[7] = GPIO_7;
                // set new pin configuration
                iop_pins[scl] = SCL;
                iop_pins[sda] = SDA;
                config_pmod_switch(iop_pins[0], iop_pins[1], iop_pins[2], 
                                   iop_pins[3], iop_pins[4], iop_pins[5], 
                                   iop_pins[6], iop_pins[7]);
                MAILBOX_CMD_ADDR = 0x0;
                break;
            case READ_DATA:
                hr = read_fingerHR();
                // write out hr, reset mailbox
                MAILBOX_DATA(0) = hr;
                MAILBOX_CMD_ADDR = 0x0;
                break;
            case READ_AND_LOG_DATA:
                // initialize logging variables, reset cmd
                cb_init(&pmod_log, LOG_BASE_ADDRESS, LOG_CAPACITY, 
                        LOG_ITEM_SIZE);
                delay = MAILBOX_DATA(1);
                MAILBOX_CMD_ADDR = 0x0;
                do{
                   // push sample to log and delay
                   hr = read_fingerHR();
                   cb_push_back(&pmod_log, &hr);
                   delay_ms(delay);
                } while((MAILBOX_CMD_ADDR & 0x1)== 0);
                break;
            default:
                MAILBOX_CMD_ADDR = 0x0; // reset command
                break;
        }
    }
    return 0;
}

