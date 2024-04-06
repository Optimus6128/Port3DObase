#include "CelRenderer.h"

// For the time being, I set the default screen width/height, but that could change if PAL or screen region
// I will have to change some functions to get it from the screenItem or something..
#define SCREEN_W 320
#define SCREEN_H 240


typedef struct CelPoint
{
	int x, y;
	uint32 c;	// Store 16bpp color, - 1 means skip (could have more flags)
} CelPoint;

#define MAX_TEXTURE_SIZE 1024
#define FP_RAST 16

#define ORDER_CW	(1<<0)
#define ORDER_CCW	(1<<1)

#define SPLAT_METHOD


static uint16 bitmapLine[MAX_TEXTURE_SIZE+1];
static uint32 unpackedSrc[MAX_TEXTURE_SIZE+1];

static CelPoint celGrid[(MAX_TEXTURE_SIZE+1) * (MAX_TEXTURE_SIZE+1)];

static int leftEdgeFlat[SCREEN_H];
static int rightEdgeFlat[SCREEN_H];


static int getCelWidth(CCB *cel)
{
	return (cel->ccb_PRE1 & PRE1_TLHPCNT_MASK) + PRE1_TLHPCNT_PREFETCH;
}

static int getCelHeight(CCB *cel)
{
	return ((cel->ccb_PRE0 & PRE0_VCNT_MASK) >> PRE0_VCNT_SHIFT) + PRE0_VCNT_PREFETCH;
}

static int getCelBpp(CCB *cel)
{
	static int bppTable[8] = {0, 1,2,4,6,8,16, 0};

	return bppTable[cel->ccb_PRE0 & PRE0_BPP_MASK];
}

