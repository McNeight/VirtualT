/* do_instruct.h */

/* $Id: do_instruct.h,v 1.2 2004/08/05 07:19:25 deuce Exp $ */

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
							l=INS; \
							INCPC; \
							h=INS; \
							INCPC; \
							cycle_delta+=(10); \
						}

#define STAX(rp,rps)	{ \
							INCPC; \
							MEMSET(rp,A); \
							cycle_delta+=(7); \
						}

#define INX(rp,h,l,rps)	{ \
							INCPC; \
							F &= ~(OV_BIT|XF_BIT); \
							if ((l==0xFF) && (h==0xFF)) \
								F |= XF_BIT; \
							l++; \
							if(!l) { \
								h++; \
								F |= OV_BIT; \
							} \
							cycle_delta+=(6); \
						}

#define INR(r,rs)		{ \
							INCPC; \
							r++; \
							if((r&0x0f)==0) /* Low order nybble wrapped */ \
								j=1; \
							else \
								j=0; \
							setflags(r,-1,-1,j,-1,-2, (r==0x80) || (r==0), -2); \
							cycle_delta+=(4); \
						}

#define DCR(r,rs)		{ \
							INCPC; \
							r--; \
							if((r&0x0f)==0x0f) /* Low order nybble wrapped */ \
								j=1; \
							else \
								j=0; \
							setflags(r,-1,-1,j,-1,-2, (r==0xFF) || (r==0x7F), -2); \
							cycle_delta+=(4); \
						}

#define MVI(r,rs)		{ \
							INCPC; \
							r=INS; \
							INCPC; \
							cycle_delta+=(7); \
						}

#define DAD(rp,rps)		{ \
							INCPC; \
							i=HL; \
							i+=rp; \
							j=(HL&0x8000)==(rp&0x8000); \
							H=(i>>8); \
							L=i; \
							if(i>0xffff) \
								i=1; \
							else \
								i=0; \
							if (j) \
								j = (HL&0x8000) != (rp&0x8000); \
							setflags(0,-2,-2,-2,-2,i,j,-2 ); \
							cycle_delta+=(10); \
						}

#define LDAX(rp,rps)	{ \
							INCPC; \
							A=MEM(rp); \
							cycle_delta+=(7); \
						}

#define DCX(rp,h,l,rps)	{ \
							INCPC; \
							F &= ~(OV_BIT|XF_BIT); \
							if ((l == 0) && (h == 0)) \
								F |= XF_BIT; \
							l--; \
							if(l==0xff) { \
								F |= OV_BIT; \
								h--; \
							} \
							cycle_delta+=(6); \
						}

#define MOV(dest,src,ds,ss)	{ \
							INCPC; \
							dest=src; \
							cycle_delta+=(4); \
						}

#define MOVM(src,ss)	{ \
							INCPC; \
							MEMSET(HL, src); \
							cycle_delta+=(7); \
						}

#define ADD(r,rs)		{ \
							INCPC; \
							i=A; \
							i+=r; \
							v=(A&0x80) == (r&0x80); \
							j = (((A&0x0F)+(r&0x0F)) &0x10)>>4; \
							A=i; \
							if(i>0xFF) \
								i=1; \
							else \
								i=0; \
							if (v)	\
								v = (A&0x80) != (r&0x80); \
							setflags(A,-1,-1,j,-1,i,v,-2); \
							cycle_delta+=(4); \
						}

#define ADC(r,rs)		{ \
							INCPC; \
							i=A; \
							i+=r; \
							i+=(CF?1:0); \
							v=(A&0x80) == (r&0x80); \
							j = (((A&0x0F)+(r&0x0F)+(CF?1:0)) &0x10)>>4; \
							A=i; \
							if(i>0xFF) \
								i=1; \
							else \
								i=0; \
							if (v) \
								v = (A&0x80) != (r&0x80); \
							setflags(A,-1,-1,j,-1,i,v,-2); \
							cycle_delta+=(4); \
						}

