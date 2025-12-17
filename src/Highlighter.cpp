#include <cstddef>
#include <cstring>

#include <cstring>
#include <cctype>
#include "Highlighter.hpp"

// Yatay Çizgi (--- veya ***)
bool is_hr(char *text, int lineLen, char *style) {
    if (lineLen < 3) return false;
    char c = text[0];
    if (c != '-' && c != '*' && c != '_') return false;
    
    for (int i = 0; i < lineLen; i++) {
        if (text[i] != c && text[i] != ' ') return false;
    }
    memset(style, 'H', lineLen); // 'H' stilini tanımlamalısın
    return true;
}

// Liste (- item veya 1. item)
bool is_list(char *text, int lineLen, char *style) {
    if (lineLen < 2) return false;
    
    // Unordered List (-, *, +)
    if ((text[0] == '-' || text[0] == '*' || text[0] == '+') && text[1] == ' ') {
        style[0] = 'L'; // Madde imini boya
        return true; // Satırın geri kalanını parse etmeye devam et (return true deyip loop'tan çıkmıyoruz, main loop'ta mantığı ayarlayacağız)
    }
    
    // Ordered List (1. )
    if (isdigit(text[0])) {
        int i = 0;
        while (i < lineLen && isdigit(text[i])) i++;
        if (i < lineLen && text[i] == '.' && (i+1 < lineLen && text[i+1] == ' ')) {
            memset(style, 'L', i + 1); // "12." kısmını boya
            return true;
        }
    }
    return false;
}

// Kod Bloğu (State korumalı)
bool codeBlog(char* text, int lineLen, char* style) {
    static bool inCodeBlock = false;

    // Reset sinyali
    if (text == NULL) { inCodeBlock = false; return false; }
    
    // ``` kontrolü (Başlangıç veya Bitiş)
    if (lineLen >= 3 && startsWith(text, lineLen, "```")) {
        inCodeBlock = !inCodeBlock;
        memset(style, 'H', lineLen);
        return true;
    }

    if (inCodeBlock) {
        memset(style, 'C', lineLen);
        return true;
    }
    return false;
}

bool yaml(char* text, int lineLen, char* style) {
    static bool inYaml = false;

    if (text == NULL){inYaml = false; return false;}
    if (lineLen >= 3 && startsWith(text, lineLen, "---")) {
        inYaml = !inYaml;
        memset(style, 'P', lineLen);
        return true;
    }

    if (inYaml) {
        memset(style, 'C', lineLen);
        return true;
    }
    return false;

    
}

bool header(char *text, int lineLen, char *style, bool isCurrentLine) {
    if (lineLen > 0 && text[0] == '#') {
        int hashCount = 0;
        while (hashCount < lineLen && text[hashCount] == '#') hashCount++;
        
        if (hashCount < lineLen && text[hashCount] == ' ') {
            if (isCurrentLine) {
                memset(style, 'H', hashCount);
                memset(style + hashCount, 'B', lineLen - hashCount);
            } else {
                memset(style, 'G', hashCount);
                memset(style + hashCount, 'B', lineLen - hashCount);
            }
            return true;
          }
    }
    return false;
}

bool horizontal(char *text, int lineLen, char *style, bool isCurrentLine) {
    //  (--- veya ***)
    if (lineLen >= 3) {
        bool isHR = true;
        char c = text[0];
        if (c == '-' || c == '*') {
            for(int z=0; z<lineLen; z++) if(text[z] != c && text[z] != ' ') isHR = false;
            if (isHR) {
                if (isCurrentLine) memset(style, 'H', lineLen);
                else memset(style, 'G', lineLen); 
                return true;
            }
        }
    }
    return false;
}

