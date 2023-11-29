
struct automute {
	int isactive;
	time_t start,stop;
	int muteseconds; // seconds taken to send mute command
	int adjust_muteseconds; // seconds for muting that are from sleeping difference
//	int isinentry;
};

void reset_automute(struct automute *a, unsigned int decdelay, unsigned int incdelay, unsigned int full, unsigned int seconds);
int check_automute(int *secleft_out, struct automute *a, int inputfd, int optfd);
#define istimedout_automute(a) ((a)->stop <= time(NULL))
#define secleft_automute(a) ((a)->stop-time(NULL))
