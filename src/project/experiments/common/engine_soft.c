#include "core.h"

#include "tools.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "system_graphics.h"

#include "cel_helpers.h"
#include "mathutil.h"
#include "tools.h"

typedef struct GouraudScanlineCCB
{
	uint32 ccb_Flags;

	struct GouraudScanlineCCB* ccb_NextPtr;
	CelData* ccb_SourcePtr;
	void* ccb_PLUTPtr;

	Coord ccb_XPos;
	Coord ccb_YPos;
	int32  ccb_HDX;
	int32  ccb_HDY;
	int32  ccb_VDX;
	int32  ccb_VDY;
	uint32 ccb_PRE0;
	uint32 ccb_PRE1;
} GouraudScanlineCCB;


//#define ASM_VRAMSET

#define SOFT_BUFF_MAX_SIZE (2 * SCREEN_WIDTH * SCREEN_HEIGHT)

#define DIV_TAB_SIZE 4096
#define DIV_TAB_SHIFT 12

// Semisoft gouraud method
// We had issues with memory, 32768 starting crashing. Cube, Vase, smooth object is from 1000 to 4000 maybe reach 8000? Maybe 16384 is ok? I also have a stopper.
// But I also reduced crap I did with memory so maybe now ever 32768 is ok if not 65536. But might reduce it to 16384 at least
// Update: Still crashing with 32768, used to not do it. I even reduced mem but still.
// Should use a small number like 4096 or 8192 and then flush
#define MAX_SCANLINES 8192

static CCB* startingScanlineCelDummy;
static GouraudScanlineCCB* scanlineCels;
static GouraudScanlineCCB* currentScanlineCel;
static int currentScanlineIndex = 0;

#define GRADIENT_SHADES 32
#define GRADIENT_LENGTH GRADIENT_SHADES
#define GRADIENT_GROUP_SIZE (GRADIENT_SHADES * GRADIENT_LENGTH)
static unsigned char *gourGradBmps;

static bool semisoftGouraud = false;
static uint32 softPixc = CEL_BLEND_OPAQUE;

static uint8* shadeMap = NULL;

typedef struct Edge
{
	int x;
	int c;
	int u,v;
}Edge;

typedef struct Gradients
{
	int dx,dc,du,dv;
}Gradients;

typedef struct SoftBuffer
{
	int bpp;
	int width;
	int height;
	int stride;
	//int currentIndex;
	unsigned char *data;
	CCB *cel;
}SoftBuffer;

static int renderSoftMethod = RENDER_SOFT_METHOD_GOURAUD;

SoftBuffer softBuffer;
void *softBufferCurrentPtr;

static Edge *leftEdge;
static Edge *rightEdge;
static int32 *divTab;
static Gradients grads;

static uint16 *lineColorShades[4] = { NULL, NULL, NULL, NULL };
static uint16 *gouraudColorShades;

static uint16 *activeGradient = NULL;
static Texture *activeTexture = NULL;

static int minX, maxX, minY, maxY;

#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)


static void(*fillEdges)(int y0, int y1);
static void(*prepareEdgeList) (ScreenElement *e0, ScreenElement *e1);


static void bindGradient(uint16 *gradient)
{
	activeGradient = gradient;
}

static void bindTexture(Texture *texture)
{
	activeTexture = texture;
}

static void bindMeshPolyData(Mesh *ms, int numPoly)
{
	if (renderSoftMethod <= RENDER_SOFT_METHOD_GOURAUD) {
		bindGradient(lineColorShades[numPoly & 3]);
	} else {
		bindTexture(&ms->tex[ms->poly[numPoly].textureId]);
	}
}

static uint16 *crateColorShades(int r, int g, int b, int numShades, bool absoluteZero) {
	uint16 *colorShades = (uint16*)AllocMem(sizeof(uint16) * numShades, MEMTYPE_ANY);
	int lb = 0;
	if (!absoluteZero) lb = 1;

	setPalGradient(0, numShades-1, 0,0,lb, r,g,b, colorShades);

	return colorShades;
}

void setPmvSemisoftGouraud(int r, int g, int b)
{
	static uint16 gouraudPmvShades[32];

	int i;
	for (i = 0; i < 32; ++i) {
		int cr = i;
		int cg = i;
		int cb = i;
		// Basically, an R=31, G=0, B=0 means that we only need R to affect the shade, so G and B below will be clamped to 31, always full
		// Next pass will work on G and next on B, not affecting the other channels but all three passes will be accumulated for full RGB lighting from 3 sources
		CLAMP_LEFT(cr, 31 - r);
		CLAMP_LEFT(cg, 31 - g);
		CLAMP_LEFT(cb, 31 - b);

		gouraudPmvShades[i] = (PMV_GOURAUD_SHADE_TAB[cr] << 10) | (PMV_GOURAUD_SHADE_TAB[cg] << 5) | PMV_GOURAUD_SHADE_TAB[cb];
		//gouraudPmvShades[i] = (PMV_GOURAUD_SHADE_SHINE_TAB[cr] << 10) | (PMV_GOURAUD_SHADE_SHINE_TAB[cg] << 5) | PMV_GOURAUD_SHADE_SHINE_TAB[cb];
	}

	updateGouraudColorShades(32, gouraudPmvShades);
}

