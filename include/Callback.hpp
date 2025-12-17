#pragma once

#include <FL/Fl.H>

void SelectFolderBtnCB(Fl_Widget *, void *);
void EditorStyleUpdateCB(int pos, int nInserted, int nDeleted, int nRestyled,
                         const char *deletedText, void *cbArg);
void CursorMoveCB(Fl_Widget* w, void* data);
void ResetEditorStyles();
void TreeCB(Fl_Widget*, void*);
void CursorMoveCB(Fl_Widget *w, void *data);
void ContextMenuCB(Fl_Widget*, void* v);
