#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_soft.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"

#include "sprite_engine.h"


#define RENDER_TEST_GOURAUD (1 << 0)
#define RENDER_TEST_TEXTURE (1 << 1)
#define RENDER_TEST_GOURAUD_TEXTURE (RENDER_TEST_GOURAUD | RENDER_TEST_TEXTURE)


static int rotX=10, rotY=20, rotZ=50;
static int zoom=512;

static const int rotVel = 2;
static const int zoomVel = 2;

static Camera *camera;

static Object3D *softObj[2];
static Object3D *hardObj[2];

static int renderTestIndex = RENDER_TEST_GOURAUD;

static bool autoRot = false;

static int selectedObj = 0;

 
static Object3D *initMeshObject(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex)
{
	Object3D *meshObj;

	Mesh *softMesh = initGenMesh(meshgenId, params, optionsFlags, tex);
	meshObj = initObject3D(softMesh);
	setObject3Dmesh(meshObj, softMesh);

	return meshObj;
}

static MeshgenParams initMeshObjectParams(int meshgenId)
{
	MeshgenParams params;

	switch(meshgenId) {
		case MESH_CUBE:
		{
			params = makeDefaultMeshgenParams(96);
		}
		break;

		case MESH_SQUARE_COLUMNOID:
		{
			int i;
			const int numPoints = 8;
			const int size = 64;
			Point2Darray *ptArray = initPoint2Darray(numPoints);

			for (i=0; i<numPoints; ++i) {
				const int y = (size/4) * (numPoints/2 - i);
				const int r = ((SinF16((i*20) << 16) * (size / 2)) >> 16) + size / 2;
				addPoint2D(ptArray, r,y);
			}
			params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

			//destroyPoint2Darray(ptArray); //why it crashes now?
		}
		break;
	}

	return params;
}


static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		rotX += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		rotX -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		rotY += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		rotY -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		rotZ += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		rotZ -= rotVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		autoRot = !autoRot;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_SELECT)) {
		selectedObj = (selectedObj+1) & 1;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		if (++renderTestIndex > RENDER_TEST_GOURAUD_TEXTURE) renderTestIndex = RENDER_TEST_GOURAUD;
	}
}


void effectMeshGouraudCelInit()
{
	const int softLightingOptions = MESH_OPTION_ENABLE_LIGHTING;// | MESH_OPTION_ENABLE_ENVMAP;
	const int celLightingOptions = 0;// MESH_OPTION_ENABLE_LIGHTING;
	MeshgenParams paramsCube = initMeshObjectParams(MESH_CUBE);
	MeshgenParams paramsColumnoid = initMeshObjectParams(MESH_SQUARE_COLUMNOID);

	const int texWidth = 64;
	const int texHeight = 64;

	Texture *cloudTex8 = initGenTexture(texWidth, texHeight, 8, NULL, 1, TEXGEN_CLOUDS, NULL);
	Texture *cloudTex16 = initGenTexture(texWidth, texHeight, 16, NULL, 1, TEXGEN_CLOUDS, NULL);

	softObj[0] = initMeshObject(MESH_CUBE, paramsCube, MESH_OPTION_RENDER_SOFT8 | softLightingOptions, cloudTex8);
	softObj[1] = initMeshObject(MESH_SQUARE_COLUMNOID, paramsColumnoid, MESH_OPTION_RENDER_SOFT8 | softLightingOptions, cloudTex8);
	hardObj[0] = initMeshObject(MESH_CUBE, paramsCube, MESH_OPTIONS_DEFAULT | celLightingOptions, cloudTex16);
	hardObj[1] = initMeshObject(MESH_SQUARE_COLUMNOID, paramsColumnoid, MESH_OPTIONS_DEFAULT | celLightingOptions, cloudTex16);

	setRenderSoftMethod(RENDER_SOFT_METHOD_GOURAUD);

	camera = createCamera();
}

static void scriptRenderObj(int posX, int posY, int posZ, int t, Object3D *obj)
{
	if (obj == NULL) return;

	setObject3Dpos(obj, posX, posY, zoom + -posZ);
	if (autoRot) {
		setObject3Drot(obj, t<<1, t>>1, t);
	} else {
		setObject3Drot(obj, rotX, rotY, rotZ);
	}
	renderObject3D(obj, camera, NULL, 0);
}

void effectMeshGouraudCelRun()
{
	const int t = getTicks() >> 5;

	inputScript();

	if (renderTestIndex & RENDER_TEST_GOURAUD) {
		scriptRenderObj(0, 0, 256, t, softObj[selectedObj]);
	}
	if (renderTestIndex & RENDER_TEST_TEXTURE) {
		scriptRenderObj(0, 0, 256, t, hardObj[selectedObj]);
	}
}
