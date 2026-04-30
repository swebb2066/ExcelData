#include <Foundation/FileIterator.h>
#include <memory>
#include <vector>

#define DECLARE_PRIVATE_MEMBER_PTR(T, V) struct T; std::shared_ptr<T> V

namespace Excel
{
using CellRow = std::vector<std::string>;
using FileIteratorPtr = std::unique_ptr<Foundation::FileIterator>;

/// Provide a sequence of string vectors from Excel files
class CellRowIterator : public Foundation::Iterator<CellRow>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);

public: // ...structors
    CellRowIterator(FileIteratorPtr xlFileSource, const std::string& sheetPattern);

public: // Property modifiers
    void PutCellPattern(const std::string& cells);
    void PutNamePattern(const std::string& names);

public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const override;

public: // Methods
    /// Move to the first item
    void Start() override;

    /// Move to the next item. Precondition: !Off()
    void Forth() override;

protected: // Support methods
    /// Load the first sheet in the current file.
    bool StartBook();

    /// Get the first sheet row
    bool StartSheet();

    /// Set m_item.
    bool SetItem();
};

/// Provide a sequence of string vectors from Excel files
class SheetIterator : public Foundation::Iterator<CellRow>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);

public: // ...structors
    SheetIterator(FileIteratorPtr xlFileSource);

public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const override;

public: // Methods
    /// Move to the first item
    void Start() override;

    /// Move to the next item. Precondition: !Off()
    void Forth() override;

protected: // Support methods
    /// Load the first sheet in the current file.
    void StartBook();

    /// Set m_item.
    void SetItem();
};

} // namespace Excel
