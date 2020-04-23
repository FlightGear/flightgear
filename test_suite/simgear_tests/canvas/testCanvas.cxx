#include "testCanvas.hxx"

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/elements/CanvasImage.hxx>

#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Main/globals.hxx>
#include <Main/fg_commands.hxx>

#include <Canvas/canvas_mgr.hxx>
#include <Canvas/FGCanvasSystemAdapter.hxx>
extern bool global_nasalMinimalInit;


void verifyPixel(osg::Image* img, int x, int y, const uint8_t* pixel)
{
    auto rawPixel = img->data(x, y);
    CPPUNIT_ASSERT(memcmp(rawPixel, pixel, 4) == 0);
}

void CanvasTests::setUp()
{
    global_nasalMinimalInit = false;
    FGTestApi::setUp::initTestGlobals("canvas");

    // Canvas needs loadxml command
    fgInitCommands();
    
    simgear::canvas::Canvas::setSystemAdapter(
       simgear::canvas::SystemAdapterPtr(new canvas::FGCanvasSystemAdapter)
     );
    
    globals->add_subsystem("Canvas", new CanvasMgr, SGSubsystemMgr::DISPLAY);

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    
    FGTestApi::setUp::initStandardNasal(true /* withCanvas */);
    globals->get_subsystem_mgr()->postinit();
}

// Clean up after each test.
void CanvasTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void CanvasTests::testCanvasBasic()
{
    bool ok = FGTestApi::executeNasal(R"(
     var my_canvas = canvas.new({
      "name": "PFD-Test",
          "size": [512, 512],
          "view": [768, 1024],
          "mipmapping": 1
        });
                                      
      globals.pfd = my_canvas;
    )");
    CPPUNIT_ASSERT(ok);
    
    auto canvasPtr = globals->get_subsystem<CanvasMgr>()->getCanvas("PFD-Test");
    CPPUNIT_ASSERT(canvasPtr.valid());
    
    ok = FGTestApi::executeNasal(R"(
        globals.pfd.createGroup("wibble");
    )");
    CPPUNIT_ASSERT(ok);
    
    auto wibbleGroupPtr = canvasPtr->getGroup("wibble");
    CPPUNIT_ASSERT(wibbleGroupPtr.valid());
}

void CanvasTests::testImagePixelOps()
{
    bool ok = FGTestApi::executeNasal(R"(
        var my_canvas = canvas.new({
         "name": "ImagePixelOpsCanvas",
             "size": [512, 512],
             "view": [768, 1024],
             "mipmapping": 1
           });
                                         
         var g = my_canvas.createGroup("root");
         var img = g.createChild('image', 'ops-image');
                         
         # deliberate non-power-of-two sizes here
         img.setSize(150, 170);
                                      
         globals.pixelOps = my_canvas;
       )");
       CPPUNIT_ASSERT(ok);
       
    auto canvasPtr = globals->get_subsystem<CanvasMgr>()->getCanvas("ImagePixelOpsCanvas");
    CPPUNIT_ASSERT(canvasPtr.valid());
    
    auto imagePtr = canvasPtr->getRootGroup()->getElementById("ops-image");
    CPPUNIT_ASSERT(imagePtr.valid());
    
    auto image = dynamic_cast<simgear::canvas::Image*>(imagePtr.get());
    image->setFill(osg::Vec4(1.0, 0.0, 0.5, 1.0));
    
    image->fillRect(SGRecti(20, 40, 70, 55), "#7f3f1f");
    
    ok = FGTestApi::executeNasal(R"(
        var canvas = globals.pixelOps;
        var img = canvas.getGroup('root').getElementById('ops-image');
                                 
        print('Image is:', img);
        img.fillRect([5, 8, 180, 37], '#aabbcc');
    )");
    CPPUNIT_ASSERT(ok);

    auto osgImage = image->getImage();
    const uint8_t testColor1[] = {0xff, 0xcc, 0xbb, 0xaa}; // little endian
    verifyPixel(osgImage.get(), 5, 8, testColor1);
    verifyPixel(osgImage.get(), 149, 44, testColor1);

    // test pixles should not have been touched by the second fillRect
    const uint8_t testColor2[] = {0xff, 0x1f, 0x3f, 0x7f}; // little endian
    verifyPixel(osgImage.get(), 20, 45, testColor2);
}