void updateGouraudColorShades(int numShades, uint16 *shades)
{
	int i;
	for (i=0; i<numShades; ++i) {
		gouraudColorShades[i] = shades[i];
	}
}

static void initDivs()
{
    int i, ii;
    for (i=0; i<DIV_TAB_SIZE; ++i) {
        ii = i - DIV_TAB_SIZE / 2;
        if (ii==0) ++ii;
        divTab[i] = (1 << DIV_TAB_SHIFT) / ii;
    }
}
static GouraudScanlineCCB* createScanlineGouraudCels(int width, int height, int bpp, int type, int num)
{
	int i;
	GouraudScanlineCCB* cels = (GouraudScanlineCCB*)AllocMem(num * sizeof(GouraudScanlineCCB), MEMTYPE_ANY);

	startingScanlineCelDummy = createCel(GRADIENT_LENGTH, 1, 8, CEL_TYPE_CODED);

	for (i = 0; i < num; ++i) {
		memcpy(&cels[i], startingScanlineCelDummy, sizeof(GouraudScanlineCCB) - 8);
		memcpy(&cels[i].ccb_PRE0, &startingScanlineCelDummy->ccb_PRE0, 8);
	}

	startingScanlineCelDummy->ccb_XPos = SCREEN_WIDTH << 16;	// outside of the screen to the right
	startingScanlineCelDummy->ccb_SourcePtr = (CelData*)gourGradBmps;
	startingScanlineCelDummy->ccb_Flags &= ~CCB_LAST;
	startingScanlineCelDummy->ccb_NextPtr = (CCB*)cels;

	return cels;
}

static void initSemiSoftGouraud()
{
	int i;
	unsigned char *dst;
	int c0,c1,x;

	gourGradBmps = (unsigned char*)AllocMem(GRADIENT_SHADES * GRADIENT_GROUP_SIZE, MEMTYPE_ANY);
	dst = gourGradBmps;
	for (c0 = 0; c0 < GRADIENT_SHADES; ++c0) {
		for (c1 = 0; c1 < GRADIENT_SHADES; ++c1) {
			const int dc = ((c1 - c0) << FP_BASE) / GRADIENT_LENGTH;
			int fc = INT_TO_FIXED(c0, FP_BASE);
			for (x = 0; x < GRADIENT_LENGTH; ++x) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 0, 31)
					* dst++ = c;
				fc += dc;
			}
		}
	}

	scanlineCels = createScanlineGouraudCels(GRADIENT_LENGTH, 1, 8, CEL_TYPE_CODED, MAX_SCANLINES);
	for (i=0; i<MAX_SCANLINES; ++i) {
		if (i > 0) {
			scanlineCels[i - 1].ccb_NextPtr = &scanlineCels[i];
		}
		scanlineCels[i].ccb_Flags |= CCB_BGND;
		scanlineCels[i].ccb_Flags &= ~CCB_LAST;
		scanlineCels[i].ccb_Flags &= ~(CCB_LDPLUT | CCB_LDPRS | CCB_LDPPMP);
	}
}

static void drawAntialiasedLine(ScreenElement *e1, ScreenElement *e2)
{
	int x1 = e1->x;
	int y1 = e1->y;
	int x2 = e2->x;
	int y2 = e2->y;

	int dx, dy, l;
	int x00, y00;
	int vramofs;

	int x, y;
	int frac, shade;

	int temp;
    int chdx, chdy;

	uint16 *vram = (uint16*)softBufferCurrentPtr;
	const int screenWidth = softBuffer.width;
	const int screenHeight = softBuffer.height;
	const int stride16 = softBuffer.stride >> 1;

    // ==== Clipping ====

    int outcode1 = 0, outcode2 = 0;

    if (y1 < 0) outcode1 |= 0x0001;
        else if (y1 > screenHeight-1) outcode1 |= 0x0010;
    if (x1 < 0) outcode1 |= 0x0100;
        else if (x1 > screenWidth-1) outcode1 |= 0x1000;

    if (y2 < 0) outcode2 |= 0x0001;
        else if (y2 > screenHeight-1) outcode2 |= 0x0010;
    if (x2 < 0) outcode2 |= 0x0100;
        else if (x2 > screenWidth-1) outcode2 |= 0x1000;

    if ((outcode1 | outcode2)!=0) return; // normally, should check for possible clip
	//I will do lame method now

    // ==================

	dx = x2 - x1;
	dy = y2 - y1;

	if (dx==0 && dy==0) return;

    chdx = dx;
	chdy = dy;
    if (dx<0) chdx = -dx;
    if (dy<0) chdy = -dy;

	if (chdy < chdx) {
		if (x1 > x2) {
			temp = x1; x1 = x2; x2 = temp;
			y1 = y2;
		}

		if (dx==0) return;
        l = (dy << LN_BASE) / dx;
        y00 = y1 << LN_BASE;
		for (x=x1; x<x2; x++) {
			const int yp = y00 >> LN_BASE;

			if (x >= 0 && x < screenWidth && yp >=0 && yp < screenHeight - 1) {
				vramofs = yp*stride16 + x;
				frac = y00 & LN_AND;

				shade = (LN_AND - frac) >> 4;
				*(vram + vramofs) |= activeGradient[shade];

				shade = frac >> 4;
				*(vram + vramofs+stride16) |= activeGradient[shade];
			}
            y00+=l;
		}
	}
	else {
		if (y1 > y2) {
			temp = y1; y1 = y2; y2 = temp;
			x1 = x2;
		}

		if (dy==0) return;
        l = (dx << LN_BASE) / dy;
        x00 = x1 << LN_BASE;

		for (y=y1; y<y2; y++) {
			const int xp = x00 >> LN_BASE;

			if (y >= 0 && y < screenHeight && xp >=0 && xp < screenWidth - 1) {
				vramofs = y*stride16 + xp;
				frac = x00 & LN_AND;

				shade = (LN_AND - frac) >> 4;
				*(vram + vramofs) |= activeGradient[shade];

				shade = frac >> 4;
				*(vram + vramofs + 1) |= activeGradient[shade];
			}
            x00+=l;
		}
	}
}

