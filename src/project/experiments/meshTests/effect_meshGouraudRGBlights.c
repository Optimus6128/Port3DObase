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
static Light *light[NUM_LIGHTS];

static Object3D *loadedObj[OBJ_NUM];
static Mesh *loadedMesh[OBJ_NUM];

static Mesh *gridMesh;
static Object3D *gridObj;

static Mesh *lightMesh[NUM_LIGHTS];
static Object3D *lightObj[NUM_LIGHTS];

static Texture *vaseTex;
static Texture *gridTex;
static Texture *blobTex;

static uint16 gridPal[32];
static uint16 vasePal[32];
static uint16 blobPal[NUM_LIGHTS * 32];

static World *myWorld;
static bool lightZero = true;


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
	const int d = 2;

	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	MeshgenParams particlesParams = makeMeshgenParticlesParams(1);

	setPalGradient(0, 31, 1, 3, 7, 31, 27, 23, gridPal);
	setPalGradient(0, 31, 23 / d, 17 / d, 11 / d, 27 / d, 23 / d, 19 / d, vasePal);

	vaseTex = initGenTexture(32, 32, 8, vasePal, 1, TEXGEN_NOISE, NULL);
	gridTex = initGenTexture(16, 16, 8, gridPal, 1, TEXGEN_GRID, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	gridObj = initObject3D(gridMesh);
	shadeGrid();

	setPalGradient(0,31, 0,0,0, 31,7,7, &blobPal[0]);
	setPalGradient(0,31, 0,0,0, 7,31,7, &blobPal[32]);
	setPalGradient(0,31, 0,0,0, 7,7,31, &blobPal[64]);
	blobTex = initGenTexture(32,32, 8, blobPal, 3, TEXGEN_BLOB_RADIAL, NULL);

	for (i = 0; i < NUM_LIGHTS; ++i) {
		lightMesh[i] = initGenMesh(MESH_PARTICLES, particlesParams, MESH_OPTION_RENDER_BILLBOARDS, blobTex);
		lightMesh[i]->cel->ccb_PLUTPtr = (uint16*)&lightMesh[i]->tex->pal[i << 5];
		setMeshTranslucency(lightMesh[i], true, true);
		lightObj[i] = initObject3D(lightMesh[i]);
	}

	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);
	addObjectToWorld(gridObj, 0, false, myWorld);

	for (i = 0; i < OBJ_NUM; ++i) {
		int meshOptions = MESH_OPTIONS_DEFAULT;
		if (lightZero) {
			meshOptions |= MESH_OPTION_ENABLE_LIGHTING;
		}
		if (i == OBJ_SOFT) meshOptions |= (MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_GOURAUD_RGB);

		loadedMesh[i] = loadMesh("data/vase.plg", MESH_LOAD_SKIP_LINES, meshOptions, vaseTex);
		loadedObj[i] = initObject3D(loadedMesh[i]);

		setObject3Dpos(loadedObj[i], 0, 320, 0);
		setObject3Drot(loadedObj[i], 0, 0, 0);

		addObjectToWorld(loadedObj[i], 1, true, myWorld);
		//if (i == OBJ_SOFT) setMeshMaxLights(loadedMesh[i], 3);
	}

	for (i = 0; i < NUM_LIGHTS; ++i) {
		addObjectToWorld(lightObj[i], 1, true, myWorld);
	}
}

void effectMeshGouraudRGBlightsInit()
{
	int i;

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,192,-1024);

	myWorld = initWorld(128, 1, 1);

	prepareMeshObjects();

	addCameraToWorld(viewer->camera, myWorld);

	for (i = 0; i < NUM_LIGHTS; ++i) {
		light[i] = createLight(false);
		addLightToWorld(light[i], myWorld);
	}

	setRenderSoftPixc(PPMPC_1S_CFBD | PPMPC_MS_PDC | PPMPC_2S_0 | PPMPC_2D_2);
	setPmvSemisoftGouraud(31, 31, 31);

	setBillboardScale(1024);
}

static void moveLights(int t, int index)
{
	const int tt = t << 10;
	Light* l = light[index];

	l->pos.x = SinF16(tt) >> 7;
	l->pos.y = 256 + (SinF16((1+index)*tt) >> 11) + (CosF16((2+index)*tt) >> 12) + index * 64;
	l->pos.z = CosF16(tt) >> 7;

	setObject3Dpos(lightObj[index], l->pos.x, l->pos.y, l->pos.z);
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

void effectMeshGouraudRGBlightsRun()
{
	int i;
	Vector3D realViewerPos;

	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	for (i = 0; i < NUM_LIGHTS; ++i) {
		moveLights((i+1) * currTicks, i);
		setGlobalLightDirFromPositionAgainstObject(&light[i]->pos, loadedObj[OBJ_HARD], i+1);
	}

	realViewerPos.x = viewer->pos.x >> FP_VPOS;
	realViewerPos.y = viewer->pos.y >> FP_VPOS;
	realViewerPos.z = viewer->pos.z >> FP_VPOS;
	setGlobalLightDirFromPositionAgainstObject(&realViewerPos, loadedObj[OBJ_HARD], 0);
	renderWorld(myWorld);
}
