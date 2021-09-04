/*!********************************************************************

Audacity: A Digital Audio Editor

@file wxWidgetsWindowPlacement.cpp

split from wxWidgetsBasicUI.cpp

Paul Licameli

**********************************************************************/
#include "wxWidgetsWindowPlacement.h"

using namespace BasicUI;

wxWidgetsWindowPlacement::~wxWidgetsWindowPlacement() = default;

wxWindow *wxWidgetsWindowPlacement::GetParent(const WindowPlacement &placement)
{
   if (auto *pPlacement =
       dynamic_cast<const wxWidgetsWindowPlacement*>(&placement))
      return pPlacement->pWindow;
   return nullptr;
}
