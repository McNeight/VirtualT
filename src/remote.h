/* remote.h */

/* $Id: remote.h,v 1.3 2011/07/09 08:16:21 kpettit1 Exp $ */

/*
 * Copyright 2008 Ken Pettit
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef _REMOTE_H_
#define _REMOTE_H_

#ifdef __cplusplus

#include <string>

extern "C" {
#endif

void	init_remote(void);
void	deinit_remote(void);
void	lock_remote(void);
void	unlock_remote(void);
void	enable_remote(int enabled);
void	set_remote_cmdline_port(int port);
void	set_remote_cmdline_telnet(int telnet);
int		get_remote_listen_port();
int		get_remote_connect_status();
int		get_remote_error_status(void);
int		get_remote_telnet(void);
int		get_remote_port(void);
int		get_remote_enabled(void);
void	set_remote_port(int port);
void	set_remote_telnet(int telnet);
void	load_remote_preferences();
void	remote_process_console_input(void);

#ifdef __cplusplus
}

std::string get_remote_error();

#endif

#define	BPTYPE_MAIN		0x01
#define	BPTYPE_OPT		0x02
#define	BPTYPE_MPLAN	0x04
#define	BPTYPE_RAM		0x08
#define	BPTYPE_RAM2		0x10
#define	BPTYPE_RAM3		0x20
#define	BPTYPE_READ		0x40
#define	BPTYPE_WRITE	0x80

typedef struct
{
	int		top_row;
	int		top_col;
	int		bottom_row;
	int		bottom_col;
} lcd_rect_t;

#define REMOTE_TERM_INIT		1

/* Define TELNET option codes */
#define	TELNET_IS			0
#define	TELNET_SEND			1		/* Send */
#define	TELNET_ECHO			1		/* Send and echo are the same */
#define	TELNET_SUPPRESS_GA	3		/* Suppres go ahead */
#define	TELNET_TERMTYPE		24		/* Terminal-type option */
#define	TELNET_WINDOW_SIZE	31		/* Window size negotiation */

/* Define TELNET Command Codes */
#define	TELNET_SE		240		/* End of sub-negotiation parameters */
#define	TELNET_DM		242		/* Data Mark */
#define	TELNET_IP		244
#define	TELNET_AO		245
#define	TELNET_AYT		246		/* Are You There */
#define	TELNET_EC		247		/* Erase Character */
#define	TELNET_EL		248		/* Erase Line */
#define	TELNET_GA		249		/* Go Ahead */
#define	TELNET_SB		250		/* Subnegotiation */
#define	TELNET_WILL		251		/* Will send */
#define	TELNET_WONT		252		/* Won't send (not willing) */
#define	TELNET_DO		253		/* Do support receive */
#define	TELNET_DONT		254		/* Don't support receive */
#define	TELNET_IAC		255		/* Intrepret As Command */

#endif
