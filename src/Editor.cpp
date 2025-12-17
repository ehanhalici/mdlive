#include "Editor.hpp"

#include <cstddef>
#include <string>
#include <vector>
#include <cstring>
#include <sstream> // Bunu eklemelisin

#include "App.hpp"
#include "Callback.hpp"
#include "Parser.hpp"
#include "Utf8.hpp"

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

    // 2. Tabloyu Yeniden Oluştur (Reconstruct)
    std::string newTable;
    for (size_t r = 0; r < rows.size(); r++) {
        newTable += "|";
        for (size_t c = 0; c < rows[r].size(); c++) {
            if (isSeparatorRow[r]) {
                newTable += std::string(colWidths[c] + 2, '-'); 
            } else {
                newTable += " " + rows[r][c];
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


int SmartEditor::handle(int event) {
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

    // --- İMLEÇ HAREKETİ TETİKLEYİCİSİ ---
    int old_pos = insert_position();
    int result = Fl_Text_Editor::handle(event);
    int new_pos = insert_position();

    if ((event == FL_KEYBOARD || event == FL_PUSH || event == FL_FOCUS) && (old_pos != new_pos)) {
        do_callback(); 
    }

    return result;
}
