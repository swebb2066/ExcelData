#ifndef FOUNDATION_FILE_ITERATOR_HEADER__
#define FOUNDATION_FILE_ITERATOR_HEADER__
#include <Foundation/Iterator.h>
#include <Foundation/Logger.h>
#include <filesystem>
#include <regex>

namespace Foundation
{

namespace fs = std::filesystem;

/// An base of file selectors
class FileSelector
{
protected: // Attributes
    int m_expectedLevel;

public: // ...structors
    /// A selector (optionally) limiting the search to an \c expectedLevel tree
    FileSelector(int expectedLevel = -1);
    virtual ~FileSelector() {}

public: // Accessors
    /// The maximum depth to decend
    int GetExpectedLevel() const { return m_expectedLevel; }

    /// Is \c entry at \c level included, and if not, can the remainder of the directory be skipped?
    virtual bool IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const;

protected: // class data
    static log4cxx::LoggerPtr m_log;
};
typedef std::shared_ptr<FileSelector> FileSelectorPtr;

/// An iterator over directory entries
class FileIterator : public Iterator<fs::path>
{
protected: // Attributes
    fs::path m_dir; //!< The source of files
    FileSelectorPtr m_test; //!< Selects the files
    fs::recursive_directory_iterator m_iter; //!< Steps through all the entries
    fs::recursive_directory_iterator m_end;
    int m_maxDepth; //!< The maximum depth to decend

public: // ...structors
    /// An iterator over entries in \c dir that pass \c test
    FileIterator(const fs::path& dir, const FileSelectorPtr& test = FileSelectorPtr());

public: // Property modifiers
    ///Set the maximum depth to decend
    void PutMaximumDepth(int newValue) { m_maxDepth = newValue; }

public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const override;

    /// The relative part of the current item
    fs::path ItemRelativePath() const;

public: // Methods
    /// Move to the first item
    void Start() override;

    /// Move to the next item. Precondition: !Off()
    void Forth() override;

protected: // Support methods
    /// Set m_item
    void SetItem();

protected: // class data
    static log4cxx::LoggerPtr m_log;
};

/// Select regular files based on their name
class FileNameSelector : public FileSelector
{
    typedef FileSelector BaseType;
protected: // Attributes
    std::string m_namePrefix;
    std::string m_nameSuffix;

public: // ...structors
    /// A selector of regular files with names
    /// starting with \c namePrefix and (optionally) ending with \c nameSuffix
    /// (optionally) limiting the search to an \c expectedLevel tree
    FileNameSelector
        ( const std::string& namePrefix
        , const std::string& nameSuffix = std::string()
        , int expectedLevel = -1
        );

public: // Methods
    // Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const override;
};

/// Select regular files based on the name of the directory they belong to
class DirectoryEntrySelector : public FileSelector
{
    typedef FileSelector BaseType;
protected: // Attributes
    std::string m_namePrefix;
    std::string m_nameSuffix;

public: // ...structors
    /// A selector of regular files inside a directory with a name
    /// starting with \c namePrefix and (optionally) ending with \c nameSuffix
    /// (optionally) limiting the search to an \c expectedLevel tree
    DirectoryEntrySelector
        ( const std::string& namePrefix
        , const std::string& nameSuffix = std::string()
        , int expectedLevel = -1
        );

public: // Methods
    // Is \c entry at \c level a selected directory, and if not, can the remainder of the directory be skipped?
    bool IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const override;
};

/// Select directories based on their name
class DirectoryNameSelector : public FileSelector
{
    typedef FileSelector BaseType;
protected: // Attributes
    std::string m_namePrefix;
    std::string m_nameSuffix;

public: // ...structors
    /// A selector of directories with a name
    /// starting with \c namePrefix and (optionally) ending with \c nameSuffix
    /// (optionally) limiting the search to an \c expectedLevel tree
    DirectoryNameSelector
        ( const std::string& namePrefix
        , const std::string& nameSuffix = std::string()
        , int expectedLevel = -1
        );

public: // Methods
    // Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const override;
};

/// Select paths that match a regular expression
class PathSelector : public FileSelector
{
    typedef FileSelector BaseType;
protected: // Attributes
    fs::path m_rootDir;
    std::regex m_pattern;

public: // ...structors
    /// A selector of paths in \c rootDir matching the glob(7) \c globPattern
    /// (optionally) limiting the search to an \c expectedLevel tree
    PathSelector(const std::string& globPattern, const fs::path& rootDir = ".", int expectedLevel = -1);

public: // Methods
    /// Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const override;

protected: // Support methods
    /// The relative part of \c entry
    fs::path RelativePath(const fs::path& entry) const;

protected: // class methods
    /// The ECMAScript equivalent to the glob(7) format \c globPattern
    static std::string GlobToECMA(const std::string& globPattern);
};

} // namespace Foundation

#endif // FOUNDATION_FILE_ITERATOR_HEADER__
