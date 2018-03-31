#ifndef AF_ADDRESS_H
#define AF_ADDRESS_H

#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/strings/string.h"

#include "AF/detail/utils/utils.h"

namespace AF
{

class Framework;
class Receiver;


/*
 * The unique address of an entity that can send or receive messages.
 */
class Address {
public:

    friend class Framework;
    friend class Receiver;

    /*
     * Static method that returns the unique 'null' address.
     * The null address is guaranteed not to be equal to the address of any actual entity.
     */
    AF_FORCEINLINE static const Address &Null() {
        return null_address_;
    }

    AF_FORCEINLINE Address() : name_(), index_() {
    }

    AF_FORCEINLINE explicit Address(const char *const name) : name_(name), index_() {
    }

    AF_FORCEINLINE Address(const Address &other) 
      : name_(other.name_),
        index_(other.index_) {
    }

    AF_FORCEINLINE Address &operator=(const Address &other) {
        name_ = other.name_;
        index_ = other.index_;
        return *this;
    }

    /* 
     * Gets an integer index identifying the framework containing the addressed entity.
     */
    AF_FORCEINLINE uint32_t GetFramework() const {
        return index_.componets_.framework_;
    }

    AF_FORCEINLINE const char *AsString() const {
        return name_.GetValue();
    }

    AF_FORCEINLINE uint32_t AsInteger() const {
        return index_.componets_.index_;
    }

    AF_FORCEINLINE uint64_t AsUInt64() const {
        return static_cast<uint64_t>(index_.uint32_);
    }

    AF_FORCEINLINE bool operator==(const Address &other) const {
        return (name_ == other.name_);
    }

    AF_FORCEINLINE bool operator!=(const Address &other) const {
        return !operator==(other);
    }

    AF_FORCEINLINE bool operator<(const Address &other) const {
        return (name_ < other.name_);
    }

private:
    /*
     * Internal explicit constructor, used by friend classes.
     */
    AF_FORCEINLINE explicit Address(const Detail::String &name, const Detail::Index &index) 
      : name_(name),
        index_(index) {
    }

    AF_FORCEINLINE const Detail::String &GetName() const {
        return name_;
    }

    static Address null_address_;       // A single static instance of the null address.

    Detail::String name_;               // The string name of the addressed entity.
    Detail::Index index_;               // Cached local framework and index.
};


} // namespace AF


#endif // AF_ADDRESS_H
