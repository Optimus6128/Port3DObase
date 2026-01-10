#include "CelRenderer.h"

typedef struct CelPoint
{
	int x, y;
	uint32 colorInfo;
} CelPoint;

#define MAX_TEXTURE_SIZE 1024
#define FP_RAST 16

#define ORDER_CW	(1<<0)
#define ORDER_CCW	(1<<1)

#define SPLAT_PRECISE_TEXEL


typedef struct CelRenderInfo
{
	bool opaque;			// if some combination of values end up with unaltered bitmap (usually it's 0x1F00, which however is S1 * 8 / 8 + 0)
	bool xor;				// does XOR instead of ADD or SUB
	bool needsFrameBuffer;	// if at least one of the two sources are framebuffer
	int source1;
	int source2;
	int avbits;
	int pmv;				// Primary multiplier value: 1-8
	int pdv;				// Primary divider value: 2,4,8,16
	int dv2;				// 2D (or 2DV), either 1 or 2, SDV might be a different story altogether

	bool mariaRender;
	bool transparentRGB0;
} CelRenderInfo;



static uint32 bitmapLine[2*MAX_TEXTURE_SIZE+1];
static uint8 unpackedSrc[MAX_TEXTURE_SIZE+1];

static CelPoint celGrid[(MAX_TEXTURE_SIZE+1) * (MAX_TEXTURE_SIZE+1)];



static int getCelBpp(CCB *cel)
{
	static int bppTable[8] = {0, 1,2,4,6,8,16, 0};

	return bppTable[cel->ccb_PRE0 & PRE0_BPP_MASK];
}

