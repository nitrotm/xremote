/**
 * Xremote - entry-point
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "client/xrclient.h"
#include "server/xrservercon.h"
#include "server/xrserver.h"
#include "server/xrservertest.h"


XRNET *xr = NULL;


void printUsage() {
	printf("xremote - %s - by nitro.tm@gmail.com\n", XREMOTE_VERSION);
	printf("usage: xremote [-p] --client serverhost [serverport]\n");
	printf("       xremote [-p] --server listenhost [listenport]\n");
}

void shutdown(int sig) {
	if (xr != NULL) {
		xr->shutdown();
		xr = NULL;
	}
}


int main(int argc, char *argv[]) {
	XRNET *localxr = NULL;

	// get display name
	char *displayName = getenv("DISPLAY");

	if (displayName == NULL) {
		fprintf(stderr, "DISPLAY is not set\n");
		return 1;
	}

	// check command-line validity
	if (argc < 3) {
		printUsage();
		return 1;
	}

	// set random seed
	srand(time(NULL));

	// read encryption key from stdin
	string password;
	int argi = 1;

	if (strcmp(argv[argi], "-p") == 0) {
		int c;

		while (!feof(stdin)) {
			c = fgetc(stdin);
			if (c == '\r' || c == '\n') {
				break;
			}
			password += (char)c;
		}
//		printf("password=%s\n", password.c_str());

		// check command-line validity
		argi++;
		if (argc < 4) {
			printUsage();
			return 1;
		}
	}

	// register quit/term signals
//	signal(SIGHUP, shutdown);
	signal(SIGQUIT, shutdown);
	signal(SIGTERM, shutdown);
//	signal(SIGINT, shutdown);
//	signal(SIGABRT, shutdown);

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
		localxr = xr = new XRCLIENT(XREMOTE_BIND_HOST, XREMOTE_CLIENT_PORT, remotehost, remoteport, displayName);
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
		localxr = xr = new XRSERVERTEST(localhost, localport, displayName);
	} else {
		printUsage();
		return 1;
	}
	localxr->setEncryptionKey(password);

	printf("xremote - %s - by nitro.tm@gmail.com\n", XREMOTE_VERSION);
	if (localxr->main()) {
		delete localxr;
		printf("xremote is closed.\n");
		return 0;
	}
	delete localxr;
	printf("xremote is closed with an error.\n");
	return -1;
}
