#include "main3DO.h"
#include "types.h"


static ScreenContext screen;

static Item bitmapItems[SCREEN_PAGES];
static Bitmap *bitmaps[SCREEN_PAGES];

static Item VRAMIOReq;
static IOInfo ioInfo;

static int visibleScreenPage = 0;

static ubyte *backgroundBufferPtr = NULL;

void loadData()
{
	backgroundBufferPtr = LoadImage("data/background.img", NULL, (VdlChunk **)NULL, &screen);
}

void initSPORTwriteValue(unsigned value)
{
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = FLASHWRITE_CMD;
	ioInfo.ioi_CmdOptions = 0xffffffff;
	ioInfo.ioi_Offset = value; // background colour
	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;
}

void initSPORTcopyImage(ubyte *srcImage)
{
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = SPORTCMD_COPY;
	ioInfo.ioi_Offset = 0xffffffff; // mask
	ioInfo.ioi_Send.iob_Buffer = srcImage;
	ioInfo.ioi_Send.iob_Len = SCREEN_SIZE_IN_BYTES;
	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;
}

void initSPORT()
{
	VRAMIOReq = CreateVRAMIOReq(); // Obtain an IOReq for all SPORT operations
	initSPORTwriteValue(0);
}

void initGraphics()
{
	int i;

	CreateBasicDisplay(&screen, DI_TYPE_DEFAULT, SCREEN_PAGES);

	for(i=0; i<SCREEN_PAGES; i++)
	{
		bitmapItems[i] = screen.sc_BitmapItems[i];
		bitmaps[i] = screen.sc_Bitmaps[i];
	}

	loadData();

	initSPORT();
}

void initSystem()
{
    OpenGraphicsFolio();
    InitEventUtility(1,0,LC_Observer);
}


void setBackgroundColor(short color)
{
	ioInfo.ioi_Offset = (color << 16) | color;
}

void display()
{
    DisplayScreen(screen.sc_Screens[visibleScreenPage], 0 );

    visibleScreenPage = (visibleScreenPage + 1) % SCREEN_PAGES;

	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	DoIO(VRAMIOReq, &ioInfo);
}

int getJoystickState()
{
	ControlPadEventData cpaddata;
	cpaddata.cped_ButtonBits = 0;
	GetControlPad(1,0,&cpaddata);

	return cpaddata.cped_ButtonBits;
}

void inputScript()
{
	int joybits = getJoystickState();

	if(joybits & ControlA) {
		initSPORTwriteValue(0);
		setBackgroundColor(MakeRGB15(31,23,7));
	}

	if(joybits & ControlB) {
		initSPORTwriteValue(rand() * rand());
	}

	if(joybits & ControlC) {
		initSPORTcopyImage(backgroundBufferPtr);
	}
}


int main3DO()
{
	static bool hasInit = false;
	if (!hasInit) {
		initSystem();
		initGraphics();
		hasInit = true;
	}

	//while(true) {
		inputScript();
		display();
	//}
	return 0;
}
