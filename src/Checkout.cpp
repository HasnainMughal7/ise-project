#include "Checkout.h"
#include "Numbers.h"

Checkout::Checkout(Store& source) : store(source) {}

bool Checkout::load(QString& error)
{
    if (blocked) { error = "Restart the app to recover the interrupted save."; return false; }
    if (!cart.isEmpty()) { error = "Finish or clear the current order before reloading."; return false; }
    return store.load(state, error);
}

const Product* Checkout::product(const QString& id) const
{
    for (const Product& p : state.products) if (p.id == id) return &p;
    return nullptr;
}

const Product* Checkout::findBarcode(const QString& barcode) const
{
    for (const Product& p : state.products) if (p.barcode == barcode) return &p;
    return nullptr;
}

bool Checkout::checkQuantity(const Product& p, qint64 quantity, QString& error) const
{
    if (!validQuantity(p.unit, quantity)) {
        error = "Use a positive whole count or a weight with up to 3 decimals.";
        return false;
    }
    // Literal assignment rule: equality is rejected as well as excess stock.
    if (quantity >= p.available) {
        error = "Required quantity must be LESS than available stock (" + fixedText(p.available, 3) + ").";
        return false;
    }
    return true;
}

bool Checkout::add(const QString& barcode, qint64 quantity, QString& error)
{
    if (blocked) { error = "Restart the app to recover the interrupted save."; return false; }
    const Product* p = findBarcode(barcode);
    if (!p) { error = "No product in Product.txt matches barcode " + barcode + "."; return false; }
    if (!validQuantity(p->unit, quantity)) { error = "Invalid quantity for this product's unit."; return false; }
    for (int i = 0; i < cart.size(); ++i) {
        if (cart[i].productId == p->id) return changeQuantity(i, cart[i].quantity + quantity, error);
    }
    if (!checkQuantity(*p, quantity, error)) return false;
    if (cart.size() >= 100) { error = "An order can contain at most 100 different products."; return false; }
    cart.append({p->id, p->barcode, quantity});
    return true;
}

bool Checkout::changeQuantity(int row, qint64 quantity, QString& error)
{
    if (blocked) { error = "Restart the app to recover the interrupted save."; return false; }
    if (row < 0 || row >= cart.size()) { error = "Select an order row first."; return false; }
    const Product* p = product(cart[row].productId);
    if (!p || !checkQuantity(*p, quantity, error)) return false;
    cart[row].quantity = quantity;
    return true;
}

void Checkout::remove(int row)
{
    if (row >= 0 && row < cart.size()) cart.removeAt(row);
}

void Checkout::clear() { cart.clear(); }

Totals Checkout::totals() const
{
    Totals total;
    for (const OrderLine& line : cart) {
        const Product* p = product(line.productId);
        if (!p) continue;
        total.subtotal += lineAmount(p->price, line.quantity);
        total.tax += lineAmount(p->tax, line.quantity);
        total.discount += lineAmount(p->discount, line.quantity);
    }
    return total;
}

static bool validText(const QString& text)
{
    return text.size() <= 100 && !text.contains('\n') && !text.contains('\r') && !text.contains('\t');
}

bool Checkout::saveState(const StoreState& next, QString& error)
{
    if (blocked) { error = "Restart the app to recover the interrupted save."; return false; }
    if (!store.save(next, error)) {
        blocked = true;
        error += "\nSaving stopped. Restart to recover before recording another payment.";
        return false;
    }
    state = next;
    return true;
}

bool Checkout::issueBill(Order order, const QDateTime& when, Bill& bill, QString& error)
{
    if (blocked) { error = "Restart the app to recover the interrupted save."; return false; }
    if (cart.isEmpty()) { error = "Scan a product before generating a bill."; return false; }
    order.customer = order.customer.trimmed();
    order.contact = order.contact.trimmed();
    order.merchant = order.merchant.trimmed();
    if (order.customer.isEmpty() || !validText(order.customer) || !validText(order.contact) ||
        !validText(order.merchant) || !when.isValid()) {
        error = "Check customer details and date/time."; return false;
    }
    if (order.contact.isEmpty()) order.contact = "-";
    if (order.mode == "cash") order.merchant = "-";
    if (order.mode != "cash" && order.mode != "card" && order.mode != "wallet") {
        error = "Select cash, card or wallet."; return false;
    }
    if (order.mode != "cash" && (order.merchant.isEmpty() || order.merchant == "-")) {
        error = "Enter the bank or wallet merchant name."; return false;
    }
    if (order.status != "paid" && order.status != "on-hold" && order.status != "in-progress") {
        error = "New bills must be paid, on-hold or in-progress."; return false;
    }
    Totals total = totals();
    qint64 expected = order.status == "paid" ? total.payable() : 0;
    if (order.payment != expected) { error = "Paid amount must equal the bill; unpaid orders use 0.00."; return false; }
    int orderSerial = 0, billSerial = 0;
    for (const Order& o : state.orders) orderSerial = qMax(orderSerial, o.id.left(6).toInt());
    for (const Bill& b : state.bills) billSerial = qMax(billSerial, b.id.toInt());
    if (orderSerial >= 999999 || billSerial >= 999999999) { error = "ID capacity reached."; return false; }
    order.id = QString::number(orderSerial + 1).rightJustified(6, '0') + when.toString("dd");
    order.date = when.toString("yyyy-MM-dd");
    order.time = when.toString("HH:mm:ss");
    order.lines = cart;
    Bill nextBill;
    nextBill.id = QString::number(billSerial + 1).rightJustified(9, '0');
    nextBill.customer = order.customer;
    nextBill.orderId = order.id;
    nextBill.status = order.status;
    nextBill.totals = total;
    StoreState next = state;
    for (const OrderLine& line : cart) {
        const Product* p = product(line.productId);
        if (!p || !checkQuantity(*p, line.quantity, error)) return false;
        nextBill.lines.append({p->acronym, p->price, p->discount, line.quantity});
        for (Product& stock : next.products) if (stock.id == p->id) stock.available -= line.quantity;
    }
    next.orders.append(order);
    next.bills.append(nextBill);
    if (!saveState(next, error)) return false;
    bill = nextBill;
    clear();
    return true;
}

bool Checkout::updateStatus(const QString& id, bool paid, QString& error)
{
    StoreState next = state;
    for (Order& order : next.orders) {
        if (order.id != id) continue;
        bool allowed = paid ? (order.status == "on-hold" || order.status == "in-progress") : order.status == "paid";
        if (!allowed) { error = "Only unpaid orders can become paid; only paid orders can be archived."; return false; }
        for (Bill& bill : next.bills) {
            if (bill.orderId != id) continue;
            order.status = paid ? "paid" : "archived";
            order.payment = bill.totals.payable();
            bill.status = order.status;
            return saveState(next, error);
        }
    }
    error = "Order or bill not found.";
    return false;
}

bool Checkout::markPaid(const QString& id, QString& error) { return updateStatus(id, true, error); }
bool Checkout::archive(const QString& id, QString& error) { return updateStatus(id, false, error); }
