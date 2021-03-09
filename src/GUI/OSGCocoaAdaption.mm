

#include <GUI/QtLauncher.hxx>

#include <QOpenGLContext>
#include <QtPlatformHeaders/QCocoaNativeContext>
#include <QWindow>

#include <osgViewer/api/Cocoa/GraphicsWindowCocoa>


namespace flightgear
{
    
QOpenGLContext* qtContextFromOSG(osg::GraphicsContext* context)
{
    osgViewer::GraphicsWindowCocoa* win = dynamic_cast<osgViewer::GraphicsWindowCocoa*>(context);
    if (!win) {
        return nullptr;
    }
    
    auto cnc = QCocoaNativeContext{win->getContext()};
    QVariant nativeHandle = QVariant::fromValue(cnc);

    if (nativeHandle.isNull())
        return nullptr;
    
    std::unique_ptr<QOpenGLContext> qtOpenGLContext(new QOpenGLContext);
    qtOpenGLContext->setNativeHandle(nativeHandle);
    if (!qtOpenGLContext->create()) {
        return nullptr;
    }
    
    return qtOpenGLContext.release();
}

QWindow* qtWindowFromOSG(osgViewer::GraphicsWindow* graphicsWindow)
{
    osgViewer::GraphicsWindowCocoa* win = dynamic_cast<osgViewer::GraphicsWindowCocoa*>(graphicsWindow);
    if (!win) {
        return nullptr;
    }
    
    NSView* osgView = [win->getContext() view];
    return QWindow::fromWinId(reinterpret_cast<qlonglong>(osgView));
}
    
} // of namespace flightgear
