#pragma once

bool is_hr(char *text, int lineLen, char *style);
bool is_list(char *text, int lineLen, char *style);
bool codeBlog(char *text, int lineLen, char *style);
bool header(char *text, int lineLen, char *style);
bool quote(char *text, int lineLen, char *style);
int inline_code(char *text, int remainingLen, char *style);
int image(char *text, int remainingLen, char *style);
int internal_link(char *text, int remainingLen, char *style);
int external_link(char *text, int remainingLen, char *style);
int bold(char *text, int lineLen, char *style);
int italic(char *text, int remainingLen, char *style);
int strikethrough(char *text, int remainingLen, char *style);

bool startsWith(char *text, int remaining, const char *pattern);
int scan_bold(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_italic(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_highlight(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_strike(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_math(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_internal_link(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_external_link(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_wikilink(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_inline_code(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_tag(char *text, int maxLen, char *style);
int scan_comment(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_footnote(char *text, int maxLen, char *style, bool isCurrentLine);
int scan_image_extended(char *text, int maxLen, char *style, bool isCurrentLine);

int scan_embed_wikilink(char* text, int maxLen, char* style, bool isCurrentLine);
int scan_block_id(char* text, int maxLen, char* style);
int scan_bare_url(char *text, int maxLen, char *style);
int scan_tag_advanced(char* text, int maxLen, char* style);

