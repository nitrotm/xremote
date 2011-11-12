/**
 * Xremote - X display wrapper
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free to use for any purpose ;)
 */
#ifndef _XRDISPLAY_H_INCLUDE_
#define _XRDISPLAY_H_INCLUDE_


/**
 * Display wrapper
 *
 */
class XRDISPLAY {
protected:
	Display          *display;
	string           displayName;
	vector<XRSCREEN> screens;
	int              screenCount;


public:
	XRDISPLAY(const string &displayName);
	XRDISPLAY(Display *display);
	XRDISPLAY(const XRDISPLAY &ref);
	virtual ~XRDISPLAY();

	string getDisplayName() const;

	int getScreenCount() const;
	XRSCREEN getScreen(int screenIndex) const;
	XRSCREEN getCurrentScreen() const;

	void flush() const;

	string getCutBuffer() const;
	void setCutBuffer(const string &value);
};


#endif //_XRDISPLAY_H_INCLUDE_
