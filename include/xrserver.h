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
class XRSERVER : public XRNETUDP, public XRWINDOW {
private:
	bool acquire(int y = 0, int flags = XRNOTIFY_NONE);
	bool release(int y = 0, int flags = XRNOTIFY_NONE);


protected:
	class XRCLIENTREF {
	public:
		XRNETPACKETMETA meta;
		time_t          alive;
		time_t          lastCheck;
//		int             y;
//		int             flags;

		XRCLIENTREF() : alive(time(NULL)), lastCheck(time(NULL)) { //, y(0), flags(XRNOTIFY_NONE) {
		}
		XRCLIENTREF(const XRCLIENTREF &ref) : meta(ref.meta), alive(ref.alive), lastCheck(ref.lastCheck) { //, y(ref.y), flags(ref.flags) {
		}

		const XRCLIENTREF & operator =(const XRCLIENTREF &ref) {
			this->meta = ref.meta;
			this->alive = ref.alive;
			this->lastCheck = ref.lastCheck;
//			this->y = ref.y;
//			this->flags = ref.flags;
		}
	};

	bool                     allows;
	map<int, bool>           buttons;
	map<unsigned int, bool>  keys;
	map<string, XRCLIENTREF> clients;

	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETEVENT *event);
	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETNOTIFYEVENT *event);
	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETPTREVENT *event);
	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETKBDEVENT *event);
//	virtual bool sendEvent(const XRNETPACKETMETA &meta, XRNETBUFFEREVENT *event, const string &buffer);
	virtual bool onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer);

	virtual bool processXEvent(const XEvent &xev);
	virtual bool processPtrEvent(XRNETPTREVENT *event);
	virtual bool processButtonEvent(XRNETPTREVENT *event);
	virtual bool processKbdEvent(XRNETKBDEVENT *event);


public:
	XRSERVER(int packetSize, XRNETFILTER *first, const string &localHost, in_port_t localPort, const string &displayName);
	virtual ~XRSERVER();


	virtual bool main();
	virtual void abort();
};


#endif //_XRSERVER_H_INCLUDE__
