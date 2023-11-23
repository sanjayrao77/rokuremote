
extern char *reboot_global[];
extern char *reboot2_global[];
extern char *left40_global[];
extern char *right40_global[];

int sendkeypress_action(int *issent_out, struct userstate *userstate, struct discover *d, char *keyname);
int sendmute_action(int *isnotsent_out, struct userstate *userstate, struct discover *d, int isup);
int sendkeypresses_action(struct userstate *userstate, struct discover *d, char **keypresses);
void volumeup_action(struct userstate *userstate);
void volumedown_action(struct userstate *userstate);
int querydeviceinfo_action(FILE *fout, uint32_t ipv4, unsigned short port, char *filter);
int changemute_action(struct userstate *userstate, struct discover *discover, int mutestep, char *finalkeypress);
