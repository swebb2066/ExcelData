#include <Foundation/StdLogger.h>
#include <Foundation/Memory.h>
#include "ExcelIterator.h"
#include "Excel.h"

namespace Excel
{

struct CellRowIterator::Impl
{
    FileIteratorPtr fileIter;
    Book currentBook;
    Sheet::Iterator sheetIter;
    std::string sheetPattern;
    Rows::Iterator rowIter;
    std::string cellPattern;
    std::string namePattern;
    Impl(FileIteratorPtr xlFileSource, const std::string& sheetPattern_)
        : fileIter(std::move(xlFileSource))
        , sheetPattern(sheetPattern_)
        {}
    Foundation::LoggerPtr log = Foundation::GetLogger("CellRowIterator");
};

CellRowIterator::CellRowIterator(FileIteratorPtr xlFileSource, const std::string& sheetPattern)
    : m_impl(std::make_shared<Impl>(std::move(xlFileSource), sheetPattern))
{
}

void CellRowIterator::PutCellPattern(const std::string& cells)
{
    LOG4CXX_DEBUG(m_impl->log, "PutCellPattern: " << cells);
    m_impl->cellPattern = cells;
}

void CellRowIterator::PutNamePattern(const std::string& names)
{
    LOG4CXX_DEBUG(m_impl->log, "PutNamePattern: " << names);
    m_impl->namePattern = names;
}

/// Is this iterator beyond the end or before the start?
bool CellRowIterator::Off() const { return m_item.empty(); }

/// Move to the first item
void CellRowIterator::Start()
{
    LOG4CXX_TRACE(m_impl->log, "Start:");
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
    else if (m_impl->namePattern.empty())
    {
        m_impl->sheetIter.Forth();
        if (!StartSheet())
        {
            m_impl->fileIter->Forth();
            StartBook();
        }
    }
    else
    {
        m_impl->fileIter->Forth();
        StartBook();
    }

}

/// Load the first sheet in the current file.
bool CellRowIterator::StartBook()
{
    LOG4CXX_TRACE(m_impl->log, "StartBook: ");
    while (!m_impl->fileIter->Off())
    {
        m_impl->currentBook.Unload();
        LOG4CXX_DEBUG(m_impl->log, "StartBook: " << m_impl->fileIter->Item() << " heapUsed " << HeapUsed());
        if (m_impl->currentBook.Load(m_impl->fileIter->Item()))
        {
            if (!m_impl->namePattern.empty())
            {
                LOG4CXX_DEBUG(m_impl->log, "StartBook: " << " name " << m_impl->namePattern);
                auto name = m_impl->currentBook.GetName(m_impl->namePattern);
                if (name.IsValid())
                {
                    m_impl->rowIter.Start(name);
                    if (SetItem())
                        return true;
                }
                else
                {
                    LOG4CXX_WARN(m_impl->log, "Failed to find " << m_impl->namePattern << " in "  << m_impl->fileIter->Item());
                }
            }
            else
            {
                m_impl->sheetIter.Start(m_impl->currentBook, m_impl->sheetPattern);
                if (StartSheet())
                    return true;
            }
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
    LOG4CXX_TRACE(m_impl->log, "StartSheet: ");
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

struct SheetIterator::Impl
{
    FileIteratorPtr fileIter;
    Book currentBook;
    Sheet::Iterator sheetIter;
    Foundation::LoggerPtr log;
    Impl(FileIteratorPtr xlFileSource)
        : fileIter(std::move(xlFileSource))
        , log(Foundation::GetLogger("SheetIterator"))
        {}
};

SheetIterator::SheetIterator(FileIteratorPtr xlFileSource)
    : m_impl(std::make_shared<Impl>(std::move(xlFileSource)))
{
}

/// Is this iterator beyond the end or before the start?
bool SheetIterator::Off() const { return m_item.empty(); }

/// Move to the first item
void SheetIterator::Start()
{
    m_impl->fileIter->Start();
    StartBook();
}

/// Move to the next item. Precondition: !Off()
void SheetIterator::Forth()
{
    m_impl->sheetIter.Forth();
    if (m_impl->sheetIter.Off())
    {
        m_impl->fileIter->Forth();
        StartBook();
    }
    else
        SetItem();
}

/// Get the first sheet in the current file.
void SheetIterator::StartBook()
{
    m_item.clear();
    while (!m_impl->fileIter->Off())
    {
        m_impl->currentBook.Unload();
        LOG4CXX_DEBUG(m_impl->log, "StartBook: " << m_impl->fileIter->Item());
        if (m_impl->currentBook.Load(m_impl->fileIter->Item()))
        {
            m_impl->sheetIter.Start(m_impl->currentBook);
            if (!m_impl->sheetIter.Off())
            {
                m_item.push_back(m_impl->fileIter->Item().string());
                m_item.push_back(m_impl->sheetIter.Item().GetName());
                break;
            }
        }
        else
        {
            LOG4CXX_WARN(m_impl->log, "Failed to load " << m_impl->fileIter->Item());
        }
        m_impl->fileIter->Forth();
    }
}

/// Set m_item. Precondition: !m_impl->sheetIter.Off()
void SheetIterator::SetItem()
{
    auto name = m_impl->sheetIter.Item().GetName();
    LOG4CXX_DEBUG(m_impl->log, "At: " << name);
    m_item.back() = name;
}

} // namespace Excel