static void calculateTriangleGradients(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	const int x0 = e0->x; const int x1 = e1->x; const int x2 = e2->x;
	const int y0 = e0->y; const int y1 = e1->y; const int y2 = e2->y;
	const int dd = ((x1 - x2) * (y0 - y2) - (x0 - x2) * (y1 - y2));

	if (renderSoftMethod & RENDER_SOFT_METHOD_GOURAUD) {
		const int c0 = e0->c; const int c1 = e1->c; const int c2 = e2->c;
		if (dd != 0) {
			grads.dc = (((c1 - c2) * (y0 - y2) - (c0 - c2) * (y1 - y2)) << FP_BASE) / dd;
		}
	}

	if (renderSoftMethod & RENDER_SOFT_METHOD_ENVMAP) {
		const int u0 = e0->u; const int u1 = e1->u; const int u2 = e2->u;
		const int v0 = e0->v; const int v1 = e1->v; const int v2 = e2->v;
		if (dd != 0) {
			grads.du = (((u1 - u2) * (y0 - y2) - (u0 - u2) * (y1 - y2)) << FP_BASE) / dd;
			grads.dv = (((v1 - v2) * (y0 - y2) - (v0 - v2) * (y1 - y2)) << FP_BASE) / dd;
		}
	}
}

// 107, 31, 20
// 35, 19, 14
// 24, 23, 12, 63
// 18d, 17, 10, 29

