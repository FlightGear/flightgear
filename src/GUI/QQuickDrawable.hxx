#ifndef FG_VIEWER_QUICK_DRAWABLE_HXX
#define FG_VIEWER_QUICK_DRAWABLE_HXX

#include <osg/Drawable>
#include <QUrl>

#include <osgViewer/GraphicsWindow>

class QQuickDrawablePrivate;

class QQuickDrawable : public osg::Drawable
{
public:
    QQuickDrawable();
    virtual ~QQuickDrawable();

  virtual osg::Object* cloneType() const { return 0; }
  virtual osg::Object* clone(const osg::CopyOp& copyop) const { return 0; }

  void drawImplementation(osg::RenderInfo& renderInfo) const override;

  /** Return true, FGPanelNode does support accept(PrimitiveFunctor&). */
 // virtual bool supports(const osg::PrimitiveFunctor&) const { return true; }

  //virtual void accept(osg::PrimitiveFunctor& functor) const;

  void setup(osgViewer::GraphicsWindow* gw);

  void setSource(QUrl url);

  void resize(int width, int height);
private:
  QQuickDrawablePrivate* d;
};

#endif