#define SUB(r,rs)		{ \
							INCPC; \
							i=A; \
							i-=r; \
							j = (((A & 0x0F)-(r & 0x0F)) & 0x10) >> 4; \
							v=(A&0x80) == (r&0x80); \
							A=i; \
							if(i>0xFF) \
								i=1; \
							else \
								i=0; \
							if (v) \
								v = (A&0x80) != (r&0x80); \
							setflags(A,-1,-1,j,-1,i,v,-2); \
							cycle_delta+=(4); \
						}

#define SBB(r,rs)		{ \
							INCPC; \
							i=A; \
							i-=r; \
							i-=(CF?1:0); \
							j = (((A & 0x0F)-(r & 0x0F)-(CF?1:0)) & 0x10) >> 4; \
							v=(A&0x80) == (r&0x80);	\
							A=i; \
							if (i>0xFF) \
								i = 1; \
							else \
								i = 0; \
							if (v) \
								v = (A&0x80) != (r&0x80); \
							setflags(A,-1,-1,j,-1,i,v,-2); \
							cycle_delta+=(4); \
						}

#define ANA(r,rs)		{ \
							INCPC; \
							A&=r; \
							setflags(A,-1,-1,1,-1,0,-2,-2); \
							cycle_delta+=(4); \
						}

#define XRA(r,rs)		{ \
							INCPC; \
							A^=r; \
							setflags(A,-1,-1,0,-1,0,-2,-2); \
							cycle_delta+=(4); \
						}

#define ORA(r,rs)		{ \
							INCPC; \
							A|=r; \
							setflags(A,-1,-1,0,-1,0,-2,-2); \
							cycle_delta+=(4); \
						}

#define CMP(r,rs)		{ \
							INCPC; \
							i=A; \
							i-=r; \
							if (i>0xFF) \
								i=1; \
							else \
								i=0; \
							j = (((A & 0x0F)-(r & 0x0F)) & 0x10) >> 4; \
							if ((A&0x80) == (r&0x80)) \
								v = (A&0x80) != (r&0x80); \
							else \
								v = 0; \
							setflags(A-r,-1,-1,j,-1,i,v,-2); \
							cycle_delta+=(4); \
						}

#define POP(rp,h,l,rps)	{ \
							INCPC; \
							l=MEM(SP); \
							h=MEM(SP+1); \
							INCSP2; \
							cycle_delta+=(10); \
						}

#define PUSH(rp,h,l,rps)	{ \
							INCPC; \
							DECSP2; \
							MEMSET(SP,l); \
							MEMSET(SP+1,h); \
							cycle_delta+=(12); \
						}

#define RST(num)		{ \
							INCPC; \
							DECSP2; \
							MEMSET(SP,PCL); MEMSET(SP+1,PCH); \
							PCH=0; \
							PCL=8*num; \
							cycle_delta+=(12); \
						}

#define CALL(cond,ins)	{ \
							INCPC; \
							INCPC2; \
							if(cond) { \
								DECSP2; \
								MEMSET(SP,PCL); MEMSET(SP+1,PCH); /* MEM16(SP)=PC; */ \
								DECPC2; \
								SETPCINS16; \
								cycle_delta+=(9); \
							} \
							cycle_delta+=(9); \
						}

#define	RETURN(cond,ins)	{ \
							INCPC; \
							if(cond) { \
								PCL=MEM(SP); PCH=MEM(SP+1); /* PC=MEM16(SP) */ \
								INCSP2; \
								cycle_delta+=(6); \
							} \
							cycle_delta+=(6); \
						}

#define JUMP(cond,ins)	{ \
							INCPC; \
							if(cond) { \
								SETPCINS16; \
								cycle_delta+=(3); \
							} \
							else \
								INCPC2; \
							cycle_delta+=(7); \
						}

