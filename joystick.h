
struct joystick {
	int fd;
	int inarow;
	int lastcode;
	uint32_t bitmask;
	struct {
		int code;
		int ischecking;
		int isshortdelay;
	} repeat;
	struct {
		int (*func)(int *, struct joystick *);
		char *name;
	} mapping;
// Full has all the buttons w/o modes
#define FULL_TYPE_DEVICE_JOYSTICK	1
// snes has a dpad, abxy and two modes to switch actions
#define SNES_TYPE_DEVICE_JOYSTICK	2
	struct {
#define MAX_FILENAME_DEVICE_JOYSTICK 127
		char filename[MAX_FILENAME_DEVICE_JOYSTICK+1];
		int type;
	} device;
	struct {
#define NONE_MODE_DETECT_JOYSTICK 0
#define MANUAL_MODE_DETECT_JOYSTICK 1
#define AUTO_MODE_DETECT_JOYSTICK 2
#define MODELNAME_MODE_DETECT_JOYSTICK 3
		int mode;
		char *modelname;
	} detect;
};

#define NONE_JOYSTICK	0

// this is original, for the SNES pad, where start and select change modes
#define LEFT_DPAD_JOYSTICK	1
#define UP_DPAD_JOYSTICK	2
#define RIGHT_DPAD_JOYSTICK	3
#define DOWN_DPAD_JOYSTICK	4
#define LEFT_SHOULDER_JOYSTICK	5
#define RIGHT_SHOULDER_JOYSTICK	6
#define A_JOYSTICK	7
#define B_JOYSTICK	8
#define X_JOYSTICK	9
#define Y_JOYSTICK	10
#define START_JOYSTICK	11
#define SELECT_JOYSTICK	12

// this is for the 8bitdo_lite, where we don't need modes
// n: navigation mode, v: viewing mode
// x: no mode
#define xHOME_JOYSTICK	13
#define xSTART_JOYSTICK	14
#define xSELECT_JOYSTICK	15
#define nLEFT_DPAD_JOYSTICK	16
#define nUP_DPAD_JOYSTICK	17
#define nRIGHT_DPAD_JOYSTICK	18
#define nDOWN_DPAD_JOYSTICK	19
#define nA_JOYSTICK	20
#define nB_JOYSTICK	21
#define nX_JOYSTICK	22
#define nY_JOYSTICK	23
#define nLEFT_SHOULDER_JOYSTICK	24
#define nRIGHT_SHOULDER_JOYSTICK	25
#define vLEFT_DPAD_JOYSTICK	26
#define vUP_DPAD_JOYSTICK	27
#define vRIGHT_DPAD_JOYSTICK	28
#define vDOWN_DPAD_JOYSTICK	29
#define vA_JOYSTICK	30
#define vB_JOYSTICK	31
#define vX_JOYSTICK	32
#define vY_JOYSTICK	33
#define vLEFT_SHOULDER_JOYSTICK	34
#define vRIGHT_SHOULDER_JOYSTICK	35

#define BIT_LEFT_DPAD_JOYSTICK	(1<<1)
#define BIT_UP_DPAD_JOYSTICK	(1<<2)
#define BIT_RIGHT_DPAD_JOYSTICK	(1<<3)
#define BIT_DOWN_DPAD_JOYSTICK	(1<<4)
#define BIT_LEFT_SHOULDER_JOYSTICK	(1<<5)
#define BIT_RIGHT_SHOULDER_JOYSTICK	(1<<6)
#define BIT_A_JOYSTICK	(1<<7)
#define BIT_B_JOYSTICK	(1<<8)
#define BIT_X_JOYSTICK	(1<<9)
#define BIT_Y_JOYSTICK	(1<<10)
#define BIT_START_JOYSTICK	(1<<11)
#define BIT_SELECT_JOYSTICK	(1<<12)
#define BIT_HOME_JOYSTICK	(1<<13)

#define voidinit_joystick(a) do { } while (0)
void clear_joystick(struct joystick *js);
void deinit_joystick(struct joystick *js);
int getbutton_joystick(int *code_out, struct joystick *js);
int scan_joystick(FILE *fout);
int finddevice_joystick(char *dest, unsigned int destlen, char *modelname);
int findanydevice_joystick(char **mapping_out, char *dest, unsigned int destlen);
int manual_set_joystick(struct joystick *js, char *devname, char *mappingname);
int modelname_set_joystick(struct joystick *js, char *modelname, char *mapping);
int auto_set_joystick(struct joystick *js, char *modelname);
int detect_joystick(struct joystick *js);
int printstatus_joystick(struct joystick *js, FILE *fout);
