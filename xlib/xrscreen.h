/**
 * Xremote - X screen wrapper
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRSCREEN_H_INCLUDE_
#define _XRSCREEN_H_INCLUDE_


/**
 * Screen wrapper
 *
 */
class XRSCREEN {
protected:
	Display *display;
	Screen	*screen;
	int		index;
	int		width;
	int		height;

	void flush() const;


public:
	XRSCREEN();
	XRSCREEN(Display *display, int index);
	XRSCREEN(const XRSCREEN &ref);
	virtual ~XRSCREEN();


	int getIndex() const;
	int getWidth() const;
	int getHeight() const;

	Window getRootWindow() const;

	Visual* getDefaultVisual() const;

	bool centerPointer();
	bool getPointer(Window *child, int *x, int *y) const;
	bool setPointer(int x, int y) const;
};


#endif //_XRSCREEN_H_INCLUDE_
