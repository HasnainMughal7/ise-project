# Text file contract

UTF-8; one record per physical line; spaces separate fields; `#` ends the row.
Blank lines are ignored. Field order matches the assignment. Names, paths or other
fields containing spaces or # are quoted: `"Ali Ahmed"`. A quote within a quoted field
is doubled: `"Tea ""Gold"""`. Newlines/tabs inside a field are rejected. Optional contact
and cash merchant use `-`, preserving their positions. Existing single-word unquoted
instructor records are accepted. No CSV commas or JSON replace the required text records.

## Product.txt — 10 fields

`id barcode acronym name unit unitPrice taxPerUnit discountPerUnit available imagePath #`

```
1 123456 MILK1L "Fresh Milk 1 L" count 250.00 10.00 5.00 24.000 barcodes/barcode_123456.png #
2 345678 RICE "Basmati Rice" weight 300.00 15.00 10.00 10.000 barcodes/barcode_345678.png #
```

IDs and barcodes must be unique. Acronym is 1–8 characters. Weight means kilograms.
Image paths are relative to the data directory unless an absolute path is used;
they describe the saved label, not the input used by the camera.
The supplied products/prices/tax amounts are demonstration data.

## Order.txt — 9 fields

`orderId date time customer contact status paymentAmount mode merchant #`

```
00000119 2026-11-19 10:30:00 "Ali Ahmed" - paid 662.50 cash - #
```

ID is exactly 8 digits. First six are the persistent serial; last two are day-of-month.
Date is yyyy-MM-dd; time is HH:mm:ss, taken from the demo computer's local clock.
Status is in-progress, on-hold, paid or archived. Mode is cash, card or wallet.
Archived means a previously paid order. IDs are stored as strings to retain zeroes.

## OrderDetail.txt — 4 fields

`orderId productId barcode quantity #`

```
00000119 1 123456 2.000 #
00000119 2 345678 0.500 #
```

One row per distinct product in an order; repeat scans merge the quantity. Rows for
an order preserve the same item order as Bill.txt. We also use # here for consistency;
the screenshot explicitly requires # for the other three files but does not settle it here.

## Bill.txt — one bill on one line

`billId customer orderId [acronym unitPrice discountPerUnit quantity]... subtotal tax discount payable status #`

```
000000001 "Ali Ahmed" 00000119 MILK1L 250.00 5.00 2.000 RICE 300.00 10.00 0.500 650.00 27.50 15.00 662.50 paid #
```

The first three fields are fixed. Each product contributes four fields in the stated
assignment order. The final five fields are fixed totals/status. The parser infers
N=(fieldCount-8)/4; no item-count field is inserted. This resolves the screenshot's
multi-product ambiguity while preserving one bill per line. Bill ID is a 9-digit serial.
Subtotal is before both tax and discount. For the example:

- Milk base 250 × 2 = 500; tax 10 × 2 = 20; discount 5 × 2 = 10.
- Rice base 300 × 0.5 = 150; tax 15 × 0.5 = 7.50; discount 10 × 0.5 = 5.
- Payable = 650 + 27.50 − 15 = 662.50.

Prices/discounts/acronyms in old bills are snapshots, so catalogue price edits do not
rewrite history. The schema stores total tax, not each historical line's tax rate.
Accordingly a reload validates bill arithmetic and links, but cannot independently
recompute historical total tax from the current catalogue. Do not claim tamper-proof storage.

## Save recovery

TextStore serializes all four after-images into Pending.json using QSaveFile, then
atomically replaces each required file individually, then removes the journal.
On startup an existing valid journal is applied again. The operation is idempotent.
A crash between file replacements can therefore roll forward. QLockFile prevents
two instances of this app from using the same folder. These mechanisms do not stop
external editors, disk loss, or malicious edits; back up the closed data folder.
