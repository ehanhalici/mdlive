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

#include <cstring>

#include "App.hpp"

#include "Callback.hpp"
#include "Ui.hpp"
#include "FileSystem.hpp"







int main(int argc, char** argv) {
    init_ui(argc, argv);
    try {
        PopulateFileTree(app.currentRoot);
    } catch (...) {}

    app.textBuf->text("# Karanlık mod aktif.");
    ResetEditorStyles();


    return Fl::run();
}