{
//	if(trace)
//		p=op+sprintf(op,"%04x (%02x) ",PC,INS);
ins = INS;
	if(!(ins&0x80)) {
		if(!(ins&0x40)) {
			if(!(ins&0x20)) {
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x00:	/* NOP */
									INCPC;
									cycle_delta+=(4);
									/* return; */
								}
								else {
									/* case 0x01:	/* LXI B */
									LXI(BC,B,C,"B");
								}
							}
							else  {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x04:	/* INR B */
									INR(B,"B");
								}
								else {
									/* case 0x05:	/* DCR B */
									DCR(B,"B");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x06:	/* MVI B */
									MVI(B,"B");
								}
								else {
									/* case 0x07:	/* RLC */
									INCPC;
									i=A>>7;
									A=(A<<1)|i;
									setflags(A,-2,-2,-2,-2,i,-2,-2);
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x08:	/* DSUB */
									INCPC;
									i = HL < BC;// - (CF?1:0);
									j = HL - BC;// - (CF?1:0);
									v = (HL&0x8000) == (BC&0x8000);
									L=j&0xFF;
									H=(j >> 8) & 0xFF;
									if (v)
										v = (HL&0x8000) != (BC&0x8000);
									setflags(H | L,H&0x80,-1,-2,-2,i,v,-2);
									cycle_delta+=(10);
									/* return; */
								}
								else {
									/* case 0x09:	/* DAD B */
									DAD(BC,"B");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x0C:	/* INR C */
									INR(C,"C");
								}
								else {
									/* case 0x0D:	/* DCR C */
									DCR(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x0E:	/* MVI C */
									MVI(C,"C");
								}
								else {
									/* case 0x0F:	/* RRC */
									INCPC;
									i=A<<7&0x80;
									A=(A>>1)|i;
									i>>=7;
									setflags(A,-2,-2,-2,-2,i,-2,-2);
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
					}
				}
				else {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x10:	/* ASHR */
									INCPC;
									i = L & CF ;
									j = HL >> 1;
									L = j & 0xFF;
									H = (H & 0x80) | ((j >> 8) & 0xFF);
									setflags(0,-2,-2,-2,-2,i,-2,-2);
									cycle_delta+=(7);
									/* return; */
								}
								else {
									/* case 0x11:	/* LXI D */
									LXI(DE,D,E,"D");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x14:	/* INR D */
									INR(D,"D");
								}
								else {
									/* case 0x15:	/* DCR D */
									DCR(D,"D");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x16:	/* MVI D */
									MVI(D,"D");
								}
								else {
									/* case 0x17:	/* RAL */
									INCPC;
									i=A>>7;
									A<<=1;
									A|=(CF?1:0);
									setflags(A,-2,-2,-2,-2,i,-2,-2);
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x18:	/* RLDE */
									INCPC;
									i = D & 0x80 ? 1 : 0;
									j = DE << 1;
									E = (j & 0xFF) | (CF ? 1 : 0);
									D = (j >> 8) & 0xFF;
									setflags(0,-2,-2,-2,-2,i,-2,-2);
									cycle_delta+=(10);
									/* return; */
								}
								else {
									/* case 0x19:	/* DAD D */
									DAD(DE,"D");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x1C:	/* INR E */
									INR(E,"E");
								}
								else {
									/* case 0x1D:	/* DCR E */
									DCR(E,"E");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x1E:	/* MVI E */
									MVI(E,"E");
								}
								else {
									/* case 0x1F:	/* RAR */
									INCPC;
									i=(A&0x01);
									A>>=1;
									A|=CF?0x80:0;
									setflags(A,-2,-2,-2,-2,i,-2,-2);
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
					}
				}
			}
			else {
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x20:	/* RIM */
									INCPC;
									A=IM;
									cycle_delta+=(4);
									/* return; */
								}
								else {
									/* case 0x21:	/* LXI H */
									LXI(HL,H,L,"H");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x22:	/* SHLD */
									INCPC;
									MEMSET(INS16,L);
									MEMSET(INS16+1,H);
									/* MEM16(INS16)=HL; */
									INCPC2;
									cycle_delta+=(16);
									/* return; */
								}
								else {
									/* case 0x23:	/* INX H */
									INX(HL,H,L,"H");
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x24:	/* INR H */
									INR(H,"H");
								}
								else {
									/* case 0x25:	/* DCR H */
									DCR(H,"H");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x26:	/* MVI H */
									MVI(H,"H");
								}
								else {
									/* case 0x27:	/* DAA */
									INCPC;
									i=j=0;
									/* Check if lower nibble greater than 9 */
									if(((A&0x0F) > 9)) {
										i=6;	/* Add 6 to lower nibble */
										j=1;    /* Aux carry to upper nibble */
									}
									/* Check for Aux Carry from last operation */
									if(AC)
										i=6;	/* Add 6 to lower nibble */
									/* Check if upper nibble will be greater than 9 */
									/* after any carry's are added */
									if((((A>>4)+j) > 9) || CF)
										i|=0x60;/* Add 6 to upper nibble */
									A+=i;
									setflags(A,-1,-1,j,-1,i>>4?1:0,-2,-2);
									cycle_delta+=(4);
									/* return; */ /* Huh? */
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x28:	/* LDEH */
									INCPC;
									j = HL + INS;
									INCPC;
									E = j & 0xFF;
									D = (j >> 8) & 0xFF;
									cycle_delta+=(10);
									/* return; */
								}
								else {
									/* case 0x29:	/* DAD H */
									DAD(HL,"H");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x2A:	/* LHLD */
									INCPC;
									L=MEM(INS16);
									H=MEM(INS16+1);
									INCPC2;
									cycle_delta+=(16);
								}
								else {
									/* case 0x2B:	/* DCX H */
									DCX(HL,H,L,"H");
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x2C:	/* INR L */
									INR(L,"L");
								}
								else {
									/* case 0x2D:	/* DCR L */
									DCR(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x2E:	/* MVI L */
									MVI(L,"L");
								}
								else {
									/* case 0x2F:	/* CMA */
									INCPC;
									A=~A;
									cycle_delta+=(4);
								}
							}
						}
					}
				}
				else {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x30:	/* SIM */
									INCPC;
									if (A & 0x08)	/* Check if Interrupt masking enabled */
										IM=(IM & 0xF8) | (A&0x07);
									/* Turn RST 7.5 pending off if bit 4 set */
									if(A&0x10) {
										IM&=0xBF;
									}
									cycle_delta+=(4);
									/* return; */
								}
								else {
									/* case 0x31:	/* LXI SP */
									LXI(SP,SPH,SPL,"SP");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x32:	/* STA */
									INCPC;
									MEMSET(INS16,A);
									INCPC2;
									cycle_delta+=(13);
									/* return; */
								}
								else {
									/* case 0x33:	/* INX SP */
									INX(SP,SPH,SPL,"SP");
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x34:	/* INR M */
									INCPC;
									MEMSET(HL,(uchar) (M+1));
									if((M&0x0f)==0) /* Low order nybble wrapped */
										j=1;
									else
										j=0;
									setflags(M,-1,-1,j,-1,-2, (M==0x80) || (M==0), -2);
									cycle_delta+=(10);
								}
								else {
									/* case 0x35:	/* DCR M */
									INCPC;
									MEMSET(HL, (uchar)(M-1));
									if((M&0x0f)==0x0f) /* Low order nybble wrapped */
										j=1;
									else 
										j=0;
									setflags(M,-1,-1,j,-1,-2, (M==0xFF) || (M==0x7F), -2);
									cycle_delta+=(10);
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x36:	/* MVI M */
									INCPC;
									MEMSET(HL,INS);
									INCPC;
									cycle_delta+=(10);
								}
								else {
									/* case 0x37:	/* STC */
									INCPC;
									setflags(0,-2,-2,-2,-2,1,-2,-2);
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x38:	/* LDES */
									INCPC;
									j = SP + INS;
									INCPC;
									E = j & 0xFF;
									D = (j >> 8) & 0xFF;
									cycle_delta+=(10);
									/* return; */
								}
								else {
									/* case 0x39:	/* DAD SP */
									DAD(SP,"SP");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x3A:	/* LDA */
									INCPC;
									A=MEM(INS16);
									INCPC2;
									cycle_delta+=(13);
									/* return; */
								}
								else {
									/* case 0x3B:	/* DCX SP */
									DCX(SP,SPH,SPL,"SP");
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x3C:	/* INR A */
									INR(A,"A");
								}
								else {
									/* case 0x3D:	/* DCR A */
									DCR(A,"A");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x3E:	/* MVI A */
									MVI(A,"A");
								}
								else {
									/* case 0x3F:	/* CMC */
									INCPC;
									setflags(A,-2,-2,-2,-2,CF?0:1,-2,-2);
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
					}
				}
			}
		}
		else {
			if(!(ins&0x20)) {
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x40:	/* MOV B,B */
									MOV(B,B,"B","B");
								}
								else {
									/* case 0x41:	/* MOV B,C */
									MOV(B,C,"B","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x44:	/* MOV B,H */
									MOV(B,H,"B","H");
								}
								else {
									/* case 0x45:	/* MOV B,L */
									MOV(B,L,"B","L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x46:	/* MOV B,M */
									MOV(B,M,"B","M");
									cycle_delta+=(3);
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
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x48:	/* MOV C,B */
									MOV(C,B,"C","B");
								}
								else {
									/* case 0x49:	/* MOV C,C */
									MOV(C,C,"C","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x4C:	/* MOV C,H */
									MOV(C,H,"C","H");
								}
								else {
									/* case 0x4D:	/* MOV C,L */
									MOV(C,L,"C","L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x4E:	/* MOV C,M */
									MOV(C,M,"C","M");
									cycle_delta+=(3);
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
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x50:	/* MOV D,B */
									MOV(D,B,"D","B");
								}
								else {
									/* case 0x51:	/* MOV D,C */
									MOV(D,C,"D","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x54:	/* MOV D,H */
									MOV(D,H,"D","H");
								}
								else {
									/* case 0x55:	/* MOV D,L */
									MOV(D,L,"D","L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x56:	/* MOV D,M */
									MOV(D,M,"D","M");
									cycle_delta+=(3);
								}
								else {
									/* case 0x57:	/* MOV D,A */
									MOV(D,A,"D","A");
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x58:	/* MOV E,B */
									MOV(E,B,"E","B");
								}
								else {
									/* case 0x59:	/* MOV E,C */
									MOV(E,C,"E","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x5C:	/* MOV E,H */
									MOV(E,H,"E","H");
								}
								else {
									/* case 0x5D:	/* MOV E,L */
									MOV(E,L,"E","L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x5E:	/* MOV E,M */
									MOV(E,M,"E","M");
									cycle_delta+=(3);
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
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x60:	/* MOV H,B */
									MOV(H,B,"H","B");
								}
								else {
									/* case 0x61:	/* MOV H,C */
									MOV(H,C,"H","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x64:	/* MOV H,H */
									MOV(H,H,"H","H");
								}
								else {
									/* case 0x65:	/* MOV H,L */
									MOV(H,L,"H","L");
								}
							}
							else {
								if(!(ins&0x01)) {
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
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x68:	/* MOV L,B */
									MOV(L,B,"L","B");
								}
								else {
									/* case 0x69:	/* MOV L,C */
									MOV(L,C,"L","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x6C:	/* MOV L,H */
									MOV(L,H,"L","H");
								}
								else {
									/* case 0x6D:	/* MOV L,L */
									MOV(L,L,"L","L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x6E:	/* MOV L,M */
									MOV(L,M,"L","M");
									cycle_delta+=(3);
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
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x70:	/* MOV M,B */
									MOVM(B,"B");
								}
								else {
									/* case 0x71:	/* MOV M,C */
									MOVM(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x72:	/* MOV M,D */
									MOVM(D,"D");
								}
								else {
									/* case 0x73:	/* MOV M,E */
									MOVM(E,"E");
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x74:	/* MOV M,H */
									MOVM(H,"H");
								}
								else {
									/* case 0x75:	/* MOV M,L */
									MOVM(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x76:	/* HLT */
									cycle_delta+=(4);
									/* return; */
								}
								else {
									/* case 0x77:	/* MOV M,A */
									MOVM(A,"A");
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x78:	/* MOV A,B */
									MOV(A,B,"A","B");
								}
								else {
									/* case 0x79:	/* MOV A,C */
									MOV(A,C,"A","C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x7C:	/* MOV A,H */
									MOV(A,H,"A","H");
								}
								else {
									/* case 0x7D:	/* MOV A,L */
									MOV(A,L,"A","L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x7E:	/* MOV A,M */
									MOV(A,M,"A","M");
									cycle_delta+=(3);
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
		if(!(ins&0x40)) {
			if(!(ins&0x20)) {
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x80:	/* ADD B */
									ADD(B,"B");
								}
								else {
									/* case 0x81:	/* ADD C */
									ADD(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x84:	/* ADD H */
									ADD(H,"H");
								}
								else {
									/* case 0x85:	/* ADD L */
									ADD(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x86:	/* ADD M */
									ADD(M,"M");
									cycle_delta+=(3);
								}
								else {
									/* case 0x87:	/* ADD A */
									ADD(A,"A");
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x88:	/* ADC B */
									ADC(B,"B");
								}
								else {
									/* case 0x89:	/* ADC C */
									ADC(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x8C:	/* ADC H */
									ADC(H,"H");
								}
								else {
									/* case 0x8D:	/* ADC L */
									ADC(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x8E:	/* ADC M */
									ADC(M,"M");
									cycle_delta+=(3);
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
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x90:	/* SUB B */
									SUB(B,"B");
								}
								else {
									/* case 0x91:	/* SUB C */
									SUB(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x94:	/* SUB H */
									SUB(H,"H");
								}
								else {
									/* case 0x95:	/* SUB L */
									SUB(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x96:	/* SUB M */
									SUB(M,"M");
									cycle_delta+=(3);
								}
								else {
									/* case 0x97:	/* SUB A */
									SUB(A,"A");
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x98:	/* SBB B */
									SBB(B,"B");
								}
								else {
									/* case 0x99:	/* SBB C */
									SBB(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0x9C:	/* SBB H */
									SBB(H,"H");
								}
								else {
									/* case 0x9D:	/* SBB L */
									SBB(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0x9E:	/* SBB M */
									SBB(M,"M");
									cycle_delta+=(3);
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
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xA0:	/* ANA B */
									ANA(B,"B");
								}
								else {
									/* case 0xA1:	/* ANA C */
									ANA(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xA4:	/* ANA H */
									ANA(H,"H");
								}
								else {
									/* case 0xA5:	/* ANA L */
									ANA(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xA6:	/* ANA M */
									ANA(M,"M");
									cycle_delta+=(3);
								}
								else {
									/* case 0xA7:	/* ANA A */
									ANA(A,"A");
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xA8:	/* XRA B */
									XRA(B,"B");
								}
								else {
									/* case 0xA9:	/* XRA C */
									XRA(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xAC:	/* XRA H */
									XRA(H,"H");
								}
								else {
									/* case 0xAD:	/* XRA L */
									XRA(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xAE:	/* XRA M */
									XRA(M,"M");
									cycle_delta+=(3);
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
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xB0:	/* ORA B */
									ORA(B,"B");
								}
								else {
									/* case 0xB1:	/* ORA C */
									ORA(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xB4:	/* ORA H */
									ORA(H,"H");
								}
								else {
									/* case 0xB5:	/* ORA L */
									ORA(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xB6:	/* ORA M */
									ORA(M,"M");
									cycle_delta+=(3);
								}
								else {
									/* case 0xB7:	/* ORA A */
									ORA(A,"A");
								}
							}
						}
					}
					else {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xB8:	/* CMP B */
									CMP(B,"B");
								}
								else {
									/* case 0xB9:	/* CMP C */
									CMP(C,"C");
								}
							}
							else {
								if(!(ins&0x01)) {
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
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xBC:	/* CMP H */
									CMP(H,"H");
								}
								else {
									/* case 0xBD:	/* CMP L */
									CMP(L,"L");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xBE:	/* CMP M */
									CMP(M,"M");
									cycle_delta+=(3);
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
			if(!(ins&0x20)) {
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xC0:	/* RNZ */
									INCPC;
									if(!ZF) {
										PCL=MEM(SP); PCH=MEM(SP+1); /* PC=MEM16(SP) */
										INCSP2;
										cycle_delta+=(6);
									}
									cycle_delta+=(6);
									/* return; */
								}
								else {
									/* case 0xC1:	/* POP B */
									POP(BC,B,C,"B");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xC2:	/* JNZ addr */
									JUMP(!ZF,"JNZ");
								}
								else {
									/* case 0xC3:	/* JMP addr */
									JUMP(1,"JMP");
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xC4:	/* CNZ addr */
									CALL(!ZF,"CNZ");
								}
								else {
									/* case 0xC5:	/* PUSH B */
									PUSH(BC,B,C,"B");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xC6:	/* ADI n */
									INCPC;
									i=A;
									i+=INS;
									j = (((A&0x0F)+(INS&0x0F)) &0x10)>>4;
									v = (A&0x80) == (INS&0x80);
									A=i;
									if(i>0xFF)
										i=1;
									else
										i=0;
									if (v)
										v = (A&0x80) != (INS&0x80);
									setflags(A,-1,-1,j,-1,i,v,-2);
									cycle_delta+=(7);
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
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xC8:	/* RZ */
									RETURN(ZF,"RZ");
								}
								else {
									/* case 0xC9:	/* RET */
									RETURN(1,"RET");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xCA:	/* JZ addr */
									JUMP(ZF,"JZ");
								}
								else {
									/* case 0xCB:	/* RSTV */
									if (OV)
										RST(8);
									/* return; */
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xCC:	/* CZ addr */
									CALL(ZF,"CZ");
								}
								else {
									/* case 0xCD:	/* CALL addr */
									CALL(1,"CALL");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xCE:	/* ACI n */
									INCPC;
									i=A;
									i+=INS;
									i+=(CF?1:0);
									j = (((A&0x0F)+(INS&0x0F)+(CF?1:0)) &0x10)>>4;
									v = (A&0x80) == (INS&0x80);
									A=i;
									if(i>0xFF)
										i=1;
									else
										i=0;
									if (v)
										v = (A&0x80) != (INS&0x80);
									setflags(A,-1,-1,j,-1,i,v,-2);
									cycle_delta+=(7);
									INCPC;
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
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xD0:	/* RNC */
									RETURN(!CF,"RNC");
								}
								else {
									/* case 0xD1:	/* POP D */
									POP(DE,D,E,"D");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xD2:	/* JNC addr */
									JUMP(!CF,"JNC");
								}
								else {
									/* case 0xD3:	/* OUT port */
									INCPC;
									out(INS,A);
									INCPC;
									cycle_delta+=(10);
									/* return; */
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xD4:	/* CNC addr */
									CALL(!CF,"CNC");
								}
								else {
									/* case 0xD5:	/* PUSH D */
									PUSH(DE,D,E,"D");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xD6:	/* SUI n */
									INCPC;
									i=A;
									i-=INS;
									j = (((A&0x0F)-(INS&0x0F)) &0x10)>>4;
									v = (A&0x80) == (INS&0x80);
									A=i;
									if (i>0xFF)
										i = 1;
									else
										i = 0;
									if (v)
										v = (A&0x80) != (INS&0x80);
									setflags(A,-1,-1,j,-1,i,v,-2);
									cycle_delta+=(7);
									INCPC;
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
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xD8:	/* RC */
									RETURN(CF,"RC");
								}
								else {
									/* case 0xD9:	/* SHLX */
									INCPC;
									MEMSET(DE, L);
									MEMSET(DE+1, H);
									cycle_delta+=(10);
									/* return; */
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xDA:	/* JC addr */
									JUMP(CF,"JC");
								}
								else {
									/* case 0xDB:	/* IN port */
									INCPC;
									A=inport(INS);
									INCPC;
									cycle_delta+=(10);
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xDC:	/* CC addr */
									CALL(CF,"CC");
								}
								else {
									/* case 0xDD:	/* JNX addr */
									JUMP(!XF,"JNX");
									/* return; */
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xDE:	/* SBI n */
									INCPC;
									i=A;
									i-=INS;
									i-=(CF?1:0);
									j = (((A&0x0F)-(INS&0x0F)-(CF?1:0)) &0x10)>>4;
									v = (A&0x80) == (INS&0x80);
									A=i;
									if (i>0xFF)
										i = 1;
									else
										i = 0;
									if (v)
										v = (A&0x80) != (INS&0x80);
									setflags(A,-1,-1,j,-1,i,v,-2);
									INCPC;
									cycle_delta+=(7);
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
				if(!(ins&0x10)) {
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xE0:	/* RPO */
									RETURN(!PF,"RPO");
								}
								else {
									/* case 0xE1:	/* POP H */
									POP(HL,H,L,"H");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xE2:	/* JPO addr */
									JUMP(!PF,"JPO");
								}
								else {
									/* case 0xE3:	/* XTHL */
									INCPC;
									i=H;
									j=L;
									L=MEM(SP);
									H=MEM(SP+1);
									/* HL=MEM16(SP); */
									MEMSET(SP, (uchar) j);
									MEMSET(SP+1, (uchar) i);
									/* MEM16(SP)=i; */
									cycle_delta+=(16);
									/* return; */
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xE4:	/* CPO addr */
									CALL(!PF,"CPO");
								}
								else {
									/* case 0xE5:	/* PUSH H */
									PUSH(HL,H,L,"H");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xE6:	/* ANI n */
									INCPC;
									A=A&INS;
									INCPC;
									setflags(A,-1,-1,1,-1,0,-2,-2);
									cycle_delta+=(7);
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
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xE8:	/* RPE */
									RETURN(PF,"RPE");
								}
								else {
									/* case 0xE9:	/* PCHL */
									PCH=H;
									PCL=L;
									/* PC=HL; */
									cycle_delta+=(6);
									/* return; */
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xEA:	/* JPE addr */
									JUMP(PF,"JPE");
								}
								else {
									/* case 0xEB:	/* XCHG */
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
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xEC:	/* CPE addr */
									CALL(PF,"CPE");
								}
								else {
									/* case 0xED:	/* LHLX */
									INCPC;
									L=MEM(DE);
									H=MEM(DE+1);
									cycle_delta+=(10);
									/* return; */
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xEE:	/* XRI n */
									INCPC;
									A=A^INS;
									INCPC;
									setflags(A,-1,-1,0,-1,0,-2,-2);
									cycle_delta+=(4);
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
					if(!(ins&0x08)) {
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xF0:	/* RP */
									RETURN(!SF,"RP");
								}
								else {
									/* case 0xF1:	/* POP PSW */
									POP(AF,A,F,"PSW");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xF2:	/* JP addr */
									JUMP(!SF,"JP");
								}
								else {
									/* case 0xF3:	/* DI */
									INCPC;
									IM|=0x08;
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xF4:	/* CP addr */
									CALL(!SF,"CP");
								}
								else {
									/* case 0xF5:	/* PUSH PSW */
									PUSH(AF,A,F,"PSW");
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xF6:	/* ORI n */
									INCPC;
									A=A|INS;
									INCPC;
									setflags(A,-1,-1,0,-1,0,-2,-2);
									cycle_delta+=(7);
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
						if(!(ins&0x04)) {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xF8:	/* RM */
									RETURN(SF,"RM");
								}
								else {
									/* case 0xF9:	/* SPHL */
									INCPC;
									SPH=H;
									SPL=L;
									cycle_delta+=(6);
									/* return; */
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xFA:	/* JM addr */
									JUMP(SF,"JM");
								}
								else {
									/* case 0xFB:	/* EI */
									INCPC;
									IM&=0xF7;
									cycle_delta+=(4);
									/* return; */
								}
							}
						}
						else {
							if(!(ins&0x02)) {
								if(!(ins&0x01)) {
									/* case 0xFC:	/* CM addr */
									CALL(SF,"CM");
								}
								else {
									/* case 0xFD:	/* JX addr */
									JUMP(XF,"JX");
									/* return; */
								}
							}
							else {
								if(!(ins&0x01)) {
									/* case 0xFE:	/* CPI n */
									INCPC;
									i=A;
									i-=INS;
									if(i>0xFF)
										i=1;
									else
										i=0;
									j = (((A & 0x0F)-(INS & 0x0F)) & 0x10) >> 4;
									if ((A&0x80) == (INS&0x80))
										v = (A&0x80) != (INS&0x80);
									else
										v = 0;
									setflags(A-INS,-1,-1,j,-1,i,v,-2);
									INCPC;
									cycle_delta+=(7);
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
