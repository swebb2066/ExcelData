#ifndef EXCEL_HEADER_
#define EXCEL_HEADER_
#include <filesystem>
#include <memory>
#include <Foundation/Iterator.h>

#define DECLARE_PRIVATE_MEMBER_PTR(T, V) struct T; std::shared_ptr<T> V

namespace Excel
{

namespace fs = std::filesystem;

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
public: // Methods
    /// Make \c excelFilePath available. Precondition: CanLoad()
    bool Load(const fs::path& excelFilePath);
};

class Sheet
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Sheet();
    ~Sheet();
public: // Accessors
    bool operator==(const Sheet& other) const;
public: // Methods
};

class SheetIterator : public Foundation::Iterator<Sheet>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    SheetIterator();
    ~SheetIterator();
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

using CellRow = std::vector<std::string>;
class RowIterator : public Foundation::Iterator<CellRow>
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    RowIterator();
    ~RowIterator();
public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const override;
public: // Methods
    /// Move to the first \c cellRange in \c sheet
    void Start(const Sheet& sheet, const std::string& cellRange);
    /// Move to the first row
    void Start() override;
    /// Move to the next row. Precondition: !Off()
    void Forth() override;
};

} // namespace Excel
#endif // EXCEL_HEADER_