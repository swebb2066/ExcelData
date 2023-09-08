#ifndef EXCEL_HEADER_
#define EXCEL_HEADER_
#include <filesystem>
#include <memory>
#include <Foundation/Iterator.h>

#define DECLARE_PRIVATE_MEMBER_PTR(T, V) struct T; std::shared_ptr<T> V

namespace Excel
{

namespace fs = std::filesystem;

class Sheet;
using SheetPosition = std::pair<Sheet, int>;

class Book
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Book();
    ~Book();
public: // Accessors
    bool CanLoad() const;
    bool IsValid() const;
    auto GetSheetCount() const -> int;
    auto GetSheet(int item) const -> Sheet;
    auto GetSheet(const std::regex& selector, int afterItem = 0) const -> SheetPosition;
public: // Methods
    /// Make \c excelFilePath available. Precondition: CanLoad()
    bool Load(const fs::path& excelFilePath);
};

class Range;
using RangePosition = std::pair<Range, int>;

class Sheet
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Sheet();
    ~Sheet();
public: // Accessors
    bool operator==(const Sheet& other) const;
    auto GetRange(const std::string& cellRange) const -> Range;
    auto GetRange(const std::string& cellRange, const std::regex& selector) const -> RangePosition;
public: // Methods
public: // Inner classes
    class Iterator;
    friend class Book;
};

class Sheet::Iterator : public Foundation::Iterator<Sheet>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Iterator();
    ~Iterator();
public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const override;
public: // Methods
    /// Move to the first sheet in \c book matching \c sheetPattern
    void Start(const Book& book, const std::string& sheetPattern);
    /// Move to the first sheet
    void Start() override;
    /// Move to the next sheet. Precondition: !Off()
    void Forth() override;
};

class Range
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Range(const Sheet& parent, const std::string& cellRange);
    ~Range();
public: // Accessors
    bool operator==(const Range& other) const;
    bool IsValid() const;
public: // Methods
public: // Inner classes
    class Iterator;
    friend class Sheet;
};

using CellRow = std::vector<std::string>;
class Range::Iterator : public Foundation::Iterator<CellRow>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Iterator();
    ~Iterator();
public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const override;
public: // Methods
    /// Move to the first \c cellRange in \c sheet
    void Start(const Sheet& sheet, const std::string& cellPattern = std::string());
    /// Move to the first row
    void Start() override;
    /// Move to the next row. Precondition: !Off()
    void Forth() override;
};

} // namespace Excel
#endif // EXCEL_HEADER_