int taskList(char *text, int lineLen, char *style, bool isCurrentLine){
    // Task List (- [ ] veya - [x])
    int tOffset = 0;
    while(tOffset < lineLen && text[tOffset] == ' ') tOffset++;
    bool isTask = false;

    if (lineLen - tOffset >= 5 && 
        (text[tOffset] == '-' || text[tOffset] == '*') && 
        text[tOffset+1] == ' ' && text[tOffset+2] == '[') {
            
        if (text[tOffset+4] == ']') {
            isTask = true; // Task olduğunu işaretle
            char checkChar = text[tOffset+3];
            int afterBracket = tOffset + 5;
            bool isChecked = (checkChar == 'x' || checkChar == 'X');

            if (isCurrentLine) {
                memset(style, 'H', afterBracket);
                if (isChecked) style[tOffset+3] = 'X'; 
            } else {
                memset(style, 'G', tOffset + 2); 
                if (isChecked) {
                    memset(style + tOffset + 2, 'X', 3); 
                    memset(style + afterBracket, 'Z', lineLen - afterBracket); 
                } else {
                    memset(style + tOffset + 2, 'L', 3); 
                }
            }
        }
    }

    if (isTask) return tOffset;
    return 0;

}
// Alıntı (> Quote)
bool quote(char *text, int lineLen, char *style) {
    if (lineLen > 0 && text[0] == '>') {
        memset(style, 'D', lineLen);
        return true;
    }
    return false;
}

// --- SATIR İÇİ (INLINE) FONKSİYONLAR ---
// Bu fonksiyonlar eşleşen uzunluğu (length) döndürür, yoksa 0.

// Satır İçi Kod (`code`)
int inline_code(char *text, int remainingLen, char *style) {
    if (remainingLen > 2 && text[0] == '`') {
        int k = 1;
        while (k < remainingLen && text[k] != '`') k++;
        if (k < remainingLen) { // Kapanış bulundu
            memset(style, 'c', k + 1); // 'c' küçük harf (inline code stili)
            return k + 1;
        }
    }
    return 0;
}

// Resim (![alt](url))
int image(char *text, int remainingLen, char *style) {
    if (remainingLen > 4 && text[0] == '!' && text[1] == '[') {
        int k = 2;
        while (k < remainingLen && text[k] != ']') k++; // ] bul
        if (k < remainingLen && text[k+1] == '(') { // hemen sonra ( var mı?
            int p = k + 2;
            while (p < remainingLen && text[p] != ')') p++; // ) bul
            if (p < remainingLen) {
                // Hepsini resim rengine boya veya link kısmını gizle
                memset(style, 'J', p + 1); 
                return p + 1;
            }
        }
    }
    return 0;
}

// Link ([text](url))
int internal_link(char *text, int remainingLen, char *style) {
    if (remainingLen > 3 && text[0] == '[') {
        int k = 1;
        while (k < remainingLen && text[k] != ']') k++;
        if (k < remainingLen && text[k+1] == '(') {
            int p = k + 2;
            while (p < remainingLen && text[p] != ')') p++;
            if (p < remainingLen) {
                memset(style, 'F', p + 1);
                return p + 1;
            }
        }
    }
    return 0;
}

// Link ([text][url])
int external_link(char *text, int remainingLen, char *style) {
    if (remainingLen > 3 && text[0] == '[') {
        int k = 1;
        while (k < remainingLen && text[k] != ']') k++;
        if (k < remainingLen && text[k+1] == '[') {
            int p = k + 2;
            while (p < remainingLen && text[p] != ']') p++;
            if (p < remainingLen) {
                memset(style, 'F', p + 1);
                return p + 1;
            }
        }
    }
    return 0;
}

// Kalın (**bold**)
int bold(char *text, int remainingLen, char *style) {
    if (remainingLen > 4 && text[0] == '*' && text[1] == '*') {
        int k = 2;
        while (k + 1 < remainingLen && !(text[k] == '*' && text[k+1] == '*')) k++;
        if (k + 1 < remainingLen) {
            memset(style, 'E', k + 2);
            return k + 2;
        }
    }
    // Alt çizgi versiyonu (__bold__)
    if (remainingLen > 4 && text[0] == '_' && text[1] == '_') {
        int k = 2;
        while (k + 1 < remainingLen && !(text[k] == '_' && text[k+1] == '_')) k++;
        if (k + 1 < remainingLen) {
            memset(style, 'E', k + 2);
            return k + 2;
        }
    }
    return 0;
}

