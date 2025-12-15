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

#include <cstddef>
#include <iostream>
#include <fstream>
#include <memory_resource>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>

#include "Highlighter.hpp"

namespace fs = std::filesystem;

// =========================================================
// 1. UYGULAMA DURUMU (STATE MANAGEMENT)
// Global değişkenleri bir yapı altında toplayarak düzenliyoruz.
// =========================================================


struct AppState {
    Fl_Text_Editor* editor = nullptr;
    Fl_Text_Buffer* textBuf = nullptr;
    Fl_Text_Buffer* styleBuf = nullptr;
    Fl_Tree* tree = nullptr;
    std::string currentRoot = ".";
    //std::string currentRoot = "~/notes/obsidian_notlar";
    //std::string currentRoot = "~/doc/karalamalar";
} app;

bool IsTableLine(const char *text);
void FormatTableAt(int pos);

class SmartEditor : public Fl_Text_Editor {
public:
    SmartEditor(int x, int y, int w, int h, const char* l = 0)
        : Fl_Text_Editor(x, y, w, h, l) {}

    int handle(int event) override {
        // --- TABLO YÖNETİMİ ---
        if (event == FL_KEYDOWN) {
            int key = Fl::event_key();
            char text = Fl::event_text()[0];
            int pos = insert_position();
            
            // Bulunduğumuz satırı al
            int lineStart = buffer()->line_start(pos);
            int lineEnd = buffer()->line_end(pos);
            char* lineStr = buffer()->text_range(lineStart, lineEnd);
            bool inTable = IsTableLine(lineStr);
            free(lineStr);

            if (inTable) {
                // 1. ENTER'a basıldıysa: Yeni satır ekle ve hizala
                if (key == FL_Enter) {
                    
                    // A. Önce mevcut tabloyu olduğu yerde hizala (Formatla)
                    FormatTableAt(pos);
                    
                    // B. Hizalanmış satırdaki '|' sayısını öğrenmemiz lazım
                    // Tekrar pozisyonları al (Formatlama sonrası kaymış olabilir)
                    int currentPos = insert_position();
                    int lStart = buffer()->line_start(currentPos);
                    int lEnd = buffer()->line_end(currentPos);
                    char* cleanLine = buffer()->text_range(lStart, lEnd);
                    
                    int pipeCount = 0;
                    for (int i = 0; cleanLine[i]; i++) {
                        if (cleanLine[i] == '|') pipeCount++;
                    }
                    free(cleanLine);
                    
                    // Güvenlik: Eğer hiç pipe yoksa en az 2 tane olsun (| |)
                    if (pipeCount < 2) pipeCount = 2;

                    // C. Yeni boş satır stringini oluştur
                    // Örn: pipeCount 4 ise -> "|  |  |  |" üretir
                    std::string newRowStr = "";
                    for (int i = 0; i < pipeCount - 1; i++) {
                        newRowStr += "|  ";
                    }
                    newRowStr += "|"; // Kapanış pipe'ı

                    // D. Satırı bölmemek için imleci satırın EN SONUNA taşı
                    // (Org-mode davranışı: Enter her zaman alta yeni satır açar)
                    insert_position(lEnd);
                    
                    // E. Yeni satır karakteri ve oluşturduğumuz boş tablo satırını ekle
                    // Not: Fl_Text_Editor::handle(event) çağırmıyoruz, işi biz yapıyoruz.
                    insert("\n");
                    insert(newRowStr.c_str());
                    
                    // F. Yeni eklenen satırı da hizala (Genişlikleri üsttekine uydurur)
                    FormatTableAt(insert_position());
                    
                    // G. İmleci yeni satırın ilk hücresinin içine koy
                    // |  |  | ...
                    //  ^ buraya
                    int finalPos = insert_position();
                    int newRowStart = buffer()->line_start(finalPos);
                    
                    // İlk '|' karakterinden sonraki boşluğa git
                    // Genelde "|  " olduğu için +2 veya +3 idealdir.
                    // Garanti olsun diye satır başından ilk '|' bulup yanına gidelim.
                    insert_position(newRowStart + 2); // Kabaca ilk hücreye düşer
                    
                    return 1; // Event'i tükettik, FLTK başka bir şey yapmasın
                }
                // 2. TAB'a basıldıysa: Hücreler arası gez ve hizala
                if (key == FL_Tab) {
                    FormatTableAt(pos);
                    // Bir sonraki '|' işaretini bul ve oraya git
                    // (Org-mode davranışı: Sonraki hücreye atla)
                    // Şimdilik sadece hizalama yapsın, imleci sona atmasın
                    return 1; // Tab karakteri girmesin
                }
                
                // 3. '|' (Pipe) karakterine basıldıysa: ANINDA HİZALA
                if (text == '|') {
                    int ret = Fl_Text_Editor::handle(event); // Karakteri yazsın
                    FormatTableAt(pos); // Sonra tabloyu düzelt
                    return ret;
                }
            }
        }
        
        // --- İMLEÇ HAREKETİ TETİKLEYİCİSİ (Eski Kod) ---
        int old_pos = insert_position();
        int result = Fl_Text_Editor::handle(event);
        int new_pos = insert_position();

        if ((event == FL_KEYBOARD || event == FL_PUSH || event == FL_FOCUS) && (old_pos != new_pos)) {
            do_callback(); 
        }

        return result;
    }
};


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