static void prepareEdgeListSoft(ScreenElement* e0, ScreenElement* e1)
{
	Edge* edgeListDst;
	int dx, dy, fx;

	int x0 = e0->x;
	int x1 = e1->x;
	int y0 = e0->y;
	int y1 = e1->y;

	// Assumes CCW
	if (y0 < y1) {	// Left Edge side
		int c0, c1, u0, u1, v0, v1;
		int dc, du, dv;
		int fc, fu = 0, fv = 0;

		bool needsGouraud = renderSoftMethod & RENDER_SOFT_METHOD_GOURAUD;
		bool needsTexture = renderSoftMethod & RENDER_SOFT_METHOD_ENVMAP;

		int repDiv = divTab[DIV_TAB_SIZE / 2 + y1 - y0];

		dx = ((x1 - x0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		fx = INT_TO_FIXED(x0, FP_BASE);

		if (needsGouraud) {
			c0 = e0->c;
			c1 = e1->c;
			dc = ((c1 - c0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
			fc = INT_TO_FIXED(c0, FP_BASE);
		}
		if (needsTexture) {
			u0 = e0->u; v0 = e0->v;
			u1 = e1->u; v1 = e1->v;
			du = ((u1 - u0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
			dv = ((v1 - v0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
			fu = INT_TO_FIXED(u0, FP_BASE);
			fv = INT_TO_FIXED(v0, FP_BASE);
		}

		if (y0 < 0) {
			fx += -y0 * dx;
			if (needsGouraud) {
				fc += -y0 * dc;
			}
			if (needsTexture) {
				fu += -y0 * du;
				fv += -y0 * dv;
			}
			y0 = 0;
		}
		if (y1 > SCREEN_HEIGHT - 1) {
			y1 = SCREEN_HEIGHT - 1;
		}
		dy = y1 - y0;

		edgeListDst = &leftEdge[y0];

		switch (renderSoftMethod) {
		case RENDER_SOFT_METHOD_GOURAUD:
		{
			while (dy-- > 0) {
				edgeListDst->x = FIXED_TO_INT(fx, FP_BASE);
				edgeListDst->c = fc;
				++edgeListDst;
				fx += dx;
				fc += dc;
			};
		} break;

		case RENDER_SOFT_METHOD_ENVMAP:
		{
			while (dy-- > 0) {
				edgeListDst->x = FIXED_TO_INT(fx, FP_BASE);
				edgeListDst->u = fu;
				edgeListDst->v = fv;
				++edgeListDst;
				fx += dx;
				fu += du;
				fv += dv;
			};
		} break;

		case (RENDER_SOFT_METHOD_GOURAUD | RENDER_SOFT_METHOD_ENVMAP):
		{
			while (dy-- > 0) {
				edgeListDst->x = FIXED_TO_INT(fx, FP_BASE);
				edgeListDst->c = fc;
				edgeListDst->u = fu;
				edgeListDst->v = fv;
				++edgeListDst;
				fx += dx;
				fc += dc;
				fu += du;
				fv += dv;
			};
		} break;

		default:
			break;
		}
	} else {
		// Reversed x0,x1 and y0,y1 inside this code (no need for temp Edge* swap)
		// Right side on this triangle algorithm only needs to interpolate X

		int repDiv = divTab[DIV_TAB_SIZE / 2 + y0 - y1];

		dx = ((x0 - x1) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		fx = INT_TO_FIXED(x1, FP_BASE);

		if (y1 < 0) {
			fx += -y1 * dx;
			y1 = 0;
		}
		if (y0 > SCREEN_HEIGHT - 1) {
			y0 = SCREEN_HEIGHT - 1;
		}
		dy = y0 - y1;

		edgeListDst = &rightEdge[y1];
		while (dy-- > 0) {
			edgeListDst->x = FIXED_TO_INT(fx, FP_BASE);
			++edgeListDst;
			fx += dx;
		};
	}
}

static void prepareEdgeListSemiGouraud(ScreenElement* e0, ScreenElement* e1)
{
	Edge* edgeListToWrite;
	int x0, x1, y0, y1, c0, c1;
	int dy, dx, dc;
	int fx, fc;

	int32* dvt = &divTab[DIV_TAB_SIZE / 2];
	int repDiv = 0;

	// Assumes CCW
	if (e0->y < e1->y) {
		edgeListToWrite = leftEdge;
	}
	else {
		ScreenElement* eTemp = e0; e0 = e1; e1 = eTemp;
		edgeListToWrite = rightEdge;
	}

	x0 = e0->x; y0 = e0->y; c0 = e0->c;
	x1 = e1->x; y1 = e1->y; c1 = e1->c;

	if (y0 > SCREEN_HEIGHT - 1 || y1 < 0 || y1 < y0) return;

	if ((y1 - y0) < DIV_TAB_SIZE / 2) {
		repDiv = dvt[y1 - y0];
	}

	dx = ((x1 - x0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
	dc = ((c1 - c0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);

	fx = INT_TO_FIXED(x0, FP_BASE);
	fc = INT_TO_FIXED(c0, FP_BASE);

	if (y0 < 0) {
		fx += -y0 * dx;
		fc += -y0 * dc;
		y0 = 0;
	}
	if (y1 > SCREEN_HEIGHT - 1) {
		y1 = SCREEN_HEIGHT - 1;
	}
	dy = y1 - y0;

	edgeListToWrite = &edgeListToWrite[y0];
	while (dy-- > 0) {
		int x = FIXED_TO_INT(fx, FP_BASE);
		edgeListToWrite->x = x;
		edgeListToWrite->c = fc;
		++edgeListToWrite;
		fx += dx;
		fc += dc;
	};
}

static void fillGouraudEdges8_SemiSoft(int y0, int y1)
{
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	int y;
	//GouraudScanlineCCB *firstCel = currentScanlineCel;

	for (y=y0; y<=y1; ++y) {
		const int xl = le->x;
		int cl = le->c;
		int cr = re->c;
		int length = re->x - xl;

		GouraudScanlineCCB *cel = currentScanlineCel++;

		if (currentScanlineIndex >= MAX_SCANLINES) return;	// fuck you I am not adding
		currentScanlineIndex++;

		cl = FIXED_TO_INT(cl, FP_BASE);
		cr = FIXED_TO_INT(cr, FP_BASE);
		CLAMP(cl,0,GRADIENT_LENGTH)
		CLAMP(cr,0,GRADIENT_LENGTH)

		cel->ccb_Flags &= ~CCB_LDPLUT;

		cel->ccb_XPos = xl<<16;
		cel->ccb_YPos = y<<16;

		cel->ccb_HDX = (length<<20) / GRADIENT_LENGTH;
		
		cel->ccb_SourcePtr = (CelData*)&gourGradBmps[cl * GRADIENT_GROUP_SIZE + cr * GRADIENT_LENGTH];

		++le;
		++re;
	}
	//If we ever need different gouraud gradient colors again I will enable that, like for every triangle to point to different color shades, but now we point to the same at startingScanlineCelDummy
	//firstCel->ccb_PLUTPtr = (void*)gouraudColorShades;
	//firstCel->ccb_Flags |= CCB_LDPLUT;
}

static void fillGouraudEdges8(int y0, int y1)
{
	const int stride8 = softBuffer.stride;
	unsigned char *vram8 = (unsigned char*)softBufferCurrentPtr + y0 * stride8;

	const int dc = grads.dc;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	do {
		int xl, cl;
		int fc, xlp;
		int length;
		unsigned char* dst;
		uint32* dst32;

		xl = le->x;
		cl = le->c;
		if (xl < 0) {
			cl += -xl * dc;
			xl = 0;
		}
		CLAMP_RIGHT(re->x, SCREEN_WIDTH - 1);
		length = re->x - xl;

		dst = vram8 + xl;
		fc = cl;

		xlp = xl & 3;
		if (xlp) {
			xlp = 4 - xlp;
			while (xlp-- > 0 && length-- > 0) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				*dst++ = c;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;
			c1 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;
			c2 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;
			c3 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;

			#ifdef BIG_ENDIAN
				*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			#else
				*dst32++ = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
			#endif
			length-=4;
		};

		dst = (unsigned char*)dst32;
		while (length-- > 0) {
			int c = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;

			*dst++ = c;
		}

		++le;
		++re;
		vram8 += stride8;
	} while(--count > 0);
}


static void fillGouraudEdges16(int y0, int y1)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;

	const int dc = grads.dc;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];
	do {
		int xl, cl;
		int fc;
		int length;
		uint16* dst;
		uint32* dst32;

		xl = le->x;
		cl = le->c;
		if (xl < 0) {
			cl += -xl * dc;
			xl = 0;
		}
		CLAMP_RIGHT(re->x, SCREEN_WIDTH - 1);
		length = re->x - xl;

		dst = vram16 + xl;
		fc = cl;

		if (length>0){
			if (xl & 1) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				*dst++ = activeGradient[c];
				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c0 = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;
				c1 = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				#ifdef BIG_ENDIAN
					*dst32++ = (activeGradient[c0] << 16) | activeGradient[c1];
				#else
					*dst32++ = (activeGradient[c1] << 16) | activeGradient[c0];
				#endif
				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				*dst++ = activeGradient[c];
			}
		}

		++le;
		++re;
		vram16 += stride16;
	} while(--count > 0);
}

static void fillEnvmapEdges8(int y0, int y1)
{
	const int stride8 = softBuffer.stride;
	uint8 *vram8 = (uint8*)softBufferCurrentPtr + y0 * stride8;

	const int du = grads.du;
	const int dv = grads.dv;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	unsigned char* texData = (unsigned char*)activeTexture->bitmap;

	do {
		int xl, ul, vl;
		int fu, fv, xlp;
		int length;
		uint8* dst;
		uint32* dst32;

		xl = le->x;
		ul = le->u;
		vl = le->v;
		if (xl < 0) {
			ul += -xl * du;
			vl += -xl * dv;
			xl = 0;
		}
		CLAMP_RIGHT(re->x, SCREEN_WIDTH - 1);
		length = re->x - xl;

		dst = vram8 + xl;

		fu = ul;
		fv = vl;

		xlp = xl & 3;
		if (xlp) {
			while (xlp++ < 4 && length-- > 0) {
				*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			c1 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			c2 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			c3 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			#ifdef BIG_ENDIAN
				*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			#else
				*dst32++ = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
			#endif
			length-=4;
		};

		dst = (uint8*)dst32;
		while (length-- > 0) {
			*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;
		}

		++le;
		++re;
		vram8 += stride8;
	} while(--count > 0);
}

static void fillEnvmapEdges16(int y0, int y1)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;

	const int du = grads.du;
	const int dv = grads.dv;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	uint16* texData = (uint16*)activeTexture->bitmap;

	do {
		int xl, ul, vl;
		int length;

		xl = le->x;
		ul = le->u;
		vl = le->v;
		if (xl < 0) {
			ul += -xl * du;
			vl += -xl * dv;
			xl = 0;
		}
		CLAMP_RIGHT(re->x, SCREEN_WIDTH - 1);
		length = re->x - xl;

		if (length>0){
			uint16 *dst = vram16 + xl;
			uint32 *dst32;

			int fu = ul;
			int fv = vl;
			
			if (xl & 1) {
				*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;

				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c0 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;

				c1 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;

				#ifdef BIG_ENDIAN
					*dst32++ = (c0 << 16) | c1;
				#else
					*dst32++ = (c1 << 16) | c0;
				#endif
				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length != 0) {
				*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;
			}
		}

		++le;
		++re;
		vram16 += stride16;
	} while(--count > 0);
}

static void fillGouraudEnvmapEdges8(int y0, int y1)
{
	const int stride8 = softBuffer.stride;
	uint8 *vram8 = (uint8*)softBufferCurrentPtr + y0 * stride8;

	const int dc = grads.dc;
	const int du = grads.du;
	const int dv = grads.dv;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	unsigned char* texData = (unsigned char*)activeTexture->bitmap;

	do {
		int xl, cl, ul, vl;
		int fc, fu, fv, xlp;
		int length;
		uint8* dst;
		uint32* dst32;

		xl = le->x;
		cl = le->c;
		ul = le->u;
		vl = le->v;
		if (xl < 0) {
			cl += -xl * dc;
			ul += -xl * du;
			vl += -xl * dv;
			xl = 0;
		}
		CLAMP_RIGHT(re->x, SCREEN_WIDTH - 1);
		length = re->x - xl;

		dst = vram8 + xl;

		fc = cl;
		fu = ul;
		fv = vl;

		xlp = xl & 3;
		if (xlp) {
			while (xlp++ < 4 && length-- > 0) {
				*dst++ = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;

				fc += dc;
				fu += du;
				fv += dv;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			c1 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			c2 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			c3 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			#ifdef BIG_ENDIAN
				* dst32++ = ((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
			#else
			* dst32++ = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0);
			#endif
			length-=4;
		};

		dst = (uint8*)dst32;
		while (length-- > 0) {
			*dst++ = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;

			fc += dc;
			fu += du;
			fv += dv;
		}

		++le;
		++re;
		vram8 += stride8;
	} while(--count > 0);
}

static void fillGouraudEnvmapEdges16(int y0, int y1)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;

	const int dc = grads.dc;
	const int du = grads.du;
	const int dv = grads.dv;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	uint16* texData = (uint16*)activeTexture->bitmap;

	do {
		int xl, cl, ul, vl;
		int length;
		uint32 rb0, g0;

		xl = le->x;
		cl = le->c;
		ul = le->u;
		vl = le->v;
		if (xl < 0) {
			cl += -xl * dc;
			ul += -xl * du;
			vl += -xl * dv;
			xl = 0;
		}
		CLAMP_RIGHT(re->x, SCREEN_WIDTH - 1);
		length = re->x - xl;

		if (length>0){
			uint16 *dst = vram16 + xl;
			uint32 *dst32;

			int fc = cl;
			int fu = ul;
			int fv = vl;

			int c, cc;
			if (xl & 1) {
				c = FIXED_TO_INT(fc, FP_BASE);
				cc = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				rb0 = (((cc & 0x7c1f) * c) >> 5) & 0x7c1f;
				g0 = (((cc & 0x03e0) * c) >> 5) & 0x03e0;
				*dst++ = (uint16)(rb0 | g0);
				fc += dc;
				fu += du;
				fv += dv;

				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;
				uint32 rb0g1, g0rb1;

				c = FIXED_TO_INT(fc, FP_BASE);
				c0 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fc += dc;
				fu += du;
				fv += dv;

				c = FIXED_TO_INT(fc, FP_BASE);
				c1 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fc += dc;
				fu += du;
				fv += dv;

				#ifdef BIG_ENDIAN
					cc = (c0 << 16) | c1;
				#else
					cc = (c1 << 16) | c0;
				#endif

				rb0g1 = (((cc & 0x7c1f03e0) >> 5) * c) & 0x7c1f03e0;
				g0rb1 = (((cc & 0x03e07c1f) * c) >> 5) & 0x03e07c1f;
				*dst32++ = rb0g1 | g0rb1;

				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				c = FIXED_TO_INT(fc, FP_BASE);
				cc = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				rb0 = (((cc & 0x7c1f) * c) >> 5) & 0x7c1f;
				g0 = (((cc & 0x03e0) * c) >> 5) & 0x03e0;
				*dst++ = (uint16)(rb0 | g0);
				fc += dc;
				fu += du;
				fv += dv;
			}
		}

		++le;
		++re;
		vram16 += stride16;
	} while(--count > 0);
}

/*static bool shouldSkipTriangle(ScreenElement* e0, ScreenElement* e1, ScreenElement* e2)
{
	int outcode1 = 0, outcode2 = 0, outcode3 = 0;

	const int edgeL = 0;
	const int edgeR = SCREEN_WIDTH - 1;
	const int edgeU = 0;
	const int edgeD = SCREEN_HEIGHT - 1;

	const int x0 = e0->x + minX;
	const int y0 = e0->y + minY;
	const int x1 = e1->x + minX;
	const int y1 = e1->y + minY;
	const int x2 = e2->x + minX;
	const int y2 = e2->y + minY;

    if (y0 < edgeU) outcode1 |= 0x0001;
        else if (y0 > edgeD) outcode1 |= 0x0010;
    if (x0 < edgeL) outcode1 |= 0x0100;
        else if (x0 > edgeR) outcode1 |= 0x1000;

    if (y1 < edgeU) outcode2 |= 0x0001;
        else if (y1 > edgeD) outcode2 |= 0x0010;
    if (x1 < edgeL) outcode2 |= 0x0100;
        else if (x1 > edgeR) outcode2 |= 0x1000;

	if (y2 < edgeU) outcode3 |= 0x0001;
        else if (y2 > edgeD) outcode3 |= 0x0010;
    if (x2 < edgeL) outcode3 |= 0x0100;
        else if (x2 > edgeR) outcode3 |= 0x1000;

    return ((outcode1 & outcode2 & outcode3)!=0);
}*/

static void drawTriangle(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	int y0, y1;

	//if (shouldSkipTriangle(e0, e1, e2)) return;

	prepareEdgeList(e0, e1);
	prepareEdgeList(e1, e2);
	prepareEdgeList(e2, e0);

	y0 = e0->y;
	y1 = y0;

	if (e1->y < y0) y0 = e1->y; if (e1->y > y1) y1 = e1->y;
	if (e2->y < y0) y0 = e2->y; if (e2->y > y1) y1 = e2->y;

	CLAMP_LEFT(y0, 0);
	CLAMP_RIGHT(y1, SCREEN_HEIGHT - 1);

	calculateTriangleGradients(e0, e1, e2);

	if (y0 < y1) {
		fillEdges(y0, y1-1);
	}
}

static void updateSoftBufferVariables(int posX, int posY, int width, int height, Mesh *ms)
{
	int celType = CEL_TYPE_UNCODED;
	int currentBufferSize;
	softBuffer.bpp = 16;
	if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
		softBuffer.bpp = 8;
		celType = CEL_TYPE_CODED;
	}

	if (width > SCREEN_WIDTH - 1) {
		width = SCREEN_WIDTH - 1;
		CLAMP_LEFT(posX, 0);
	}

	if (height > SCREEN_HEIGHT - 1) {
		height = SCREEN_HEIGHT - 1;
		CLAMP_LEFT(posY, 0);
	}


	softBuffer.width = width;
	softBuffer.height = height;
	softBuffer.stride = (((softBuffer.width * softBuffer.bpp) + 31) >> 5) << 2;	// must be multiples of 4 bytes
	if (softBuffer.stride < 8) softBuffer.stride = 8;					// and no less than 8 bytes

	currentBufferSize = (((softBuffer.stride * softBuffer.height) + 255) >> 8) << 8; // must be in multiples of 256 bytes for the unrolled optimized memset
	/*if (softBuffer.currentIndex + currentBufferSize > SOFT_BUFF_MAX_SIZE) {
		softBuffer.currentIndex = 0;
	}*/
	//softBufferCurrentPtr = &softBuffer.data[softBuffer.currentIndex];
	softBufferCurrentPtr = &softBuffer.data[0];

	// I can't for the love of god remember why I needed this currentIndex
	// Like instead of the software rendered 3d object on a CEL drawn at the start of the buffer, I offset it every frame
	// Was it in the case I would render more than one objects in the scene at the same time? But then I might still give one drawcall per object instead of link those CELs together, wouldn't I?
	// I will comment out everything and then maybe delete later when I fix this

	if (currentBufferSize <= SOFT_BUFF_MAX_SIZE) {
		#ifdef ASM_VRAMSET
			vramSet(0, softBufferCurrentPtr, currentBufferSize);
		#else
			memset(softBufferCurrentPtr, 0, currentBufferSize);
		#endif

		//softBuffer.currentIndex += currentBufferSize;
	}	// else something went wrong

	setCelWidth(softBuffer.width, softBuffer.cel);
	setCelHeight(softBuffer.height, softBuffer.cel);
	setCelStride(softBuffer.stride, softBuffer.cel);
	setCelBpp(softBuffer.bpp, softBuffer.cel);
	setCelType(celType, softBuffer.cel);
	setCelBitmap(softBufferCurrentPtr, softBuffer.cel);
	setCelPosition(posX, posY, softBuffer.cel);
}

static void prepareAndPositionSoftBuffer(Mesh *ms, ScreenElement *elements)
{
	int i;
	const int count = ms->verticesNum;
	int width, height;

	minX = maxX = elements[0].x;
	minY = maxY = elements[0].y;

	for (i=1; i<count; ++i) {
		const int x = elements[i].x;
		const int y = elements[i].y;
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
	}

	width = maxX - minX + 1;
	height = maxY - minY + 1;

	// Offset element positions to upper left min corner
	for (i=0; i<count; ++i) {
		if (width <= SCREEN_WIDTH) elements[i].x -= minX;
		if (height <= SCREEN_HEIGHT) elements[i].y -= minY;
	}

	updateSoftBufferVariables(minX, minY, width, height, ms);
}

static bool mustUseSemisoftGouraud(Mesh *ms)
{
	return semisoftGouraud && (renderSoftMethod == RENDER_SOFT_METHOD_GOURAUD) && (ms->renderType & MESH_OPTION_RENDER_SOFT8);
}

static void prepareMeshSoftRender(Mesh *ms, ScreenElement *elements)
{
	const bool useSemisoftGouraud = mustUseSemisoftGouraud(ms);

	if (useSemisoftGouraud) {
		currentScanlineIndex = 0;
		currentScanlineCel = scanlineCels;
		startingScanlineCelDummy->ccb_PLUTPtr = (void*)gouraudColorShades;
		startingScanlineCelDummy->ccb_PIXC = softPixc;
		prepareEdgeList = prepareEdgeListSemiGouraud;
	} else {
		prepareAndPositionSoftBuffer(ms, elements);
		prepareEdgeList = prepareEdgeListSoft;
	}

	switch(renderSoftMethod) {
		case RENDER_SOFT_METHOD_GOURAUD:
		{
			if (useSemisoftGouraud) {
				prepareEdgeList = prepareEdgeListSemiGouraud;
				fillEdges = fillGouraudEdges8_SemiSoft;
			} else {
				if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
					fillEdges = fillGouraudEdges8;
				} else {
					fillEdges = fillGouraudEdges16;
				}
			}
		}
		break;

		case RENDER_SOFT_METHOD_ENVMAP:
		{
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillEnvmapEdges8;
			} else {
				fillEdges = fillEnvmapEdges16;
			}
		}
		break;

		case RENDER_SOFT_METHOD_GOURAUD | RENDER_SOFT_METHOD_ENVMAP:
		{
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillGouraudEnvmapEdges8;
			} else {
				fillEdges = fillGouraudEnvmapEdges16;
			}
		}
		break;

		default:
		break;
	}
}

static void renderMeshSoft(Mesh *ms, ScreenElement *elements)
{
	ScreenElement *e0, *e1, *e2;
	int i,n;

	int *index = ms->index;

	prepareMeshSoftRender(ms, elements);

	for (i=0; i<ms->polysNum; ++i) {
		e0 = &elements[*index++];
		e1 = &elements[*index++];
		e2 = &elements[*index++];

		n = (e0->x - e1->x) * (e2->y - e1->y) - (e2->x - e1->x) * (e0->y - e1->y);
		if (n > 0) {
			bindMeshPolyData(ms, i);
			drawTriangle(e0, e1, e2);

			if (ms->poly[i].numPoints == 4) {	// if quad then render another triangle
				e1 = e2;
				e2 = &elements[*index];
				drawTriangle(e0, e1, e2);
			}
		}
		if (ms->poly[i].numPoints == 4) ++index;
	}

	if (mustUseSemisoftGouraud(ms)) {
		if (currentScanlineCel != scanlineCels) {	// something added
			GouraudScanlineCCB* lastScanlineCel = currentScanlineCel - 1;
			lastScanlineCel->ccb_Flags |= CCB_LAST;
			if (ms->maxLights == 1) {
				drawCels(startingScanlineCelDummy);
			} else {	// hack for now RGB (but will also need to update the light normal calcs later on)
				setPmvSemisoftGouraud(31, 0, 0); drawCels(startingScanlineCelDummy);
				setPmvSemisoftGouraud(0, 31, 0); drawCels(startingScanlineCelDummy);
				setPmvSemisoftGouraud(0, 0, 31); drawCels(startingScanlineCelDummy);
			}
			lastScanlineCel->ccb_Flags &= ~CCB_LAST;
		}
	} else {
		softBuffer.cel->ccb_PIXC = softPixc;
		drawCels(softBuffer.cel);
	}
}

static void renderMeshSoftWireframe(Mesh *ms, ScreenElement *elements)
{
	ScreenElement *e0, *e1;

	int *lineIndex = ms->lineIndex;
	int i;

	prepareMeshSoftRender(ms, elements);

	for (i=0; i<ms->linesNum; ++i) {
		e0 = &elements[*lineIndex++];
		e1 = &elements[*lineIndex++];

		bindMeshPolyData(ms, i);
		drawAntialiasedLine(e0, e1);
	}

	drawCels(softBuffer.cel);
}

void renderTransformedMeshSoft(Mesh *ms, ScreenElement *elements)
{
	if (renderSoftMethod == RENDER_SOFT_METHOD_WIREFRAME) {
		renderMeshSoftWireframe(ms, elements);
	} else {
		renderMeshSoft(ms, elements);
	}
}

void setRenderSoftMethod(int method)
{
	renderSoftMethod = method;
}

void setRenderSoftPixc(uint32 pixc)
{
	softPixc = pixc;
}

void updateRenderSoftShademap(uint8* shadeMapSrc)
{
	int i;
	for (i = 0; i < COLOR_GRADIENTS_SIZE; ++i) {
		shadeMap[i] = shadeMapSrc[i];
	}
}

uint8* getRenderSoftShademap()
{
	return shadeMap;
}

void initDefaultShadeMaps()
{
	int i;
	for (i=0; i<COLOR_GRADIENTS_SIZE; ++i) {
		int c = i;
		CLAMP(c, 4, COLOR_GRADIENTS_SIZE - 4);
		shadeMap[i] = c;
	}
}

void initInvertedShadeMaps()
{
	int i;
	for (i=0; i<COLOR_GRADIENTS_SIZE; ++i) {
		int c = COLOR_GRADIENTS_SIZE - 1 - i;
		c -= 16;
		CLAMP(c, 0, COLOR_GRADIENTS_SIZE - 1);
		shadeMap[i] = c;
	}
}

static void initSoftBuffer()
{
	softBuffer.bpp = 16;
	softBuffer.width = SCREEN_WIDTH;
	softBuffer.height = SCREEN_HEIGHT;
	//softBuffer.currentIndex = 0;

	softBuffer.data = AllocMem(SOFT_BUFF_MAX_SIZE, MEMTYPE_ANY);
	softBuffer.cel = createCel(softBuffer.width, softBuffer.height, softBuffer.bpp, CEL_TYPE_UNCODED);
	setupCelData(gouraudColorShades, softBuffer.data, softBuffer.cel);
}

static void initSoftEngineArrays()
{
	leftEdge = (Edge*)AllocMem(SCREEN_HEIGHT * sizeof(Edge), MEMTYPE_ANY);
	rightEdge = (Edge*)AllocMem(SCREEN_HEIGHT * sizeof(Edge), MEMTYPE_ANY);
	divTab = (int32*)AllocMem(DIV_TAB_SIZE * sizeof(int32), MEMTYPE_ANY);
	shadeMap = (uint8*)AllocMem(COLOR_GRADIENTS_SIZE, MEMTYPE_ANY);

	initDefaultShadeMaps();
	initDivs();
}

void initEngineSoft(bool usesSemisoftGouraud, bool needsSoftBuffer)
{
	initSoftEngineArrays();

	semisoftGouraud = usesSemisoftGouraud;
	if (semisoftGouraud) initSemiSoftGouraud();

	if (!lineColorShades[0]) lineColorShades[0] = crateColorShades(31,23,15, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[1]) lineColorShades[1] = crateColorShades(15,23,31, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[2]) lineColorShades[2] = crateColorShades(15,31,23, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[3]) lineColorShades[3] = crateColorShades(31,15,23, COLOR_GRADIENTS_SIZE, false);

	if (!gouraudColorShades) gouraudColorShades = crateColorShades(31,31,31, COLOR_GRADIENTS_SIZE, true);

	if (needsSoftBuffer) initSoftBuffer();

	// Bind something to avoid crash
	bindGradient(lineColorShades[0]);
}
