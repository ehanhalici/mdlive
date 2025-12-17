#pragma once
#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H> // Daha yumuşak çizim için Double Window
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_message.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <string>

struct AppState {
    Fl_Text_Editor* editor = nullptr;
    Fl_Text_Buffer* textBuf = nullptr;
    Fl_Text_Buffer* styleBuf = nullptr;
    Fl_Tree* tree = nullptr;
    //std::string currentRoot = "."; // todo geri al
    //std::string currentRoot = "~/notes/obsidian_notlar";
    std::string currentRoot = "/home/emrehan/notes/obsidian_notlar";
    //std::string currentRoot = "~/doc/karalamalar";
};


inline AppState app;
