# Corner Store — ISE Increment 2

A C++17 / Qt Widgets desktop PoS with camera-based Code 39 reading, product lookup,
a cart, stock validation, bill display and the four required text files.
The barcode generator is our manual Increment 1 encoder; ZXing is used only to READ.
No database, web server, account or payment gateway is needed.

## Windows setup

Use **MSYS2 UCRT64**, the same environment as the corrected Increment 1 project.
Install/update dependencies there:

```bash
pacman -Syu
```

If MSYS2 asks you to close the terminal, reopen UCRT64 and repeat that command first.
Then:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-multimedia mingw-w64-ucrt-x86_64-zxing-cpp
cd "/d/Study/ISE/Project/ISE_Increment2/Project"
bash build_windows.sh
```

Change the path to your extracted folder. The script configures CMake, builds,
runs all tests and launches the application only if they pass.
Use `bash build_windows.sh --no-run` for build/tests only.

Alternatively double-click `BUILD_AND_RUN.bat`. Later use `RUN.bat`.
These launchers look for C:\msys64 then D:\msys64. For another installation,
set the Windows environment variable `MSYS2_ROOT` to its real path.

If running from the UCRT64 terminal, use `./build/CornerStore.exe`.
Do not use `bash CornerStore.exe`: an executable is not a shell script.
Do not double-click the raw EXE outside the launcher: Qt and GCC DLLs are supplied
by the UCRT64 PATH. This avoids the earlier Qt6Core/Qt6Gui/libgcc DLL errors.
Do not download individual DLLs from unrelated websites or mix MINGW64 with UCRT64.
If CMake reports an old source path/generator, rename the `build` folder and build again.
Never delete `data` to fix a build.

## First demo

Start with the untouched demo data. In New order set quantity 2 and scan label 123456
(milk). Set quantity 0.500 and scan 345678 (rice). Enter `Ali Ahmed`; choose paid/cash.
Expected subtotal 650.00, tax 27.50, discount 15.00, payable **PKR 662.50**.
Generate bill, inspect it, then open Saved orders. Close/reopen the app to verify persistence.
Product specifications appear after scanning; select a cart row and press Description
for all ten fields. Products also supports camera lookup without making an order.

## Camera

Allow Windows Settings → Privacy & security → Camera → desktop app access.
Close apps already using the webcam. Select the camera in the scan dialog, Start,
and hold the entire barcode including its white margins horizontally in clear light.
Two matching processed frames confirm the scan; capture stops afterwards.
A repeated scan requires pressing Scan again, preventing one held label from filling the cart.
The 8-second watchdog detects absent/unusable frames, not a general scan timeout.
If live frames are visible but no result appears, improve focus/lighting and move farther away.

## Working data

`data` beside this source folder holds Product.txt, Order.txt, OrderDetail.txt and Bill.txt.
The app discovers it relative to the executable; launching from a different working directory
is supported. An explicit folder can be selected with:

```bash
./build/CornerStore.exe --data-dir "D:/My Store/data"
```

All four files must exist. Order, OrderDetail and Bill may be empty. Keep a backup of
the entire data folder while the app is closed. Never restore just one history file.
`Store.lock` prevents a second app instance from writing the same folder. A pending
save is recovered on startup using `Pending.json`; do not delete it to dismiss an error.
If a save fails, stop using that session and restart after correcting disk/permission issues.
A structurally damaged journal requires restoring a complete backup.

To edit demo products, close the app first, preserve historical product IDs/barcodes/units,
edit Product.txt using the documented format, then reopen. The Products reload button
accepts edits only when the cart is empty. Product prices are loaded from this file into memory;
file edits are not watched continuously. Creating a barcode label does not add a product.
Product IDs referenced by previous orders must remain in the catalogue.

## Assignment interpretations

- Exactly six digits, first digit nonzero. Only standard Code 39 is accepted.
- Required quantity is strictly LESS than available stock, including merged scans.
- Count means whole items; weight means kg with up to three decimals.
- Tax and discount are fixed PKR amounts PER UNIT. They are not percentages.
- Money uses integer paisa; fractional quantities are thousandths. Each line component
  rounds half-up to a paisa. Totals sum those rounded components.
- Generating ANY bill reduces available stock once, including unpaid on-hold/in-progress
  bills. Those units are reserved. Mark paid does not deduct again. Only paid orders can archive.
- New paid bills require exact payable; unpaid bills use zero. Cash uses merchant `-`;
  card/wallet require a merchant name. Mark paid records the full amount using that saved mode.
- Saved line edits, cancellations, refunds and partial payments are outside this increment.
- Cashier and manager share the product-view GUI; the screenshot did not request authentication.
- Multiword fields use quotes. Bill product groups repeat in one physical line.
  See FILE_FORMATS.md for all field orders and examples.

## Design

Barcode/ImageReader handle barcode logic; CameraScanner captures frames; ScanDialog
collects a confirmed result. Checkout owns order rules. Store is a small abstraction;
TextStore owns parsing/persistence. MainWindow handles interaction; Receipt formats output.
MemoryStore in tests demonstrates an alternative storage implementation without rewriting Checkout.
These separations provide concrete SRP, anticipation-of-change and OCP examples.

Limits: 10,000 records per file, 10 MB per file, 100 different products per order;
unit price/tax up to PKR 1,000,000, quantity up to 1,000,000 units. Small single-user store only.
Whole-file rewrites and simple lookups favor understandable coursework code over large-store scale.
The journal gives application-level roll-forward recovery, not a database ACID guarantee.

## Other platforms and tests

Use CMake 3.21+, a C++17 compiler, Qt 6.4+ Widgets/Multimedia and ZXing-C++ 2.3+.
With those development packages available:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/CornerStore
```

See TEST_REPORT.md for actual test evidence and untested hardware. Tests use temporary
folders and do not change the supplied product/history data. No network is used at runtime.

References: https://www.msys2.org/docs/environments/ ;
https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-qt6-multimedia ;
https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-zxing-cpp ;
https://doc.qt.io/qt-6/qsavefile.html ; https://github.com/zxing-cpp/zxing-cpp
