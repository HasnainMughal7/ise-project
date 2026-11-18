#pragma once
#include "Models.h"

// Checkout knows this contract, not the details of a storage format.
class Store {
public:
    virtual ~Store() = default;
    virtual bool load(StoreState& state, QString& error) = 0;
    virtual bool save(const StoreState& state, QString& error) = 0;
};
