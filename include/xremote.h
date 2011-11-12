/**
 * Xremote - core configuration and include file
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free to use for any purpose ;)
 */
#ifndef _XREMOTE_H_INCLUDE_
#define _XREMOTE_H_INCLUDE_


// version definition
#define XREMOTE_VERSION      "0.6.0"

// default network parameters
#define XREMOTE_BIND_HOST     "0.0.0.0" // any
#define XREMOTE_CLIENT_PORT   9875      // client port
#define XREMOTE_SERVER_PORT   9876      // server port

#define XREMOTE_PACKET_SIZE   32        // packet size (bytes)
#define XREMOTE_READ_TIMEOUT  1000      // read timeout (us)
#define XREMOTE_WRITE_TIMEOUT 1000      // write timeout (us)
#define XREMOTE_BEGIN_TIMEOUT 10000     // client "begin" read timeout (us)

#define XREMOTE_CHECK_FREQ    1         // check status every 1 [s]
#define XREMOTE_ALIVE_CHECK   5         // check alive every 5 [s]
#define XREMOTE_DEAD_CHECK    15        // is dead after 15 [s]


// system includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


// std includes
#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>

using namespace std;


// X11 includes
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XTest.h>


// event types
#define XREVENT_NONE     0x00
#define XREVENT_ACQUIRE  0x01
#define XREVENT_RELEASE  0x02
#define XREVENT_ALIVE    0x03
#define XREVENT_PTR_MOVE 0x10
#define XREVENT_PTR_DOWN 0x11
#define XREVENT_PTR_UP   0x12
#define XREVENT_KBD_DOWN 0x20
#define XREVENT_KBD_UP   0x21
#define XREVENT_BUFFER   0x30

// notify flags
#define XRNOTIFY_NONE  0x00
#define XRNOTIFY_LEFT  0x01
#define XRNOTIFY_RIGHT 0x02
#define XRNOTIFY_REPLY 0x04

// Network data code
#define XRBUFFER_NONE            0x00
#define XRBUFFER_PRIMARYBUFFER   0x01
#define XRBUFFER_SECONDARYBUFFER 0x02
#define XRBUFFER_CLIPBOARDBUFFER 0x03


/**
 * Network event message structure (3+5 bytes)
 *
 */
typedef struct {
	unsigned short port;
	unsigned char  type;
	char           r[7];
} XRNETEVENT;

/**
 * Network event message structure (6+2 bytes)
 *
 */
typedef struct {
	unsigned short port;
	unsigned char  type;
	unsigned char  flags;
	unsigned short y;
	char           r[2];
} XRNETNOTIFYEVENT;

/**
 * Network event message structure (8 bytes)
 *
 */
typedef struct {
	unsigned short port;
	unsigned char  type;
	unsigned char  button;
	short          x;
	short          y;
} XRNETPTREVENT;

/**
 * Network event message structure (5+1 bytes)
 *
 */
typedef struct {
	unsigned short port;
	unsigned char  type;
	unsigned int   keycode;
	char           r[1];
} XRNETKBDEVENT;

/**
 * Network event message structure (8+n bytes)
 *
 */
typedef struct {
	unsigned short port;
	unsigned char  type;
	unsigned char  code;
	unsigned int   size;
} XRNETBUFFEREVENT;


/**
 * Lock wrapper
 *
 */
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
		if (pthread_mutex_lock(&this->mutex) != 0) {
			printf("pthread_mutex_lock() error !!\n");
		}
	}

	void unlock() {
		if (pthread_mutex_unlock(&this->mutex) != 0) {
			printf("pthread_mutex_unlock() error !!\n");
		}
	}
};

/**
 * Automatic lock wrapper
 *
 */
class XRLOCKER {
private:
	XRLOCK *plock;
	int    lockCount;


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


#endif //_XREMOTE_H_INCLUDE_
