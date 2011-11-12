#include <pthread.h>

#include "xremote.h"
#include "xrnet.h"

#define MAX_ITERATIONS 1000
#define PAYLOAD_SIZE 1024 * 64
#define READ_TIMEOUT 1000 * 1000
#define PACKET_SIZE 256


class CLIENTSTREAM : public XRNETUDP {
protected:
	virtual bool onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) {
		int size = buffer[0] % 256 + PAYLOAD_SIZE;
		int error = 0;

		if (buffer.length() != size) {
			error++;
		}
		for (int j = 1; j < buffer.length(); j++) {
			if (buffer[j] != (unsigned char)j) {
				error++;
			}
		}
		if (error > 0) {
			printf("PACKET_RECV[%d]: length=%d, expectedLength=%d, errors=%d\n", meta.getLocalPort(), buffer.length(), size, error);
		} else {
			success++;
		}
		received++;
		return true;
	}


public:
	int success;
	int received;
	
	
	CLIENTSTREAM(int packetSize, XRNETFILTER *first, const string &localHost, const in_port_t localPort, const string &remoteHost, const in_port_t remotePort) : XRNETUDP(packetSize, first, localHost, localPort, remoteHost, remotePort), success(0), received(0) {
	}
	virtual ~CLIENTSTREAM() {
	}
};

class TESTCLIENT {
public:
	XRLOCK    lock;
	in_port_t localPort;
	in_port_t remotePort;
	in_addr_t addr;
	bool started;

	TESTCLIENT(in_port_t localPort, in_port_t remotePort) : localPort(localPort), remotePort(remotePort), started(false) {
		// resolve localhost
		struct hostent *local = gethostbyname("localhost");
	
		if (local != NULL) {
			memcpy(&this->addr, local->h_addr, sizeof(in_addr_t));
		} else {
			memset(&this->addr, 0, sizeof(in_addr_t));
		}
	}
	virtual ~TESTCLIENT() {
	}


	virtual bool running() {
		if (this->lock.tryLock()) {
			this->lock.unlock();
			return !this->started;
		}
		return true;
	}

	virtual void join() {
		XRLOCKER locker(&this->lock);
		while (!this->started) {
			locker.unlock();
			this->sleep(1);
			locker.lock();
		}
	}

	virtual void main() = 0;

	void sleep(int ms) {
		struct timespec ts;

		ts.tv_sec = 0;
		ts.tv_nsec = ms * 1000 * 1000;
		nanosleep(&ts, NULL);
	}
};

class TESTCLIENT1 : public TESTCLIENT {
public:
	int packetSize;

	TESTCLIENT1(in_port_t localPort, in_port_t remotePort, int packetSize) : TESTCLIENT(localPort, remotePort), packetSize(packetSize) {
	}
	virtual ~TESTCLIENT1() {
	}


	virtual void main() {
		XRLOCKER locker(&this->lock);

		this->started = true;

		// create client
		CLIENTSTREAM stream(this->packetSize, NULL, "localhost", this->localPort, "localhost", this->remotePort);

		if (!stream.createSocket()) {
			printf("ERR: cannot create socket\n");
			return;
		}

		// wait 5 [s]
		this->sleep(5000);

		// send/receive data
		for (int i = 0; i < MAX_ITERATIONS; i++) {
			// receive data
			if (!stream.receiveAll(READ_TIMEOUT)) {
				printf("ERR: failed to receive packet(s)\n");
			}

			// send data
			int size = i % 256 + PAYLOAD_SIZE;

			if (i % 2 != 0) {
				XRNETPACKETMETA meta(this->addr, this->localPort, this->addr, this->remotePort);
				XRNETBUFFER buffer(size);

				buffer[0] = (unsigned char)(i % 256);
				for (int j = 1; j < buffer.length(); j++) {
					buffer[j] = (unsigned char)j;
				}

//				printf("CLIENT[%d]: PACKET_SEND[%d]: length=%d\n", this->localPort, i, size);
				if (!stream.send(meta, buffer)) {
					printf("ERR: failed to send packet\n");
				}
			}
		}
		printf("CLIENT[%d]: success=%d, received=%d, total=%d\n", this->localPort, stream.success, stream.received, MAX_ITERATIONS / 2);
	}
};

class TESTCLIENT2 : public TESTCLIENT {
public:
	int packetSize;
	string password;

	TESTCLIENT2(in_port_t localPort, in_port_t remotePort, int packetSize, const string &password) : TESTCLIENT(localPort, remotePort), packetSize(packetSize), password(password) {
	}
	virtual ~TESTCLIENT2() {
	}


	virtual void main() {
		XRLOCKER locker(&this->lock);

		this->started = true;

		// create client
		XRNETCRYPTFILTER cryptFilter(password);
		XRNETCHECKSUMFILTER checksumFilter(&cryptFilter);
		CLIENTSTREAM stream(this->packetSize, &cryptFilter, "localhost", this->localPort, "localhost", this->remotePort);

		if (!stream.createSocket()) {
			printf("ERR: cannot create socket\n");
			return;
		}

		// wait 5 [s]
		this->sleep(5000);

		// send/receive data
		for (int i = 0; i < MAX_ITERATIONS; i++) {
			// receive data
			if (!stream.receiveAll(READ_TIMEOUT)) {
				printf("ERR: failed to receive packet(s)\n");
			}

			// send data
			int size = i % 256 + PAYLOAD_SIZE;

			if (i % 2 != 0) {
				XRNETPACKETMETA meta(this->addr, this->localPort, this->addr, this->remotePort);
				XRNETBUFFER buffer(size);

				buffer[0] = (unsigned char)(i % 256);
				for (int j = 1; j < buffer.length(); j++) {
					buffer[j] = (unsigned char)j;
				}

//				printf("CLIENT[%d]: PACKET_SEND[%d]: length=%d\n", this->localPort, i, size);
				if (!stream.send(meta, buffer)) {
					printf("ERR: failed to send packet\n");
				}
			}
		}
		printf("CLIENT[%d]: success=%d, received=%d, total=%d\n", this->localPort, stream.success, stream.received, MAX_ITERATIONS / 2);
	}
};


void clientThread(TESTCLIENT *client) {
	client->main();
}

void startClient(TESTCLIENT *client) {
	pthread_attr_t attr;
	pthread_t thread;

	::pthread_attr_init(&attr);
	::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (::pthread_create(&thread, &attr, (void*(*)(void*))clientThread, client)) {
		::pthread_attr_destroy(&attr);
		printf("ERR: failed to create thread\n");
		return;
	}
	::pthread_attr_destroy(&attr);

}


int main(int argc, char *argv[]) {
	// test raw transmission
	TESTCLIENT *client1 = new TESTCLIENT1(10000, 10001, PACKET_SIZE);
	TESTCLIENT *client2 = new TESTCLIENT1(10001, 10000, PACKET_SIZE);

	startClient(client1);
	startClient(client2);

	client1->join();
	delete client1;

	client2->join();
	delete client2;

	// test encrypted transmission
	TESTCLIENT *client3 = new TESTCLIENT2(10000, 10001, PACKET_SIZE, "abc");
	TESTCLIENT *client4 = new TESTCLIENT2(10001, 10000, PACKET_SIZE, "abc");

	startClient(client3);
	startClient(client4);

	client3->join();
	delete client3;

	client4->join();
	delete client4;
	return 0;
}
