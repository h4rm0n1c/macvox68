#ifndef UI_THEME_H
#define UI_THEME_H

#include <Quickdraw.h>

typedef struct UIThemeColors
{
    RGBColor windowFill;
    RGBColor separatorDark;
    RGBColor separatorLight;
    RGBColor groupFill;
    RGBColor groupBorder;
    RGBColor groupInner;
    RGBColor text;
    RGBColor textFieldFill;
    RGBColor textFieldBorder;
    RGBColor textFieldInner;
} UIThemeColors;

typedef struct UIThemeMetrics
{
    short textInsetH;
    short textInsetV;
    short textScrollbarW;
    short groupLabelInsetH;
    short groupLabelBaseline;
} UIThemeMetrics;

typedef struct UIThemeFonts
{
    short textFont;
    short textSize;
} UIThemeFonts;

typedef struct UITheme
{
    UIThemeColors colors;
    UIThemeMetrics metrics;
    UIThemeFonts fonts;
} UITheme;

const UITheme *ui_theme_get(void);

void ui_theme_apply_text_colors(void);
void ui_theme_apply_field_colors(void);

#endif /* UI_THEME_H */
