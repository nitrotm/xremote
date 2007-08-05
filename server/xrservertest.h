/**
 * Xremote - server-side (Xtest extension)
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRSERVERTEST_H_INCLUDE_
#define _XRSERVERTEST_H_INCLUDE_


/**
 * XRemote server : XTest extension
 *
 */
class XRSERVERTEST : public XRSERVER {
protected:
	bool supported;


	virtual bool processPtrEvent(PXRPTREVENT event);
	virtual bool processButtonEvent(PXRPTREVENT event, bool down);
	virtual bool processKbdEvent(PXRKBDEVENT event, bool down);


public:
	XRSERVERTEST(const string &localHost, in_port_t localPort, const string &displayName);
	XRSERVERTEST(const XRSERVERTEST &ref);
	virtual ~XRSERVERTEST();


	virtual bool isSupported() const;
};


#endif //_XRSERVERTEST_H_INCLUDE_
