
struct id_roku {
#define MAX_USN_ROKU	15
	char usn[MAX_USN_ROKU+1]; // presumably, unique serial number
};
int querydeviceinfo_discover(FILE *fout, uint32_t ipv4, unsigned short port, char *filter);
