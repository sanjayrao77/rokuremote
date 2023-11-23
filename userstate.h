
#define MAIN_TYPE_MENU	1
#define KEYBOARD_TYPE_MENU	2
#define POWER_TYPE_MENU	3
#define REBOOT_TYPE_MENU	4
#define SETTINGS_TYPE_MENU 5
#define UINTENTRY_TYPE_MENU 6
#define BOOLENTRY_MENUTYPE 7
// #define AUTOMUTE_MENUTYPE 8 // works but unused

// #define MUTESTEP_FIELDINDEX_MENU	1
// #define DEF_AUTOMUTE_FIELDINDEX_MENU	2
// #define SET_AUTOMUTE_FIELDINDEX_MENU	3
// #define TIEMUTESTEP_FIELDINDEX_MENU	4
#define FULLVOLUME_FIELDINDEX_MENU	5
#define UP_VOLUMEDELAY_FIELDINDEX_MENU	6
#define DOWN_VOLUMEDELAY_FIELDINDEX_MENU	7
#define QUITPOWER_FIELDINDEX_MENU	8

struct userstate {
	struct {
		int isnomute; // if we don't have VolumeMute, we can similuate mute
		int isquitonpoweroff;
		unsigned int minvolume; // don't automute below this level
		uint32_t multicastipv4;
	} options;
	struct {
		unsigned int up_volumestep; // in microseconds
		unsigned int down_volumestep; // ditto
	} delays;
	struct {
		int isjoystick;
		int devicetype;
	} joystick;
#if 0
	unsigned int def_automute; // initial seconds for automute
#endif
	struct {
#define NONE_PRINTSTATE_TERMINAL 0
#define AUTOMUTE_PRINTSTATE_TERMINAL	1
#define AUTOMUTE2_PRINTSTATE_TERMINAL	2
#define NEWVOLUME_PRINTSTATE_TERMINAL	3
		int printstate;
		int isverbose;
		int issilent;
	} terminal;
	struct {
		int type;
		int lastprinted;
#define NONE_JSMODE_MENUS	0
#define VIEW_JSMODE_MENUS	1
#define NAV_JSMODE_MENUS	2
		int jsmode; // mode for joystick
		int (*handlebutton)(struct userstate *, struct discover *, int code, struct joystick *);
		int (*repeat_handlebutton)(struct userstate *, struct discover *, int);
		union {
			struct {
				int fieldindex;
				unsigned int value;
				int isedited;
			} uintentry;
			struct {
				int fieldindex;
				int value; // 0,1
				int isedited;
			} boolentry;
		};
	} menus;
	struct automute automute;
	struct {
		int _isdirty;
		unsigned int full;
		unsigned int current;
		unsigned int target;
		char *finalkeypress;
		int errorfuse;
	} volume;
};

struct mainoptions {
	int isquit;
	int stdinfd;
	int isanyjoystick;
	struct {
		int isscan;
		char *devicefile;
		char *mapping;
		unsigned int vendor,product;
		char *modelname;
		int isauto;
	} joystick;
	uint32_t ipv4;
	unsigned short port;
	char *sn;
	int discovertimeout;
	int isprecmd;
	int isinteractive;
	int isbackground;
};

void clear_mainoptions(struct mainoptions *o);
void clear_userstate(struct userstate *g);
void set_printstate(struct userstate *uo, int newstate);
void reset_printstate(struct userstate *uo);
void setdirty_volume(struct userstate *u);
