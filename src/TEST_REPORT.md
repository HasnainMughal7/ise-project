# Increment 1 verification

Verified on 10 November 2026. Test host: Debian GNU/Linux 13, GCC 14.2.0, CMake 3.31.6, Qt 6.8.2 and ZXing-C++ 2.3.0. Windows setup targets the MSYS2 UCRT64 packages listed in README.

## Completed

| Check | Result |
|---|---|
| Compile all final application sources and Qt metadata | Passed |
| Configure and build through the supplied CMake project | Passed |
| CTest barcode_roundtrip | Passed, 634 checked conditions, 0 failures |
| 120 valid sample numbers, including 100000, 999999, 345678 and 888888 | Exact round-trip results |
| PNG saving and reloading for all 120 samples | Exact results |
| Wrong length, leading zero, letters, spaces and signs | Rejected |
| 90, 180 and 270 degree rotations | Correct result |
| Inverted colours and resized image | Correct result |
| Caption removed from barcode | Correct result from bars |
| Blank, null, missing and corrupt images | Rejected without a crash |
| Save to missing directory and save null image | Failure reported |
| Offscreen GUI generation and automatic save | Passed |
| Input edit and invalid-input stale-preview clearing | Passed |
| Actual Qt file dialog selection and uploaded-image display | Passed |
| Clear action resets displayed result | Passed |
| Invalid camera index / no-device error path | Passed |
| Synthetic QVideoFrame processing through camera class | Two matching reads emitted one result and stopped scanning |

The single CTest case contains 634 individual condition checks. These sample 120 valid numbers; they do not exhaust all 900000 possible inputs. The GUI and synthetic-frame checks were additional development checks. The reproducible barcode regression test source is included in `tests/barcode_tests.cpp`.

## Windows compatibility fix on 11 November

The Windows desktop and laptop logs both showed successful compilation followed by 26 failed conditions on the same 13 numbers. The laptop log identifies ZXing-C++ 3.1.1 and Qt 6.12.0. A header-only compatibility check had missed the changed runtime interpretation.

In ZXing 3.x, the broad Code39 selection includes Code32. Thirteen sample labels also satisfied Code32's checksum rules and were interpreted as different pharmaceutical-code strings. The reader now explicitly selects Code39Std on 3.x and retains Code39 on 2.x through a compile-time version guard. No generation logic or input validation was relaxed.

| Reproduction and regression check | Result |
|---|---|
| Original source with upstream ZXing 3.1.1 reader | Reproduced exactly the user's 26 failures |
| Corrected source with upstream ZXing 3.1.1 reader | 634 checks, 0 failures; full application build passed |
| Corrected source with upstream ZXing 3.0.2 reader | 634 checks, 0 failures |
| Corrected source with ZXing 2.3.0 | CTest passed |

These new executions used the Linux host and Qt 6.8.2. The 3.x reader libraries were built from the upstream release sources with writer support disabled. The 13 reported numbers were already present in the regression suite, so the 634-condition count is unchanged. Native Windows execution of the corrected source is still to be confirmed on the user's laptop.

The corrupt-image test now handles QFile::open failure instead of ignoring its return value. RUN.bat adds the installed UCRT64 bin directory to the process PATH for later launches, addressing the reported missing runtime DLL search path. It is a launch helper for a machine with MSYS2 installed, not a bundled portable executable. The batch launcher was reviewed but was not executed on this Linux test host.

Sources for this fix: https://github.com/zxing-cpp/zxing-cpp/blob/v3.1.1/core/src/oned/ODCode39Reader.cpp and https://doc.qt.io/qt-6/windows-deployment.html

## Demo laptop checks still required

- Build and launch on the actual Windows laptop using UCRT64.
- Allow desktop-camera access and confirm the chosen physical webcam delivers frames.
- Scan a printed label or a barcode on a second screen under classroom lighting.
- Check Start, Stop, Clear and camera release after a tab change and window close.
- Test the real native Windows save/open dialogs and your actual output folder.

The offscreen UI and synthetic-frame checks do not prove physical Windows camera capture. This package contains source, not a prebuilt or Windows-certified executable. Camera focus, glare, permission and driver behaviour depend on the laptop.

## Original defect and replacement

The original Code 128 table had a trailing backslash in the comment after symbol value 60. C++ backslash-newline splicing absorbed the next row into the comment. A compiled extraction of that table showed index 104 containing the Start C pattern, while later entries became zero. The decoder's twelve-digit conversion attempted to compensate for malformed output and was removed with the old implementation. The final version uses a smaller manual Code 39 encoder and exact validation without that conversion. The README now matches the final code and assignment.

## Requirement traceability

| Instructor requirement | Implementation |
|---|---|
| Standalone desktop C++ application | Qt Widgets executable, C++17 |
| Six digits, first non-zero | validateNumber in Barcode.cpp, checked before generation and after decoding |
| Generate without a barcode-generation library | encodeCode39 builds modules using an explicit digit-pattern table; makeBarcode draws rectangles |
| Display generated barcode | MainWindow::generate and generated preview |
| Save barcode as an image | Automatic PNG save plus Save as, using QSaveFile and QImage |
| Read uploaded image and show six-digit number | loadImage, readBarcode and MainWindow::upload |
| Read through laptop camera | CameraScanner, QCamera, QVideoSink and the shared readBarcode function |
| GitHub submission | Separate source ownership folders and GitHub_Team_Workflow.md |

ZXing is used only for reading. Qt supplies general-purpose UI, image/file and camera functions. The generation module does not invoke an external barcode writer. Basic Code 39 without its optional checksum is used; keep damaged-label and false-positive limits in mind during demonstrations.
