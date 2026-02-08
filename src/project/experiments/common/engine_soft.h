#ifndef ENGINE_SOFT_H
#define ENGINE_SOFT_H

#include "engine_main.h"
#include "engine_mesh.h"

#define COLOR_GRADIENTS_SHR 5
#define COLOR_ENVMAP_SHR COLOR_GRADIENTS_SHR
#define COLOR_GRADIENTS_SIZE (1 << COLOR_GRADIENTS_SHR)

#define SHADED_TEXTURES_SHR 4
#define SHADED_TEXTURES_NUM (1 << SHADED_TEXTURES_SHR)

#define GRADIENT_TO_SHADED_SHR (COLOR_GRADIENTS_SHR - SHADED_TEXTURES_SHR)

#define RENDER_SOFT_METHOD_WIREFRAME	0
#define RENDER_SOFT_METHOD_GOURAUD		(1 << 0)
#define RENDER_SOFT_METHOD_ENVMAP		(1 << 1)
#define RENDER_SOFT_METHOD_LAST			(1 << 2)

void initEngineSoft(bool usesSemisoftGouraud, bool needsSoftBuffer);
void renderTransformedMeshSoft(Mesh *ms, ScreenElement *elements);

void setRenderSoftMethod(int method);
void setRenderSoftPixc(uint32 pixc);

void initDefaultShadeMaps(void);
void initInvertedShadeMaps(void);
void updateRenderSoftShademap(uint8* shadeMapSrc);
uint8* getRenderSoftShademap(void);

void updateGouraudColorShades(int numShades, uint16* shades);
void setPmvSemisoftGouraud(int r, int g, int b);

#endif
