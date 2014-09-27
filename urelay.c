/****************************************************************************
*	Serial relay output interface
*
*	Uses STC89C52RC MCU with 8 relays connected to output Port2.
*	Build system - SDCC + MCU 8051 IDE
*	Programming supported by STC-ISP version 4.83 (Chinese UI)
*
*	Supported serial interface commands:
*	Nx - Turn ON port x. If x == 0, turn ON all ports 1 to 8
*	Fx - Turn OFF port x. If x == 0, turn OFF all ports 1 to 8
*	Tx - Toggle port x. If x == 0, toggle all ports 1 to 8
*	RHH - Set output port according to HEX bitmask HH (H = [0-9]|[A-F])
*	Sx - Get status of port x. If x == 0, return status of all ports 1 to 8
*****************************************************************************
* Copyright (c) 2014, Konstantin Porotchkin kostap@porotchkin.com
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*    * Neither the name of the Konstantin Porotchkin nor the
*      names of its contributors may be used to endorse or promote products
*      derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL KONSTANTIN POROTCHKIN BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/
#include <8052.h>

typedef unsigned char BYTE;

#define FOSC	11059200	/* Board oscillator frequency */
#define BAUD	9600		/* Working baudrate */

__bit tx_busy = 0;			/* TX is running */
__bit cmd_ready = 0;		/* Command received */

char count = 0;					/* Input command symbol count */
BYTE command[3] = {0, 0, 0};	/* ASCII command buffer */

/**********************************************************/
void UART_ISR(void)  __interrupt 4 __using 1
{
	if (RI) { /* RX buffer ready */
		RI = 0;

		if (cmd_ready)
			return; /* Ignore long strings */

		command[count] = SBUF;
		/* send the received character back */
		SBUF = command[count];
		tx_busy = 1;

		/* Long command R is 3 symbols, rest commands are 2 symbols */
		if (((command[0] == 'R') && (count == 2)) ||
			((command[0] != 'R') && (count == 1))) {
			count = 0;
			cmd_ready = 1;
		} else 
			count++;
	}
	if (TI) { /* TX complete */
		TI = 0;
		tx_busy = 0;
	}
}

/**********************************************************/
void send_data (BYTE data)
{
	while(tx_busy); /* Wait for previous transmit completion */
	tx_busy = 1;
	SBUF = data;
}

/**********************************************************/
void send_string (char *str)
{
	while (*str)
		send_data(*str++);
}

/**********************************************************/
void main (void) 
{
	BYTE	p_out, p_in, tmp;
	__bit	cmd_error;
	__bit	send_status;

	SCON = 0x50;	/* 8-vit variable baud UART */
	TMOD = 0x20;	/* Timer1 in 8 bit auto reload mode */
	TH1 = TL1 = -(FOSC/12/32/BAUD);	/* auto-reload value */
	TR1 = 1;		/* Timer1 start to run */
	ES = 1;			/* Enable UART interrupt */
	EA = 1; 		/* Open master interrupt switch */

	send_string("URelay v1.0. (c) 2014, Konstantin Porotchkin\r\n");
	P2 = 0xFF;

	while(1) { 	/* main loop */

		cmd_error = 1;
		send_status = 0;

		if (cmd_ready) { /* ISR finished to read command */
			p_out = command[1] - 0x30; /* ASCII to integer */
			p_in = P2;

			if (command[0] != 'R') { /* Short commands */

				if (p_out <= 8) {
					cmd_error = 0;
					
					switch (command[0]) {
					case 'N':	/* Port ON */
						p_out = (p_out == 0) ? 0x0 : ~(1 << (p_out - 1)) & p_in;
						break;
					case 'F':	/* Port OFF */
						p_out = (p_out == 0) ? 0xFF : (1 << (p_out - 1)) | p_in;
						break;
					case 'T':	/* Port toggle */
						p_out = (p_out == 0) ? ~p_in : (1 << (p_out - 1)) ^ p_in;
						break;
					case 'S':	/* Port status */
						send_status = 1;
						command[2] = 0; /* Use command buffer for ASCII output */
						if (p_out == 0) { /* all ports */
							tmp = ~p_in & 0xF;
							command[1] = (tmp <= 9) ? (tmp + 0x30) : (tmp + 0x37);
							tmp = (~p_in & 0xF0) >> 4;
							command[0] = (tmp <= 9) ? (tmp + 0x30) : (tmp + 0x37);
						} else { /* Single port */
							command[1] = 0;
							tmp = ~p_in & (1 << (p_out - 1));
							command[0] = (tmp != 0) ? 0x31 : 0x30;
						}
						break;
					default:
						cmd_error = 1;
					}
				} 

			} else { /* Long command 'R' */

				tmp = command[2] - 0x30;

				if ((p_out <= 9) || ((p_out >= 0x11) && (p_out <= 0x16))) {
					if (p_out >= 0x11) /* Allow ABCDEF ASCII */
						p_out -= 7;

					if ((tmp <= 9) || ((tmp >= 0x11) && (tmp <= 0x16))) {
						if (tmp >= 0x11) /* Allow ABCDEF ASCII */
							tmp -= 7;

						p_out = ~((p_out << 4) | tmp);
						cmd_error = 0;
					} 
				} 
			}

			cmd_ready = 0;
			send_string("\r\n");

			if (cmd_error)
				send_string("ERROR\r\n");
			else if (send_status) {
				send_string(command);
				send_string("\r\n");
				send_status = 0;
			} else
				P2 = p_out;

		}/* cmd_ready */
	} /* while */
}

