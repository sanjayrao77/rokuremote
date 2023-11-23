
struct notify {
	int fd;
};

void clear_notify(struct notify *n);
void deinit_notify(struct notify *n);
int init_notify(struct notify *n, char *path);
int readrecord_notify(struct notify *notify);