// İtalik (*italic* veya _italic_)
int italic(char *text, int remainingLen, char *style) {
    if (remainingLen > 2 && (text[0] == '*' || text[0] == '_')) {
        char marker = text[0];
        int k = 1;
        while (k < remainingLen && text[k] != marker) k++;
        if (k < remainingLen && k > 1) { // Boş olmayan (** değil)
            // Eğer *'ın hemen yanında bir tane daha * varsa bu Bold'dur, İtalik değil.
            // Ama biz zaten Bold'u önce kontrol edeceğiz, o yüzden burası güvenli.
            memset(style, 'I', k + 1);
            return k + 1;
        }
    }
    return 0;
}

// Üstü Çizili (~~strike~~)
int strikethrough(char *text, int remainingLen, char *style) {
    if (remainingLen > 4 && text[0] == '~' && text[1] == '~') {
        int k = 2;
        while (k + 1 < remainingLen && !(text[k] == '~' && text[k+1] == '~')) k++;
        if (k + 1 < remainingLen) {
            memset(style, 'S', k + 2);
            return k + 2;
        }
    }
    return 0;
}









// --- YARDIMCI: KARŞILAŞTIRMA ---
bool startsWith(char* text, int remaining, const char* pattern) {
    int len = strlen(pattern);
    if (remaining < len) return false;
    return strncmp(text, pattern, len) == 0;
}

// 1. BOLD (**...**)
int scan_bold(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4 || !startsWith(text, maxLen, "**")) return 0;
    
    int k = 2;
    while (k + 1 < maxLen && !startsWith(text + k, maxLen - k, "**")) k++;
    
    if (k + 1 < maxLen) { // Kapanış bulundu
        int totalLen = k + 2;
        if (isCurrentLine) {
            // Aktif Satır: ** (Silik), İçerik (Kalın), ** (Silik)
            memset(style, 'S', 2);              // **
            memset(style + 2, 'E', totalLen - 4); // İçerik
            memset(style + totalLen - 2, 'S', 2); // **
        } else {
            // Pasif Satır: ** (Gizli), İçerik (Kalın)
            memset(style, 'G', 2);
            memset(style + 2, 'E', totalLen - 4);
            memset(style + totalLen - 2, 'G', 2);
        }
        return totalLen;
    }
    return 0;
}

// 2. ITALIC (*...* veya _..._)
int scan_italic(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 3) return 0;
    char marker = text[0];
    if (marker != '*' && marker != '_') return 0;
    // Bold ile karışmasın diye kontrol (** veya __ değilse)
    if (text[1] == marker) return 0; 

    int k = 1;
    while (k < maxLen && text[k] != marker) k++;
    
    if (k < maxLen) {
        int totalLen = k + 1;
        if (isCurrentLine) {
            style[0] = 'S'; // Syntax
            memset(style + 1, 'I', totalLen - 2);
            style[totalLen - 1] = 'S';
        } else {
            style[0] = 'G'; // Gizli
            memset(style + 1, 'I', totalLen - 2);
            style[totalLen - 1] = 'G';
        }
        return totalLen;
    }
    return 0;
}

// 3. HIGHLIGHT (==...==)
int scan_highlight(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4 || !startsWith(text, maxLen, "==")) return 0;
    
    int k = 2;
    while (k + 1 < maxLen && !startsWith(text + k, maxLen - k, "==")) k++;
    
    if (k + 1 < maxLen) {
        int totalLen = k + 2;
        // Not: FLTK stil tablosunda arka plan rengi ("Highlight") desteği sınırlıdır.
        // Genellikle yazı rengini değiştirerek simüle ederiz (Örn: Turuncu 'O')
        char contentStyle = 'O'; 
        
        if (isCurrentLine) {
            memset(style, 'S', 2);
            memset(style + 2, contentStyle, totalLen - 4);
            memset(style + totalLen - 2, 'S', 2);
        } else {
            memset(style, 'G', 2);
            memset(style + 2, contentStyle, totalLen - 4);
            memset(style + totalLen - 2, 'G', 2);
        }
        return totalLen;
    }
    return 0;
}

// 4. STRIKETHROUGH (~~...~~)
int scan_strike(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4 || !startsWith(text, maxLen, "~~")) return 0;
    
    int k = 2;
    while (k + 1 < maxLen && !startsWith(text + k, maxLen - k, "~~")) k++;
    
    if (k + 1 < maxLen) {
        int totalLen = k + 2;
        if (isCurrentLine) {
            memset(style, 'S', 2);
            memset(style + 2, 'Z', totalLen - 4);
            memset(style + totalLen - 2, 'S', 2);
        } else {
            memset(style, 'G', 2);
            memset(style + 2, 'Z', totalLen - 4);
            memset(style + totalLen - 2, 'G', 2);
        }
        return totalLen;
    }
    return 0;
}

