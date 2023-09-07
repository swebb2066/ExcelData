#ifndef FOUNDATION_ITERATOR_HEADER__
#define FOUNDATION_ITERATOR_HEADER__
//
namespace Foundation
{

/// The abstract interface of an iterator over type \c T items
    template <class T>
class IteratorInterface
{
    using ThisType = IteratorInterface<T>;
public: // Types
    using ItemType = T;

public: // ...structors
    /// Ensure the derived destructor is called
    virtual ~IteratorInterface()
    {}

public: // Operators
    /// Move to the next item. Precondition: !Off()
    ThisType& operator++() { Forth(); return *this; }
    /// Is \c other the same as this?
    bool operator==(const ThisType& other) const { return IsEqual(other); }
    /// Is \c other different to this?
    bool operator!=(const ThisType& other) const { return !IsEqual(other); }
    /// The current item. Precondition: !Off()
    const ItemType& operator*() const { return Item(); }
    /// The current item. Precondition: !Off()
    const ItemType* operator->() const { return &Item(); }

public: // Accessors
    /// Does this iterator refer to the same item as other?
    virtual bool IsEqual(const ThisType& other) const = 0;

    /// The current item. Precondition: !Off()
    virtual const ItemType& Item() const = 0;

    /// Is this iterator beyond the end or before the start?
    virtual bool Off() const = 0;

public: // Methods
    /// Move to the first item
    virtual void Start() = 0;

    /// Move to the next item. Precondition: !Off()
    virtual void Forth() = 0;
};

/// An abstract iterator over type \c T items (T must define a default constructor and operator==)
    template <class T>
class Iterator : public IteratorInterface<T>
{
    using BaseType = IteratorInterface<T>;
public: // Types
    using ItemType = T;

protected: // Attributes
    ItemType m_item; //!< The current item

public: // ...structors
    /// An Off() iterator
    Iterator()
    {}

public: // Accessors
    /// Does this iterator refer to the same item as other?
    bool IsEqual(const BaseType& other) const override
    {
        if (this->Off() || other.Off()) // Either Off()?
            return this->Off() && other.Off(); // Both Off()?
        return this->Item() == other.Item();
    }

    /// The current item. Precondition: !Off()
    const ItemType& Item() const override { return m_item; }
};

} // namespace Foundation

#endif // FOUNDATION_ITERATOR_HEADER__
