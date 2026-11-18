# Requirement traceability

| Instructor requirement | Implementation | Evidence/demo |
|---|---|---|
| Camera-based reader | CameraScanner, ScanDialog | Live laptop rehearsal; decoder regression tests |
| GUI PoS | MainWindow four tabs | gui_workflow and visual review |
| Scan shows product specs | scan, productDescription | All ten fields plus Description button |
| Product.txt exact fields | TextStore load/serialize | pos_workflow; FILE_FORMATS.md |
| Order quantity less than stock | Checkout checkQuantity/add | Below/equal/above and merged scan tests |
| Record order and generate bill | Checkout issueBill | Golden multi-item bill 662.50 |
| Order.txt sequence and IDs | TextStore, issueBill | Eight digits/day suffix, restart sequence |
| OrderDetail product/quantity links | TextStore | Roundtrip, orphan reference rejection |
| Bill.txt one bill per line | TextStore serialize | Repeated product groups; totals checks |
| On-screen complete bill | receiptHtml, showBill | GUI receipt, saved-order reopening |
| SRP and modularity | GUI, checkout, storage, camera modules | Member guide and viva code examples |
| AoC and OCP | Store interface, TextStore, MemoryStore | Same Checkout tested with alternate Store |
| Quality and reuse | Retained manual encoder/reader; tests | TEST_REPORT.md and viva guide |

Interpretations needing instructor alignment are explicitly recorded in FILE_FORMATS.md
and README.md: quoted multiword fields, repeated Bill groups, # on OrderDetail,
strict less-than, kg weight, fixed per-unit tax/discount and stock reservation on bill creation.
