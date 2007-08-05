/**
 * Xremote - client/server udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETUDP_H_INCLUDE_
#define _XRNETUDP_H_INCLUDE_


/**
 * XRemote client/server network udp interface
 *
 */
class XRNETUDP : public XRNETBASE {
private:
	string		localHost;
	in_addr_t	localAddress;
	in_port_t	localPort;
	string		remoteHost;
	in_addr_t	remoteAddress;
	in_port_t	remotePort;
	int			s;


protected:
	virtual int receiveOne(PXRNETRAWPACKET *packet, long timeout);
	virtual int sendOne(PXRNETRAWPACKET packet, long timeout);


public:
	XRNETUDP(PXRNETLISTENER listener, const string &localHost, in_port_t localPort);
	XRNETUDP(PXRNETLISTENER listener, const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort);
	XRNETUDP(const XRNETUDP &ref);
	virtual ~XRNETUDP();


	virtual bool createSocket();
	virtual bool destroySocket();
};


#endif //_XRNETUDP_H_INCLUDE_
