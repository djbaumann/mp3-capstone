#ifndef AUODAC_H
#define AUODAC_H

#include <vstypes.h>
#include <aucommon.h>

#ifndef ASM

extern struct AudioIn audioIn;
extern struct AudioOut audioOut;
extern SIMPLE_FILE audioFile;

ioresult AudioClose(void);

#endif /* !ASM */

#endif /* !AUXI2SM_H */
