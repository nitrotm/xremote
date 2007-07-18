/**
 * Xremote - server-side
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRSERVER_H_INCLUDE_
#define _XRSERVER_H_INCLUDE_


/**
 * XRemote server
 *
 */
class XRSERVER : public XRNET {
private:
	bool						allows;
	vector<XRSERVERCON>		connections;
	vector<unsigned int>		buttons;
	vector<unsigned int>		keys;
	int						bufferCount;

	bool setConnectionAlive(in_addr_t address, in_port_t port);
	void registerConnection(in_addr_t address, in_port_t port);
	void unregisterConnection(in_addr_t address, in_port_t port);
	bool hasConnection(in_addr_t address, in_port_t port);

	bool begin(int y, int flags);
	bool end(int y, int flags);


protected:
	virtual bool onReceive(PXREVENT event);
	virtual bool onSend(PXREVENT event);

	virtual bool processEvent(PXREVENT event);
	virtual bool processPtrEvent(PXRPTREVENT event) = 0;
	virtual bool processButtonEvent(PXRPTREVENT event, bool down) = 0;
	virtual bool processKbdEvent(PXRKBDEVENT event, bool down) = 0;

	bool sendEnd(int y, int flags);

	virtual bool processXEvent(const XEvent &xev);


public:
	XRSERVER(const string &localHost, in_port_t localPort, const string &displayName);
	XRSERVER(const XRSERVER &ref);
	virtual ~XRSERVER();


	virtual bool main();
	virtual void shutdown();

	virtual bool isSupported() const = 0;
};


#endif //_XRSERVER_H_INCLUDE__
