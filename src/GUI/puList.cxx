// puList.cxx - implementation of a scrolling list box.

#include "puList.hxx"


/**
 * Static function: handle slider movements.
 */
static void
handle_slider (puObject * slider)
{
    puListBox * box = (puListBox *)slider->getUserData();
    int index = int(box->getNumItems() * (1.0 - slider->getFloatValue()));
    if (index >= box->getNumItems())
        index = box->getNumItems() - 1;
    box->setTopItem(index);
}


/**
 * Static function: handle arrow clicks.
 */
static void
handle_arrow (puObject * arrow)
{
    puSlider * slider = (puSlider *)arrow->getUserData();
    puListBox * list_box = (puListBox *)slider->getUserData();

    int step;
    switch (((puArrowButton *)arrow)->getArrowType()) {
    case PUARROW_DOWN:
        step = 1;
        break;
    case PUARROW_UP:
        step = -1;
        break;
    default:
        step = 0;
        break;
    }

    int index = list_box->getTopItem();
    index += step;
    if (index < 0)
        index = 0;
    else if (index >= list_box->getNumItems())
        index = list_box->getNumItems() - 1;
    list_box->setTopItem(index);

    slider->setValue(1.0f - float(index)/list_box->getNumItems());
}

puList::puList (int x, int y, int w, int h)
    : puGroup(x, y)
{
    type |= PUCLASS_LIST;
    init(w, h);
}

puList::puList (int x, int y, int w, int h, char ** contents)
    : puGroup(x, y)
{
    type |= PUCLASS_LIST;
    init(w, h);
    newList(contents);
}

puList::~puList ()
{
}

void
puList::newList (char ** contents)
{
    _list_box->newList(contents);
    _contents = contents;
}

char *
puList::getListStringValue ()
{
    int i = _list_box->getIntegerValue();
    return i < 0 ? 0 : _contents[i];
}

int
puList::getListIntegerValue()
{
  return _list_box->getIntegerValue();
}

void
puList::init (int w, int h)
{
    _frame = new puFrame(0, 0, w, h);

    _list_box = new puListBox(0, 0, w-30, h);
    _list_box->setStyle(-PUSTYLE_SMALL_SHADED);
    _list_box->setUserData(this);
    _list_box->setValue(0);

    _slider = new puSlider(w-30, 30, h-60, true);
    _slider->setValue(1.0f);
    _slider->setUserData(_list_box);
    _slider->setCallback(handle_slider);
    _slider->setCBMode(PUSLIDER_ALWAYS);

    _down_arrow = new puArrowButton(w-30, 0, w, 30, PUARROW_DOWN) ;
    _down_arrow->setUserData(_slider);
    _down_arrow->setCallback(handle_arrow);

    _up_arrow = new puArrowButton(w-30, h-30, w, h, PUARROW_UP);
    _up_arrow->setUserData(_slider);
    _up_arrow->setCallback(handle_arrow);

    close();
}

// end of puList.cxx
