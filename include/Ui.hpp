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



// Koyu Tema İçin Stil Tablosu
// =========================================================
// RENK VE STİL AYARLARI (MARKDOWN İÇİN)
// =========================================================
static Fl_Text_Editor::Style_Table_Entry styles[] = {
    // ID   Renk                           Font                  Size
    // ----------------------------------------------------------------
    { fl_rgb_color(200, 200, 200), FL_HELVETICA,          14 }, // A - Normal
    { fl_rgb_color(86, 156, 214),  FL_HELVETICA_BOLD,     24 }, // B - Başlık
    { fl_rgb_color(206, 145, 120), FL_COURIER,            14 }, // C - Kod
    { fl_rgb_color(106, 153, 85),  FL_HELVETICA_ITALIC,   14 }, // D - Yorum
    { fl_rgb_color(220, 220, 170), FL_HELVETICA_BOLD,     14 }, // E - Kalın
    { fl_rgb_color(86, 156, 214),  FL_HELVETICA,          14 }, // F - Link
    { fl_rgb_color(30, 30, 30),    FL_HELVETICA,          1  }, // G - Gizli
    { fl_rgb_color(100, 100, 100), FL_HELVETICA,          14 }, // H - Silik (Eskiden S idi)
    { fl_rgb_color(214, 100, 100), FL_HELVETICA_ITALIC,   14 }, // I - İtalik
    { fl_rgb_color(100, 200, 200), FL_COURIER_ITALIC,     14 }, // J - Matematik (Eskiden M idi)
    { fl_rgb_color(150, 100, 255), FL_HELVETICA_BOLD,     14 }, // K - Wiki/Tag (Eskiden W idi)
    { fl_rgb_color(255, 200, 100), FL_HELVETICA,          14 }, // L - Turuncu (Eskiden O idi)
    { fl_rgb_color(100, 255, 100), FL_HELVETICA_BOLD,     14 }, // M - Yeşil (Eskiden X idi)
    { fl_rgb_color(128, 128, 128), FL_HELVETICA,          14 }, // N - Üstü Çizili (Eskiden Z idi)
    
    // YENİ EKLENEN TABLO STİLİ (Sıradaki harf 'O')
    { fl_rgb_color(206, 145, 120), FL_COURIER,             14 }, // O - Tablo 
    { fl_rgb_color(100, 100, 100), FL_COURIER,             14 }, // P -

    { fl_rgb_color(86, 156, 214),  FL_COURIER,            12 }, // P - YAML Frontmatter (Metadata)
    { fl_rgb_color(255, 215, 0),   FL_HELVETICA_BOLD,     14 }, // Q - Embed ![[...]] (Altın Sarısı)
    { fl_rgb_color(100, 149, 237), FL_HELVETICA,          14 }, // R - Bare URL (Cornflower Blue)
};


void init_ui(int argc, char** argv);
