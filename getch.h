
struct getch {
#define MAX_ESCBUFFER_GETCH	16
	unsigned char escbuffer[MAX_ESCBUFFER_GETCH];
	unsigned char nextchar;
	int escbufferlen;
	int isnextchar;
	int iscompleting; // is finishing escape sequence
	struct {
		struct termios orig_termios_fd;
		int fd;
	} nofree;
};
H_CLEARFUNC(getch);
#define KEY_DOWN	0402
#define KEY_UP	0403
#define KEY_LEFT	0404
#define KEY_RIGHT	0405

int init_getch(struct getch *g, int fd);
void deinit_getch(struct getch *g);
int getch_getch(struct getch *g);
