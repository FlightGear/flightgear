#pragma once

#include <QPointer>

QT_BEGIN_NAMESPACE

// fake the qguiapplication_p.h header, which is problematic
// to include directly on macOS, becuase we don't synchronize
// our macOS min version with the Qt one. As a result, we
// get errors including the real version. To work around
// this, delcare the one symnbol we need
class Q_GUI_EXPORT QGuiApplicationPrivate
{
public:
#if QT_VERSION < QT_VERSION_CHECK(5, 12,7)
    static QWindow* focus_window;
#else
    static QPointer<QWindow> focus_window;
#endif
};

QT_END_NAMESPACE

