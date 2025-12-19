# Default push buttons (double-border / default ring)

Classic Mac OS dialogs use a *default push button* style that draws the extra (double-border) ring around the control. When someone asks for the button with the extra/double border, they are asking for the **default push button** appearance, not a different control type.

Reference notes:
- The Control Manager defines `kControlPushButtonDefaultTag` (resource tag `'dflt'`) as the flag that indicates a push button should draw the default-ring/double-border appearance.
- Use this tag when creating or configuring push buttons to match the classic “default” visual affordance.
