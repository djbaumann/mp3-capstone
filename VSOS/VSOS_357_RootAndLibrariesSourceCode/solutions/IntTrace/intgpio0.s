#include <vstypes.h>
#include <vs1005h.h>


/*
   GPIO0 interrupt routine.
   Never call explicitly!
   This routine calls void IntGpio0C(void);

   Set the interrupt vector from C as follows:

   extern u_int16 IntGpio0Vector;
   [...]
   WriteIMem((void *)(0x20+INTV_GPIO0), ReadIMem((void *)(&IntGpio0Vector)));

   After setting the interrupt vector, activate the interrupt as follows
     (example for interrupt priority 1):
   PERIP(INT_ENABLE0_LP) |= INTF_GPIO0;
   PERIP(INT_ENABLE0_HP) &= ~INTF_GPIO0;

   Before exiting your application, deactivate the interrupt as follows:
   PERIP(INT_ENABLE0_LP) &= ~INTF_GPIO0;
   PERIP(INT_ENABLE0_HP) &= ~INTF_GPIO0;
*/
	.sect code,IntGpio0Vector
	.export _IntGpio0Vector
_IntGpio0Vector:
	jmpi _IntGpio0Asm,(i6)+1

	.sect code,IntGpio0Asm
	.export _IntGpio0Asm
_IntGpio0Asm:
	stx i7,(i6)+1;	sty mr0,(i6)
	stx lr0,(i6)+1;	sty i5,(i6)
	stx a2,(i6)+1;	sty b2,(i6)
	stx c2,(i6)+1;	sty d2,(i6)
	stx a1,(i6)+1;	sty a0,(i6)
	add null,p,a
	stx a1,(i6);	sty a0,(i6)

	.import _IntGpio0C
	call _IntGpio0C
	ldc MR_INT,mr0

	ldx (i6)-1,a1;	ldy (i6),a0
	resp a0,a1
	ldx (i6)-1,a1;	ldy (i6),a0
	ldx (i6)-1,c2;	ldy (i6),d2
	ldx (i6)-1,a2;	ldy (i6),b2
	ldx (i6)-1,lr0;	ldy (i6),i5
	ldy (I6),mr0
	ldc INT_GLOB_ENA,i7
	reti
	ldx (i6)-1,I7;	sty i7,(i7)



	.end
