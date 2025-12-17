#include "App.hpp"
#include "Ui.hpp"
#include "Callback.hpp"
#include "Editor.hpp"


void SetDarkTheme() {
    // Şemayı "base" yapıyoruz ki GTK+ teması renklerimizi ezmesin.
    Fl::scheme("base"); 

    // GLOBAL RENKLER (Kritik Nokta)
    // FLTK 1.3'te satır numaraları rengini burası belirler!
    
    // 1. Global Arka Plan (Koyu Gri)
    Fl::background(18, 18, 18);
    
    // 2. Global Yazı Rengi (Açık Gri) -> Satır numaraları bu rengi alacak!
    Fl::foreground(184, 184, 184); 
    
    // 3. Seçim Rengi
    Fl::set_color(FL_SELECTION_COLOR, 60, 100, 160);
}


void init_ui(int argc, char** argv) {

    SetDarkTheme(); 

    Fl_Double_Window* window = new Fl_Double_Window(1000, 600, "Güvenli FLTK Editör");

    // --- SOL PANEL (AĞAÇ) ---
    Fl_Button* btnOpen = new Fl_Button(10, 10, 100, 10, "@fileopen  Seç");
    btnOpen->callback(SelectFolderBtnCB);
    // Butonun yazı rengini manuel düzeltmek gerekebilir
    btnOpen->labelcolor(fl_rgb_color(184, 184, 184)); 

    app.tree = new Fl_Tree(10, 30, 200, 600);
    
    // >> AĞAÇ RENK AYARLARI (Burada Yapılmalı) <<
    app.tree->box(FL_FLAT_BOX);
    app.tree->color(fl_rgb_color(18, 18, 18));        // Zemin
    app.tree->item_labelfgcolor(fl_rgb_color(184, 184, 184)); // Yazı rengi
    app.tree->connectorcolor(fl_rgb_color(18, 18, 18)); // Çizgiler
    app.tree->showroot(1);
    app.tree->selectmode(FL_TREE_SELECT_SINGLE);
    app.tree->callback(TreeCB);
    
    // İkon ayarları
    app.tree->openicon(0);
    app.tree->closeicon(0);
    app.tree->connectorstyle(FL_TREE_CONNECTOR_DOTTED);


    // --- SAĞ PANEL (EDİTÖR) ---
    app.textBuf = new Fl_Text_Buffer();
    app.styleBuf = new Fl_Text_Buffer();

    app.editor = new SmartEditor(320, 10, 770, 580);
    // FL_BORDER_BOX: İnce siyah çizgili, içi boyanabilir kutu.
    app.editor->box(FL_BORDER_BOX);
    // >> EDİTÖR RENK AYARLARI (Kritik Bölüm) <<
    
    // 1. Ana Yazı Alanı Rengi (Bunu unutmuş olabilirsin)
    app.editor->color(fl_rgb_color(18, 18, 18));
    app.editor->box(FL_FLAT_BOX); 
    app.editor->wrap_mode(Fl_Text_Display::WRAP_AT_COLUMN, 120);
    
    // Artık bu renkler çalışacak:
    app.editor->color(fl_rgb_color(18, 18, 18)); // Ana metin zemini
    app.editor->cursor_color(FL_WHITE);
    app.editor->linenumber_bgcolor(fl_rgb_color(18,18,18));
    app.editor->linenumber_fgcolor(fl_rgb_color(184,184,184));


    app.editor->cursor_color(FL_WHITE);

    app.editor->buffer(app.textBuf);
    app.editor->textfont(FL_HELVETICA);
    app.editor->textsize(14);
    app.editor->linenumber_width(40);
    
    // Stil bağlantıları
    app.editor->highlight_data(app.styleBuf, styles, sizeof(styles)/sizeof(styles[0]), 'A', 0, 0);
    
    app.textBuf->add_modify_callback(EditorStyleUpdateCB, nullptr);
    app.editor->callback(CursorMoveCB); 
    app.editor->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED); // Her hareketi yakala

    window->resizable(app.editor);
    window->end();
    window->show(argc, argv);

    
}
