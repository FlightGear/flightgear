// puList.hxx - a scrolling PUI list box.

#ifndef __PULIST_HXX
#define __PULIST_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/pu.h>


/**
 * A scrolling list for PUI.
 *
 * Believe it or not, PUI does not have one of these.
 */
class puList : public puGroup
{
 public:
    puList (int x, int y, int w, int h);
    puList (int x, int y, int w, int h, char ** contents);
    virtual ~puList ();

    virtual void newList (char ** contents);
    // TODO: other string value funcs
    virtual char * getStringValue ();

 protected:
    virtual void init (int w, int h);

 private:
    char ** _contents;
    puFrame * _frame;
    puListBox * _list_box;
    puSlider * _slider;
    puArrowButton * _up_arrow;
    puArrowButton * _down_arrow;
};

#endif // __PULIST_HXX
