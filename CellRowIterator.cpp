#include <Foundation/FileIterator.h>
#include <Foundation/StdLogger.h>
#include <Foundation/Memory.h>
#include "CellRowIterator.h"
#include "Excel.h"

namespace Excel
{
using FileIteratorPtr = std::unique_ptr<Foundation::FileIterator>;

struct CellRowIterator::Impl
{
    FileIteratorPtr fileIter;
    Book currentBook;
    Sheet::Iterator sheetIter;
    std::string sheetPattern;
    Rows::Iterator rowIter;
    std::string cellPattern;
    Foundation::LoggerPtr log = Foundation::GetLogger("CellRowIterator");
};

CellRowIterator::CellRowIterator(const fs::path& dir, const YAML::Node& cellRangeSelector)
    : m_impl(std::make_shared<Impl>())
{
    if (auto sheets = cellRangeSelector["Sheet"])
        m_impl->sheetPattern = GetPattern(sheets);
    if (auto cells = cellRangeSelector["Cells"])
        m_impl->cellPattern = GetPattern(cells);
    auto fileSelector = cellRangeSelector["Path"];
    std::string namePattern;
    if (!fileSelector)
        namePattern = "*(.xls|.xlsx)"; // All Excel files in dir
    else if (fileSelector.IsScalar())
        namePattern = fileSelector.Scalar();
    else for (auto node : fileSelector)
        namePattern += node.Scalar();
    auto rootDir = dir.empty() ? fs::current_path() : dir;
    LOG4CXX_DEBUG(m_impl->log, "namePattern " << namePattern << " in " << rootDir);
    auto selector = std::make_shared<Foundation::PathSelector>(namePattern, rootDir);
    m_impl->fileIter = std::make_unique<Foundation::FileIterator>(rootDir, selector);
}

/// Is this iterator beyond the end or before the start?
bool CellRowIterator::Off() const { return m_item.empty(); }

/// Move to the first item
void CellRowIterator::Start()
{
    m_item.clear();
    m_impl->fileIter->Start();
    StartBook();
}

/// Move to the next item. Precondition: !Off()
void CellRowIterator::Forth()
{
    m_item.clear();
    m_impl->rowIter.Forth();
    if (!m_impl->rowIter.Off())
        SetItem();
    else
    {
        m_impl->sheetIter.Forth();
        if (!StartSheet())
        {
            m_impl->fileIter->Forth();
            StartBook();
        }
    }
}

/// Load the first sheet in the current file. Precondition: m_impl->currentBook.CanLoad()
bool CellRowIterator::StartBook()
{
    while (!m_impl->fileIter->Off())
    {
        m_impl->currentBook.Unload();
        LOG4CXX_DEBUG(m_impl->log, "StartBook: " << m_impl->fileIter->Item() << " heapUsed " << HeapUsed());
        if (m_impl->currentBook.Load(m_impl->fileIter->Item()))
        {
            m_impl->sheetIter.Start(m_impl->currentBook, m_impl->sheetPattern);
            if (StartSheet())
                return true;
        }
        else
        {
            LOG4CXX_WARN(m_impl->log, "Failed to load " << m_impl->fileIter->Item());
        }
        m_impl->fileIter->Forth();
    }
    return false;
}

/// Set m_impl->rowIter to a sheet row
bool CellRowIterator::StartSheet()
{
    while (!m_impl->sheetIter.Off())
    {
        LOG4CXX_DEBUG(m_impl->log, "StartSheet: " << m_impl->sheetIter.Item().GetName());
        m_impl->rowIter.Start(m_impl->sheetIter.Item());
        if (!m_impl->rowIter.Off() && SetItem())
            return true;
        m_impl->sheetIter.Forth();
    }
    return false;
}

/// Set m_item. Precondition: !m_impl->rowIter.Off()
bool CellRowIterator::SetItem()
{
    m_item = m_impl->rowIter.Item();
    LOG4CXX_TRACE(m_impl->log, "At: " << m_impl->rowIter.Item());
    return !m_item.empty();
}

std::string CellRowIterator::GetPattern(const YAML::Node& selector)
{
    std::string pattern;
    if (selector.IsScalar())
        pattern = selector.Scalar();
    else if (!selector.IsSequence())
        ; // All items
    else for (auto node : selector)
        pattern += node.Scalar();
    return pattern;
}

} // namespace Excel
