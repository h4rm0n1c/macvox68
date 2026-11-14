#include "ui/ui_theme.h"

static const UITheme kUITheme = {
    /* colors */
    {
        { 0xD8D8, 0xD8D8, 0xD8D8 }, /* windowFill */
        { 0x7A7A, 0x7A7A, 0x7A7A }, /* separatorDark */
        { 0xEEEE, 0xEEEE, 0xEEEE }, /* separatorLight */
        { 0xF2F2, 0xF2F2, 0xF2F2 }, /* groupFill */
        { 0x4444, 0x4444, 0x4444 }, /* groupBorder */
        { 0xB5B5, 0xB5B5, 0xB5B5 }, /* groupInner */
        { 0x0000, 0x0000, 0x0000 }, /* text */
        { 0xFFFF, 0xFFFF, 0xFFFF }, /* textFieldFill */
        { 0x4444, 0x4444, 0x4444 }, /* textFieldBorder */
        { 0x9C9C, 0x9C9C, 0x9C9C }, /* textFieldInner */
    },
    /* metrics */
    {
        6,  /* textInsetH */
        4,  /* textInsetV */
        15, /* textScrollbarW */
        10, /* groupLabelInsetH */
        14  /* groupLabelBaseline */
    },
    /* fonts */
    {
        0, /* system font */
        12 /* system size */
    }
};

const UITheme *ui_theme_get(void)
{
    return &kUITheme;
}

void ui_theme_apply_text_colors(void)
{
    const UITheme *theme = ui_theme_get();

    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.windowFill);
}

void ui_theme_apply_field_colors(void)
{
    const UITheme *theme = ui_theme_get();

    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.textFieldFill);
}
