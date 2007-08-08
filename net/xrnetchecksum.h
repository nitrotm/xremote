/**
 * Xremote - raw packet checksum
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETCHECKSUM_H_INCLUDE_
#define _XRNETCHECKSUM_H_INCLUDE_


/**
 * XRemote network packet checksum listener
 *
 */
class XRNETCHECKSUMLISTENER : public XRNETLISTENER {
protected:
	bool	 enabled;


public:
	XRNETCHECKSUMLISTENER(const bool enabled);
	virtual ~XRNETCHECKSUMLISTENER();

	virtual int getHeaderSize() const;
	virtual bool onReceivePacket(const XRNETPACKET &packet);
	virtual bool onSendPacket(XRNETPACKET &packet);
};


#endif //_XRNETCHECKSUM_H_INCLUDE_
