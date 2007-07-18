/**
 * Xremote - event wrappers & network structures
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XREVENTS_H_INCLUDE_
#define _XREVENTS_H_INCLUDE_


// event types
#define XREVENT_NONE		0x00
#define XREVENT_PTR		0x01
#define XREVENT_KBD		0x02
#define XREVENT_BEGIN	0x04
#define XREVENT_END		0x08
#define XREVENT_ALIVE	0x10
#define XREVENT_DOWN		0x20
#define XREVENT_UP		0x40
#define XREVENT_BUFFER	0x80

// notify flags
#define XRNOTIFY_NONE	0x00
#define XRNOTIFY_LEFT	0x01
#define XRNOTIFY_RIGHT	0x02
#define XRNOTIFY_REPLY	0x04


/**
 * Network event message structure (8 bytes)
 *
 */
typedef struct {
	unsigned char	type;
	unsigned char	r1;
	unsigned short	localPort;
	unsigned int		r2;
} XRNETEVENT, *PXRNETEVENT;

/**
 * Event wrapper
 *
 */
class XREVENT {
protected:
	unsigned char type;
	in_port_t localPort;
	in_addr_t remoteAddress;
	in_port_t remotePort;


public:
	XREVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort);
	XREVENT(const XREVENT &ref);
	XREVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress);
	virtual ~XREVENT();


	in_port_t getLocalPort() const;
	in_addr_t getRemoteAddress() const;
	in_port_t getRemotePort() const;
	string getRemoteHost() const;
	unsigned char getType() const;

	virtual void convert(PXRNETEVENT nev) const;
};
typedef XREVENT *PXREVENT;



/**
 * Network event message structure (8 bytes)
 *
 */
typedef struct {
	unsigned char	type;
	unsigned char	button;
	unsigned short	localPort;
	short			x;
	short			y;
} XRNETPTREVENT, *PXRNETPTREVENT;

/**
 * Pointer event wrapper
 *
 */
class XRPTREVENT : public XREVENT {
protected:
	short x;
	short y;
	unsigned char button;


public:
	XRPTREVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int x, int y);
	XRPTREVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int x, int y, int button);
	XRPTREVENT(const XRPTREVENT &ref);
	XRPTREVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress);
	virtual ~XRPTREVENT();


	short getX() const;
	short getY() const;
	unsigned char getButton() const;

	virtual void convert(PXRNETEVENT nev) const;
};
typedef XRPTREVENT *PXRPTREVENT;



/**
 * Network event message structure (8 bytes)
 *
 */
typedef struct {
	unsigned char	type;
	unsigned char	r1;
	unsigned short	localPort;
	unsigned int		keycode;
} XRNETKBDEVENT, *PXRNETKBDEVENT;

/**
 * Keyboard event wrapper
 *
 */
class XRKBDEVENT : public XREVENT {
protected:
	unsigned int keycode;


public:
	XRKBDEVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int keycode);
	XRKBDEVENT(const XRKBDEVENT &ref);
	XRKBDEVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress);
	virtual ~XRKBDEVENT();


	unsigned int getKeyCode() const;

	virtual void convert(PXRNETEVENT nev) const;
};
typedef XRKBDEVENT *PXRKBDEVENT;



/**
 * Network event message structure (8 bytes)
 *
 */
typedef struct {
	unsigned char	type;
	unsigned char	flags;
	unsigned short	localPort;
	unsigned short	r1;
	unsigned short	y;
} XRNETNOTIFYEVENT, *PXRNETNOTIFYEVENT;

/**
 * Notify event wrapper
 *
 */
class XRNOTIFYEVENT : public XREVENT {
protected:
	unsigned short y;
	unsigned char flags;


public:
	XRNOTIFYEVENT(int type, unsigned short y, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int flags);
	XRNOTIFYEVENT(const XRNOTIFYEVENT &ref);
	XRNOTIFYEVENT(const XRNOTIFYEVENT &ref, int flags);
	XRNOTIFYEVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress);
	virtual ~XRNOTIFYEVENT();


	unsigned short getY() const;
	unsigned char getFlags() const;

	virtual void convert(PXRNETEVENT nev) const;
};
typedef XRNOTIFYEVENT *PXRNOTIFYEVENT;



/**
 * Network event message structure (8 bytes)
 *
 */
typedef struct {
	unsigned char	type;
	unsigned char	code;
	unsigned short	localPort;
	unsigned int		size;
} XRNETBUFFEREVENT, *PXRNETBUFFEREVENT;

/**
 * Buffer chunk (8 bytes)
 *
 */
class XRBUFFERCHUNK {
public:
	unsigned char	size;
	unsigned short	sequence;
	unsigned char	data[sizeof(XRNETEVENT)];


	XRBUFFERCHUNK() : size(0), sequence(0) {
		memset(this->data, 0, sizeof(XRNETEVENT));
	}
	XRBUFFERCHUNK(unsigned char size, unsigned short sequence, unsigned char data[sizeof(XRNETEVENT)]) : size(size), sequence(sequence) {
		memcpy(this->data, data, sizeof(XRNETEVENT));
	}
	XRBUFFERCHUNK(const XRBUFFERCHUNK &ref) : size(ref.size), sequence(ref.sequence) {
		memcpy(this->data, ref.data, sizeof(XRNETEVENT));
	}

	const int operator <(const XRBUFFERCHUNK &ref) const {
		return (this->sequence < ref.sequence);
	}
	const XRBUFFERCHUNK & operator =(const XRBUFFERCHUNK &ref) {
		this->size = ref.size;
		this->sequence = ref.sequence;
		memcpy(this->data, ref.data, sizeof(XRNETEVENT));
		return *this;
	}
};

/**
 * CutBuffer-paste event wrapper
 *
 */
class XRBUFFEREVENT : public XREVENT {
protected:
	unsigned char			code;
	unsigned int				size;

	vector<XRBUFFERCHUNK>	chunks;
	unsigned short			count;


public:
	XRBUFFEREVENT(in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, unsigned char code, unsigned int size);
	XRBUFFEREVENT(in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, unsigned char code, const string &str);
	XRBUFFEREVENT(const XRBUFFEREVENT &ref);
	XRBUFFEREVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress);
	virtual ~XRBUFFEREVENT();


	bool isComplete() const;
	unsigned char getCode() const;
	unsigned int getSize() const;

	string getAsString() const;

	void setChunk(const XRBUFFERCHUNK &chunk);
	const vector<XRBUFFERCHUNK> & getChunks() const;

	virtual void convert(PXRNETEVENT nev) const;
};
typedef XRBUFFEREVENT *PXRBUFFEREVENT;


#endif //_XREVENTS_H_INCLUDE_
