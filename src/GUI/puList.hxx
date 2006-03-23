// puList.hxx - a scrolling PUI list box.

#ifndef __PULIST_HXX
#define __PULIST_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/pu.h>

# define PUCLASS_LIST   0x80000000  // Hopefully this value will never be used by plib

/**
 * A scrolling list for PUI.
 *
 * Believe it or not, PUI does not have one of these.
 */
class puList : public puGroup
{
 public:
    puList (int x, int y, int w, int h, int sl_width = 20);
    puList (int x, int y, int w, int h, char ** contents, int sl_width = 20);
    virtual ~puList ();

    virtual void newList (char ** contents);
    // TODO: other string value funcs
    virtual char * getListStringValue ();
    virtual int getListIntegerValue();
    virtual void setColourScheme (float r, float g, float b, float a);
    virtual void setColour (int which, float r, float g, float b, float a);
    virtual void setSize (int w, int h);


 protected:
    virtual void init (int w, int h);

 private:
    int sw;            // slider width
    char ** _contents;
    puFrame * _frame;
    puListBox * _list_box;
    puSlider * _slider;
    puArrowButton * _up_arrow;
    puArrowButton * _down_arrow;
};

#endif // __PULIST_HXX
