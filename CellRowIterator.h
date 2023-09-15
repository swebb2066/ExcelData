#include <Foundation/Iterator.h>
#include <Foundation/Yaml.h>
#include <memory>
#include <vector>

#define DECLARE_PRIVATE_MEMBER_PTR(T, V) struct T; std::shared_ptr<T> V

namespace fs = std::filesystem;

namespace Excel
{
using CellRow = std::vector<std::string>;

/// Provide a sequence of string vectors from Excel files
class CellRowIterator : public Foundation::Iterator<CellRow>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);

public: // ...structors
    CellRowIterator(const fs::path& dir, const YAML::Node& cellRangeSelector);

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

    static std::string GetPattern(const YAML::Node& selector);
};

} // namespace Excel
