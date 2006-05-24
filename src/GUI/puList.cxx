// puList.cxx - implementation of a scrolling list box.

#include "puList.hxx"


/**
 * Static function: handle slider movements.
 */
static void
handle_slider (puObject * slider)
{
    puListBox * box = (puListBox *)slider->getUserData();
    int total = box->getNumItems();
    int visible = box->getNumVisible();
    // negative numbers are OK -- setTopItem() clamps anyway
    int index = int((total - visible) * (1.0 - slider->getFloatValue()));
    box->setTopItem(index);
}


/**
 * Static function: handle list entry clicks.
 */
static void
handle_list_entry (puObject * listbox)
{
    puListBox * box = (puListBox *)listbox->getUserData();
    box->invokeCallback();
}


/**
 * Static function: handle arrow clicks.
 */
static void
handle_arrow (puObject * arrow)
{
    puSlider * slider = (puSlider *)arrow->getUserData();
    puListBox * list_box = (puListBox *)slider->getUserData();
    puList * list = (puList *)list_box->getUserData();

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

    int index = list->getTopItem();
    list->setTopItem(index + step);
    slider->setValue(1.0f - float(list->getTopItem()) / (list->getNumItems() - list->getNumVisible()));
}

puList::puList (int x, int y, int w, int h, int sl_width)
    : puGroup(x, y),
      _sw(sl_width)
{
    type |= PUCLASS_LIST;
    init(w, h, 1);
}

puList::puList (int x, int y, int w, int h, char ** contents, int sl_width)
    : puGroup(x, y),
      _sw(sl_width)
{
    type |= PUCLASS_LIST;
    init(w, h, 1);
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

    // new size calculation to consider slider visibility
    setSize(_width, _height);
}

void
puList::setTopItem( int top )
{
    int visible = _list_box->getNumVisible();
    int num = _list_box->getNumItems();
    if ( top < 0 || num <= visible )
        top = 0 ;
    else if ( num > 0 && top > num-visible )
        top = num-visible;

    _list_box->setTopItem(top);
    top = _list_box->getTopItem();
    // read clamped value back in, and only set slider if it doesn't match the new
    // index to avoid jumps
    int slider_index = int((1.0f - _slider->getFloatValue()) * (getNumItems() - getNumVisible()));
    if (slider_index != top)
        _slider->setValue(1.0f - float(getTopItem()) / (getNumItems() - getNumVisible()));
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
puList::setColourScheme (float r, float g, float b, float a)
{
    puObject::setColourScheme(r, g, b, a);
    _list_box->setColourScheme(r, g, b, a);
}

void
puList::setColour (int which, float r, float g, float b, float a)
{
    puObject::setColour(which, r, g, b, a);
    _list_box->setColour(which, r, g, b, a);
}

void
puList::setSize (int w, int h)
{
    _width = w;
    _height = h;
    puObject::setSize(w, h);
    if (_frame)
        _frame->setSize(w, h);

    int total = _list_box->getNumItems();
    int visible = _list_box->getNumVisible();

    if (total > visible)
    {
        if (!_slider->isVisible())
        {
            _slider->setValue(1.0f);
            _slider->reveal();
            _up_arrow->reveal();
            _down_arrow->reveal();
        }
        _list_box->setSize(w-_sw, h);

        _slider->setPosition(w-_sw, _sw);
        _slider->setSize(_sw, h-2*_sw);
        _slider->setSliderFraction(float(visible) / total);

        _down_arrow->setPosition(w-_sw, 0);
        _up_arrow->setPosition(w-_sw, h-_sw);

    }
    else
    {
        if (_slider->isVisible())
        {
            _slider->hide();
            _up_arrow->hide();
            _down_arrow->hide();
        }
        _list_box->setSize(w, h);
    }
}

void
puList::init (int w, int h, short transparent)
{
    if ( transparent )
        _frame = NULL ;
    else
        _frame = new puFrame(0, 0, w, h);

    _list_box = new puListBox(0, 0, w-_sw, h);
    _list_box->setStyle(-PUSTYLE_SMALL_SHADED);
    _list_box->setUserData(this);
    _list_box->setCallback(handle_list_entry);
    _list_box->setValue(0);

    _slider = new puSlider(w-_sw, _sw, h-2*_sw, true, _sw);
    _slider->setValue(1.0f);
    _slider->setUserData(_list_box);
    _slider->setCallback(handle_slider);
    _slider->setCBMode(PUSLIDER_ALWAYS);

    _down_arrow = new puArrowButton(w-_sw, 0, w, _sw, PUARROW_DOWN) ;
    _down_arrow->setUserData(_slider);
    _down_arrow->setCallback(handle_arrow);

    _up_arrow = new puArrowButton(w-_sw, h-_sw, w, h, PUARROW_UP);
    _up_arrow->setUserData(_slider);
    _up_arrow->setCallback(handle_arrow);

    setSize(w, h);
    close();
}

// end of puList.cxx
