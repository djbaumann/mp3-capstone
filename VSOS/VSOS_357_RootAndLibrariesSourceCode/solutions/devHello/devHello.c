
/// \file devHello.c VSOS3 example device driver
/// \author Panu-Kristian Poiksalo, VLSI Solution Oy

/// This driver creates a disk, which when any file is opened from that disk,
/// always returns the "Hello" mp3 data from the array below.
/// Try for instance to add files DEVHELLO H and PLAYFILE H: in your CONFIG.TXT


#include <vo_stdio.h>
#include <volink.h>     // Linker directives like DLLENTRY
#include <apploader.h>  // RunLibraryFunction etc
#include "simpledevice.h"
#include <string.h>

// Small MP3 file encoded with Lame and converted to C table with Bin2C.
u_int16 hello_mp3[] = { /* 16-bit big-endian table */
  0xFFF2,0x40C0,0x19B7,0x0014,0x02E6,0x5C01,0x9268,0x01F1, /*..@.......\..h..*/
  0x5E03,0x08F0,0x2480,0x059E,0x20C6,0xFC12,0x325C,0xBFF9, /*^...$... ...2\..*/
  0xB920,0x4A7F,0x85EC,0x4CCD,0xC727,0xFE5C,0x3425,0xCBE6, /*. J...L..'.\4%..*/
  0xFFFF,0x8E42,0xE1A0,0x5ECA,0x6E30,0x9FFF,0xF8C2,0x1284, /*...B..^.n0......*/
  0xB97C,0xDC61,0x094A,0x7FFF,0xFFF9,0x7D32,0x5109,0x7CE1, /*.|.a.J....}2Q.|.*/
  0xA56E,0xB4FF,0xFFFF,0xFFD3,0x3441,0x91F0,0x118F,0x000F, /*.n......4A......*/
  0x819C,0x10EE,0x59CE,0x5667,0xFFF2,0x42C0,0xEC53,0x0915, /*....Y.Vg..B..S..*/
  0xF9AA,0xA80D,0xD940,0x00CA,0x3453,0xD918,0xAB7D,0xF789, /*.....@..4S...}..*/
  0x3F11,0x3894,0x8259,0x9320,0x6A0C,0xEE8E,0x58FA,0x3882, /*?.8..Y. j...X.8.*/
  0xCAF0,0x58BB,0xDA0C,0x5056,0x1FBB,0x185D,0x8B9F,0xDA71, /*..X...PV...]...q*/
  0x4FFF,0xBDFE,0xEF69,0x3686,0x3C50,0xBB0A,0x0789,0x54F0, /*O....i6.<P....T.*/
  0x889F,0x9095,0x3094,0x2E7E,0xF064,0x9679,0x083E,0x2097, /*....0..~.d.y.> .*/
  0x2834,0x9C09,0x7FD2,0xC001,0x75F8,0x056B,0x5F41,0x170B, /*(4......u..k_A..*/
  0xE7FF,0xF240,0xC061,0xE50B,0x1609,0xC6C5,0x747B,0xCC94, /*...@.a......t{..*/
  0x7AF7,0x8076,0xB2D2,0xF839,0x0638,0xFD71,0xC5DE,0x3A38, /*z..v...9.8.q..:8*/
  0xBFD5,0xF712,0x37CB,0xF563,0x0C9B,0xCE77,0x25ED,0xFB3D, /*....7..c...w%..=*/
  0x6B35,0xF96D,0xD7F9,0x2CD1,0x9715,0x8793,0xA449,0x4A18, /*k5.m..,......IJ.*/
  0x1607,0xA160,0xF752,0x94DB,0x0216,0x70B2,0xD880,0x30C2, /*...`.R....p...0.*/
  0x9440,0x8174,0x5A19,0x7A80,0x6041,0x2146,0x95D5,0xC440, /*.@.tZ.z.`A!F...@*/
  0xD201,0xC001,0xDAD9,0xA0B1,0x01FF,0xF242,0xC082,0x100B, /*...........B....*/
  0x12F9,0x9EC9,0x7E7A,0xC695,0x5509,0x8B19,0x5E8B,0x26CA, /*....~z..U...^.&.*/
  0xEB68,0x8A05,0x8F36,0xA5A5,0x03B8,0x9CED,0x2451,0x5990, /*.h...6......$QY.*/
  0xF6C5,0x7DB5,0xADAF,0xF63B,0x18EF,0x3FFF,0xFF4E,0xDE16, /*..}....;..?..N..*/
  0x660B,0xAA33,0x23DD,0x9C4E,0x6E55,0x229D,0xA240,0xA636, /*f..3#..NnU"..@.6*/
  0x3169,0xA5E1,0xD97F,0xF7C6,0xCC48,0x000E,0x9016,0x000F, /*1i.......H......*/
  0xDE6E,0x8011,0x0C9A,0x4F56,0xDB88,0xD3B2,0x1C00,0xE02E, /*.n....OV........*/
  0x3EAC,0xFFF2,0x40C0,0x1CE5,0x1913,0x314E,0xCD9E,0xC306, /*>...@.....1N....*/
  0x7103,0x85E5,0xB56D,0x8850,0x8E0E,0x173B,0x19FB,0x4E3B, /*q....m.P...;..N;*/
  0x99EF,0x4C9E,0xF77B,0x317C,0x3C5F,0xFFF4,0xF8E3,0x9242, /*..L..{1|<_.....B*/
  0x078E,0x838E,0x0F05,0x0891,0xA316,0xE2DF,0xB762,0x6048, /*.............b`H*/
  0x313C,0xFFD4,0x9E0C,0x6800,0x7754,0xE31E,0x05C5,0xF8EA, /*1<....h.wT......*/
  0x8D82,0x9D08,0xA906,0x8D1E,0x5D7C,0x7F08,0xC050,0x4542, /*........]|...PEB*/
  0xD036,0xF8B2,0x4D53,0x0C80,0x3B4D,0xFFF2,0x42C0,0x2F3C, /*.6..MS..;M..B./<*/
  0x2519,0x29FE,0xBC2E,0xC4D0,0x994C,0x48B0,0x9C49,0xD21A, /*%.)......LH..I..*/
  0x2D02,0xC279,0x6916,0x92A8,0xC5AB,0x455A,0x68E8,0x7557, /*-..yi.....EZh.uW*/
  0xCDF1,0xB9AA,0x1388,0xE487,0x4215,0xB358,0xF5A3,0x46B1, /*........B..X..F.*/
  0xCFD3,0x597E,0xBAB5,0xA76B,0x0B17,0x576B,0x5C4A,0xCD53, /*..Y~...k..Wk\J.S*/
  0x762A,0x1D28,0xC51C,0x765C,0xDD0A,0x004B,0xC01B,0xCAA8, /*v*.(..v\...K....*/
  0xE981,0x5BA6,0xDCA4,0x5913,0xFCBA,0x8F98,0x7944,0x25C9, /*..[...Y.....yD%.*/
  0x3538,0xCAFF,0xF240,0xC0B9,0x7D1A,0x1379,0x6AC8,0x3EC4, /*58...@..}..yj.>.*/
  0x4694,0x8D3C,0x6785,0xB1A8,0x89C0,0xF2E6,0x2F9D,0x7CC9, /*F..<g......./.|.*/
  0xB4BE,0xCFE1,0x7DFE,0x1F03,0x0012,0x8472,0x8CE7,0xD85E, /*....}......r...^*/
  0xC9A9,0x01BA,0x9BC4,0x105C,0x702E,0x6C48,0xE78C,0x150B, /*.......\p.lH....*/
  0x0601,0xE5FF,0xFFD4,0x0D00,0x0FCE,0x5895,0x61A8,0x9E7B, /*..........X.a..{*/
  0x1998,0xB0F0,0xC672,0x82D5,0x2706,0x4741,0x220F,0x6593, /*.....r..'.GA".e.*/
  0xC98A,0x0919,0x481B,0xBDD6,0x641A,0xACFF,0xF242,0xC0F1, /*....H...d....B..*/
  0x1125,0x1422,0x06BC,0x0ED4,0x4E99,0x90A8,0xD8B7,0xAD5D, /*.%."....N......]*/
  0x3EAF,0x6EBE,0x6683,0xA4E3,0xC2E0,0x2943,0x875F,0x4F27, /*>.n.f.....)C._O'*/
  0x9C2C,0xD091,0xF387,0x9B54,0xEDD1,0xB4F3,0x3987,0x2206, /*.,.....T....9.".*/
  0x860D,0x71E4,0x6F2A,0x0804,0xC003,0x2AB1,0xE205,0x4D64, /*..q.o*....*...Md*/
  0xA19C,0xA60D,0x41A6,0xF27A,0xC130,0xC338,0x2609,0x5008, /*....A..z.0.8&.P.*/
  0xC4F6,0x300C,0xA6A9,0x1700,0x130C,0xDCC4,0x2F28,0xEB3F, /*..0........./(.?*/
  0xCD7A,0x3D2F,0xFFF2,0x40C0,0x186F,0x2E13,0xA1F2,0xBC36, /*.z=/..@..o.....6*/
  0xCB4E,0x996E,0xFCEE,0xC5F0,0xA0B7,0x92D4,0xEE79,0x7C50, /*.N.n.........y|P*/
  0x5DE5,0x0494,0xA976,0xCF6C,0x70DD,0x0DD4,0xEEED,0x98E8, /*]....v.lp.......*/
  0xC835,0x367A,0x0C05,0x8003,0xBCBE,0x9100,0x7CAE,0x65B8, /*.56z........|.e.*/
  0x91A3,0x33BA,0x6860,0xD41A,0x66F8,0x43A0,0x2089,0xE780, /*..3.h`..f.C. ...*/
  0xD81E,0x4FA0,0x0460,0x060A,0xA491,0x24FA,0x9F57,0x53F4, /*..O..`....$..WS.*/
  0x7ADB,0x5F56,0xE36E,0x0B8B,0x3A1C,0xF95E,0xFFF2,0x42C0, /*z._V.n..:..^..B.*/
  0xB100,0x3814,0x09EE,0xB436,0xD34E,0x99A4,0x7894,0x73C4, /*..8....6.N..x.s.*/
  0x6630,0xF5EA,0xDBBA,0x6767,0x956B,0xAB68,0x5D08,0xA139, /*f0....gg.k.h]..9*/
  0x56AB,0x1ED5,0x03E8,0x0170,0x00B3,0x9333,0x198C,0x618F, /*V......p...3..a.*/
  0xBB5D,0x2412,0x63D3,0x4B5D,0x9108,0x4322,0x561A,0xC510, /*.]$.c.K]..C"V...*/
  0x2184,0xA8EA,0x80BF,0x168E,0x3D46,0x189C,0x6E9A,0x91E6, /*!.......=F..n...*/
  0xC96F,0xD27D,0x27D7,0xE96B,0xFF0A,0x0343,0x89D5,0xBF52, /*.o.}'..k...C...R*/
  0x970A,0x2595,0x0DFF,0xF240,0xC0F5,0xC341,0x1381,0xEEA8, /*..%....@...A....*/
  0x5ED3,0x4498,0xFCCF,0x97F9,0x58B5,0x33B1,0x8547,0x86D7, /*^.D.....X.3..G..*/
  0x9801,0x3BA3,0x4F7E,0x04A6,0xC339,0x2170,0x2762,0xB518, /*..;.O~...9!p'b..*/
  0x1009,0x9900,0x8B7E,0xF2BF,0x5218,0x2630,0x1CB0,0x0149, /*.....~..R.&0...I*/
  0x30E0,0xC311,0x4605,0xCC49,0x1428,0xB2ED,0x4B57,0x5A2F, /*0...F..I.(..KWZ/*/
  0xB746,0x6334,0xD2DA,0x9F56,0x32B7,0xA225,0xFF94,0x2833, /*.Fc4...V2..%..(3*/
  0x7F3B,0xC450,0xECB1,0xE226,0xA1B7,0x077F,0xFBFF,0xF242, /*.;.P...&.......B*/
  0xC067,0x6A4C,0x13F9,0x6A90,0x7EDB,0x4494,0x3FFF,0x14D6, /*.gjL..j.~.D.?...*/
  0x2AFF,0xFFC1,0x348C,0x4822,0x0006,0x8F21,0xFD64,0x6004, /**...4.H"...!.d`.*/
  0x9242,0xEA74,0x3237,0xAA5A,0x9F67,0x018B,0x3F37,0x31DD, /*.B.t27.Z.g..?71.*/
  0x063C,0x0134,0x30E0,0x5C78,0x78CB,0xD6F1,0x318A,0x6961, /*.<.40.\xx...1.ia*/
  0x9392,0x42CE,0x4BC5,0x024E,0x73C6,0x2430,0xCD08,0x66C6, /*..B.K..Ns.$0..f.*/
  0x35AB,0xA23D,0x2FB3,0xBD34,0x8713,0xEE71,0x4568,0xFAEA, /*5..=/..4...qEh..*/
  0x0584,0x4136,0x4C9A,0xFFF2,0x40C0,0xC992,0x5613,0xD06E, /*..A6L...@...V..n*/
  0x7054,0xD3CC,0x2806,0xD70E,0xA41D,0x9C9D,0xD9A9,0x887B, /*pT..(..........{*/
  0xB5A3,0x56B7,0x4B4B,0x5A9B,0x2CA9,0xAD6F,0x996C,0xC04C, /*..V.KKZ.,..o.l.L*/
  0x1414,0xEFB4,0x2091,0x5FBC,0x8141,0x415D,0xD420,0xBD05, /*.... ._..AA]. ..*/
  0x1A6F,0xE268,0x5641,0x4157,0xF9BF,0x8982,0x8EC7,0x8F0A, /*.o.hVAAW........*/
  0x0A09,0x37F1,0x050A,0x0A0A,0x0A09,0x0537,0xFF10,0x5050, /*..7........7..PP*/
  0x5365,0xFFFF,0xFD75,0xDFFF,0xFF68,0x4FFF,0x8470,0xFFF2, /*Se...u...hO..p..*/
  0x42C0,0x2750,0x5F17,0xE882,0x3C11,0x5818,0x0155,0x48BC, /*B.'P_...<.X..UH.*/
  0x52FC,0x4A4C,0x3CD5,0xF611,0x2DBF,0xEA03,0x5C57,0x29BF, /*R.JL<...-...\W).*/
  0xC375,0x1CE6,0xDDBF,0xEDEF,0xD098,0x7771,0x9573,0xFFED, /*.u........wq.s..*/
  0x54BE,0xD5EE,0xAEC2,0xD50B,0xFFF1,0x978A,0xE442,0x0999, /*T............B..*/
  0xB1EA,0x94DC,0x78B5,0x340F,0xF18F,0xFC15,0xF6FA,0xB147, /*....x.4........G*/
  0xA96C,0x6743,0x8BF2,0x7622,0xEDDA,0x85BA,0x2FC7,0xF9CF, /*.lgC..v"..../...*/
  0xFCDB,0x462E,0x500A,0x84FF,0xF240,0xC0C6,0x4A59,0x282B, /*..F.P....@..JY(+*/
  0x19E0,0x0189,0x7800,0x5285,0x3C8E,0x549A,0x485A,0x7232, /*....x.R.<.T.HZr2*/
  0x94BF,0x434F,0x2453,0x4BEC,0x4B99,0x0E66,0x1FFF,0xCE7F, /*..CO$SK.K..f....*/
  0xFF3F,0x10AE,0x8262,0x7134,0x1859,0x9B51,0xC759,0xCEEE, /*.?...bq4.Y.Q.Y..*/
  0xA5FE,0x02BB,0x3091,0x49D5,0x4BF3,0xDC9A,0xA957,0x8E72, /*....0.I.K....W.r*/
  0x10C0,0x5D60,0x67FC,0x7DD6,0xBADD,0xB38B,0x5A0A,0x4C41, /*..]`g.}.....Z.LA*/
  0x4D45,0x332E,0x3933,0xAAAA,0xAAAA,0xAAAA,0xAAAA,0xAA54, /*ME3.93.........T*/
  0x4147,0x4865,0x6C6C,0x6F20,0x2020,0x2020,0x2020,0x2020, /*AGHello         */
  0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020, /*                */
  0x5061,0x6E75,0x2D4B,0x7269,0x7374,0x6961,0x6E20,0x506F, /*Panu-Kristian Po*/
  0x696B,0x7361,0x6C6F,0x2020,0x2020,0x2020,0x2020,0x5653, /*iksalo        VS*/
  0x4453,0x5020,0x5465,0x7374,0x696E,0x6720,0x2020,0x2020, /*DSP Testing     */
  0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020, /*                */
  0x4D50,0x3320,0x4865,0x6C6C,0x6F2C,0x2057,0x6F72,0x6C64, /*MP3 Hello, World*/
  0x2120,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020, /*!             */
};


