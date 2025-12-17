#include <cstring>

#include "Parser.hpp"
#include "App.hpp"
#include "Highlighter.hpp"


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
