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

static Viewer *viewer;
static Light *light;

static Object3D *loadedObj;
static Mesh *loadedMesh;

static Mesh *gridMesh;
static Object3D *gridObj;

static Texture *flatTex;
static Texture *gridTex;

static uint16 gridPal[32];
static uint16 vasePal[32];

static World *myWorld;


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

void effectMeshGouraudRGBlightsInit()
{
	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	
	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);
	setPalGradient(0,31, 23,17,11, 27,23,19, vasePal);

	flatTex = initGenTexture(32,32, 8, vasePal, 1, TEXGEN_NOISE, NULL);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	
	loadedMesh = loadMesh("data/vase.plg", MESH_LOAD_SKIP_LINES, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, flatTex);
	loadedObj = initObject3D(loadedMesh);

	gridObj = initObject3D(gridMesh);
	shadeGrid();

	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);

	setObject3Dpos(loadedObj, 0, 320, 0);
	setObject3Drot(loadedObj, 0, 0, 0);

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,192,-1024);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);
	
	addObjectToWorld(gridObj, 0, false, myWorld);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);
	addObjectToWorld(loadedObj, 1, true, myWorld);
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

void effectMeshGouraudRGBlightsRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	renderWorld(myWorld);
}
