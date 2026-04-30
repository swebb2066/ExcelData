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
class Name;
using NamePosition = std::pair<Name, int>;

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

    auto GetNameCount() const -> int;
    auto GetName(int item) const -> Name;
    auto GetName(const std::string& item) const -> Name;
    auto GetName(const std::regex& selector, int afterItem = 0) const -> NamePosition;
public: // Methods
    /// Make \c excelFilePath available. Precondition: CanLoad()
    bool Load(const fs::path& excelFilePath);
    /// Release the current book.
    void Unload();
};

class Rows;
using RowsPosition = std::pair<Rows, int>;

class Name
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Name();
    ~Name();
public: // Accessors
    bool operator==(const Name& other) const;
    bool IsValid() const;
    auto GetName() const -> std::string;
    auto GetRows() const -> Rows;
    auto GetRows(const std::regex& selector) const -> RowsPosition;
public: // Methods
    /// Release the current name.
    void Reset();
public: // Inner classes
    friend class Book;
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
    bool IsValid() const;
    auto GetName() const -> std::string;
    auto GetRows() const -> Rows;
    auto GetRows(const std::regex& selector) const -> RowsPosition;
public: // Methods
    /// Release the current sheet.
    void Reset();
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
    /// Move to the first sheet in \c book matching the regex \c sheetPattern
    void Start(const Book& book, const std::string& sheetPattern = std::string());
    /// Move to the first sheet
    void Start() override;
    /// Move to the next sheet. Precondition: !Off()
    void Forth() override;
};

class Cells;

class Rows
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Rows();
    Rows(const Sheet& parent);
    Rows(const Name& parent);
    ~Rows();
public: // Accessors
    bool operator==(const Rows& other) const;
    bool IsValid() const;
    auto GetCells(int row) const -> Cells;
public: // Methods
    /// Release the current sheet.
    void Reset();
public: // Inner classes
    class Iterator;
    friend class Sheet;
    friend class Name;
};

using CellRow = std::vector<std::string>;
class Rows::Iterator : public Foundation::Iterator<CellRow>
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
    /// Move to the first row in \c name
    void Start(const Name& name);
    /// Move to the first row in \c sheet
    void Start(const Sheet& sheet);
    /// Move to the first row
    void Start() override;
    /// Move to the next row. Precondition: !Off()
    void Forth() override;
public: // Inner classes
    friend class Rows;
};

class Cells
{
private: // Attributes
    DECLARE_PRIVATE_MEMBER_PTR(Impl, m_impl);
public: // ...structors
    Cells();
    ~Cells();
public: // Accessors
    bool operator==(const Cells& other) const;
    bool IsValid() const;
    auto GetData() const -> CellRow;
public: // Methods
public: // Inner classes
    friend class Rows;
};

} // namespace Excel
#endif // EXCEL_HEADER_