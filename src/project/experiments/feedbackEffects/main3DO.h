#ifndef MAIN_3DO_H
#define MAIN_3DO_H

void effectFeedbackCubeInit(void);
void effectFeedbackCubeRun(void);

void effectDotcubeInit(void);
void effectDotcubeRun(void);

void effectMosaikInit(void);
void effectMosaikRun(void);

void effectSlimecubeInit(void);
void effectSlimecubeRun(void);


enum { EFFECT_FEEDBACK_CUBE, EFFECT_FEEDBACK_DOTCUBE, EFFECT_FEEDBACK_MOSAIK, EFFECT_FEEDBACK_SLIMECUBE, EFFECTS_NUM };

void(*effectInitFunc[EFFECTS_NUM])() = { effectFeedbackCubeInit, effectDotcubeInit, effectMosaikInit, effectSlimecubeInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectFeedbackCubeRun, effectDotcubeRun, effectMosaikRun, effectSlimecubeRun };

char *effectName[EFFECTS_NUM] = { "feedback cube", "dot cube", "mosaik effect", "slime cube" };

#ifndef PROJECT_3DO
	int main3DO();
#endif

#endif