// 5. MATH ($...$ veya $$...$$)
int scan_math(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 3 || text[0] != '$') return 0;
    
    bool doubleDollar = (maxLen > 4 && text[1] == '$');
    int startOffset = doubleDollar ? 2 : 1;
    
    int k = startOffset;
    while (k < maxLen) {
        if (doubleDollar) {
            if (startsWith(text + k, maxLen - k, "$$")) break;
        } else {
            if (text[k] == '$') break;
        }
        k++;
    }
    
    if (k < maxLen) {
        int totalLen = k + startOffset;
        if (isCurrentLine) {
            memset(style, 'S', startOffset);
            memset(style + startOffset, 'J', totalLen - (2 * startOffset));
            memset(style + totalLen - startOffset, 'S', startOffset);
        } else {
            memset(style, 'G', startOffset);
            memset(style + startOffset, 'J', totalLen - (2 * startOffset));
            memset(style + totalLen - startOffset, 'G', startOffset);
        }
        return totalLen;
    }
    return 0;
}

// 6. LINKS ([text](url))
int scan_internal_link(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4 || text[0] != '[') return 0;
    
    int k = 1;
    while (k < maxLen && text[k] != ']') k++; // ] bul
    if (k >= maxLen || text[k+1] != '(') return 0; // ]( yoksa çık
    
    int p = k + 2;
    while (p < maxLen && text[p] != ')') p++; // ) bul
    
    if (p < maxLen) {
        int totalLen = p + 1;
        int textLen = k - 1; // [ hariç
        int urlLen = p - (k + 2); // ( hariç
        
        if (isCurrentLine) {
            style[0] = 'S'; // [
            memset(style + 1, 'F', textLen); // Text
            memset(style + k, 'S', 2); // ](
            memset(style + k + 2, 'S', urlLen); // URL (Silik)
            style[p] = 'S'; // )
        } else {
            style[0] = 'G'; // [ (Gizli)
            memset(style + 1, 'F', textLen); // Text (Mavi)
            // Geri kalan her şeyi (](url)) gizle
            memset(style + k, 'G', totalLen - k);
        }
        return totalLen;
    }
    return 0;
}


// 6. LINKS ([text][url])
int scan_external_link(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4 || text[0] != '[') return 0;
    
    int k = 1;
    while (k < maxLen && text[k] != ']') k++; // ] bul
    if (k >= maxLen || text[k+1] != '[') return 0; // ]( yoksa çık
    
    int p = k + 2;
    while (p < maxLen && text[p] != ']') p++; // ) bul
    
    if (p < maxLen) {
        int totalLen = p + 1;
        int textLen = k - 1; // [ hariç
        int urlLen = p - (k + 2); // ( hariç
        
        if (isCurrentLine) {
            style[0] = 'S'; // [
            memset(style + 1, 'F', textLen); // Text
            memset(style + k, 'S', 2); // ](
            memset(style + k + 2, 'S', urlLen); // URL (Silik)
            style[p] = 'S'; // )
        } else {
            style[0] = 'G'; // [ (Gizli)
            memset(style + 1, 'F', textLen); // Text (Mavi)
            // Geri kalan her şeyi (](url)) gizle
            memset(style + k, 'G', totalLen - k);
        }
        return totalLen;
    }
    return 0;
}


