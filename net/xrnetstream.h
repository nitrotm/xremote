/**
 * Xremote - user packet streamer
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETSTREAM_H_INCLUDE_
#define _XRNETSTREAM_H_INCLUDE_


/**
 * XRemote network data reader
 *
 */
class XRNETDATAREADER {
private:
public:
	XRNETDATAREADER();
	virtual ~XRNETDATAREADER();


	virtual bool onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) = 0;
};
typedef XRNETDATAREADER *PXRNETDATAREADER;


/**
 * XRemote network data writer
 *
 */
class XRNETDATAWRITER {
private:
	PXRNETBASE    net;
	unsigned char uid;


public:
	XRNETDATAWRITER(PXRNETBASE net);
	virtual ~XRNETDATAWRITER();


	bool send(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer);
};
typedef XRNETDATAWRITER *PXRNETDATAWRITER;
                                           

/**
 * XRemote network packet stream listener
 *
 */
class XRNETSTREAMLISTENER : public XRNETLISTENER {
private:
	PXRNETDATAREADER		reader;
	list<XRNETPACKET>	packets;


public:
	XRNETSTREAMLISTENER(PXRNETDATAREADER reader);
	virtual ~XRNETSTREAMLISTENER();

	virtual int getHeaderSize() const;
	virtual bool onReceivePacket(const XRNETPACKET &packet);
	virtual bool onSendPacket(XRNETPACKET &packet);

};


#endif //_XRNETSTREAM_H_INCLUDE_
