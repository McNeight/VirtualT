/* do_instruct.h */

/* $Id: $ */

/*
 * Copyright 2004 Stephen Hurd and Ken Pettit
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


#define LXI(rp,h,l,rps)	{ \
							INCPC; \
							if(trace) \
								sprintf(p,"LXI "rps",%04x",INS16); \
							l=INS; \
							INCPC; \
							h=INS; \
							INCPC; \
							cpu_delay(10); \
						}

#define STAX(rp,rps)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"STAX "rps); \
							MEM(rp)=A; \
							cpu_delay(7); \
						}

#define INX(rp,h,l,rps)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"INX "rps); \
							l++; \
							if(!l) \
								h++; \
							cpu_delay(6); \
						}

#define INR(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"INR "rs); \
							r++; \
							if(r&0x0f==0) /* Low order nybble wrapped */ \
								j=1; \
							else \
								j=0; \
							setflags(r,-1,-1,j,-1,-2); \
							cpu_delay(4); \
						}

#define DCR(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"DCR "rs); \
							r--; \
							if(r&0x0f==0x0f) /* Low order nybble wrapped */ \
								j=1; \
							else \
								j=0; \
							setflags(r,-1,-1,j,-1,-2); \
							cpu_delay(4); \
						}

#define MVI(r,rs)		{ \
							INCPC; \
							if(trace) \
								sprintf(p,"MVI "rs",%02x",INS); \
							r=INS; \
							INCPC; \
							cpu_delay(7); \
						}

#define DAD(rp,rps)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"DAD "rps); \
							i=HL; \
							i+=rp; \
							H=(i>>8); \
							L=i; \
							if(i>0xffff) \
								i=1; \
							else \
								i=0; \
							setflags(0,-2,-2,-2,-2,i); \
							cpu_delay(10); \
						}

#define LDAX(rp,rps)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"LDAX "rps); \
							A=MEM(rp); \
							cpu_delay(7); \
						}

#define DCX(rp,h,l,rps)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"DCX "rps); \
							l--; \
							if(l==0xff) \
								h--; \
							cpu_delay(6); \
						}

#define MOV(dest,src,ds,ss)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"MOV "ds","ss); \
							dest=src; \
							cpu_delay(4); \
						}

#define ADD(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"ADD "rs); \
							i=A; \
							i+=r; \
							j = (((A&0x0F)+(r&0x0F)) &0x10)>>4; \
							A=i; \
							if(i>0xFF) \
								i=1; \
							else \
								i=0; \
							setflags(A,-1,-1,j,-1,i); \
							cpu_delay(4); \
						}

#define ADC(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"ADC "rs); \
							i=A; \
							i+=r; \
							i+=(CF?1:0); \
							j = (((A&0x0F)+(r&0x0F)+(CF?1:0)) &0x10)>>4; \
							A=i; \
							if(i>0xFF) \
								i=1; \
							else \
								i=0; \
							setflags(A,-1,-1,j,-1,i); \
							cpu_delay(4); \
						}

#define SUB(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"SUB "rs); \
							i=A; \
							i-=r; \
							j = (((A & 0x0F)-(r & 0x0F)) & 0x10) >> 4; \
							A=i; \
							if(i>0xFF) \
								i=1; \
							else \
								i=0; \
							setflags(A,-1,-1,j,-1,i); \
							cpu_delay(4); \
						}

#define SBB(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"SCB "rs); \
							i=A; \
							i-=r; \
							i-=(CF?1:0); \
							j = (((A & 0x0F)-(r & 0x0F)-(CF?1:0)) & 0x10) >> 4; \
							A=i; \
							if (i>0xFF) \
								i = 1; \
							else \
								i = 0; \
							setflags(A,-1,-1,j,-1,i); \
							cpu_delay(4); \
						}

#define ANA(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"ANA "rs); \
							A&=r; \
							setflags(A,-1,-1,1,-1,0); \
							cpu_delay(4); \
						}

#define XRA(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"XRA "rs); \
							A^=r; \
							setflags(A,-1,-1,0,-1,0); \
							cpu_delay(4); \
						}

#define ORA(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"ORA "rs); \
							A|=r; \
							setflags(A,-1,-1,0,-1,0); \
							cpu_delay(4); \
						}

#define CMP(r,rs)		{ \
							INCPC; \
							if(trace) \
								strcat(p,"CMP "rs); \
							i=A; \
							i-=r; \
							if (i>0xFF) \
								i=1; \
							else \
								i=0; \
							j = (((A & 0x0F)-(r & 0x0F)) & 0x10) >> 4; \
							setflags(A-r,-1,-1,j,-1,i); \
							cpu_delay(4); \
						}

