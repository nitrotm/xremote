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
class XRCLIENT : public XRNETUDP, public XRWINDOW {
private:
	bool acquire(int y = 0, int flags = XRNOTIFY_NONE);
	bool release(int y = 0, int flags = XRNOTIFY_NONE);


protected:
	bool   debug;
	bool   allows;
	time_t alive;
	time_t lastCheck;

	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETEVENT *event);
	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETNOTIFYEVENT *event);
	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETPTREVENT *event);
	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETKBDEVENT *event);
//	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETBUFFEREVENT *event, const string &buffer);
	virtual bool onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer);

	virtual bool processXEvent(const XEvent &xev);


public:
	XRCLIENT(int packetSize, XRNETFILTER *first, const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort, const string &displayName);
	virtual ~XRCLIENT();


	virtual bool main();
	virtual void abort();
};


#endif //_XRCLIENT_H_INCLUDE__