// 7. WIKILINK ([[Link|Label]] veya [[Link]])
int scan_wikilink(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 5 || !startsWith(text, maxLen, "[[")) return 0;
    
    int k = 2;
    while (k + 1 < maxLen && !startsWith(text + k, maxLen - k, "]]")) k++;
    
    if (k + 1 < maxLen) {
        int totalLen = k + 2;
        
        // '|' var mı diye bak (Label kontrolü)
        int pipePos = -1;
        for (int i = 2; i < k; i++) {
            if (text[i] == '|') { pipePos = i; break; }
        }
        
        if (isCurrentLine) {
            // Aktif: [[Gri|Gri]]
            memset(style, 'S', 2); // [[
            style[totalLen - 2] = 'S'; // ]
            style[totalLen - 1] = 'S'; // ]
            
            if (pipePos != -1) {
                // Link|Label
                memset(style + 2, 'W', pipePos - 2); // Link
                style[pipePos] = 'S'; // |
                memset(style + pipePos + 1, 'W', k - pipePos - 1); // Label
            } else {
                memset(style + 2, 'W', k - 2);
            }
        } else {
            // Pasif: [[ (Gizli) ... ]] (Gizli)
            if (pipePos != -1) {
                // [[Link|Label]] -> Sadece Label göster
                memset(style, 'G', pipePos + 1); // [[Link| kısmını gizle
                memset(style + pipePos + 1, 'W', k - pipePos - 1); // Label göster
                memset(style + k, 'G', 2); // ]] gizle
            } else {
                // [[Link]] -> Sadece Link göster
                memset(style, 'G', 2);
                memset(style + 2, 'W', k - 2);
                memset(style + k, 'G', 2);
            }
        }
        return totalLen;
    }
    return 0;
}

// 8. INLINE CODE (`...`)
int scan_inline_code(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 3 || text[0] != '`') return 0;
    int k = 1;
    while (k < maxLen && text[k] != '`') k++;
    
    if (k < maxLen) {
        int totalLen = k + 1;
        if (isCurrentLine) {
            memset(style, 'C', totalLen);
        } else {
            style[0] = 'G'; // `
            memset(style + 1, 'C', totalLen - 2);
            style[totalLen - 1] = 'G'; // `
        }
        return totalLen;
    }
    return 0;
}

// 9. TAGS (#Tag)
int scan_tag(char* text, int maxLen, char* style) {
    if (maxLen < 2 || text[0] != '#') return 0;
    // Tag'den önce boşluk olması gerektiği kontrolünü main loopta yaparız
    
    int k = 1;
    while (k < maxLen && (isalnum(text[k]) || text[k] == '_' || text[k] == '-')) k++;
    
    if (k > 1) { // # tek başına tag değildir
        memset(style, 'W', k);
        return k;
    }
    return 0;
}


// --- YENİ EKLENTİLER (OBSIDIAN EXTENDED) ---

// 1. OBSIDIAN COMMENTS (%% ... %%)
// Bu yorumlar "Preview" modunda tamamen gizlenir.
int scan_comment(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4 || !startsWith(text, maxLen, "%%")) return 0;
    
    int k = 2;
    while (k + 1 < maxLen && !startsWith(text + k, maxLen - k, "%%")) k++;
    
    if (k + 1 < maxLen) {
        int totalLen = k + 2;
        if (isCurrentLine) {
            // Aktif satır: Silik şekilde göster (Yazarın görmesi lazım)
            memset(style, 'S', totalLen); 
        } else {
            // Pasif satır: TAMAMEN GİZLE (Obsidian davranışı)
            memset(style, 'G', totalLen);
        }
        return totalLen;
    }
    return 0;
}

// 2. FOOTNOTES ([^1] veya ^[text])
int scan_footnote(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 4) return 0;
    
    // Tip A: [^Ref]
    if (text[0] == '[' && text[1] == '^') {
        int k = 2;
        while (k < maxLen && text[k] != ']') k++;
        if (k < maxLen) {
            int totalLen = k + 1;
            if (isCurrentLine) {
                style[0] = 'S'; // [
                style[1] = 'S'; // ^
                memset(style + 2, 'F', totalLen - 3); // Ref (Link rengi)
                style[totalLen - 1] = 'S'; // ]
            } else {
                style[0] = 'G'; // [
                style[1] = 'S'; // ^ (Küçük bir işaret kalsın)
                memset(style + 2, 'F', totalLen - 3); 
                style[totalLen - 1] = 'G'; // ]
            }
            return totalLen;
        }
    }
    return 0;
}