#define POP(rp,h,l,rps)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"POP "rps); \
							l=MEM(SP); \
							h=MEM(SP+1); \
							INCSP2; \
							cpu_delay(10); \
						}

#define PUSH(rp,h,l,rps)	{ \
							INCPC; \
							if(trace) \
								strcat(p,"PUSH "rps); \
							DECSP2; \
							MEM(SP)=l; \
							MEM(SP+1)=h; \
							cpu_delay(12); \
						}

#define RST(num)		{ \
							INCPC; \
							if(trace) \
								sprintf(p,"RST %d",num); \
							DECSP2; \
							MEM(SP)=PCL; MEM(SP+1)=PCH; \
							PCH=0; \
							PCL=8*num; \
							cpu_delay(12); \
						}

#define DOCALL			{ \
							DECSP2; \
							MEM(SP)=PCL; MEM(SP+1)=PCH; /* MEM16(SP)=PC; */ \
							DECPC2; \
							SETPCINS16; \
							cpu_delay(9); \
						}

#define DORET			{ \
							PCL=MEM(SP); PCH=MEM(SP+1); /* PC=MEM16(SP) */ \
							INCSP2; \
							cpu_delay(6); \
						}


{
	if(trace)
		p=op+sprintf(op,"%04x (%02x) ",PC,INS);
	if(!(INS&0x80)) {
		if(!(INS&0x40)) {
			if(!(INS&0x20)) {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x00:	/* NOP */
									INCPC;
									if(trace)
										strcat(p,"NOP");
									cpu_delay(4);
									/* return; */
								}
								else {
									/* case 0x01:	/* LXI B */
									LXI(BC,B,C,"B");
								}
							}
							else  {
								if(!(INS&0x01)) {
									/* case 0x02:	/* STAX B */
									STAX(BC,"B");
								}
								else {
									/* case 0x03:	/* INX B */
									INX(BC,B,C,"B");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x04:	/* INR B */
									INR(B,"B");
								}
								else {
									/* case 0x05:	/* DCR B */
									DCR(B,"B");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x06:	/* MVI B */
									MVI(B,"B");
								}
								else {
									/* case 0x07:	/* RLC */
									INCPC;
									if(trace)
										strcat(p,"RLC");
									i=A>>7;
									A=(A<<1)|i;
									setflags(A,-2,-2,-2,-2,i);
									cpu_delay(4);
									/* return; */
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x08:	/* INVALID? */
									bail("0x08 is invalid");
									/* return; */
								}
								else {
									/* case 0x09:	/* DAD B */
									DAD(BC,"B");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x0A:	/* LDAX B */
									LDAX(BC,"B");
								}
								else {
									/* case 0x0B:	/* DCX B */
									DCX(BC,B,C,"B");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x0C:	/* INR C */
									INR(C,"C");
								}
								else {
									/* case 0x0D:	/* DCR C */
									DCR(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x0E:	/* MVI C */
									MVI(C,"C");
								}
								else {
									/* case 0x0F:	/* RRC */
									INCPC;
									if(trace)
										strcat(p,"RRC");
									i=A<<7&0x80;
									A=(A>>1)|i;
									i>>=7;
									setflags(A,-2,-2,-2,-2,i);
									cpu_delay(4);
									/* return; */
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x10:	/* INVALID? */
									bail("0x10 is invalid");
									/* return; */
								}
								else {
									/* case 0x11:	/* LXI D */
									LXI(DE,D,E,"D");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x12:	/* STAX D */
									STAX(DE,"D");
								}
								else {
									/* case 0x13:	/* INX D */
									INX(DE,D,E,"D");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x14:	/* INR D */
									INR(D,"D");
								}
								else {
									/* case 0x15:	/* DCR D */
									DCR(D,"D");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x16:	/* MVI D */
									MVI(D,"D");
								}
								else {
									/* case 0x17:	/* RAL */
									INCPC;
									if(trace)
										strcat(p,"RAL");
									i=A>>7;
									A<<=1;
									A|=(CF?1:0);
									setflags(A,-2,-2,-2,-2,i);
									cpu_delay(4);
									/* return; */
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x18:	/* INVALID? */
									bail("0x18 is invalid");
									/* return; */
								}
								else {
									/* case 0x19:	/* DAD D */
									DAD(DE,"D");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x1A:	/* LDAX D */
									LDAX(DE,"D");
								}
								else {
									/* case 0x1B:	/* DCX D */
									DCX(DE,D,E,"D");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x1C:	/* INR E */
									INR(E,"E");
								}
								else {
									/* case 0x1D:	/* DCR E */
									DCR(E,"E");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x1E:	/* MVI E */
									MVI(E,"E");
								}
								else {
									/* case 0x1F:	/* RAR */
									INCPC;
									if(trace)
										strcat(p,"RAR");
									i=(A&0x01);
									A>>=1;
									A|=CF?0x80:0;
									setflags(A,-2,-2,-2,-2,i);
									cpu_delay(4);
									/* return; */
								}
							}
						}
					}
				}
			}
			else {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x20:	/* RIM */
									INCPC;
									if(trace)
										strcat(p,"RIM");
									A=IM;
									cpu_delay(4);
									/* return; */
								}
								else {
									/* case 0x21:	/* LXI H */
									LXI(HL,H,L,"H");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x22:	/* SHLD */
									INCPC;
									if(trace)
										sprintf(p,"SHLD %x",INS16);
									MEM(INS16)=L;
									MEM(INS16+1)=H;
									/* MEM16(INS16)=HL; */
									INCPC2;
									cpu_delay(16);
									/* return; */
								}
								else {
									/* case 0x23:	/* INX H */
									INX(HL,H,L,"H");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x24:	/* INR H */
									INR(H,"H");
								}
								else {
									/* case 0x25:	/* DCR H */
									DCR(H,"H");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x26:	/* MVI H */
									MVI(H,"H");
								}
								else {
									/* case 0x27:	/* DAA */
									INCPC;
									if(trace)
										strcat(p,"DAA");
									i=j=0;
									if(((A&0x0F) > 9)) {
										i=6;
										j=1;
									}
									if(AC)
										i=6;
									if((((A>>4)+j) > 9) || CF)
										i|=0x60;
									A+=i;
									setflags(A,-1,-1,j,-1,i>>4?1:0);
									cpu_delay(4);
									/* return; */ /* Huh? */
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x28:	/* INVALID? */
									bail("0x28 is invalid");
									/* return; */
								}
								else {
									/* case 0x29:	/* DAD H */
									DAD(HL,"H");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x2A:	/* LHLD */
									INCPC;
									if(trace)
										sprintf(p,"LHLD %x",INS16);
									L=MEM(INS16);
									H=MEM(INS16+1);
									INCPC2;
									cpu_delay(16);
								}
								else {
									/* case 0x2B:	/* DCX H */
									DCX(HL,H,L,"H");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x2C:	/* INR L */
									INR(L,"L");
								}
								else {
									/* case 0x2D:	/* DCR L */
									DCR(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x2E:	/* MVI L */
									MVI(L,"L");
								}
								else {
									/* case 0x2F:	/* CMA */
									INCPC;
									if(trace)
										strcat(p,"CMA");
									A=~A;
									cpu_delay(4);
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x30:	/* SIM */
									INCPC;
									if(trace)
										strcat(p,"SIM");
									if (A & 0x08)	/* Check if Interrupt masking enabled */
										IM=(IM & 0xF8) | (A&0x07);
									/* Turn RST 7.5 pending off if bit 4 set */
									if(A&0x10) {
										IM&=0xBF;
									}
									cpu_delay(4);
									/* return; */
								}
								else {
									/* case 0x31:	/* LXI SP */
									LXI(SP,SPH,SPL,"SP");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x32:	/* STA */
									INCPC;
									if(trace)
										sprintf(p,"STA %04x",INS16);
									MEM(INS16)=A;
									INCPC2;
									cpu_delay(13);
									/* return; */
								}
								else {
									/* case 0x33:	/* INX SP */
									INX(SP,SPH,SPL,"SP");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x34:	/* INR M */
									INR(M,"M");
									cpu_delay(6);
								}
								else {
									/* case 0x35:	/* DCR M */
									DCR(M,"M");
									cpu_delay(6);
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x36:	/* MVI M */
									MVI(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0x37:	/* STC */
									INCPC;
									if(trace)
										strcat(p,"STC");
									setflags(0,-2,-2,-2,-2,1);
									cpu_delay(4);
									/* return; */
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x38:	/* UNSUPPORTED */
									bail("0x38 is invalid");
									/* return; */
								}
								else {
									/* case 0x39:	/* DAD SP */
									DAD(SP,"SP");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x3A:	/* LDA */
									INCPC;
									if(trace)
										sprintf(p,"LDA %04x",INS16);
									A=MEM(INS16);
									INCPC2;
									cpu_delay(13);
									/* return; */
								}
								else {
									/* case 0x3B:	/* DCX SP */
									DCX(SP,SPH,SPL,"SP");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x3C:	/* INR A */
									INR(A,"A");
								}
								else {
									/* case 0x3D:	/* DCR A */
									DCR(A,"A");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x3E:	/* MVI A */
									MVI(A,"A");
								}
								else {
									/* case 0x3F:	/* CMC */
									INCPC;
									if(trace)
										strcat(p,"CMC");
									setflags(A,-2,-2,-2,-2,CF?0:1);
									cpu_delay(4);
									/* return; */
								}
							}
						}
					}
				}
			}
		}
		else {
			if(!(INS&0x20)) {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x40:	/* MOV B,B */
									MOV(B,B,"B","B");
								}
								else {
									/* case 0x41:	/* MOV B,C */
									MOV(B,C,"B","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x42:	/* MOV B,D */
									MOV(B,D,"B","D");
								}
								else {
									/* case 0x43:	/* MOV B,E */
									MOV(B,E,"B","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x44:	/* MOV B,H */
									MOV(B,H,"B","H");
								}
								else {
									/* case 0x45:	/* MOV B,L */
									MOV(B,L,"B","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x46:	/* MOV B,M */
									MOV(B,M,"B","M");
									cpu_delay(3);
									/* return; */
								}
								else {
									/* case 0x47:	/* MOV B,A */
									MOV(B,A,"B","A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x48:	/* MOV C,B */
									MOV(C,B,"C","B");
								}
								else {
									/* case 0x49:	/* MOV C,C */
									MOV(C,C,"C","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x4A:	/* MOV C,D */
									MOV(C,D,"C","D");
								}
								else {
									/* case 0x4B:	/* MOV C,E */
									MOV(C,E,"C","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x4C:	/* MOV C,H */
									MOV(C,H,"C","H");
								}
								else {
									/* case 0x4D:	/* MOV C,L */
									MOV(C,L,"C","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x4E:	/* MOV C,M */
									MOV(C,M,"C","M");
									cpu_delay(3);
								}
								else {
									/* case 0x4F:	/* MOV C,A */
									MOV(C,A,"C","A");
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x50:	/* MOV D,B */
									MOV(D,B,"D","B");
								}
								else {
									/* case 0x51:	/* MOV D,C */
									MOV(D,C,"D","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x52:	/* MOV D,D */
									MOV(D,D,"D","D");
								}
								else {
									/* case 0x53:	/* MOV D,E */
									MOV(D,E,"D","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x54:	/* MOV D,H */
									MOV(D,H,"D","H");
								}
								else {
									/* case 0x55:	/* MOV D,L */
									MOV(D,L,"D","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x56:	/* MOV D,M */
									MOV(D,M,"D","M");
									cpu_delay(3);
								}
								else {
									/* case 0x57:	/* MOV D,A */
									MOV(D,A,"D","A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x58:	/* MOV E,B */
									MOV(E,B,"E","B");
								}
								else {
									/* case 0x59:	/* MOV E,C */
									MOV(E,C,"E","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x5A:	/* MOV E,D */
									MOV(E,D,"E","D");
								}
								else {
									/* case 0x5B:	/* MOV E,E */
									MOV(E,E,"E","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x5C:	/* MOV E,H */
									MOV(E,H,"E","H");
								}
								else {
									/* case 0x5D:	/* MOV E,L */
									MOV(E,L,"E","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x5E:	/* MOV E,M */
									MOV(E,M,"E","M");
									cpu_delay(3);
								}
								else {
									/* case 0x5F:	/* MOV E,A */
									MOV(E,A,"E","A");
								}
							}
						}
					}
				}
			}
			else {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x60:	/* MOV H,B */
									MOV(H,B,"H","B");
								}
								else {
									/* case 0x61:	/* MOV H,C */
									MOV(H,C,"H","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x62:	/* MOV H,D */
									MOV(H,D,"H","D");
								}
								else {
									/* case 0x63:	/* MOV H,E */
									MOV(H,E,"H","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x64:	/* MOV H,H */
									MOV(H,H,"H","H");
								}
								else {
									/* case 0x65:	/* MOV H,L */
									MOV(H,L,"H","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x66:	/* MOV H,M */
									MOV(H,M,"H","M");
								}
								else {
									/* case 0x67:	/* MOV H,A */
									MOV(H,A,"H","A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x68:	/* MOV L,B */
									MOV(L,B,"L","B");
								}
								else {
									/* case 0x69:	/* MOV L,C */
									MOV(L,C,"L","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x6A:	/* MOV L,D */
									MOV(L,D,"L","D");
								}
								else {
									/* case 0x6B:	/* MOV L,E */
									MOV(L,E,"L","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x6C:	/* MOV L,H */
									MOV(L,H,"L","H");
								}
								else {
									/* case 0x6D:	/* MOV L,L */
									MOV(L,L,"L","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x6E:	/* MOV L,M */
									MOV(L,M,"L","M");
									cpu_delay(3);
								}
								else {
									/* case 0x6F:	/* MOV L,A */
									MOV(L,A,"L","A");
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x70:	/* MOV M,B */
									MOV(M,B,"M","B");
									cpu_delay(3);
								}
								else {
									/* case 0x71:	/* MOV M,C */
									MOV(M,C,"M","C");
									cpu_delay(3);
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x72:	/* MOV M,D */
									MOV(M,D,"M","D");
									cpu_delay(3);
								}
								else {
									/* case 0x73:	/* MOV M,E */
									MOV(M,E,"M","E");
									cpu_delay(3);
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x74:	/* MOV M,H */
									MOV(M,H,"M","H");
									cpu_delay(3);
								}
								else {
									/* case 0x75:	/* MOV M,L */
									MOV(M,L,"M","L");
									cpu_delay(3);
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x76:	/* HLT */
									if(trace)
										strcat(p,"HLT");
									cpu_delay(4);
									/* return; */
								}
								else {
									/* case 0x77:	/* MOV M,A */
									MOV(M,A,"M","A");
									cpu_delay(3);
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x78:	/* MOV A,B */
									MOV(A,B,"A","B");
								}
								else {
									/* case 0x79:	/* MOV A,C */
									MOV(A,C,"A","C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x7A:	/* MOV A,D */
									MOV(A,D,"A","D");
								}
								else {
									/* case 0x7B:	/* MOV A,E */
									MOV(A,E,"A","E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x7C:	/* MOV A,H */
									MOV(A,H,"A","H");
								}
								else {
									/* case 0x7D:	/* MOV A,L */
									MOV(A,L,"A","L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x7E:	/* MOV A,M */
									MOV(A,M,"A","M");
									cpu_delay(3);
								}
								else {
									/* case 0x7F:	/* MOV A,A */
									MOV(A,A,"A","A");
								}
							}
						}
					}
				}
			}
		}
	}
	else {
		if(!(INS&0x40)) {
			if(!(INS&0x20)) {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x80:	/* ADD B */
									ADD(B,"B");
								}
								else {
									/* case 0x81:	/* ADD C */
									ADD(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x82:	/* ADD D */
									ADD(D,"D");
								}
								else {
									/* case 0x83:	/* ADD E */
									ADD(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x84:	/* ADD H */
									ADD(H,"H");
								}
								else {
									/* case 0x85:	/* ADD L */
									ADD(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x86:	/* ADD M */
									ADD(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0x87:	/* ADD A */
									ADD(A,"A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x88:	/* ADC B */
									ADC(B,"B");
								}
								else {
									/* case 0x89:	/* ADC C */
									ADC(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x8A:	/* ADC D */
									ADC(D,"D");
								}
								else {
									/* case 0x8B:	/* ADC E */
									ADC(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x8C:	/* ADC H */
									ADC(H,"H");
								}
								else {
									/* case 0x8D:	/* ADC L */
									ADC(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x8E:	/* ADC M */
									ADC(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0x8F:	/* ADC A */
									ADC(A,"A");
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x90:	/* SUB B */
									SUB(B,"B");
								}
								else {
									/* case 0x91:	/* SUB C */
									SUB(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x92:	/* SUB D */
									SUB(D,"D");
								}
								else {
									/* case 0x93:	/* SUB E */
									SUB(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x94:	/* SUB H */
									SUB(H,"H");
								}
								else {
									/* case 0x95:	/* SUB L */
									SUB(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x96:	/* SUB M */
									SUB(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0x97:	/* SUB A */
									SUB(A,"A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x98:	/* SBB B */
									SBB(B,"B");
								}
								else {
									/* case 0x99:	/* SBB C */
									SBB(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x9A:	/* SBB D */
									SBB(D,"D");
								}
								else {
									/* case 0x9B:	/* SBB E */
									SBB(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0x9C:	/* SBB H */
									SBB(H,"H");
								}
								else {
									/* case 0x9D:	/* SBB L */
									SBB(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0x9E:	/* SBB M */
									SBB(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0x9F:	/* SBB A */
									SBB(A,"A");
								}
							}
						}
					}
				}
			}
			else {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xA0:	/* ANA B */
									ANA(B,"B");
								}
								else {
									/* case 0xA1:	/* ANA C */
									ANA(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xA2:	/* ANA D */
									ANA(D,"D");
								}
								else {
									/* case 0xA3:	/* ANA E */
									ANA(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xA4:	/* ANA H */
									ANA(H,"H");
								}
								else {
									/* case 0xA5:	/* ANA L */
									ANA(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xA6:	/* ANA M */
									ANA(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0xA7:	/* ANA A */
									ANA(A,"A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xA8:	/* XRA B */
									XRA(B,"B");
								}
								else {
									/* case 0xA9:	/* XRA C */
									XRA(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xAA:	/* XRA D */
									XRA(D,"D");
								}
								else {
									/* case 0xAB:	/* XRA E */
									XRA(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xAC:	/* XRA H */
									XRA(H,"H");
								}
								else {
									/* case 0xAD:	/* XRA L */
									XRA(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xAE:	/* XRA M */
									XRA(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0xAF:	/* XRA A */
									XRA(A,"A");
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xB0:	/* ORA B */
									ORA(B,"B");
								}
								else {
									/* case 0xB1:	/* ORA C */
									ORA(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xB2:	/* ORA D */
									ORA(D,"D");
								}
								else {
									/* case 0xB3:	/* ORA E */
									ORA(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xB4:	/* ORA H */
									ORA(H,"H");
								}
								else {
									/* case 0xB5:	/* ORA L */
									ORA(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xB6:	/* ORA M */
									ORA(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0xB7:	/* ORA A */
									ORA(A,"A");
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xB8:	/* CMP B */
									CMP(B,"B");
								}
								else {
									/* case 0xB9:	/* CMP C */
									CMP(C,"C");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xBA:	/* CMP D */
									CMP(D,"D");
								}
								else {
									/* case 0xBB:	/* CMP E */
									CMP(E,"E");
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xBC:	/* CMP H */
									CMP(H,"H");
								}
								else {
									/* case 0xBD:	/* CMP L */
									CMP(L,"L");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xBE:	/* CMP M */
									CMP(M,"M");
									cpu_delay(3);
								}
								else {
									/* case 0xBF:	/* CMP A */
									CMP(A,"A");
								}
							}
						}
					}
				}
			}
		}
		else {
			if(!(INS&0x20)) {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xC0:	/* RNZ */
									INCPC;
									if(trace)
										strcat(p,"RNZ");
									if(!ZF) {
										PCL=MEM(SP); PCH=MEM(SP+1); /* PC=MEM16(SP) */
										INCSP2;
										cpu_delay(6);
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xC1:	/* POP B */
									POP(BC,B,C,"B");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xC2:	/* JNZ addr */
									INCPC;
									if(trace)
										sprintf(p,"JNZ %04x",INS16);
									if(!ZF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xC3:	/* JMP addr */
									INCPC;
									if(trace)
										sprintf(p,"JMP %04x",INS16);
									SETPCINS16;
									cpu_delay(7);
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xC4:	/* CNZ addr */
									INCPC;
									if(trace)
										sprintf(p,"CNZ %04x",INS16);
									INCPC2;
									if(!ZF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xC5:	/* PUSH B */
									PUSH(BC,B,C,"B");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xC6:	/* ADI n */
									INCPC;
									if(trace)
										sprintf(p,"ADI %02x",INS);
									i=A;
									i+=INS;
									j = (((A&0x0F)+(INS&0x0F)) &0x10)>>4;
									A=i;
									if(i>0xFF)
										i=1;
									else
										i=0;
									setflags(A,-1,-1,j,-1,i);
									cpu_delay(7);
									INCPC;
									/* return; */
								}
								else {
									/* case 0xC7:	/* RST 0 */
									RST(0);
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xC8:	/* RZ */
									if(trace)
										strcat(p,"RZ");
									INCPC;
									if(ZF) {
										DORET;
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xC9:	/* RET */
									if(trace)
										strcat(p,"RET");
									DORET;
									cpu_delay(4);
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xCA:	/* JZ addr */
									INCPC;
									if(trace)
										sprintf(p,"JZ %04x",INS16);
									if(ZF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xCB:	/* INVALID? */
									bail("0xCB is invalid");
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xCC:	/* CZ addr */
									INCPC;
									if(trace)
										sprintf(p,"CZ %04x",INS16);
									INCPC2;
									if(ZF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xCD:	/* CALL addr */
									INCPC;
									if(trace)
										sprintf(p,"CALL %04x",INS16);
									INCPC2;
									DOCALL;
									cpu_delay(9);
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xCE:	/* ACI n */
									INCPC;
									if(trace)
										sprintf(p,"ACI %02x",INS);
									i=A;
									i+=INS;
									i+=(CF?1:0);
									j = (((A&0x0F)+(INS&0x0F)+(CF?1:0)) &0x10)>>4;
									A=i;
									if(i>0xFF)
										i=1;
									else
										i=0;
									INCPC;
									setflags(A,-1,-1,j,-1,i);
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xCF:	/* RST 1 */
									RST(1);
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xD0:	/* RNC */
									INCPC;
									if(trace)
										strcat(p,"RNC");
									if(!CF) {
										DORET;
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xD1:	/* POP D */
									POP(DE,D,E,"D");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xD2:	/* JNC addr */
									INCPC;
									if(trace)
										sprintf(p,"JNC %04x",INS16);
									if(!CF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xD3:	/* OUT port */
									INCPC;
									if(trace)
										sprintf(p,"OUT %02x",INS);
									out(INS,A);
									INCPC;
									cpu_delay(10);
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xD4:	/* CNC addr */
									INCPC;
									if(trace)
										sprintf(p,"CNC %04x",INS);
									INCPC2;
									if(!CF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xD5:	/* PUSH D */
									PUSH(DE,D,E,"D");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xD6:	/* SUI n */
									INCPC;
									if(trace)
										sprintf(p,"SUI %02x",INS);
									i=A;
									i-=INS;
									j = (((A&0x0F)-(INS&0x0F)) &0x10)>>4;
									A=i;
									if (i>0xFF)
										i = 1;
									else
										i = 0;
									INCPC;
									setflags(A,-1,-1,j,-1,i);
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xD7:	/* RST 2 */
									RST(2);
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xD8:	/* RC */
									if(trace)
										strcat(p,"RC");
									INCPC;
									if(CF) {
										DORET;
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xD9:	/* INVALID? */
									bail("0xD9 is invalid");
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xDA:	/* JC addr */
									INCPC;
									if(trace)
										sprintf(p,"JC %04x",INS16);
									if(CF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
								}
								else {
									/* case 0xDB:	/* IN port */
									INCPC;
									if(trace)
										sprintf(p,"IN %x",INS);
									A=inport(INS);
									INCPC;
									cpu_delay(10);
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xDC:	/* CC addr */
									INCPC;
									if(trace)
										sprintf(p,"CC %04x",INS16);
									INCPC2;
									if(CF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xDD:	/* INVALID? */
									bail("0xDD is invalid");
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xDE:	/* SBI n */
									INCPC;
									if(trace)
										sprintf(p,"SBI %02x",INS);
									i=A;
									i-=INS;
									i-=(CF?1:0);
									j = (((A&0x0F)-(INS&0x0F)-(CF?1:0)) &0x10)>>4;
									A=i;
									if (i>0xFF)
										i = 1;
									else
										i = 0;
									setflags(A,-1,-1,j,-1,i);
									INCPC;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xDF:	/* RST 3 */
									RST(3);
								}
							}
						}
					}
				}
			}
			else {
				if(!(INS&0x10)) {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xE0:	/* RPO */
									INCPC;
									if(!PF) {
										DORET;
									}
									cpu_delay(6);
								}
								else {
									/* case 0xE1:	/* POP H */
									POP(HL,H,L,"H");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xE2:	/* JPO addr */
									INCPC;
									if(trace)
										sprintf(p,"JPO %04x",INS16);
									if(!PF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
								}
								else {
									/* case 0xE3:	/* XTHL */
									INCPC;
									if(trace)
										strcat(p,"XTHL");
									i=H;
									j=L;
									L=MEM(SP);
									H=MEM(SP+1);
									/* HL=MEM16(SP); */
									MEM(SP)=j;
									MEM(SP+1)=i;
									/* MEM16(SP)=i; */
									cpu_delay(16);
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xE4:	/* CPO addr */
									INCPC;
									INCPC2;
									if(!PF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xE5:	/* PUSH H */
									PUSH(HL,H,L,"H");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xE6:	/* ANI n */
									INCPC;
									if(trace)
										sprintf(p,"ANI %02x",INS);
									A=A&INS;
									INCPC;
									setflags(A,-1,-1,1,-1,0);
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xE7:	/* RST 4 */
									RST(4);
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xE8:	/* RPE */
									INCPC;
									if(trace)
										strcat(p,"RPE");
									if(PF) {
										DORET;
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xE9:	/* PCHL */
									PCH=H;
									PCL=L;
									/* PC=HL; */
									cpu_delay(6);
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xEA:	/* JPE addr */
									INCPC;
									if(trace)
										sprintf(p,"JPE %04x",INS16);
									if(PF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xEB:	/* XCHG */
									if(trace)
										strcat(p,"XCHG");
									INCPC;
									i=H;
									H=D;
									D=i;
									i=L;
									L=E;
									E=i;
									/* i=HL; */
									/* HL=DE; */
									/* DE=i; */
									cpu_delay(4);
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xEC:	/* CPE addr */
									INCPC;
									if(trace)
										sprintf(p,"CPE %04x",INS16);
									INCPC2;
									if(PF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xED:	/* INVALID? */
									bail("0xED is invalid");
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xEE:	/* XRI n */
									INCPC;
									if(trace)
										sprintf(p,"XRI %02x",INS);
									A=A^INS;
									INCPC;
									setflags(A,-1,-1,0,-1,0);
									cpu_delay(4);
									/* return; */
								}
								else {
									/* case 0xEF:	/* RST 5 */
									RST(5);
								}
							}
						}
					}
				}
				else {
					if(!(INS&0x08)) {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xF0:	/* RP */
									INCPC;
									if(trace)
										strcat(p,"RP");
									if(!SF) {
										DORET;
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xF1:	/* POP PSW */
									POP(AF,A,F,"PSW");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xF2:	/* JP addr */
									INCPC;
									if(trace)
										sprintf(p,"JP %04x",INS16);
									if(!SF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xF3:	/* DI */
									INCPC;
									if(trace)
										strcat(p,"DI");
									IM|=0x08;
									cpu_delay(4);
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xF4:	/* CP addr */
									INCPC;
									if(trace)
										sprintf(p,"CP %04x",INS16);
									INCPC2;
									if(!SF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xF5:	/* PUSH PSW */
									PUSH(AF,A,F,"PSW");
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xF6:	/* ORI n */
									INCPC;
									if(trace)
										sprintf(p,"ORI %02x",INS);
									A=A|INS;
									INCPC;
									setflags(A,-1,-1,0,-1,0);
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xF7:	/* RST 6 */
									RST(6);
								}
							}
						}
					}
					else {
						if(!(INS&0x04)) {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xF8:	/* RM */
									INCPC;
									if(trace)
										strcat(p,"RM");
									if(SF) {
										DORET;
									}
									cpu_delay(6);
									/* return; */
								}
								else {
									/* case 0xF9:	/* SPHL */
									INCPC;
									if(trace)
										strcat(p,"SPHL");
									SPH=H;
									SPL=L;
									cpu_delay(6);
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xFA:	/* JM addr */
									INCPC;
									if(trace)
										sprintf(p,"JM %04x",INS16);
									if(SF) {
										SETPCINS16;
										cpu_delay(3);
									}
									else
										INCPC2;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xFB:	/* EI */
									if(trace)
										strcat(p,"EI");
									INCPC;
									IM&=0xF7;
									cpu_delay(4);
									/* return; */
								}
							}
						}
						else {
							if(!(INS&0x02)) {
								if(!(INS&0x01)) {
									/* case 0xFC:	/* CM addr */
									INCPC;
									if(trace)
										sprintf(p,"CM %04x",INS16);
									INCPC2;
									if(SF) {
										DOCALL;
									}
									cpu_delay(9);
									/* return; */
								}
								else {
									/* case 0xFD:	/* INVALID? */
									bail("0xFD is invalid");
									/* return; */
								}
							}
							else {
								if(!(INS&0x01)) {
									/* case 0xFE:	/* CPI n */
									INCPC;
									if(trace)
										p+=sprintf(p,"CPI %02x",INS);
									i=A;
									i-=INS;
									if(i>0xFF)
										i=1;
									else
										i=0;
									j = (((A & 0x0F)-(INS & 0x0F)) & 0x10) >> 4;
									setflags(A-INS,-1,-1,j,-1,i);
									INCPC;
									cpu_delay(7);
									/* return; */
								}
								else {
									/* case 0xFF:	/* RST 7 */
									RST(7);
								}
							}
						}
					}
				}
			}
		}
	}
}
