/**
 * Xremote - X "InputOnly" window with input grab
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free to use for any purpose ;)
 */
#ifndef _XRWINDOW_H_INCLUDE_
#define _XRWINDOW_H_INCLUDE_


/**
 * XRemote X window
 *
 */
class XRWINDOW : public XRDISPLAY {
private:
	bool   grab;
	Window window;
	Cursor cursor;
	string primaryTextSelection;
	string secondaryTextSelection;
	string clipboardTextSelection;
	int    atomId;
	Atom   XA_CLIPBOARD;


protected:
	bool createWindow(int x, int y, int width, int height);
	bool destroyWindow();

	bool createCursor();
	bool destroyCursor();

	bool grabInput();
	bool ungrabInput();
	bool isGrabbing();

	virtual bool pollXEvents();
	virtual bool processXEvent(const XEvent &xev);

	bool centerPointerOnScreen();

	Atom getClipboardAtom() const;

	void askSelection(Atom selection);
	string getSelection(Atom selection, Atom property) const;

	void setSelection(Atom selection, const string &text);
	void sendSelectionNotify(const XEvent &xev);

	void clearSelection(Atom selection);


public:
	XRWINDOW(const string &displayName);
	XRWINDOW(const XRWINDOW &ref);
	virtual ~XRWINDOW();
};


#endif //_XRWINDOW_H_INCLUDE_
