/**
 * Xremote - client/server udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETCRYPT_H_INCLUDE_
#define _XRNETCRYPT_H_INCLUDE_


/**
 * AES-256 encrypted network packet structure (XREMOTE_PACKET_SIZE bytes of data)
 *
 */
class XRNETAESPACKET : public XRNETRAWPACKET {
protected:
	bool				encrypted;
	unsigned char	key[32];


public:
	XRNETAESPACKET(const XRNETRAWPACKET &packet, bool encrypt, unsigned char key[32]);
	virtual ~XRNETAESPACKET();
};
typedef XRNETAESPACKET *PXRNETAESPACKET;


/**
 * XRemote network packet empty listener
 *
 */
class XRNETCRYPTLISTENER : public XRNETLISTENER {
protected:
	unsigned char	key[32];
	bool				encrypt;


public:
	XRNETCRYPTLISTENER(XRNETLISTENER *listener, const string &password);
	virtual ~XRNETCRYPTLISTENER();


	virtual bool onReceivePacket(PXRNETRAWPACKET packet);
	virtual PXRNETRAWPACKET onSendPacket(PXRNETRAWPACKET packet);
};
typedef XRNETCRYPTLISTENER *PXRNETCRYPTLISTENER;


#endif //_XRNETCRYPT_H_INCLUDE_