extern const SIMPLE_DEVICE helloDevice;

ioresult main(char *parameters) {
	vo_pdevices[(*parameters|0x20)-'a'] = &helloDevice;
}

const char*  DevHelloIdentify (register __i0 void *self, char *buf, u_int16 bufsize) {
	return "DevHello";
}

ioresult DevHelloOpen(register __i0 SIMPLE_FILE *self, const char *name, const char *mode){ ///< Start device, populate descriptor, find and start filesystem.
	self->pos = 0;
	return S_OK;
}

ioresult DevHelloClose(register __i0 SIMPLE_FILE *self){ ///< Flush, Stop, Clean and Free the descriptor of the device
	return S_OK;
}

IOCTL_RESULT DevHelloIoctl(register __i0 SIMPLE_FILE *self, s_int16 request, IOCTL_ARGUMENT arg){ ///< Reset, Start, Stop, Restart, Flush, Check Media etc
	return S_ERROR;
}

u_int16  DevHelloRead(register __i0 SIMPLE_FILE *self, void *buf, u_int16 destinationIndex, u_int16 bytes){ ///< Read bytes to *buf, at byte index
	s_int16 bytesToRead = sizeof(hello_mp3) * 2/*bytes per word*/ - self->pos;
	if (bytes < bytesToRead) {
		bytesToRead = bytes;
	}
	if (bytesToRead < 1) {
		self->flags |= __MASK_EOF;
		return 0;
	}
	
	MemCopyPackedBigEndian(buf, destinationIndex, hello_mp3, self->pos, bytesToRead);
	self->pos += bytesToRead;
	return bytesToRead;
}

u_int16  DevHelloWrite(register __i0 SIMPLE_FILE *self, void *buf, u_int16 sourceIndex, u_int16 bytes){ ///< Write bytes from *buf at byte index
	return 0;
}

const SIMPLE_DEVICE helloDevice = {
	__MASK_PRESENT | __MASK_CHARACTER_DEVICE | __MASK_READABLE | __MASK_OPEN,
	DevHelloIdentify,
	DevHelloOpen,
	DevHelloClose,
	DevHelloIoctl,
	DevHelloRead,
	DevHelloWrite
};

