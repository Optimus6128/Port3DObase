#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"
#include "file_utils.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"
#include "engine_world.h"
#include "engine_view.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"


#define GRID_SIZE 16
#define NUM_LIGHTS 3

enum { OBJ_SOFT, OBJ_HARD, OBJ_NUM };

static Viewer *viewer;
static Light *light;

static Object3D *loadedObj[OBJ_NUM];
static Mesh *loadedMesh[OBJ_NUM];

static Mesh *gridMesh;
static Object3D *gridObj;

static Mesh *particlesMesh;
static Object3D *particlesObj;

static Texture *vaseTex;
static Texture *gridTex;
static Texture *blobTex;

static uint16 gridPal[32];
static uint16 vasePal[32];
static uint16 blobPal[NUM_LIGHTS * 32];

static World *myWorld;


static void setPmvGouraud(int r, int g, int b)
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

static void shadeGrid()
{
	int x,y;
	CCB *cel = gridMesh->cel;

	for (y=0; y<GRID_SIZE; ++y) {
		const int yc = y - GRID_SIZE / 2;
		for (x=0; x<GRID_SIZE; ++x) {
			const int xc = x - GRID_SIZE / 2;
			int r = (isqrt(xc*xc + yc*yc) * 16) / (GRID_SIZE / 2);
			CLAMP(r,0,15)
			cel->ccb_PIXC = shadeTable[15-r];
			++cel;
		}
	}
}

static void prepareMeshObjects()
{
	int i;

	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	MeshgenParams particlesParams = makeMeshgenParticlesParams(NUM_LIGHTS);

	setPalGradient(0, 31, 1, 3, 7, 31, 27, 23, gridPal);
	setPalGradient(0, 31, 23, 17, 11, 27, 23, 19, vasePal);

	vaseTex = initGenTexture(32, 32, 8, vasePal, 1, TEXGEN_NOISE, NULL);
	gridTex = initGenTexture(16, 16, 8, gridPal, 1, TEXGEN_GRID, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	gridObj = initObject3D(gridMesh);
	shadeGrid();

	setPalGradient(0,31, 0,0,0, 31,7,7, &blobPal[0]);
	setPalGradient(0,31, 0,0,0, 7,31,7, &blobPal[32]);
	setPalGradient(0,31, 0,0,0, 7,7,31, &blobPal[64]);
	blobTex = initGenTexture(32,32, 8, blobPal, 3, TEXGEN_BLOB_RADIAL, NULL);

	particlesMesh = initGenMesh(MESH_PARTICLES, particlesParams, MESH_OPTION_RENDER_BILLBOARDS | MESH_OPTION_NO_POLYSORT, blobTex);
	setMeshTranslucency(particlesMesh, true, true);
	particlesObj = initObject3D(particlesMesh);

	for (i = 0; i < NUM_LIGHTS; ++i) {
		PolyData* poly = &particlesMesh->poly[i];
		poly->palId = i;
		particlesMesh->cel[i].ccb_PLUTPtr = (uint16*)&particlesMesh->tex->pal[poly->palId << 5];
	}

	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);
	addObjectToWorld(gridObj, 0, false, myWorld);

	for (i = 0; i < OBJ_NUM; ++i) {
		int meshOptions = MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING;
		if (i == OBJ_SOFT) meshOptions |= MESH_OPTION_RENDER_SOFT8;
		loadedMesh[i] = loadMesh("data/vase.plg", MESH_LOAD_SKIP_LINES, meshOptions, vaseTex);
		loadedObj[i] = initObject3D(loadedMesh[i]);

		setObject3Dpos(loadedObj[i], 0, 320, 0);
		setObject3Drot(loadedObj[i], 0, 0, 0);

		if (i == OBJ_HARD) addObjectToWorld(loadedObj[i], 1, true, myWorld);
	}

	addObjectToWorld(particlesObj, 1, false, myWorld);
}

void effectMeshGouraudRGBlightsInit()
{
	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,192,-1024);

	light = createLight(false);

	myWorld = initWorld(128, 1, 1);

	prepareMeshObjects();

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);

	setRenderSoftPixc(PPMPC_1S_CFBD | PPMPC_MS_PDC | PPMPC_2S_0 | PPMPC_2D_2);

	setBillboardScale(1024);
}

static void moveLights(int t, Vertex *v)
{
	const int tt = t << 10;

	light->pos.x = SinF16(tt) >> 7;
	light->pos.y = 256 + (SinF16(2*tt) >> 10) + (CosF16(3*tt) >> 9);
	light->pos.z = CosF16(tt) >> 7;

	v->x = light->pos.x;
	v->y = light->pos.y;
	v->z = light->pos.z;

	setGlobalLightDirFromMovingLightAgainstObject(light, loadedObj[1]);
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

void effectMeshGouraudRGBlightsRun()
{
	const int restCol = 0;
	const int thisCol = 31;

	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	renderWorld(myWorld);

	// I have commented out adding the vase objects to world as I didn't have much control on switching PmvGouraud and obviously lighting directions too.
	// This is a manual hack, I could add control to the world engine, setting different pmvGouraud options and recalculate light for each object.
	// I am trying to do the 3-pass solution for gouraud RGB lights
	//renderObject3D(loadedObj[1], viewer->camera, NULL, 0);

	moveLights(currTicks, &particlesMesh->vertex[0]);
	setPmvGouraud(thisCol, restCol, restCol);
	renderObject3D(loadedObj[0], viewer->camera, NULL, 0);

	moveLights(2*currTicks, &particlesMesh->vertex[1]);
	setPmvGouraud(restCol, thisCol, restCol);
	renderObject3D(loadedObj[0], viewer->camera, NULL, 0);

	moveLights(3*currTicks, &particlesMesh->vertex[2]);
	setPmvGouraud(restCol, restCol, thisCol);
	renderObject3D(loadedObj[0], viewer->camera, NULL, 0);
}
