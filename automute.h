
struct automute {
	int isactive;
	int isprinted;
	time_t start,stop;
	int muteseconds; // seconds taken to send mute command
	int isinentry;
};

void reset_automute(struct automute *a, unsigned int seconds);
int check_automute(int *secleft_out, struct automute *a, int inputfd, int optfd);
#define istimedout_automute(a) ((a)->stop <= time(NULL))
