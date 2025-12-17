#include <cstdint>
#include <string>

#include "Utf8.hpp"
// =========================================================
// UNICODE WIDTH HESAPLAMA MOTORU (NO EXTERNAL LIB)
// Kaynak: Markus Kuhn's wcwidth implementation (Adapted for C++)
// =========================================================

// 1. Yardımcı: UTF-8 bayt dizisinden Codepoint (Sayısal Değer) çıkarır
uint32_t utf8_decode_next(const char*& p, const char* end) {
    if (p >= end) return 0;

    unsigned char c = *p;
    uint32_t codepoint = 0;
    int seq_len = 0;

    if (c < 0x80) { // 1 byte (ASCII)
        p++;
        return c;
    } 
    else if ((c & 0xE0) == 0xC0) { // 2 bytes
        codepoint = c & 0x1F;
        seq_len = 1;
    } 
    else if ((c & 0xF0) == 0xE0) { // 3 bytes
        codepoint = c & 0x0F;
        seq_len = 2;
    } 
    else if ((c & 0xF8) == 0xF0) { // 4 bytes
        codepoint = c & 0x07;
        seq_len = 3;
    } 
    else {
        p++; // Geçersiz bayt
        return 0xFFFD; // Replacement char
    }

    p++;
    for (int i = 0; i < seq_len; ++i) {
        if (p >= end) return 0; // Hata: String bitti
        unsigned char next = *p;
        if ((next & 0xC0) != 0x80) return 0xFFFD; // Hata: Geçersiz devam baytı
        codepoint = (codepoint << 6) | (next & 0x3F);
        p++;
    }
    return codepoint;
}

// 2. Yardımcı: Bir Codepoint'in genişliğini döndürür (0, 1 veya 2)
int mk_wcwidth(uint32_t ucs) {
    // 0 Genişlik (Control characters, Null, vb.)
    if (ucs == 0) return 0;
    if (ucs < 32 || (ucs >= 0x7F && ucs < 0xA0)) return 0; // Kontrol karakterlerini yoksay

    // Combining characters (Birleştirici İşaretler - Genişlik 0)
    // Bunlar önceki harfin üzerine biner, yer kaplamaz.
    if (ucs >= 0x0300 && ucs <= 0x036F) return 0; 
    // ... (Daha kapsamlı aralıklar eklenebilir ama temel batı dilleri için bu yeterli)

    // Wide characters (Geniş Karakterler - Genişlik 2)
    // CJK (Çince, Japonca, Korece), Fullwidth formlar ve Emojiler
    if (ucs >= 0x1100 && (
        (ucs <= 0x115F) ||                    // Hangul Jamo
        (ucs == 0x2329) || (ucs == 0x232A) ||
        (ucs >= 0x2E80 && ucs <= 0xA4CF && ucs != 0x303F) || // CJK ... Yi
        (ucs >= 0xAC00 && ucs <= 0xD7A3) ||   // Hangul Syllables
        (ucs >= 0xF900 && ucs <= 0xFAFF) ||   // CJK Compatibility Ideographs
        (ucs >= 0xFE10 && ucs <= 0xFE19) ||   // Vertical forms
        (ucs >= 0xFE30 && ucs <= 0xFE6F) ||   // CJK Compatibility Forms
        (ucs >= 0xFF00 && ucs <= 0xFF60) ||   // Fullwidth Forms
        (ucs >= 0xFFE0 && ucs <= 0xFFE6) ||
        (ucs >= 0x1F300 && ucs <= 0x1F64F) || // Miscellaneous Symbols and Pictographs (Emojiler)
        (ucs >= 0x1F900 && ucs <= 0x1F9FF)))  // Supplemental Symbols and Pictographs
    {
        return 2;
    }

    return 1; // Diğer her şey (Latin, Kiril, vb.)
}

// 3. ANA FONKSİYON: Stringin toplam görsel genişliğini hesaplar
size_t utf8_width(const std::string& str) {
    const char* p = str.data();
    const char* end = p + str.size();
    size_t width = 0;

    while (p < end) {
        uint32_t cp = utf8_decode_next(p, end);
        int w = mk_wcwidth(cp);
        if (w > 0) width += w;
    }
    return width;
}
