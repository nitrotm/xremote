/**
 * Xremote - entry-point
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "xrscreen.h"
#include "xrdisplay.h"
#include "xrwindow.h"
#include "xrnet.h"
#include "xrclient.h"
#include "xrserver.h"


XRCLIENT *client = NULL;
XRSERVER *server = NULL;


void printUsage() {
	printf("usage: xremote [-d] [-p] --client serverhost [serverport]\n");
	printf("       xremote [-d] [-p] --server listenhost [listenport]\n");
}

void shutdown(int sig) {
	if (client != NULL) {
		client->abort();
		client = NULL;
	}
	if (server != NULL) {
		server->abort();
		server = NULL;
	}
}


int main(int argc, char *argv[]) {
	int argi = 1;

	printf("xremote - v%s - by nitro.tm@gmail.com\n", XREMOTE_VERSION);

	// set random seed
	srand(time(NULL));

	// get display name
	char *displayName = getenv("DISPLAY");

	if (displayName == NULL) {
		fprintf(stderr, "DISPLAY is not set\n");
		return 1;
	}

	// check command-line validity
	if (argc - argi < 2) {
		printUsage();
		return 1;
	}

	// enable debugging
	bool debug = false;

	if (strcmp(argv[argi], "-d") == 0) {
		debug = true;
		argi++;
	}

	// check command-line validity
	if (argc - argi < 2) {
		printUsage();
		return 1;
	}

	// read encryption key from stdin
	string password;

	if (strcmp(argv[argi], "-p") == 0) {
		int c;

		while (!feof(stdin)) {
			c = fgetc(stdin);
			if (c == '\r' || c == '\n') {
				break;
			}
			password += (char)c;
		}
		argi++;
	}

	// check command-line validity
	if (argc - argi < 2) {
		printUsage();
		return 1;
	}

	// register quit/term signals
//	signal(SIGHUP, shutdown);
	signal(SIGQUIT, shutdown);
	signal(SIGTERM, shutdown);
	signal(SIGINT, shutdown);
//	signal(SIGABRT, shutdown);

	// prepare filter
	XRNETCRYPTFILTER cryptFilter(password);
	XRNETFILTER *filter = NULL;

	if (password.length() > 0) {
		filter = &cryptFilter;
	}

	// spawn client or server
	if (strcmp(argv[argi], "--client") == 0) {
		// check command-line validity
		if (argc > (argi + 3)) {
			printUsage();
			return 1;
		}

		// parse command-line
		string remotehost = argv[argi + 1];
		in_port_t remoteport = XREMOTE_SERVER_PORT;

		if (argc == (argi + 3)) {
			remoteport = (in_port_t)atoi(argv[argi + 2]);
		}

		// setup xremote client
		XRCLIENT *net = new XRCLIENT(XREMOTE_PACKET_SIZE, filter, XREMOTE_BIND_HOST, XREMOTE_CLIENT_PORT, remotehost, remoteport, displayName);

		client = net;
		if (!net->main(debug)) {
			delete net;
			printf("xremote is closed with an error.\n");
			return 2;
		}
		delete net;
	} else if (strcmp(argv[argi], "--server") == 0) {
		// check command-line validity
		if (argc > (argi + 3)) {
			printUsage();
			return 1;
		}

		// parse command-line
		string localhost = argv[argi + 1];
		in_port_t localport = XREMOTE_SERVER_PORT;

		if (argc == (argi + 3)) {
			localport = (in_port_t)atoi(argv[argi + 2]);
		}

		// setup xremote server
		XRSERVER *net = new XRSERVER(XREMOTE_PACKET_SIZE, filter, localhost, localport, displayName);

		server = net;
		if (!net->main(debug)) {
			delete net;
			printf("xremote is closed with an error.\n");
			return 2;
		}
		delete net;
	} else {
		printUsage();
		return 1;
	}
	return 0;
}
