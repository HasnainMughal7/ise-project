# Test report — 17 November 2026

Tested: Linux / GCC 14.2 / Qt 6.8.2 / ZXing-C++ 3.1.1, Release C++17.
All three CTest suites pass: 634 barcode assertions, 97 business/storage assertions,
23 GUI assertions, 754 total. GUI tests run with QT_QPA_PLATFORM=offscreen.
These are conditions checked inside three executables, not 754 separate CTest cases.

- Barcode: 120 valid examples, manual encode/decode, PNG save/reload, rotations,
  inversion, caption removal, validation and unreadable/missing file cases.
- PoS: numeric parsing/rounding, exact-stock rejection, merged scans, fractional weight,
  payment validation, golden 662.50 bill, unique IDs across dates/restart, hold/pay/archive,
  one-time stock deduction, failing Store, text record roundtrip, duplicate/orphan records,
  malformed lines, corrupt totals, historical price preservation and journal recovery.
- GUI: actual widgets show two cart rows and 662.50, Generate opens a receipt, saves files,
  clears cart, disables repeated billing, history reloads, barcode preview clears on edit.
  Eight sample labels are generated with the application encoder and decoded successfully.
- Visual review: checkout at 1180x810 and 1020x720; Products, Saved orders, barcode page,
  receipt and no-camera dialog. Build and shell scripts checked for syntax/structure.

Not tested here: native Windows executable, physical webcam capture/autofocus, permissions,
real payment processing (not implemented), printer drivers, every possible power failure,
all 900000 six-digit values, or performance at maximum record capacity.

Run on your laptop: bash build_windows.sh --no-run, then RUN.bat. Rehearse real camera
scans of 123456, 345678, 131676 and 195028; the latter two are regression examples.
Tests create temporary files and preserve the submitted clean sample data.
