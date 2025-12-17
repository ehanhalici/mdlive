#pragma once
#include <FL/Fl_Tree.H>

#include <filesystem>
#include <string>
#include <cstring>

namespace fs = std::filesystem;

void PopulateFileTree(const std::string& rootPath);
void LoadFileToEditor(const std::string &path);
void ActionCreateFile();
void ActionCreateDir();
void ActionDelete();
std::string SafeGetPathFromItem(Fl_Tree_Item* item);
void SafeSetPathToItem(Fl_Tree_Item *item, const std::string &path);
std::string GetSelectedParentPath();
