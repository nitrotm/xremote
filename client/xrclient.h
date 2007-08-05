/**
 * Xremote - client-side
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRCLIENT_H_INCLUDE_
#define _XRCLIENT_H_INCLUDE_


/**
 * XRemote client
 *
 */
class XRCLIENT : public XRNET {
private:
	bool		allows;
	time_t	alive;


	bool begin(int y, int flags);
	bool end(int y, int flags);


protected:
	virtual bool onReceive(PXREVENT event);
	virtual bool onSend(PXREVENT event);

	virtual bool processXEvent(const XEvent &xev);


public:
	XRCLIENT(const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort, const string &displayName);
	XRCLIENT(const XRCLIENT &ref);
	virtual ~XRCLIENT();


	virtual bool main();
	virtual void shutdown();
};


#endif //_XRCLIENT_H_INCLUDE__
