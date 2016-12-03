#ifndef FGCANVASPAINTCONTEXT_H
#define FGCANVASPAINTCONTEXT_H

#include <QPainter>

class FGCanvasPaintContext
{
public:
    FGCanvasPaintContext(QPainter* painter);

    QPainter* painter() const
    { return _painter; }

private:
    QPainter* _painter;
};

#endif // FGCANVASPAINTCONTEXT_H
