#include "Callback.hpp"
#include "App.hpp"
#include "FileSystem.hpp"
#include "Parser.hpp"


void SelectFolderBtnCB(Fl_Widget*, void*) {
    const char* dir = fl_dir_chooser("Çalışma Klasörünü Seç", app.currentRoot.c_str(), 0);
    if (dir) {
        app.currentRoot = dir;
        PopulateFileTree(app.currentRoot);
        
        // Pencere başlığını güncelle
        if (app.tree->window()) {
            app.tree->window()->copy_label(app.currentRoot.c_str());
        }
    }
}


void EditorStyleUpdateCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg) {
    // 1. Önce StyleBuffer ile TextBuffer'ı senkronize et (ZORUNLU)
    if (nInserted == 0 && nDeleted == 0) { 
        app.styleBuf->unselect(); 
        return; 
    }
    
    if (nDeleted > 0) {
        app.styleBuf->remove(pos, pos + nDeleted);
    }
    
    if (nInserted > 0) {
        char* style = new char[nInserted + 1];
        memset(style, 'A', nInserted); 
        style[nInserted] = '\0';
        app.styleBuf->insert(pos, style);
        delete[] style;
    }

    // 2. OPTİMİZASYON: Sadece etkilenen satırları bul
    // static int lastModLine = -1; // (İstersen burada tutabilirsin ama FLTK metodları daha güvenli)

    // Değişimin başladığı satırın başı
    int startPos = app.textBuf->line_start(pos);
    
    // Değişimin bittiği satırın sonu 
    // (nInserted eklendiği için yeni pozisyonu hesaba katmalıyız)
    int endPos = app.textBuf->line_end(pos + nInserted);

    // 3. Sadece bu aralığı yeniden boya
    // Enter'a basarsan startPos üst satırda, endPos alt satırda olur.
    // Böylece her iki satır da otomatik olarak yeniden boyanır.
    ParseRange(startPos, endPos); 
}

void ResetEditorStyles() {
    app.textBuf->remove_modify_callback(EditorStyleUpdateCB, nullptr);
    
    int len = app.textBuf->length();
    app.styleBuf->text(""); // Temizle
    
    if (len > 0) {
        char* dummy = new char[len + 1];
        memset(dummy, 'A', len);
        dummy[len] = '\0';
        app.styleBuf->text(dummy);
        delete[] dummy;
    }
    
    app.textBuf->add_modify_callback(EditorStyleUpdateCB, nullptr);
}


// Ağaç Tıklama Olayı
void TreeCB(Fl_Widget*, void*) {
    // 1. Tıklama nedenini al
    int reason = app.tree->callback_reason();
    Fl_Tree_Item* item = app.tree->callback_item();

    if (!item) return;

    // 2. Sağ Tık Kontrolü
    if (Fl::event_button() == FL_RIGHT_MOUSE) {
        // Item'ı seçili hale getir (kullanıcı deneyimi için)
        app.tree->select(item, 1);
        
        Fl_Menu_Item rclick_menu[] = {
            { "Yeni Dosya",  0, ContextMenuCB, (void*)1 },
            { "Yeni Klasör", 0, ContextMenuCB, (void*)2 },
            { "Sil",         0, ContextMenuCB, (void*)3 },
            { 0 }
        };
        const Fl_Menu_Item* m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
        if (m) m->do_callback(0, m->user_data());
        return;
    }

    // 3. Sol Tık (Seçim) Kontrolü
    if (reason == FL_TREE_REASON_SELECTED) {
        std::string path = SafeGetPathFromItem(item);
        if (path.empty()) return;

        if (fs::is_directory(path)) {
            // Klasörse aç/kapa
            if (item->is_open()) item->close();
            else item->open();
            app.tree->redraw();
        } else {
            // Dosyaysa editöre yükle
            LoadFileToEditor(path);
        }
    }
}



// İmleç her hareket ettiğinde çağrılır
void CursorMoveCB(Fl_Widget* w, void* data) {
    static int lastLineStart = -1;
    int pos = app.editor->insert_position();
    int currentLineStart = app.textBuf->line_start(pos);
    int currentLineEnd = app.textBuf->line_end(pos);

    // Eğer satır değiştiyse
    if (currentLineStart != lastLineStart) {
        // 1. Yeni satırı boya (Aktif yapmak için)
        ParseRange(currentLineStart, currentLineEnd);

        // 2. Eski satırı boya (Pasif yapmak için)
        if (lastLineStart != -1) {
            // Eski satır hala geçerli mi diye kontrol et (silinmiş olabilir)
            if (lastLineStart < app.textBuf->length()) {
                int lastLineEnd = app.textBuf->line_end(lastLineStart);
                ParseRange(lastLineStart, lastLineEnd);
            }
        }
        
        lastLineStart = currentLineStart;
    }
}



// Sağ Tık Menüsü Yönlendiricisi
void ContextMenuCB(Fl_Widget*, void* v) {
    long choice = (long)v;
    switch(choice) {
        case 1: ActionCreateFile(); break;
        case 2: ActionCreateDir(); break;
        case 3: ActionDelete(); break;
    }
}