static int getWoffsetFromData(uint32 value, int bpp)
{
	int woffset;

	if (bpp < 8) {
		woffset = (value & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT;
	}
	else {
		woffset = (value & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;
	}

	return woffset + PRE1_WOFFSET_PREFETCH;
}

static int getCelWoffset(CCB* cel)
{
	return getWoffsetFromData(cel->ccb_PRE1, getCelBpp(cel));
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

				if (command==1) {
					do {
						*dst++ = *src++;
					} while (--count != 0);
				} else {
					uint8 value = 0;
					if (command==3) {
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

				if (command==1) {
					do {
						uint16 value = *src16++;
						value = SHORT_ENDIAN_FLIP(value);
						*dst++ = value;
					} while (--count != 0);
				} else {
					uint16 value = 0;
					if (command==3) {
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


static void decodeLine(int width, int bpp, uint32* src, uint16* pal, bool raw, bool packed)
{
	uint16* dst = bitmapLine;
	int wordLength = (width * bpp + 31) >> 5;

	if (packed) {
		unpackLine((uint8*)src + 2, bpp);
		src = unpackedSrc;
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
					*dst = c0;
					*(dst + 1) = c1;
					*(dst + 2) = c2;
					*(dst + 3) = c3;
					dst += 4;

					*dst++ = 1;
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
				case 4:
				{
					const uint16 c1 = pal[c & 15];
					const uint16 c0 = pal[(c >> 4) & 15];
					const uint16 c3 = pal[(c >> 8) & 15];
					const uint16 c2 = pal[(c >> 12) & 15];
					const uint16 c5 = pal[(c >> 16) & 15];
					const uint16 c4 = pal[(c >> 20) & 15];
					const uint16 c7 = pal[(c >> 24) & 15];
					const uint16 c6 = pal[(c >> 28) & 15];

					*dst = c0;
					*(dst + 1) = c1;
					*(dst + 2) = c2;
					*(dst + 3) = c3;
					*(dst + 4) = c4;
					*(dst + 5) = c5;
					*(dst + 6) = c6;
					*(dst + 7) = c7;
					dst += 8;
					break;
				}

				case 8:
				{
					const uint16 c0 = pal[c & 31];
					const uint16 c1 = pal[(c >> 8) & 31];
					const uint16 c2 = pal[(c >> 16) & 31];
					const uint16 c3 = pal[(c >> 24) & 31];

					*dst = c0;
					*(dst + 1) = c1;
					*(dst + 2) = c2;
					*(dst + 3) = c3;
					dst += 4;
					break;
				}

				default:
					break;
			}
		}
	}
}


// ======== Specialized functions for simpler flat quad rasterizer ========

static inline void prepareEdgeListFlat(CelPoint *p0, CelPoint *p1)
{
	if (p0->y == p1->y) return;

	// Assumes CCW
	int *edgeListToWriteFlat;
	if (p0->y < p1->y) {
		edgeListToWriteFlat = leftEdgeFlat;
	}
	else {
		edgeListToWriteFlat = rightEdgeFlat;

		CelPoint *pTemp = p0;
		p0 = p1;
		p1 = pTemp;
	}

	const int x0 = p0->x; const int y0 = p0->y;
	const int x1 = p1->x; const int y1 = p1->y;

	const int screenHeight = SCREEN_H;
	const int dx = ((x1 - x0) << FP_RAST) / (y1 - y0);

	int xp = x0 << FP_RAST;
	int yp = y0;
	do
	{
		if (yp >= 0 && yp < screenHeight)
		{
			edgeListToWriteFlat[yp] = xp >> FP_RAST;
		}
		xp += dx;

	} while (yp++ != y1);
}

static void drawFlatQuad(CelPoint *p0, CelPoint *p1, CelPoint *p2, CelPoint *p3, uint16 color, uint16* dst)
{
	const int x0 = p0->x; const int y0 = p0->y;
	const int x1 = p1->x; const int y1 = p1->y;
	const int x2 = p2->x; const int y2 = p2->y;
	const int x3 = p3->x; const int y3 = p3->y;

	int yMin = y0;
	int yMax = yMin;
	if (y1 < yMin) yMin = y1;
	if (y1 > yMax) yMax = y1;
	if (y2 < yMin) yMin = y2;
	if (y2 > yMax) yMax = y2;
	if (y3 < yMin) yMin = y3;
	if (y3 > yMax) yMax = y3;

	if (yMin < 0) yMin = 0;
	if (yMax > SCREEN_H - 1) yMax = SCREEN_H - 1;

	//prepareEdgeListFlat(p0, p1);
	//prepareEdgeListFlat(p1, p2);
	//prepareEdgeListFlat(p2, p3);
	//prepareEdgeListFlat(p3, p0);

	// Why twice? This alone works for now.
	prepareEdgeListFlat(p1, p0);
	prepareEdgeListFlat(p2, p1);
	prepareEdgeListFlat(p3, p2);
	prepareEdgeListFlat(p0, p3);

	for (int y = yMin; y <= yMax; y++)
	{
		int xl = leftEdgeFlat[y];
		int xr = rightEdgeFlat[y];
		int offset;

		if (xl < 0) xl = 0;
		if (xr > SCREEN_W - 1) xr = SCREEN_W - 1;

		if (xl == xr) ++xr;

		// prepareEdgeListFlat assumes CCW
		// If quad is clockwise, following the grid texel quads in clockwise is easy. But that will change in bizarro polygono
		// I could do a check and flip x here, as it's just rendering single flat color, so one more swap would be easy
		// EDIT: The above prepareEdgeListFlat twice does everything?

		offset = (y >> 1) * SCREEN_W * 2 + (y & 1);
		for (int x = xl; x < xr; ++x) {
			*(dst + offset + (x<<1)) = color;
		}
	}
}

static void splatTexel(CelPoint *p0, CelPoint *p1, CelPoint *p2, CelPoint *p3, uint16 color, uint16* dst)
{
	int x, y;
	int xMin, yMin, xMax, yMax;

	if (color) {
		xMin = p0->x;
		yMin = p0->y;
		xMax = xMin;
		yMax = yMin;

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

		for (y = yMin; y < yMax; ++y) {
			for (x = xMin; x < xMax; ++x) {
				if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
					const int offset = (y >> 1) * SCREEN_W * 2 + (y & 1) + (x << 1);
					*(dst + offset) = color;
				}
			}
		}
	}
}

static void renderCelGridTexels(uint16* dst, int width, int height, int order, bool mariaFill)
{
	int x, y;
	CelPoint* celGridSrc = celGrid;
	const int gridWoffset = width + 1;

	for (y=0; y<height; ++y) {
		for (x = 0; x < width; ++x) {
			const uint16 color = celGridSrc->c & 65535;
			if (color) {
				CelPoint* p0 = &celGridSrc[0];
				CelPoint* p1 = &celGridSrc[1];
				CelPoint* p2 = &celGridSrc[gridWoffset];
				CelPoint* p3 = &celGridSrc[gridWoffset + 1];

				const bool skipFill = (p0->y == p1->y && p1->y == p2->y && p2->y == p3->y && p0->x == p1->x && p1->x == p2->x && p2->x == p3->x);
				if (!mariaFill && !skipFill) {
					#ifdef SPLAT_METHOD
						splatTexel(p0, p1, p2, p3, color, dst);
					#else
						if (order & ORDER_CW) {
							drawFlatQuad(p0, p1, p3, p2, color, dst);
						}
						if (order & ORDER_CCW) {
							drawFlatQuad(p2, p3, p1, p0, color, dst);
						}
					#endif
				} else {
					const int xp = p0->x;
					const int yp = p0->y;
					if (xp >= 0 && xp < SCREEN_W && yp >= 0 && yp < SCREEN_H) {
						const int offset = (yp >> 1) * SCREEN_W * 2 + (yp & 1) + (xp << 1);
						*(dst + offset) = color;
					}
				}
			}
			++celGridSrc;
		}
		++celGridSrc;
	}
}

static void renderCelPolygon(CCB* cel, uint16* dst)
{
	int x, y;

	const int width = getCelWidth(cel);
	const int height = getCelHeight(cel);
	const int bpp = getCelBpp(cel);

	const bool raw = cel->ccb_PRE0 & PRE0_LINEAR;
	const bool packed = cel->ccb_Flags & CCB_PACKED;

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


	posX = cel->ccb_XPos;
	posY = cel->ccb_YPos;
	for (y = 0; y < height+1; ++y) {
		int pposX = posX << 4;
		int pposY = posY << 4;

		decodeLine(width, bpp, src, pal, raw, packed);

		if (packed) {
			uint32 value = *src;
			value = LONG_ENDIAN_FLIP(value);
			woffset = getWoffsetFromData(value, bpp);
		}

		for (x = 0; x < width+1; ++x) {
			celGridDst->x = pposX >> 20;
			celGridDst->y = pposY >> 20;
			celGridDst->c = bitmapLine[x];
			++celGridDst;

			pposX += hdx;
			pposY += hdy;
		}

		hdx += hddx;
		hdy += hddy;

		posX += vdx;
		posY += vdy;
		src += woffset;
	}

	// 40/50-136
	//cel->ccb_Flags |= CCB_MARIA;
	renderCelGridTexels(dst, width, height, order, cel->ccb_Flags & CCB_MARIA);
}

static void renderCelSprite(CCB* cel, uint16* dst)
{
	int x, y;

	const int posX = cel->ccb_XPos >> 16;
	const int posY = cel->ccb_YPos >> 16;
	const int width = getCelWidth(cel);
	const int height = getCelHeight(cel);
	const int bpp = getCelBpp(cel);

	const bool raw = cel->ccb_PRE0 & PRE0_LINEAR;
	const bool packed = cel->ccb_Flags & CCB_PACKED;

	uint32* src = (uint32*)cel->ccb_SourcePtr;
	uint16* pal = (uint16*)cel->ccb_PLUTPtr;

	CelPoint* celGridDst = celGrid;

	int woffset;

	if (width <= 0 || height <= 0 || width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE) return;

	if (!packed) {
		woffset = getCelWoffset(cel);
	}

	for (y = 0; y < height; ++y) {
		const int yp = posY + y;

		decodeLine(width, bpp, src, pal, raw, packed);

		if (packed) {
			uint32 value = *src;
			value = LONG_ENDIAN_FLIP(value);
			woffset = getWoffsetFromData(value, bpp);
		}

		if (yp >= 0 && yp < SCREEN_H) {
			for (x = 0; x < width; ++x) {
				const int xp = posX + x;
				if (xp >= 0 && xp < SCREEN_W) {
					const int offset = (yp >> 1) * SCREEN_W * 2 + (yp & 1) + (xp << 1);
					const uint16 c = bitmapLine[x];
					if (c!=0) *(dst + offset) = c;
				}
			}
		}
		src += woffset;
	}
}

void renderCel(CCB* cel, uint16* dst)
{
	if (cel->ccb_HDY == 0 && cel->ccb_VDX == 0 && cel->ccb_HDDX == 0 && cel->ccb_HDDY == 0 && cel->ccb_HDX == (1 << 20) && cel->ccb_VDY == (1 << 16)) {
		renderCelSprite(cel, dst);
	} else {
		renderCelPolygon(cel, dst);
	}
}
