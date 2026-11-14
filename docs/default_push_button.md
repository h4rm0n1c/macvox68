# Default push buttons (double-border / default ring)

**Runtime requirement (2026-11-14):** `SetControlData` and its default-button
tag belong to the Appearance control APIs. Check Appearance availability
before using this path; System 7 alone does not establish support. See
[the audit](audit_2026-11-14.md).

Classic Mac OS dialogs use a *default push button* style that draws the extra (double-border) ring around the control. When someone asks for the button with the extra/double border, they are asking for the **default push button** appearance, not a different control type.

Reference notes:
- The Control Manager defines `kControlPushButtonDefaultTag` (resource tag `'dflt'`) as the flag that indicates a push button should draw the default-ring/double-border appearance.
- Use this tag when creating or configuring push buttons to match the classic “default” visual affordance.
