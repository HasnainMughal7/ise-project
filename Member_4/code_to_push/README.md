# Barcode Studio

ISE Increment 1: a standalone C++17 desktop barcode generator and reader.

Enter exactly six ASCII digits, with a non-zero first digit. The app draws a Code 39 barcode using our own encoding table and loops, displays it and automatically saves a PNG. Read that PNG from an image file or scan a printed/displayed barcode with a webcam.

## What is included

- Manual Code 39 generation. No barcode writer or generation API is used.
- Automatic PNG save in your Pictures/BarcodeStudio folder. If Pictures is unavailable, the app uses your home folder. The on-screen message shows the actual path. Generating the same number again replaces the same generated PNG.
- A Save as button for another location, with native file dialogs.
- PNG, JPEG and BMP upload reading. Results come from the bars, not the caption or filename.
- Webcam selection, live preview, Start, Stop and Clear. Two matching camera reads confirm a result.
- Validation and errors for wrong input, corrupt files, save failures and unavailable cameras.

The increment does not include inventory, prices, sales or billing. These belong to later increments in the instructor's description.

## Windows setup

1. Install MSYS2 from https://www.msys2.org/ if it is not already installed.
2. Open **MSYS2 UCRT64** from the Start menu, not MSYS, MINGW64 or a normal terminal.
3. Run `pacman -Syu`. If asked to close the terminal, close it, reopen UCRT64 and run `pacman -Syu` again.
4. Install the compiler, build tools and libraries:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-multimedia mingw-w64-ucrt-x86_64-zxing-cpp
```

5. Extract the ZIP fully. Open the `Project` folder in File Explorer. After setup, double-click **BUILD_AND_RUN.bat**. It builds, runs the tests and launches the app.
6. If MSYS2 is in a custom location, use the commands below from UCRT64. C: becomes `/c`, D: becomes `/d`. Replace the example path with the real extracted location.

```bash
cd "/c/Users/YourName/Downloads/ISE_Increment1/Project"
bash build_windows.sh
```

The output is `build/BarcodeStudio.exe`. After a successful build, double-click **RUN.bat** in the Project folder to launch without rebuilding. It adds the installed UCRT64 runtime DLL directory to PATH for this launch only.

From the Project folder in UCRT64, launch with:

```bash
./build/BarcodeStudio.exe
```

If already inside `build`, use `./BarcodeStudio.exe`. Do not use `bash BarcodeStudio.exe`: Bash runs shell scripts, while this is a compiled Windows program. Double-clicking the bare EXE in Explorer can show missing `Qt6Core.dll`, `Qt6Gui.dll` or `libgcc_s_seh-1.dll`; use RUN.bat or UCRT64 instead.

A bare EXE copied to another computer needs its runtime DLLs and Qt plugins; use the source and setup instructions on each teammate's computer. No prebuilt Windows executable is included in this source package.

To build and test without opening the app:

```bash
bash build_windows.sh --no-run
```

## Linux build

Use Qt 6.4 or newer and ZXing-C++ 2.2 or newer with the compatible reader API. On a recent Debian or Ubuntu installation:

```bash
sudo apt install g++ cmake ninja-build qt6-base-dev qt6-multimedia-dev libzxing-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/BarcodeStudio
```

Some older distributions ship an older ZXing API. Use the documented Windows setup or a newer distribution in that case.

## Quick demo

1. Type `345678`, select **Generate + save PNG** and point out the barcode and saved path.
2. Switch to **Read barcode**, select **Upload image**, open that PNG and show `345678`.
3. Try `012345` and `12345` to show validation.
4. For the camera, connect it before launching the app. Select it, click **Start camera**, and hold a printed barcode or another device's screen steady. Include both white side margins. Avoid glare and adjust the distance for focus. The camera stops when it reads the same number twice.
5. Stop manually or switch tabs to release the camera.

## How it works

| File | Responsibility | Owner |
|---|---|---|
| src/Barcode.h and src/Barcode.cpp | Validation, manual Code 39 encoding, drawing the PNG pixels | member1 |
| src/ImageReader.h and src/ImageReader.cpp | Image files, decoding and decoded-number validation | member2 |
| tests/barcode_tests.cpp | Generation and decoding regression checks | member2 |
| src/CameraScanner.h and src/CameraScanner.cpp | Qt camera lifecycle, frames and stable-result detection | member3 |
| src/MainWindow.h and src/MainWindow.cpp | Layout, user actions and messages | member4 |
| src/main.cpp, CMakeLists.txt, build scripts and README | App startup and integration | member4 |

Qt supplies widgets, image drawing/file handling and camera access. ZXing-C++ reads barcode pixels. The encoder does not include or call ZXing, Zint, MultiFormatWriter, CreateBarcode or any other barcode generation API. MSYS2 may install additional dependencies of ZXing; installed dependencies do not form part of our encoder.

The reader selects `Code39Std` with ZXing 3.x and `Code39` with ZXing 2.x. In 3.x, the broad `Code39` selection also permits Code32 interpretation; some six-digit values would otherwise become a different pharmaceutical-code string. The version guard keeps our exact six-digit label unchanged. The supplied regression cases include all 13 values reported in the Windows failure log.

Code 39 uses 9 alternating bars/spaces per symbol, with 3 wide elements. We use a 3:1 wide-to-narrow ratio, an asterisk start/stop symbol, a narrow gap between symbols and 10-module white quiet zones. Six digits produce 147 modules, drawn at 4 pixels each: a 588 by 240 PNG. This basic Code 39 variant has no optional checksum. It is an internal store label, not a registered retail EAN/UPC identifier.

## Troubleshooting

- **Qt6 or ZXing not found:** install the listed packages in UCRT64 and use that same terminal for building. Do not mix MSYS, MINGW64 and UCRT64 libraries.
- **Old build cache or generator mismatch:** this rewritten version must use its own extracted folder. Rename its `build` folder to `build_old` and rebuild if needed.
- **Missing DLL or Qt platform plugin:** start through RUN.bat, BUILD_AND_RUN.bat or UCRT64. Do not move just the EXE. RUN.bat uses the installed runtime; it is not a portable distribution.
- **26 barcode checks fail with the old source:** replace `src/ImageReader.cpp` with this corrected version and rebuild. Do not skip the failed tests; ZXing 3 requires the explicit standard Code39 selection described above.
- **No camera:** connect it before launch, allow desktop apps to use it in Windows camera privacy settings and close Teams/Zoom or other camera apps. Restart the app after connecting a new device. Image upload works without a camera.
- **Nothing reads:** use the new sample images or generate fresh PNGs. Keep all bars and white margins visible. This increment reads Code 39, not QR, EAN or the earlier broken Code 128 images.
- **Automatic save fails:** the barcode is still visible. Use Save as and choose a writable folder.
- **Unexpected scan:** clear the view, scan one barcode at a time and check focus. Code 39 has no checksum here, so damaged labels can occasionally decode incorrectly.

See `TEST_REPORT.md` for the tested environment and remaining device checks.

## References

- Instructor: supplied Increment 1 assignment screenshot, posted 8 November 2026. Graded demonstration on 12 November 2026.
- Qt widgets and image drawing: https://doc.qt.io/qt-6/qwidget.html and https://doc.qt.io/qt-6/qpainter.html
- Qt camera session: https://doc.qt.io/qt-6/qmediacapturesession.html
- ZXing reader API and Code 39 support: https://github.com/zxing-cpp/zxing-cpp
- MSYS2 packages: https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-qt6-multimedia and https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-zxing-cpp

Qt and ZXing are third-party libraries with their own licenses. They are installed separately and are not bundled in this ZIP. See their upstream license notices before redistributing compiled libraries.
