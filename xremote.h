/**
 * Xremote - core configuration and include file
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XREMOTE_H_INCLUDE_
#define _XREMOTE_H_INCLUDE_


// base includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <list>
#include <string>
#include <vector>

using namespace std;

// X11 includes
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XTest.h>


// version definition
#define XREMOTE_VERSION			"v0.5.1"

// default network parameters
#define XREMOTE_BIND_HOST		"0.0.0.0"		// any
#define XREMOTE_CLIENT_PORT		9875				// client port
#define XREMOTE_SERVER_PORT		9876				// server port

#define XREMOTE_READ_TIMEOUT		1000				// read timeout (us)
#define XREMOTE_WRITE_TIMEOUT	100				// write timeout (us)
#define XREMOTE_BEGIN_TIMEOUT	10000			// client "begin" read timeout (us)

#define XREMOTE_ALIVE_CHECK		5				// check alive every 5 s
#define XREMOTE_DEAD_CHECK		7				// is dead after 7 s


// lock utility
class XRLOCK {
private:
	pthread_mutex_t mutex;


public:
	XRLOCK() {
		pthread_mutexattr_t attr;

		if (pthread_mutexattr_init(&attr) != 0) {
			printf("pthread_mutexattr_init() error !!\n");
		}
		if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
			printf("pthread_mutexattr_settype() error !!\n");
		}
		if (pthread_mutex_init(&this->mutex, &attr) != 0) {
			printf("pthread_mutex_init() error !!\n");
		}
	}
	virtual ~XRLOCK() {
		if (pthread_mutex_destroy(&this->mutex) != 0) {
			printf("pthread_mutex_destroy() error !!\n");
		}
	}


	bool tryLock() {
		return (pthread_mutex_trylock(&this->mutex) == 0);
	}

	void lock() {
//		printf("mutex lock...\n");
		if (pthread_mutex_lock(&this->mutex) != 0) {
			printf("pthread_mutex_lock() error !!\n");
		}
//		printf(" -> locked.\n");
	}

	void unlock() {
//		printf("mutex unlock...\n");
		if (pthread_mutex_unlock(&this->mutex) != 0) {
			printf("pthread_mutex_unlock() error !!\n");
		}
//		printf(" -> unlocked.\n");
	}
};

// lock utility
class XRLOCKER {
private:
	XRLOCK *plock;
	int		lockCount;


public:
	XRLOCKER(XRLOCK *plock) : plock(plock), lockCount(0) {
		this->lock();
	}
	virtual ~XRLOCKER() {
		while (this->lockCount > 0) {
			this->unlock();
		}
	}


	bool tryLock() {
		if (this->plock->tryLock()) {
			this->lockCount++;
			return true;
		}
		return false;
	}
	void lock() {
		this->plock->lock();
		this->lockCount++;
	}
	void unlock() {
		if (this->lockCount > 0) {
			this->plock->unlock();
			this->lockCount--;
		}
	}
	void usleep(long time) {
		int oldCount = this->lockCount;

		if (oldCount > 0) {
			while (this->lockCount > 0) {
				this->unlock();
			}
		}

		struct timespec ts = {
			0,
			time * 1000
		};
		nanosleep(&ts, NULL);

		for (int i = 0; i < oldCount; i++) {
			this->lock();
		}
	}
};


// Local includes
//#include "xrevents.h"
//#include "xlib/xrscreen.h"
//#include "xlib/xrdisplay.h"
//#include "xlib/xrwindow.h"
//#include "net/xrnet.h"

#include "net/xrnetbase.h"
#include "net/xrnetudp.h"
#include "net/xrnetcrypt.h"
#include "net/xrnetchecksum.h"
#include "net/xrnetstream.h"
//#include "client/xrclient.h"
//#include "server/xrservercon.h"
//#include "server/xrserver.h"


#endif //_XREMOTE_H_INCLUDE_
