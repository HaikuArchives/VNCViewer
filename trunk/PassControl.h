#if !defined(PASSCONTROL_H)
#define PASSCONTROL_H
/* PassControl (C) 1998 by John Wiggings */

#include <TextControl.h>
#include <MessageFilter.h>

#define MAGIC_SIZE 9	// this is the length of the "actual" buffer

class PassControl : public BTextControl {
public:
					PassControl(BRect frame,
								const char *name,
								const char *label, 
								const char *initial_text, 
								BMessage *message /*,
								uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE*/);
								// Same as Be's Constructor
virtual				~PassControl(); // deletes filter
void				PopChar(); // deletes last character
void				PushChar(BMessage* msg); // adds a char to actual

inline const char	*PassControl::actualText() const
					{
						return actual;
					}

protected:

	class MyFilter : public BMessageFilter
	{
	public:
							MyFilter();
	virtual filter_result	Filter(BMessage *message, BHandler **target);
	};
	
private:

MyFilter	*filter;
char		*actual;
int			actual_size; // Current size of "actual" buffer
int			length;
};
#endif