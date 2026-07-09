#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <Types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UILayoutMetrics
{
    short margin;
    short gutter;
    short buttonW;
    short buttonH;
    short sectionGutter;
    short textAreaH;
    short prosodyH;
    short settingsH;
    short tcpH;
    short sliderH;
    short sliderW;
    short fieldH;
    short fieldW;
    short portFieldW;
} UILayoutMetrics;

extern const UILayoutMetrics kUILayoutMetrics;
extern const short kUILayoutStartButtonW;
extern const short kUILayoutButtonInset;

#ifdef __cplusplus
}
#endif

#endif /* UI_LAYOUT_H */