void ParseRange(int startPos, int endPos) {
    if (startPos >= endPos) return;

    // 1. Metnin sadece o kısmını al
    char* textPart = app.textBuf->text_range(startPos, endPos);
    int length = endPos - startPos;
    
    // 2. Stil dizisini hazırla
    char* style = new char[length + 1];
    memset(style, 'A', length);
    style[length] = '\0';

    // 3. Global Cursor Pozisyonunu Al
    int cursorPos = app.editor->insert_position();

    int i = 0;
    bool inCodeFence = false;

    // --- YAML FRONTMATTER STATE ---
    // Dosyanın EN BAŞINDAN başlıyorsak YAML kontrolü yap
    bool inYaml = false;
    if (startPos == 0 && length >= 3 && startsWith(textPart, length, "---")) {
        inYaml = true; // YAML bloğuna girdik
    }


    while (i < length) {
        // Satır Sınırlarını Bul
        int lineStart = i;
        int lineEnd = lineStart;
        while (lineEnd < length && textPart[lineEnd] != '\n') lineEnd++;
        
        int lineLen = lineEnd - lineStart;
        char* lineText = &textPart[lineStart];
        char* lineStyle = &style[lineStart];

        // --- GLOBAL POZİSYON HESABI ---
        int absLineStart = startPos + lineStart;
        int absLineEnd = startPos + lineEnd;
        bool isCurrentLine = (cursorPos >= absLineStart && cursorPos <= absLineEnd);


        // ----------------------------------------------------
        // 1. YAML FRONTMATTER LOGIC (En Yüksek Öncelik)
        // ----------------------------------------------------
        if (inYaml) {
            // Tüm satırı YAML rengine ('P') boya
            memset(lineStyle, 'P', lineLen);
            
            // Eğer bu satır '---' ise ve ilk satır değilse, YAML bitmiştir.
            // (lineStart > 0 kontrolü önemli, yoksa açılışta kapatırız)
            if (lineStart > 0 && lineLen >= 3 && startsWith(lineText, lineLen, "---")) {
                inYaml = false; 
            }
            i = lineEnd + 1;
            continue; // YAML içindeyken başka kurala bakma
        }
        
        // --- BLOK KONTROLLERİ ---

        // A. Fenced Code Block (```)
        if (lineLen >= 3 && startsWith(lineText, lineLen, "```")) {
            inCodeFence = !inCodeFence;
            memset(lineStyle, 'H', lineLen); 
            i = lineEnd + 1;
            continue;
        }
        if (inCodeFence) {
            memset(lineStyle, 'C', lineLen); 
            i = lineEnd + 1;
            continue;
        }

        // B. Headers (#)
        if (lineLen > 0 && lineText[0] == '#') {
            int hCount = 0;
            while (hCount < lineLen && lineText[hCount] == '#') hCount++;
            
            if (hCount < lineLen && lineText[hCount] == ' ') {
                if (isCurrentLine) {
                    memset(lineStyle, 'H', hCount);
                    memset(lineStyle + hCount, 'B', lineLen - hCount);
                } else {
                    memset(lineStyle, 'G', hCount);
                    memset(lineStyle + hCount, 'B', lineLen - hCount);
                }
                i = lineEnd + 1;
                continue;
            }
        }

        // C. Horizontal Rule (--- veya ***)
        if (lineLen >= 3) {
            bool isHR = true;
            char c = lineText[0];
            if (c == '-' || c == '*') {
                for(int z=0; z<lineLen; z++) if(lineText[z] != c && lineText[z] != ' ') isHR = false;
                if (isHR) {
                    if (isCurrentLine) memset(lineStyle, 'H', lineLen);
                    else memset(lineStyle, 'G', lineLen); 
                    i = lineEnd + 1;
                    continue;
                }
            }
        }

        // D. Task List (- [ ] veya - [x])
        int tOffset = 0;
        while(tOffset < lineLen && lineText[tOffset] == ' ') tOffset++;
        
        bool isTask = false;
        if (lineLen - tOffset >= 5 && 
           (lineText[tOffset] == '-' || lineText[tOffset] == '*') && 
            lineText[tOffset+1] == ' ' && lineText[tOffset+2] == '[') {
            
            if (lineText[tOffset+4] == ']') {
                isTask = true; // Task olduğunu işaretle
                char checkChar = lineText[tOffset+3];
                int afterBracket = tOffset + 5;
                bool isChecked = (checkChar == 'x' || checkChar == 'X');

                if (isCurrentLine) {
                    memset(lineStyle, 'H', afterBracket);
                    if (isChecked) lineStyle[tOffset+3] = 'X'; 
                } else {
                    memset(lineStyle, 'G', tOffset + 2); 
                    if (isChecked) {
                        memset(lineStyle + tOffset + 2, 'X', 3); 
                        memset(lineStyle + afterBracket, 'Z', lineLen - afterBracket); 
                    } else {
                        memset(lineStyle + tOffset + 2, 'L', 3); 
                    }
                }
                // Task list'in metin kısmını (inline) parse etmek için continue DEMİYORUZ.
                // Sadece başını işledik.
            }
        }

        // E. ORG-MODE TABLE (| ... |)
        // Regex yerine basit mantık: Satır | ile başlıyorsa (trimlenmiş)
        // yukaridaki offset ile birlesebilir ama burasi ayri fonksiyon olacak simdilk kalsin
        int pOffset = 0;
        while(pOffset < lineLen && lineText[pOffset] == ' ') pOffset++;
        
        if (lineLen - pOffset > 0 && lineText[pOffset] == '|') {
            // Tüm satır boyunca | ve + karakterlerini bulup boya
            // Geri kalan metni normal bırak (veya içerik rengi yap)
            
            for (int k = pOffset; k < lineLen; k++) {
                if (lineText[k] == '|' || lineText[k] == '+') {
                    if (isCurrentLine) lineStyle[k] = 'P'; // Silik (Aktif)
                    else lineStyle[k] = 'O'; // Turuncu (Pasif) - Tablo çerçevesi belli olsun
                } 
                else if (lineText[k] == '-') {
                    // Separator satırı mı? |---|
                    lineStyle[k] = 'P'; 
                }
                else {
                    // Tablo içeriği: 
                    lineStyle[k] = 'O';
                }
            }
            // Tablo içi inline parsing devam etsin (Linkler vs. çalışsın)
        }

        // E. BLOCKQUOTES & CALLOUTS (> Text veya > [!INFO] Text)
        // Eğer Task list değilse kontrol et (çakışmasın)
        if (!isTask && lineLen - tOffset > 0 && lineText[tOffset] == '>') {
             // 1. Callout Kontrolü (> [!TYPE])
             // Basit kontrol: > [! ile başlıyor mu?
             bool isCallout = false;
             if (lineLen - tOffset > 4 && lineText[tOffset+1] == ' ' && lineText[tOffset+2] == '[' && lineText[tOffset+3] == '!') {
                 isCallout = true;
             }

             if (isCurrentLine) {
                 lineStyle[tOffset] = 'H'; // > işareti gri
                 // Callout başlığını renklendir
                 if (isCallout) {
                     int closeBracket = tOffset + 4;
                     while(closeBracket < lineLen && lineText[closeBracket] != ']') closeBracket++;
                     if (closeBracket < lineLen) {
                         memset(lineStyle + tOffset + 2, 'W', closeBracket - (tOffset+2) + 1); // [!TYPE] Mor
                     }
                 }
             } else {
                 lineStyle[tOffset] = 'G'; // > işaretini gizle
                 if (isCallout) {
                     // Pasif Callout: > işaretini gizle, [!TYPE] kısmını Mor ve Kalın yap
                     int closeBracket = tOffset + 4;
                     while(closeBracket < lineLen && lineText[closeBracket] != ']') closeBracket++;
                     if (closeBracket < lineLen) {
                         memset(lineStyle + tOffset + 2, 'W', closeBracket - (tOffset+2) + 1);
                     }
                 } else {
                     // Standart Quote: Tüm satırı italik/yeşil yap ('D' stili)
                     memset(lineStyle + tOffset + 1, 'D', lineLen - (tOffset+1));
                 }
             }
             // Quote içindeki linkleri vs. görmek için continue demiyoruz.
        }

        // --- INLINE KONTROLLER ---
        for (int j = 0; j < lineLen; j++) {
            int rem = lineLen - j;
            char* ptr = &lineText[j];
            char* sty = &lineStyle[j];
            int k = 0;

            // 1. Comments (%%) - En Üst Öncelik
            if ((k = scan_comment(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }

            // 2. Inline Code (`)
            if ((k = scan_inline_code(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 3. Embed (![[...]]) - WikiLink'ten önce kontrol edilmeli
            if ((k = scan_embed_wikilink(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 4. WikiLink ([[...]])
            if ((k = scan_wikilink(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 5. Image Extended (![...])
            if ((k = scan_image_extended(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 6. Link ([...])
            if ((k = scan_internal_link(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            if ((k = scan_external_link(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 7. Bare URL (http...) - Yeni
            if ((k = scan_bare_url(ptr, rem, sty))) { j += k - 1; continue; }

            // 8. Footnote ([^...])
            if ((k = scan_footnote(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }

            // 9. Math ($)
            if ((k = scan_math(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }

            // 10. Highlight (==)
            if ((k = scan_highlight(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 11. Strike (~~)
            if ((k = scan_strike(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 12. Bold (**)
            if ((k = scan_bold(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }
            
            // 13. Italic (*)
            if ((k = scan_italic(ptr, rem, sty, isCurrentLine))) { j += k - 1; continue; }

            // 14. Block ID (^id) - Satır sonuna yakın
            if ((k = scan_block_id(ptr, rem, sty))) { j += k - 1; continue; }

            // 15. Tags (#tag)
            if (j == 0 || lineText[j-1] == ' ') {
                if ((k = scan_tag_advanced(ptr, rem, sty))) { j += k - 1; continue; }
            }
        }

        i = lineEnd + 1;
    }

    // 4. Style Buffer'ı Güncelle
    app.styleBuf->replace(startPos, endPos, style);

    delete[] style;
    free(textPart);
}

void ParseAndColorize() {
    ParseRange(0, app.textBuf->length());
}


// =========================================================
// 2. YARDIMCI GÜVENLİK FONKSİYONLARI
// Segfault'u önleyen kritik fonksiyonlar.
// =========================================================

// Bir Tree Item'ından dosya yolunu güvenle alır.
// Item yoksa veya veri boşsa boş string döner. Program çökmez.
std::string SafeGetPathFromItem(Fl_Tree_Item* item) {
    if (!item) return "";
    void* data = item->user_data();
    if (!data) return "";
    return std::string(static_cast<char*>(data));
}

// Tree Item'ına veri atarken güvenli kopyalama yapar.
void SafeSetPathToItem(Fl_Tree_Item* item, const std::string& path) {
    if (!item) return;
    // strdup belleği heap'te ayırır. (Not: Basitlik için free etmiyoruz,
    // modern C++'ta bu memory leak sayılır ama segfault'tan iyidir.)
    item->user_data(strdup(path.c_str()));
    // modern yol
    // item->user_data(new std::string(path));
    // silerken de 
    // delete static_cast<std::string*>(item->user_data());

}

// =========================================================
// GÜNCELLENMİŞ DOSYA YÜKLEME
// =========================================================
void LoadFileToEditor(const std::string& path) {
    if (!fs::exists(path)) return;
    
    // 1. Dosyayı yükle
    int result = app.textBuf->loadfile(path.c_str());
    
    if (result == 0) {
        // 2. Yükleme başarılıysa hemen renklendirme fonksiyonunu çağır!
        ParseAndColorize();
    } else {
        fl_alert("Dosya okunamadı: %s", path.c_str());
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

// Yeni dosya yüklenince stilleri sıfırla
void ResetEditorStyles() {
    // Callback'i geçici durdur (sonsuz döngü olmasın)
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
    
    // Callback'i geri aç
    app.textBuf->add_modify_callback(EditorStyleUpdateCB, nullptr);
}



// =========================================================
// 4. DOSYA SİSTEMİ VE AĞAÇ MANTIĞI
// =========================================================

// Ağacı belirtilen yolla doldurur. Hata olursa çökmez.
void PopulateFileTree(const std::string& rootPath) {
    if (!app.tree) return;

    app.tree->clear();
    
    // Kök klasör ayarları
    fs::path p(rootPath);
    app.tree->root_label(p.filename().c_str());
    SafeSetPathToItem(app.tree->root(), rootPath);

    // Dosyaları tara
    try {
        // recursive_directory_iterator yerine hatalara karşı daha toleranslı bir yapı
        // skip_permission_denied: İzin verilmeyen dosyaları atla (Segfault nedeni #1)
        auto options = fs::directory_options::skip_permission_denied;
        
        for (const auto& entry : fs::recursive_directory_iterator(rootPath, options)) {
            try {
                std::string fullPath = entry.path().string();
                std::string relativePath = fs::relative(entry.path(), rootPath).string();
                
                // FLTK Tree'ye ekle
                Fl_Tree_Item* item = app.tree->add(relativePath.c_str());
                
                if (item) {
                    SafeSetPathToItem(item, fullPath);
                    
                    // Klasörse kapat (UI temizliği için)
                    if (entry.is_directory()) {
                        item->close();
                    }
                }
            } catch (const std::exception& e) {
                // Tekil dosya hatasını yut, döngüyü kırma
                std::cerr << "Dosya atlandı: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& ex) {
        fl_alert("Klasör taranırken kritik hata: %s", ex.what());
    }
    
    app.tree->redraw();
}

// =========================================================
// 5. MENÜ AKSİYONLARI (Create, Delete)
// =========================================================

std::string GetSelectedParentPath() {
    Fl_Tree_Item* item = app.tree->first_selected_item();
    std::string path = SafeGetPathFromItem(item);
    
    if (path.empty()) return app.currentRoot;
    
    // Eğer dosya seçiliyse, onun bulunduğu klasörü döndür
    if (!fs::is_directory(path)) {
        return fs::path(path).parent_path().string();
    }
    return path;
}

void ActionCreateFile() {
    std::string parent = GetSelectedParentPath();
    const char* name = fl_input("Yeni dosya adı:", "yeni.txt");
    
    if (name) {
        fs::path p = fs::path(parent) / name;
        std::ofstream outfile(p); // Dosyayı oluştur
        outfile.close();
        PopulateFileTree(app.currentRoot); // Yenile
    }
}

void ActionCreateDir() {
    std::string parent = GetSelectedParentPath();
    const char* name = fl_input("Yeni klasör adı:", "YeniKlasor");
    
    if (name) {
        fs::path p = fs::path(parent) / name;
        try {
            fs::create_directory(p);
            PopulateFileTree(app.currentRoot);
        } catch (const std::exception& e) {
            fl_alert("Klasör oluşturulamadı: %s", e.what());
        }
    }
}

void ActionDelete() {
    Fl_Tree_Item* item = app.tree->first_selected_item();
    std::string path = SafeGetPathFromItem(item);
    
    if (path.empty() || path == app.currentRoot) {
        fl_message("Silmek için geçerli bir alt öğe seçmelisin.");
        return;
    }

    if (fl_choice("EMİN MİSİN?\nBu işlem geri alınamaz:\n%s", "İptal", "SİL", 0, path.c_str()) == 1) {
        try {
            fs::remove_all(path);
            PopulateFileTree(app.currentRoot);
        } catch (const std::exception& e) {
            fl_alert("Silme hatası: %s", e.what());
        }
    }
}

// =========================================================
// 6. UI CALLBACKS (OLAY YÖNETİMİ)
// =========================================================

// Sağ Tık Menüsü Yönlendiricisi
void ContextMenuCB(Fl_Widget*, void* v) {
    long choice = (long)v;
    switch(choice) {
        case 1: ActionCreateFile(); break;
        case 2: ActionCreateDir(); break;
        case 3: ActionDelete(); break;
    }
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

// Klasör Seç Butonu
void SelectFolderBtnCB(Fl_Widget*, void*) {
    const char* dir = fl_dir_chooser("Çalışma Klasörünü Seç", app.currentRoot.c_str(), 0);
    if (dir) {
        app.currentRoot = dir;
        PopulateFileTree(app.currentRoot);
        
        // Pencere başlığını güncelle
        if (app.tree->window()) {
            std::string title = "Güvenli IDE - " + app.currentRoot;
            app.tree->window()->copy_label(title.c_str());
        }
    }
}

// =========================================================
// 7. MAIN (KURULUM)
// =========================================================

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



size_t utf8_width(const std::string& str) {
    size_t len = 0;
    for (size_t i = 0; i < str.length(); ++i) {
        // UTF-8'de devam baytları 10xxxxxx formatındadır (0x80 ile 0xBF arası)
        // Bunları saymayarak sadece karakter başlangıçlarını sayıyoruz.
        if ((str[i] & 0xC0) != 0x80) {
            len++;
        }
    }
    return len;
}

// Bir satırın tablo parçası olup olmadığını kontrol et
bool IsTableLine(const char* text) {
    // Boşlukları atla, ilk karakter '|' mi bak
    while (*text == ' ' || *text == '\t') text++;
    return (*text == '|');
}


// Tablonun başlangıç ve bitiş satırlarını bul
void GetTableBounds(int cursorPos, int& startRow, int& endRow) {
    int currentLineStart = app.textBuf->line_start(cursorPos);
    
    // Yukarı doğru tara
    startRow = currentLineStart;
    while (startRow > 0) {
        int prevLineStart = app.textBuf->line_start(startRow - 1);
        char* line = app.textBuf->text_range(prevLineStart, startRow - 1); // Satırı al
        bool isTable = IsTableLine(line);
        free(line);
        if (!isTable) break;
        startRow = prevLineStart;
    }

    // Aşağı doğru tara
    endRow = app.textBuf->line_end(cursorPos);
    int length = app.textBuf->length();
    while (endRow < length) {
        int nextLineEnd = app.textBuf->line_end(endRow + 1);
        int nextLineStart = app.textBuf->line_start(endRow + 1);
        char* line = app.textBuf->text_range(nextLineStart, nextLineEnd);
        bool isTable = IsTableLine(line);
        free(line);
        if (!isTable) break;
        endRow = nextLineEnd;
    }
}

// Tabloyu hizalar (Formatter Core)
void FormatTableAt(int pos) {
    int startPos, endPos;
    GetTableBounds(pos, startPos, endPos);

    // Tablo değilse çık
    char* check = app.textBuf->text_range(startPos, app.textBuf->line_end(startPos));
    if (!IsTableLine(check)) { free(check); return; }
    free(check);

    // 1. Tabloyu Oku ve Parse Et
    char* rawTable = app.textBuf->text_range(startPos, endPos);
    std::stringstream ss(rawTable);
    std::string line;
    std::vector<std::vector<std::string>> rows;
    std::vector<bool> isSeparatorRow;
    
    // Max genişlikleri tut
    std::vector<size_t> colWidths;

    while (std::getline(ss, line)) {
        // Trim leading/trailing whitespace
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue; // Boş satır
        
        std::vector<std::string> cells;
        bool isSep = true; // Bu satır sadece separator mı? (|-...-|)
        
        // Pipe'lara göre böl
        size_t pipePos = line.find('|', first);
        size_t prevPipe = pipePos;
        
        while (pipePos != std::string::npos) {
            size_t nextPipe = line.find('|', pipePos + 1);
            if (nextPipe == std::string::npos) break;

            std::string content = line.substr(pipePos + 1, nextPipe - pipePos - 1);
            
            // Trim content
            size_t cFirst = content.find_first_not_of(" \t");
            size_t cLast = content.find_last_not_of(" \t");
            
            if (cFirst == std::string::npos) {
                content = "";
            } else {
                content = content.substr(cFirst, cLast - cFirst + 1);
            }

            // Separator kontrolü (sadece - ve + içeriyorsa)
            if (content.find_first_not_of("-+") != std::string::npos) {
                isSep = false;
            }

            cells.push_back(content);
            pipePos = nextPipe;
        }
        
        rows.push_back(cells);
        isSeparatorRow.push_back(isSep);

        for (size_t i = 0; i < cells.size(); i++) {
            if (i >= colWidths.size()) colWidths.push_back(0);
            if (!isSep) {
                // std::string::length() YERİNE utf8_width() kullanıyoruz
                size_t w = utf8_width(cells[i]);
                if (w > colWidths[i]) colWidths[i] = w;
            }
        }

    }
    free(rawTable);

    // --- DEĞİŞİKLİK BURADA: Genişlik Hesaplama ---
    

    // 2. Tabloyu Yeniden Oluştur (Reconstruct)
    std::string newTable;
    for (size_t r = 0; r < rows.size(); r++) {
        newTable += "|";
        for (size_t c = 0; c < rows[r].size(); c++) {
            if (isSeparatorRow[r]) {
                newTable += std::string(colWidths[c] + 2, '-'); 
            } else {
                newTable += " " + rows[r][c];
                
                // --- DEĞİŞİKLİK BURADA: Padding Hesabı ---
                // UTF-8 genişliğine göre boşluk ekle
                size_t currentLen = utf8_width(rows[r][c]);
                if (colWidths[c] > currentLen) {
                    newTable += std::string(colWidths[c] - currentLen, ' ');
                }
                newTable += " ";
            }
            newTable += "|";
        }
        newTable += "\n";
    }
// Son \n'i sil (editörde fazladan satır açmasın)
    if (!newTable.empty()) newTable.pop_back();

    // 3. Buffer'ı Güncelle
    // TODO: Reentrancy riskli Cursor pozisyonu kayabilir Undo stack bozulur (ileride) Doğru mimari: TextBuffer değişimini transaction gibi ele al Tek noktadan “document mutation” API’si yaz
    // Text buffer değişimi callback'i tetikler, sonsuz döngüyü engellemeliyiz
    app.textBuf->remove_modify_callback(EditorStyleUpdateCB, nullptr);
    app.textBuf->replace(startPos, endPos, newTable.c_str());
    app.textBuf->add_modify_callback(EditorStyleUpdateCB, nullptr);
    // 4. Stilleri anında güncelle (Tablo bozuk görünmesin)
    ParseRange(startPos, startPos + newTable.length());
}



int main(int argc, char** argv) {
    // 1. ÖNCE GENEL TEMAYI YÜKLE
    SetDarkTheme(); 

    Fl_Double_Window* window = new Fl_Double_Window(1000, 600, "Güvenli FLTK Editör");

    // --- SOL PANEL (AĞAÇ) ---
    Fl_Button* btnOpen = new Fl_Button(10, 10, 100, 10, "@fileopen  Seç");
    btnOpen->callback(SelectFolderBtnCB);
    // Butonun yazı rengini manuel düzeltmek gerekebilir
    btnOpen->labelcolor(fl_rgb_color(184, 184, 184)); 

    app.tree = new Fl_Tree(10, 30, 500, 600);
    
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

    app.editor = new SmartEditor(520, 10, 770, 580);
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

    try {
        PopulateFileTree(app.currentRoot);
    } catch (...) {}

    app.textBuf->text("# Karanlık mod aktif.");
    ResetEditorStyles();

    window->resizable(app.editor);
    window->end();
    window->show(argc, argv);

    return Fl::run();
}
