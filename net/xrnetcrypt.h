/**
 * Xremote - raw packet aes encryption
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETCRYPT_H_INCLUDE_
#define _XRNETCRYPT_H_INCLUDE_


/**
 * XRemote network packet aes encryption listener
 *
 */
class XRNETCRYPTLISTENER : public XRNETLISTENER {
protected:
	unsigned char	key[32];
	bool				enabled;


public:
	XRNETCRYPTLISTENER(const string &password);
	virtual ~XRNETCRYPTLISTENER();

	virtual int getHeaderSize() const;
	virtual bool onReceivePacket(const XRNETPACKET &packet);
	virtual bool onSendPacket(XRNETPACKET &packet);
};


#endif //_XRNETCRYPT_H_INCLUDE_
