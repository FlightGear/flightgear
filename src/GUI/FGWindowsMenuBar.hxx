/*
 * SPDX-FileName: FGWindowsMenuBar.hxx
 * SPDX-FileComment: XML-configured menu bar.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <simgear/structure/SGBinding.hxx>

#include <GUI/menubar.hxx>

#include <memory>

/**
 * XML-configured Windows menu bar.
 *
 * This class creates a menu bar from a tree of XML properties. These
 * properties are not part of the main FlightGear property tree, but
 * are read from a separate file ($FG_ROOT/gui/menubar.xml).
 */
class FGWindowsMenuBar : public FGMenuBar
{
public:
  /**
   * Constructor.
   */
  FGWindowsMenuBar();
  virtual ~FGWindowsMenuBar() = default;
  
  
  /**
   * Initialize the menu bar from $FG_ROOT/gui/menubar.xml
   */
  virtual void init();
  
  /**
   * Make the menu bar visible.
   */
  virtual void show();
  
  
  /**
   * Make the menu bar invisible.
   */
  virtual void hide();
  
  
  /**
   * Test whether the menu bar is visible.
   */
  virtual bool isVisible() const;

  void setHideIfOverlapsWindow(bool hide) override;

  bool getHideIfOverlapsWindow() const override;

  std::vector<SGBindingList> getItemBindings() const;
  
private:
  class WindowsMenuBarPrivate;
  std::unique_ptr<WindowsMenuBarPrivate> _p;
};
