#include <Foundation/FileIterator.h>
#include <boost/algorithm/string.hpp>

namespace Foundation
{

namespace fs = std::filesystem;

    log4cxx::LoggerPtr
FileIterator::m_log(log4cxx::Logger::getLogger("FileIterator"));

// An iterator over entries in \c dir that pass \c test
FileIterator::FileIterator(const fs::path& dir, const FileSelectorPtr& test)
    : m_dir(dir)
    , m_test(test)
    , m_maxDepth(test ? test->GetExpectedLevel() : -1)
{
    LOG4CXX_TRACE(m_log, "create: " << this << " dir " << m_dir << " maxDepth " << m_maxDepth);
}

// Move to the first item
    void
FileIterator::Start()
{
    std::error_code ec;
    if (fs::exists(m_dir, ec) && fs::is_directory(m_dir, ec))
    {
        LOG4CXX_DEBUG(m_log, "Start: " << m_dir);
        m_iter = fs::recursive_directory_iterator(m_dir, ec);
        SetItem();
    }
}

// Move to the next item. Precondition: !Off()
    void
FileIterator::Forth()
{
    ++m_iter;
    SetItem();
}

// The relative part of the current item
    fs::path
FileIterator::ItemRelativePath() const
{
    fs::path::iterator pDir = m_dir.begin();
    fs::path::iterator pItem = m_item.begin();
    while (m_dir.end() != pDir && *pItem == *pDir)
        ++pDir, ++pItem;
    fs::path result(*pItem);
    for (++pItem; m_item.end() != pItem; ++pItem)
        result /= *pItem;
    return result;
}

// Is this iterator beyond the end or before the start?
    bool
FileIterator::Off() const
{
    return m_iter == m_end;
}

// Set m_item
    void
FileIterator::SetItem()
{
    while (!Off())
    {
        LOG4CXX_TRACE(m_log, "SetItem:" << " level " << m_iter.depth() << ' ' << m_iter->path() << " isDir? " << fs::is_directory(m_iter->path()));
        bool skipDirectory = false;
        if (!m_test || m_test->IsIncluded(m_iter.depth(), m_iter->path(), skipDirectory))
            break;
        if (skipDirectory)
            m_iter.pop();
        else
        {
            if (0 <= m_maxDepth && m_maxDepth < m_iter.depth())
                m_iter.disable_recursion_pending();
            std::error_code ec;
            m_iter.increment(ec);
            if (ec) // Permission denied?
                m_iter.pop();
        }
    }
    if (!Off())
    {
        LOG4CXX_DEBUG(m_log, "At: " << m_iter->path());
        m_item = m_iter->path();
    }
}

    log4cxx::LoggerPtr
FileSelector::m_log(log4cxx::Logger::getLogger("FileSelector"));

// A selector (optionally) limiting the search to an \c expectedLevel tree
FileSelector::FileSelector(int expectedLevel)
    : m_expectedLevel(expectedLevel)
{
    LOG4CXX_TRACE(m_log, "create: " << this << " expectedLevel " << m_expectedLevel);
}

// Is \c entry at \c level included, and if not, can the remainder of the directory be skipped?
    bool
FileSelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = m_expectedLevel < 0 || level <= m_expectedLevel;
    if (!result)
        skipDirectory = true;
    LOG4CXX_TRACE(m_log, "IsIncluded:" << " level " << level << ' ' << entry << " result " << result << " skipDirectory? " << skipDirectory);
    return result;
}

/// A selector that chooses files with names
/// starting with \c namePrefix and (optionally) ending with \c nameSuffix
FileNameSelector::FileNameSelector
    ( const std::string& namePrefix
    , const std::string& nameSuffix
    , int expectedLevel
    )
    : BaseType(expectedLevel)
    , m_namePrefix(namePrefix)
    , m_nameSuffix(nameSuffix)
{
    LOG4CXX_DEBUG(m_log, "prefix " << m_namePrefix << " suffix " << m_nameSuffix);
}

// Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool
FileNameSelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = BaseType::IsIncluded(level, entry, skipDirectory);
    std::error_code ec;
    if (!fs::is_regular_file(entry, ec))
        result = false;
    else if (result)
    {
        std::string fileName = entry.filename().string();
        result = (m_namePrefix.empty() || boost::starts_with(fileName, m_namePrefix)) &&
                (m_nameSuffix.empty() || boost::ends_with(fileName, m_nameSuffix));
    }
    LOG4CXX_DEBUG(m_log, "IsIncluded:" << " level " << level << ' ' << entry << " result " << result << " skipDirectory? " << skipDirectory);
    return result;
}

// A selector that includes files inside a directory with a name
// starting with \c namePrefix and (optionally) ending with \c nameSuffix
DirectoryEntrySelector::DirectoryEntrySelector
    ( const std::string& namePrefix
    , const std::string& nameSuffix
    , int expectedLevel
    )
    : BaseType(expectedLevel)
    , m_namePrefix(namePrefix)
    , m_nameSuffix(nameSuffix)
{
    LOG4CXX_DEBUG(m_log, "prefix " << m_namePrefix << " suffix " << m_nameSuffix);
}

// Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool
DirectoryEntrySelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = false;
    std::error_code ec;
    if (fs::is_regular_file(entry, ec))
    {
        std::string directoryName = entry.parent_path().filename().string();
        result = (m_namePrefix.empty() || boost::starts_with(directoryName, m_namePrefix)) &&
                (m_nameSuffix.empty() || boost::ends_with(directoryName, m_nameSuffix));
    }
    if (0 <= m_expectedLevel && m_expectedLevel < level && !result)
        skipDirectory = true;
    LOG4CXX_DEBUG(m_log, "IsIncluded:" << " level " << level << ' ' << entry << " result " << result << " skipDirectory? " << skipDirectory);
    return result;
}

// A selector of directories with a name
// starting with \c namePrefix and (optionally) ending with \c nameSuffix
DirectoryNameSelector::DirectoryNameSelector
    ( const std::string& namePrefix
    , const std::string& nameSuffix
    , int expectedLevel
    )
    : BaseType(expectedLevel)
    , m_namePrefix(namePrefix)
    , m_nameSuffix(nameSuffix)
{
    LOG4CXX_DEBUG(m_log, "prefix " << m_namePrefix << " suffix " << m_nameSuffix);
}

// Is \c entry at \c level a selected directory, and if not, can the remainder of the directory be skipped?
    bool
DirectoryNameSelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = BaseType::IsIncluded(level, entry, skipDirectory);
    if (!fs::is_directory(entry))
        result = false;
    else if (result)
    {
        std::string directoryName = entry.filename().string();
        result = (m_namePrefix.empty() || boost::starts_with(directoryName, m_namePrefix)) &&
                (m_nameSuffix.empty() || boost::ends_with(directoryName, m_nameSuffix));
    }
    LOG4CXX_DEBUG(m_log, "IsIncluded:" << " level " << level << ' ' << entry << " result " << result << " skipDirectory? " << skipDirectory);
    return result;
}

// A selector of paths in \c rootDir matching the glob(7) \c globPattern
// (optionally) limiting the search to an \c expectedLevel tree
PathSelector::PathSelector(const std::string& globPattern, const fs::path& rootDir, int expectedLevel)
    : BaseType(expectedLevel)
    , m_rootDir(rootDir)
    , m_pattern
        ( GlobToECMA(globPattern)
        , std::regex_constants::ECMAScript
        | std::regex_constants::icase
        | std::regex_constants::nosubs
        )
{
    LOG4CXX_DEBUG(m_log, "rootDir " << rootDir << " expectedLevel " << expectedLevel);
}

// The ECMAScript equivalent to the glob(7) format \c globPattern
    std::string
PathSelector::GlobToECMA(const std::string& globPattern)
{
    LOG4CXX_DEBUG(m_log, "GlobToECMA:" << " glob " << globPattern);
    std::string result;
    int alternateIndex = -1;
    for (auto ch : globPattern)
    {
        if ('!' == ch && 0 == alternateIndex)
            result.push_back('^');
        else if (0 <= alternateIndex)
            result.push_back(ch);
        else if ('?' == ch)
            result.push_back('.');
        else if ('*' == ch)
            result += "[^/\\\\]*";
        else if ('.' == ch)
            result += "\\.";
        else if ('?' == ch)
            result += "\\?";
        else if ('$' == ch)
            result += "\\$";
        else if ('/' == ch || '\\' == ch)
            result += "[/\\\\]";
        else
            result.push_back(ch);
        if ('[' == ch && alternateIndex < 0)
            alternateIndex = 0;
        else if (']' == ch && 0 <= alternateIndex)
            alternateIndex = -1;
        else if (0 <= alternateIndex)
            ++alternateIndex;
    }
    LOG4CXX_DEBUG(m_log, "GlobToECMA:" << " result " << result);
    return result;
}

// Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool
PathSelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = BaseType::IsIncluded(level, entry, skipDirectory);
    if (result)
        result = std::regex_match(RelativePath(entry).string(), m_pattern);
    LOG4CXX_DEBUG(m_log, "IsIncluded:" << " level " << level << ' ' << entry << " result " << result << " skipDirectory? " << skipDirectory);
    return result;
}

// The relative part of \c entry
    fs::path
PathSelector::RelativePath(const fs::path& entry) const
{
    LOG4CXX_TRACE(m_log, entry);
    fs::path::iterator pDir = m_rootDir.begin();
    fs::path::iterator pItem = entry.begin();
    while (m_rootDir.end() != pDir && *pItem == *pDir)
        ++pDir, ++pItem;
    fs::path result(*pItem);
    for (++pItem; entry.end() != pItem; ++pItem)
        result /= *pItem;
    LOG4CXX_TRACE(m_log, "RelativePath: " << result);
    return result;
}

} // namespace Foundation
