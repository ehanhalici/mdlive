#include <FL/Fl_Tree.H>

#include <filesystem>
#include <string>
#include <cstring>
#include "iostream"
#include "fstream"
#include "App.hpp"
#include "FileSystem.hpp"
#include "Parser.hpp"


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