// 3. EXTERNAL IMAGE RESIZING (![Alt|100](url))
// Mevcut scan_link/image fonksiyonunu güncellemek yerine bunu ekleyebiliriz.
// (Mevcut scan_link standart markdown içindir, bu pipe desteği ekler)
// Not: Wikilink zaten pipe destekliyor, bu standart resimler için.
int scan_image_extended(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 5 || text[0] != '!' || text[1] != '[') return 0;
    
    int k = 2; 
    while (k < maxLen && text[k] != ']') k++; // ] bul
    if (k >= maxLen || text[k+1] != '(') return 0;
    
    int p = k + 2;
    while (p < maxLen && text[p] != ')') p++; // ) bul
    
    if (p < maxLen) {
        int totalLen = p + 1;
        if (isCurrentLine) {
            memset(style, 'S', totalLen); // Komple silik göster
        } else {
            // ![Alt|100] kısmındaki '!' işaretini Turuncu yap
            style[0] = 'O'; 
            memset(style + 1, 'S', k); // [Alt|100]
            memset(style + k + 1, 'G', totalLen - (k + 1)); // (url) gizle
        }
        return totalLen;
    }
    return 0;
}


// --- YENİ OBSIDIAN ÖZELLİKLERİ ---

// 1. EMBED (TRANSCLUSION): ![[Note]]
// WikiLink ile karışmaması için ! ile başlıyorsa bunu çağıracağız.
int scan_embed_wikilink(char* text, int maxLen, char* style, bool isCurrentLine) {
    if (maxLen < 5 || !startsWith(text, maxLen, "![[")) return 0;

    int k = 3;
    while (k + 1 < maxLen && !startsWith(text + k, maxLen - k, "]]")) k++;

    if (k + 1 < maxLen) {
        int totalLen = k + 2;
        if (isCurrentLine) {
            memset(style, 'H', totalLen); // ![[...]] Silik
        } else {
            // Pasif: ![[ (Gizle), İçerik (Altın Sarısı), ]] (Gizle)
            memset(style, 'G', 3); 
            memset(style + 3, 'Q', totalLen - 5); // İçerik 'Q' stili
            memset(style + totalLen - 2, 'G', 2);
        }
        return totalLen;
    }
    return 0;
}

// 2. BLOCK ID: ^blockid (Satır sonunda olur)
// Örnek: Bu bir paragraftır. ^ref123
int scan_block_id(char* text, int maxLen, char* style) {
    if (maxLen < 2 || text[0] != '^') return 0;
    
    // Sadece alfanümerik karakterler
    int k = 1;
    while (k < maxLen && (isalnum(text[k]) || text[k] == '-')) k++;
    
    if (k > 1) {
        // Block ID'leri genelde çok silik gösterilir
        memset(style, 'H', k); 
        return k;
    }
    return 0;
}

// 3. BARE URL: https://google.com (Köşeli parantezsiz)
int scan_bare_url(char* text, int maxLen, char* style) {
    if (maxLen < 8) return 0;
    
    // http:// veya https:// kontrolü
    bool isHttp = startsWith(text, maxLen, "http://");
    bool isHttps = startsWith(text, maxLen, "https://");
    
    if (!isHttp && !isHttps) return 0;
    
    int k = 0;
    // URL'nin bittiği yere kadar git (boşluk, satır sonu veya parantez gelene kadar)
    while (k < maxLen && !isspace(text[k]) && text[k] != ')' && text[k] != ']') k++;
    
    if (k > 0) {
        memset(style, 'R', k); // 'R' stili (Mavi)
        return k;
    }
    return 0;
}

// 4. GELİŞMİŞ TAG: #tag/sub-tag
// Mevcut scan_tag fonksiyonunu bununla değiştir.
int scan_tag_advanced(char* text, int maxLen, char* style) {
    if (maxLen < 2 || text[0] != '#') return 0;
    
    int k = 1;
    // Obsidian tagleri: harf, sayı, alt çizgi, tire ve slash (/) içerebilir
    while (k < maxLen && (isalnum(text[k]) || text[k] == '_' || text[k] == '-' || text[k] == '/')) k++;
    
    // # tek başına tag değildir ve #123 (sadece sayı) tag değildir (Obsidian kuralı)
    // Ama basitlik için sadece uzunluğa bakıyoruz.
    if (k > 1) {
        memset(style, 'K', k); // WikiLink rengiyle aynı (Genelde mor/lila)
        return k;
    }
    return 0;
}
