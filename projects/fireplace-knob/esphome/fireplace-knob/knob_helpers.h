#pragma once

// Rows of the knob's Settings list for the current fireplace state.
// Page ids: 4 Power, 5 Mode, 6 Fan, 7 Light, 8 Flame, 9 Brightness, 1 Back.
// Flame is listed only in Manual while the fireplace is burning; in the
// other modes the IFC sets the flame itself.
static inline int settings_rows(int *rows, bool fireplace_on, int mode_sel) {
  int n = 0;
  rows[n++] = 4;
  rows[n++] = 5;
  rows[n++] = 6;
  rows[n++] = 7;
  if (mode_sel == 0 && fireplace_on) rows[n++] = 8;
  rows[n++] = 9;
  rows[n++] = 1;
  return n;
}