static int getCelWoffset(CCB* cel)
{
	int bpp = getCelBpp(cel);
	int value = cel->ccb_PRE1;

	int woffset;
	if (bpp < 8) {
		woffset = (value & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT;
	}
	else {
		woffset = (value & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;
	}

	return woffset + PRE1_WOFFSET_PREFETCH;
}


static void pixelProcessorRender(uint16* vramDst, uint32 colorInfo, CelRenderInfo *info)
{
	uint16 color = colorInfo & 65535;

	if (info->opaque) {
		if (!(color == 0 && info->transparentRGB0)) {
			*vramDst = (uint16)color;
		}
	} else {
		int r1, g1, b1;
		int r2, g2, b2;
		uint16 src1, src2;
		bool addSrc2 = false;

		const int pmv = info->pmv;
		const int pdv = info->pdv;
		const int dv2 = info->dv2;
		const int avBits = info->avbits;
		const bool doXor = info-> xor;

		if (info->source1 == PPMPC_1S_PDC) {
			src1 = color;
		}
		else {
			src1 = *vramDst;
		}

		r1 = (((src1 >> 10) & 31) * pmv) / pdv;
		g1 = (((src1 >> 5) & 31) * pmv) / pdv;
		b1 = ((src1 & 31) * pmv) / pdv;

		switch (info->source2) {
		case PPMPC_2S_0:
			r1 >>= dv2;
			g1 >>= dv2;
			b1 >>= dv2;
			break;

		case PPMPC_2S_CCB:
			if (!doXor) {
				r1 = (r1 + avBits) >> dv2;
				g1 = (g1 + avBits) >> dv2;
				b1 = (b1 + avBits) >> dv2;
			}
			else {
				r1 = (r1 ^ avBits) >> dv2;
				g1 = (g1 ^ avBits) >> dv2;
				b1 = (b1 ^ avBits) >> dv2;
			}
			break;

		case PPMPC_2S_CFBD:
			src2 = *vramDst;
			addSrc2 = true;
			break;

		case PPMPC_2S_PDC:
			src2 = color;
			addSrc2 = true;
			break;
		}

		if (addSrc2) {
			r2 = (src2 >> 10) & 31;
			g2 = (src2 >> 5) & 31;
			b2 = src2 & 31;
			if (!doXor) {
				r1 = (r1 + r2) >> dv2;
				g1 = (g1 + g2) >> dv2;
				b1 = (b1 + b2) >> dv2;
			}
			else {
				r1 = (r1 ^ r2) >> dv2;
				g1 = (g1 ^ g2) >> dv2;
				b1 = (b1 ^ b2) >> dv2;
			}
		}
		if (r1 > 31) r1 = 31;
		if (g1 > 31) g1 = 31;
		if (b1 > 31) b1 = 31;

		color = (r1 << 10) | (g1 << 5) | b1;
		if (!(color == 0 && info->transparentRGB0)) {
			*vramDst = color;
		}
	}
}

static void unpackLine(uint8 *src, int bpp)
{
	// 2 bits:						6 bits:			data:
	// 00: PACK_EOL					nothing			nothing
	// 01: PACK_LITERAL				count+1			(count+1) * pixel bits
	// 10: PACK_TRANSPARENT			count+1			nothing
	// 11: PACK_REPEAT				count+1			pixel bits

	uint8 header, command, count;

	switch(bpp) {
		case 4:
		{
			while(header = *src++, command = header >> 6, command) {
				count = (header & 63) + 1;
			}
			break;
		}

		case 8:
		{
			uint8* dst = (uint8*)unpackedSrc;

			while(header = *src++, command = header >> 6, command) {
				count = (header & 63) + 1;

				if (command==PACK_LITERAL) {
					do {
						*dst++ = *src++;
					} while (--count != 0);
				} else {
					uint8 value = 0;
					if (command==PACK_PACKED) {
						value = *src++;
					}
					do {
						*dst++ = value;
					} while (--count != 0);
				}
			}
			break;
		}

		case 16:
		{
			uint16* dst = (uint16*)unpackedSrc;

			while(header = *src++, command = header >> 6, command) {
				uint16 *src16 = (uint16*)src;
				count = (header & 63) + 1;

				if (command==PACK_LITERAL) {
					do {
						uint16 value = *src16++;
						value = SHORT_ENDIAN_FLIP(value);
						*dst++ = value;
					} while (--count != 0);
				} else {
					uint16 value = 0;
					if (command==PACK_PACKED) {
						value = *src16++;
						value = SHORT_ENDIAN_FLIP(value);
					}
					do {
						*dst++ = value;
					} while (--count != 0);
				}
				src = (uint8*)src16;
			}
			break;
		}

		default:
			break;
	}
}

#define RRRGGGBB_TO_C16(c) ((((c) >> 5) << 12) | ((((c) >> 2) & 7) << 7) | (((c) & 3) << 3))

static void decodeLine(int width, int bpp, uint32* src, uint16* pal, bool raw, bool packed, bool lrform)
{
	uint32* dst = bitmapLine;
	int wordLength = (width * bpp + 31) >> 5;

	if (lrform) {
		wordLength *= 2;
		while (wordLength-- > 0) {
			const uint32 c = *src++;
			const uint16 c0 = c & 65535;
			const uint16 c1 = c >> 16;
			*dst = c0;
			*(dst+width) = c1;
			++dst;
		}
	} else {
		if (packed) {
			unpackLine((uint8*)src + 2, bpp);
			src = (uint32*)unpackedSrc;
		}

		if (raw) {
			while (wordLength-- > 0) {
				const uint32 c = *src++;

				switch (bpp) {
				case 8:
				{
					const uint16 c0 = c & 255;
					const uint16 c1 = (c >> 8) & 255;
					const uint16 c2 = (c >> 16) & 255;
					const uint16 c3 = c >> 24;

					// need another map to raw rrrgggbb
					*dst = RRRGGGBB_TO_C16(c0);
					*(dst + 1) = RRRGGGBB_TO_C16(c1);
					*(dst + 2) = RRRGGGBB_TO_C16(c2);
					*(dst + 3) = RRRGGGBB_TO_C16(c3);
					dst += 4;

					break;
				}

				case 16:
				{
					const uint16 c0 = c & 65535;
					const uint16 c1 = c >> 16;
					*dst++ = c0;
					*dst++ = c1;
					break;
				}

				default:
					break;
				}
			}
	} else {
			if (!pal) return;

			while (wordLength-- > 0) {
				const uint32 c = *src++;

				switch (bpp) {

				case 1:
					*dst = pal[(c >> 7) & 1];
					*(dst + 1) = pal[(c >> 6) & 1];
					*(dst + 2) = pal[(c >> 5) & 1];
					*(dst + 3) = pal[(c >> 4) & 1];
					*(dst + 4) = pal[(c >> 3) & 1];
					*(dst + 5) = pal[(c >> 2) & 1];
					*(dst + 6) = pal[(c >> 1) & 1];
					*(dst + 7) = pal[c & 1];
					*(dst + 8) = pal[(c >> 15) & 1];
					*(dst + 9) = pal[(c >> 14) & 1];
					*(dst + 10) = pal[(c >> 13) & 1];
					*(dst + 11) = pal[(c >> 12) & 1];
					*(dst + 12) = pal[(c >> 11) & 1];
					*(dst + 13) = pal[(c >> 10) & 1];
					*(dst + 14) = pal[(c >> 9) & 1];
					*(dst + 15) = pal[(c >> 8) & 1];
					*(dst + 16) = pal[(c >> 23) & 1];
					*(dst + 17) = pal[(c >> 22) & 1];
					*(dst + 18) = pal[(c >> 21) & 1];
					*(dst + 19) = pal[(c >> 20) & 1];
					*(dst + 20) = pal[(c >> 19) & 1];
					*(dst + 21) = pal[(c >> 18) & 1];
					*(dst + 22) = pal[(c >> 17) & 1];
					*(dst + 23) = pal[(c >> 16) & 1];
					*(dst + 24) = pal[(c >> 31) & 1];
					*(dst + 25) = pal[(c >> 30) & 1];
					*(dst + 26) = pal[(c >> 29) & 1];
					*(dst + 27) = pal[(c >> 28) & 1];
					*(dst + 28) = pal[(c >> 27) & 1];
					*(dst + 29) = pal[(c >> 26) & 1];
					*(dst + 30) = pal[(c >> 25) & 1];
					*(dst + 31) = pal[(c >> 24) & 1];
					dst += 32;
					break;

				case 2:
					*dst = pal[(c >> 6) & 3];
					*(dst + 1) = pal[(c >> 4) & 3];
					*(dst + 2) = pal[(c >> 2) & 3];
					*(dst + 3) = pal[c & 3];
					*(dst + 4) = pal[(c >> 14) & 3];
					*(dst + 5) = pal[(c >> 12) & 3];
					*(dst + 6) = pal[(c >> 10) & 3];
					*(dst + 7) = pal[(c >> 8) & 3];
					*(dst + 8) = pal[(c >> 22) & 3];
					*(dst + 9) = pal[(c >> 20) & 3];
					*(dst + 10) = pal[(c >> 18) & 3];
					*(dst + 11) = pal[(c >> 16) & 3];
					*(dst + 12) = pal[(c >> 30) & 3];
					*(dst + 13) = pal[(c >> 28) & 3];
					*(dst + 14) = pal[(c >> 26) & 3];
					*(dst + 15) = pal[(c >> 24) & 3];
					dst += 16;
					break;

				case 4:
					*dst = pal[(c >> 4) & 15];
					*(dst + 1) = pal[c & 15];
					*(dst + 2) = pal[(c >> 12) & 15];
					*(dst + 3) = pal[(c >> 8) & 15];
					*(dst + 4) = pal[(c >> 20) & 15];
					*(dst + 5) = pal[(c >> 16) & 15];
					*(dst + 6) = pal[(c >> 28) & 15];
					*(dst + 7) = pal[(c >> 24) & 15];
					dst += 8;
				break;

				case 8:
					*dst = pal[c & 31];
					*(dst + 1) = pal[(c >> 8) & 31];
					*(dst + 2) = pal[(c >> 16) & 31];
					*(dst + 3) = pal[(c >> 24) & 31];
					dst += 4;
					break;

				default:
					break;
				}
			}
		}
	}
}

static bool pointInsideQuad(int x, int y, CelPoint* p0, CelPoint* p1, CelPoint* p2, CelPoint* p3)
{
	return	((y - p0->y) * (p1->x - p0->x) - (x - p0->x) * (p1->y - p0->y) >= 0) &&
			((y - p1->y) * (p2->x - p1->x) - (x - p1->x) * (p2->y - p1->y) >= 0) &&
			((y - p2->y) * (p3->x - p2->x) - (x - p2->x) * (p3->y - p2->y) >= 0) &&
			((y - p3->y) * (p0->x - p3->x) - (x - p3->x) * (p0->y - p3->y) >= 0);
}

static void splatTexel(CelPoint *p0, CelPoint *p1, CelPoint *p2, CelPoint *p3, uint32 color, uint16* vramDst, CelRenderInfo *info)
{
	int x, y;

	int xMin = p0->x;
	int yMin = p0->y;
	int xMax = xMin;
	int yMax = yMin;

	if (p1->x < xMin) xMin = p1->x;
	if (p1->x > xMax) xMax = p1->x;
	if (p1->y < yMin) yMin = p1->y;
	if (p1->y > yMax) yMax = p1->y;

	if (p2->x < xMin) xMin = p2->x;
	if (p2->x > xMax) xMax = p2->x;
	if (p2->y < yMin) yMin = p2->y;
	if (p2->y > yMax) yMax = p2->y;

	if (p3->x < xMin) xMin = p3->x;
	if (p3->x > xMax) xMax = p3->x;
	if (p3->y < yMin) yMin = p3->y;
	if (p3->y > yMax) yMax = p3->y;

	if (xMin < 0) xMin = 0;
	if (xMax > SCREEN_W - 1) xMax = SCREEN_W - 1;
	if (yMin < 0) yMin = 0;
	if (yMax > SCREEN_H - 1) yMax = SCREEN_H - 1;

	for (y = yMin; y < yMax; ++y) {
		for (x = xMin; x < xMax; ++x) {
			#ifdef SPLAT_PRECISE_TEXEL
				if (pointInsideQuad(x,y, p0,p1,p2,p3)) {
			#endif
			const int offset = VRAM_OFS(x, y);
			pixelProcessorRender(vramDst + offset, color, info);
			#ifdef SPLAT_PRECISE_TEXEL
				}
			#endif
		}
	}
}

static void renderCelGridTexels(uint16* vramDst, int width, int height, int order, CelRenderInfo *info)
{
	int x, y;
	CelPoint* celGridSrc = celGrid;
	const int gridWoffset = width + 1;
	const bool mariaFill = info->mariaRender;

	for (y=0; y<height; ++y) {
		for (x = 0; x < width; ++x) {
			const uint32 color = celGridSrc->colorInfo;
			if (!(color == 0 && info->transparentRGB0)) {
				CelPoint* p0 = &celGridSrc[0];
				CelPoint* p1 = &celGridSrc[1];
				CelPoint* p2 = &celGridSrc[gridWoffset];
				CelPoint* p3 = &celGridSrc[gridWoffset + 1];

				if (!mariaFill) {
					if (order & ORDER_CW) {
						splatTexel(p0, p1, p3, p2, color, vramDst, info);
					}
					if (order & ORDER_CCW) {
						splatTexel(p2, p3, p1, p0, color, vramDst, info);
					}
				} else {
					const int xp = p0->x;
					const int yp = p0->y;
					if (xp >= 0 && xp < SCREEN_W && yp >= 0 && yp < SCREEN_H) {
						const int offset = VRAM_OFS(xp, yp);
						pixelProcessorRender(vramDst + offset, color, info);
					}
				}
			}
			++celGridSrc;
		}
		++celGridSrc;
	}
}

static int getWoffsetFromPackedData(void *src, int bpp)
{
	uint8* src8 = (uint8*)src;
	if (bpp < 8) {
		return *src8 + PRE1_WOFFSET_PREFETCH;
	} else {
		return (src8[1] | (src8[0] << 8)) + PRE1_WOFFSET_PREFETCH;
	}
}

static void renderCelPolygon(CCB* cel, uint16* vramDst, CelRenderInfo *info)
{
	int x, y, i, n = 1;

	const int width = GET_CEL_WIDTH(cel);
	int height = GET_CEL_HEIGHT(cel);
	const int bpp = getCelBpp(cel);

	const bool raw = cel->ccb_PRE0 & PRE0_LINEAR;
	const bool packed = cel->ccb_Flags & CCB_PACKED;
	const bool lrform = cel->ccb_PRE1 & PRE1_LRFORM;

	int posX, posY;
	int hdx = cel->ccb_HDX;
	int hdy = cel->ccb_HDY;
	const int vdx = cel->ccb_VDX;
	const int vdy = cel->ccb_VDY;
	const int hddx = cel->ccb_HDDX;
	const int hddy = cel->ccb_HDDY;

	uint32* src = (uint32*)cel->ccb_SourcePtr;
	uint16* pal = (uint16*)cel->ccb_PLUTPtr;

	CelPoint* celGridDst = celGrid;

	int woffset;
	int order = 0;


	if (width <= 0 || height <= 0 || width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE) return;

	if (!packed) {
		woffset = getCelWoffset(cel);
	}

	if (cel->ccb_Flags & CCB_ACW) {
		order |= ORDER_CW;
	}
	if (cel->ccb_Flags & CCB_ACCW) {
		order |= ORDER_CCW;
	}

	if (lrform) {
		height *= 2;
		n = 2;
	}

	posX = cel->ccb_XPos;
	posY = cel->ccb_YPos;
	for (y = 0; y < height+1; ++y) {
		uint32* bitmapLinePtr = bitmapLine;

		if (y < height) decodeLine(width, bpp, src, pal, raw, packed, lrform);

		if (packed) {
			woffset = getWoffsetFromPackedData(src, bpp);
		}

		for (i = 0; i < n; ++i) {
			int pposX = posX << 4;
			int pposY = posY << 4;
			for (x = 0; x < width; ++x) {
				celGridDst->x = pposX >> 20;
				celGridDst->y = pposY >> 20;
				celGridDst->colorInfo = *bitmapLinePtr++;
				++celGridDst;

				pposX += hdx;
				pposY += hdy;
			}
			celGridDst->x = pposX >> 20;
			celGridDst->y = pposY >> 20;
			++celGridDst;

			hdx += hddx;
			hdy += hddy;

			posX += vdx;
			posY += vdy;
		}
		src += woffset;
	}
	renderCelGridTexels(vramDst, width, height, order, info);
}

static void renderCelSprite(CCB* cel, uint16* dst, CelRenderInfo *info)
{
	int x, y, i, n = 1;

	const int posX = cel->ccb_XPos >> 16;
	const int posY = cel->ccb_YPos >> 16;
	const int width = GET_CEL_WIDTH(cel);
	int height = GET_CEL_HEIGHT(cel);
	const int bpp = getCelBpp(cel);

	const bool raw = cel->ccb_PRE0 & PRE0_LINEAR;
	const bool packed = cel->ccb_Flags & CCB_PACKED;
	const bool lrform = cel->ccb_PRE1 & PRE1_LRFORM;

	uint32* src = (uint32*)cel->ccb_SourcePtr;
	uint16* pal = (uint16*)cel->ccb_PLUTPtr;

	CelPoint* celGridDst = celGrid;

	int woffset;

	if (width <= 0 || height <= 0 || width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE) return;

	if (!packed) {
		woffset = getCelWoffset(cel);
	}

	if (lrform) {
		height *= 2;
		n = 2;
	}

	for (y = 0; y < height; y+=n) {
		uint32* bitmapLinePtr = bitmapLine;

		decodeLine(width, bpp, src, pal, raw, packed, lrform);

		if (packed) {
			woffset = getWoffsetFromPackedData(src, bpp);
		}

		for (i = 0; i < n; ++i) {
			const int yp = posY + y + i;
			if (yp >= 0 && yp < SCREEN_H) {
				for (x = 0; x < width; ++x) {
					const int xp = posX + x;
					if (xp >= 0 && xp < SCREEN_W) {
						const int offset = (yp >> 1) * SCREEN_W * 2 + (yp & 1) + (xp << 1);
						pixelProcessorRender(dst + offset, *bitmapLinePtr++, info);
					}
				}
			}
		}
		src += woffset;
	}
}

static void decodePIXCinfo(CelRenderInfo *info, uint32 pixc)
{
	if (pixc == 0x1F00) {
		info->opaque = true;
		return;
	}

	info->opaque = false;
	info->source1 = pixc & PPMPC_1S_MASK;
	info->source2 = pixc & PPMPC_2S_MASK;

	info->needsFrameBuffer = (info->source1 == PPMPC_1S_CFBD) | (info->source2 == PPMPC_2S_CFBD);

	info->pmv = ((pixc & PPMPC_MF_MASK) >> PPMPC_MF_SHIFT) + 1;
	info->pdv = 1 << (((((pixc & PPMPC_SF_MASK) >> PPMPC_SF_SHIFT) - 1) & 3) + 1);
	info->dv2 = pixc & PPMPC_2D_MASK;	// will be shift right
	info->avbits = (pixc & PPMPC_AV_MASK) >> PPMPC_AV_SHIFT;
}

static CelRenderInfo* setupCelRenderInfo(CCB* cel)
{
	static CelRenderInfo info[2];

	const uint32 pixc = cel->ccb_PIXC;
	const uint32 p1 = (pixc >> 16) & 65535;
	const uint32 p0 = pixc & 65535;

	const uint32 pOver = cel->ccb_Flags & CCB_POVER_MASK;
	switch(pOver) {
		case PMODE_ZERO:
			decodePIXCinfo(&info[0], p0);
		break;

		case PMODE_ONE:
			decodePIXCinfo(&info[0], p1);
		break;

		case PMODE_PDC:
		default:
			// If both are the same, use the same anyway
			if (p0 == p1) {
				decodePIXCinfo(&info[0], p0);
			} else {
				decodePIXCinfo(&info[0], p0);
				decodePIXCinfo(&info[1], p1);
			}
		break;
	}

	info[0].xor = cel->ccb_Flags & CCB_PXOR;
	info[0].transparentRGB0 = !(cel->ccb_Flags & CCB_BGND);
	info[0].mariaRender = cel->ccb_Flags & CCB_MARIA;

	return info;
}

void renderCel(CCB* cel, uint16* dst)
{
	CelRenderInfo *info = setupCelRenderInfo(cel);

	if (cel->ccb_HDY == 0 && cel->ccb_VDX == 0 && cel->ccb_HDDX == 0 && cel->ccb_HDDY == 0 && cel->ccb_HDX == (1 << 20) && cel->ccb_VDY == (1 << 16)) {
		renderCelSprite(cel, dst, info);
	} else {
		renderCelPolygon(cel, dst, info);
	}
}
