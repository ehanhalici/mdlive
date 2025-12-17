#pragma once

#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_message.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>


class SmartEditor : public Fl_Text_Editor {
public:
    SmartEditor(int x, int y, int w, int h, const char* l = 0)
        : Fl_Text_Editor(x, y, w, h, l) {}

    int handle(int event) override;
};
