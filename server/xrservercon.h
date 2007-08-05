/**
 * Xremote - server-side connection wrapper
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRSERVERCON_H_INCLUDE__
#define _XRSERVERCON_H_INCLUDE__


/**
 * XRemote server connection
 *
 */
class XRSERVERCON {
protected:
	in_addr_t	remoteAddress;
	in_port_t	remotePort;
	time_t		alive;
	int			endY;
	int			endFlags;


public:
	XRSERVERCON();
	XRSERVERCON(in_addr_t remoteAddress, in_port_t remotePort);
	XRSERVERCON(const XRSERVERCON &ref);
	~XRSERVERCON();


	in_addr_t	getRemoteAddress() const;
	in_port_t	getRemotePort() const;
	string		getRemoteHost() const;
	time_t		getAlive() const;
	void			setAlive();

	void			setEnd(int y, int flags);
	int			getEndY() const;
	int			getEndFlags() const;
};


#endif //_XRSERVERCON_H_INCLUDE__
