#include "Checkout.h"
#include "Numbers.h"

static bool fail(QString& error, const QString& message) {
    error = message;
    return false;
}
static bool withinWeek(const Order& o, const QDateTime& now) {
    return o.paid.isValid() && now.isValid() && now >= o.paid &&
           now <= o.paid.addSecs(7 * 24 * 60 * 60);
}
Checkout::Checkout(Store& disk, Access& auth) : store(disk), access(auth) {}
bool Checkout::load(QString& error) {
    if (writeBlocked || !draft.lines.isEmpty() || !draft.id.isEmpty())
        return fail(error, "Finish the cart or restart before reloading.");
    return store.load(state, error);
}
const Order* Checkout::findOrder(const QString& id) const {
    for (const Order& o : state.orders)
        if (o.id == id)
            return &o;
    return nullptr;
}
const Product* Checkout::findCode(const QString& code) const {
    for (const Product& p : state.products)
        if (p.barcode == code)
            return &p;
    return nullptr;
}
void Checkout::discard() {
    draft = Order{};
    approvedId.clear();
}
bool Checkout::openOrder(const QString& id, const QString& pin, QDateTime now, QString& error) {
    if (writeBlocked || !draft.id.isEmpty() || !draft.lines.isEmpty())
        return fail(error, "Hold or discard the current cart first.");
    const Order* saved = findOrder(id);
    if (!saved || saved->status == "archived")
        return fail(error, "This order cannot be opened for editing.");
    if (saved->status == "paid") {
        if (!withinWeek(*saved, now))
            return fail(error, "The seven-day payment edit window has closed.");
        if (!access.accepts(pin))
            return fail(error, "Manager approval is required for this paid order.");
        approvedId = saved->id;
    }
    draft = *saved;
    return true;
}
bool Checkout::checkStock(const Product& p, qint64 desired, QString& error) const {
    if (!validQuantity(p.unit, desired))
        return fail(error, "Enter a positive quantity. Count items must be whole.");
    const Order* old = findOrder(draft.id);
    qint64 previous = old ? quantityOf(old->lines, p.id) : 0;
    qint64 extra = desired - previous;
    if (extra > 0 && extra >= p.available)
        return fail(error, "Additional quantity must be less than free stock.");
    return true;
}
bool Checkout::add(const QString& code, qint64 value, QString& error) {
    const Product* p = findCode(code);
    if (!p)
        return fail(error, "Unknown barcode. Check the catalogue.");
    if (!validQuantity(p->unit, value))
        return fail(error, "Invalid scan quantity for this product.");
    for (int i = 0; i < draft.lines.size(); ++i)
        if (draft.lines[i].productId == p->id)
            return quantity(i, draft.lines[i].quantity + value, error);
    if (draft.lines.size() >= 100 || !checkStock(*p, value, error)) {
        if (draft.lines.size() >= 100)
            error = "At most 100 different products per cart.";
        return false;
    }
    draft.lines.append({p->id, p->barcode, p->acronym, p->unit, value, p->price, p->tax, p->discount});
    draft.legacyTax = -1;
    return true;
}
bool Checkout::quantity(int row, qint64 value, QString& error) {
    if (row < 0 || row >= draft.lines.size())
        return fail(error, "Select a cart line.");
    Line& line = draft.lines[row];
    for (const Product& p : state.products)
        if (p.id == line.productId) {
            if (!checkStock(p, value, error))
                return false;
            if (value != line.quantity)
                draft.legacyTax = -1;
            line.quantity = value;
            return true;
        }
    return fail(error, "The selected product is missing.");
}
bool Checkout::remove(int row) {
    if (row < 0 || row >= draft.lines.size())
        return false;
    draft.lines.removeAt(row);
    draft.legacyTax = -1;
    return true;
}
Order Checkout::priced(int specialRate, QDateTime now) const {
    Order result = draft;
    result.specialRate = qBound(0, specialRate, 800);
    if (result.status != "paid")
        result.seasonRate = activeRate(state.seasons, now.date());
    return result;
}
bool Checkout::publish(const StoreState& next, QString& error) {
    if (writeBlocked)
        return fail(error, "Restart to recover the earlier save.");
    if (!store.save(next, error)) {
        writeBlocked = true;
        return false;
    }
    state = next;
    return true;
}
bool Checkout::finish(const QString& status, QString customer, QString contact, QString mode,
                      QString merchant, int specialRate, qint64 received, QDateTime now,
                      QString& orderId, QString& error) {
    if (!now.isValid() || specialRate < 0 || specialRate > 800 || received < 0)
        return fail(error, "Invalid date, amount or special discount (maximum 8%).");
    if (status != "on-hold" && status != "in-progress" && status != "paid")
        return fail(error, "Invalid order status.");
    if (draft.lines.isEmpty())
        return fail(error, "Add an item, or cancel the saved held order to release all its stock.");
    const Order* old = findOrder(draft.id);
    if (old && old->status == "paid" &&
        (approvedId != old->id || status != "paid" || !withinWeek(*old, now)))
        return fail(error,
                    "Paid edits require approval and must finish inside the original seven days.");
    Order o = priced(specialRate, now);
    o.customer = customer.trimmed().isEmpty() ? "Walk-in" : customer.trimmed();
    o.contact = contact.trimmed().isEmpty() ? "-" : contact.trimmed();
    o.mode = mode;
    o.merchant = mode == "cash" ? "-" : merchant.trimmed();
    if (!cleanText(o.customer) || !cleanText(o.contact) || !cleanText(o.merchant) ||
        (mode != "cash" && mode != "card" && mode != "wallet") || (mode != "cash" && o.merchant == "-"))
        return fail(error, "Check customer details and enter the bank or wallet name.");
    o.status = status;
    qint64 payable = calculate(o).payable();
    if (status == "paid" && (received < payable || (mode != "cash" && received != payable)))
        return fail(error, "Cash must cover the bill. Card or wallet must equal the total.");
    if (status != "paid" && received != 0)
        return fail(error, "An unpaid order must record zero received.");
    o.received = received;
    StoreState next = state;
    if (!old) {
        if (next.lastOrder >= 999999 || next.lastBill >= 999999999)
            return fail(error, "ID capacity reached.");
        o.id = QString("%1").arg(++next.lastOrder, 6, 10, QChar('0')) + now.toString("dd");
        o.billId = QString("%1").arg(++next.lastBill, 9, 10, QChar('0'));
        o.opened = now;
    }
    if (!old || old->status != "paid")
        o.billed = now;
    if (status == "paid" && !o.paid.isValid())
        o.paid = now;
    for (Product& p : next.products) {
        // An existing hold has already reserved its old quantity. Apply the difference.
        qint64 before = old ? quantityOf(old->lines, p.id) : 0;
        qint64 after = quantityOf(o.lines, p.id);
        if (after && !checkStock(p, after, error))
            return false;
        p.available += before - after;
        if (p.available > 1000000000)
            return fail(error, "Returning stock exceeds the inventory limit.");
    }
    if (old) {
        for (Order& entry : next.orders)
            if (entry.id == o.id) {
                entry = o;
                break;
            }
    } else
        next.orders.append(o);
    if (!publish(next, error))
        return false;
    orderId = o.id;
    discard();
    return true;
}
bool Checkout::cancelHeld(const QString& id, QString& error) {
    StoreState next = state;
    for (int i = 0; i < next.orders.size(); ++i)
        if (next.orders[i].id == id) {
            const Order& o = next.orders[i];
            if (settled(o))
                return fail(error, "Only unpaid orders can be cancelled.");
            for (Product& p : next.products) {
                p.available += quantityOf(o.lines, p.id);
                if (p.available > 1000000000)
                    return fail(error, "Stock limit exceeded.");
            }
            next.orders.removeAt(i);
            if (!publish(next, error))
                return false;
            if (draft.id == id)
                discard();
            return true;
        }
    return fail(error, "Order not found.");
}
bool Checkout::archive(const QString& id, QString& error) {
    StoreState next = state;
    for (Order& o : next.orders)
        if (o.id == id && o.status == "paid" && draft.id != id) {
            o.status = "archived";
            return publish(next, error);
        }
    return fail(error, "Select a paid order outside the active editor.");
}
