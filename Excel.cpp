#ifndef   UNICODE
    #define   UNICODE
#endif
#ifndef   _UNICODE
    #define   _UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <optional>
#include <regex>
#include <exception>
#include "Excel.h"
#include <Foundation/SafeArrayWrapper.h>
#include <Foundation/Logger.h>
#include "comutil.h"

namespace
{

void logMembers(const Foundation::LoggerPtr& log, IDispatch* pObj, LCID lcid)
{
    struct GUIDComparer
    {
        bool operator()(const GUID& left, const GUID& right) const
        {
            return memcmp(&left, &right, sizeof(right)) < 0;
        }
    };
    static std::map<GUID, int, GUIDComparer> alreadyLogged;
    UINT tinfoCount = 0;
    if (!log->isDebugEnabled() || FAILED(pObj->GetTypeInfoCount(&tinfoCount)))
        return;
    while (0 < tinfoCount)
    {
        --tinfoCount;
        ITypeInfo* pTInfo;
        if (FAILED(pObj->GetTypeInfo(tinfoCount, lcid, &pTInfo)))
            continue;
        TYPEATTR* pTypeAttr = 0;
        if (FAILED(pTInfo->GetTypeAttr(&pTypeAttr)))
            continue;
        if (!alreadyLogged[pTypeAttr->guid])
        {
            BSTR guidString;
            if (SUCCEEDED(StringFromCLSID(pTypeAttr->guid, &guidString)))
            {
                LOG4CXX_DEBUG(log, (const char*)_bstr_t(guidString));
                CoTaskMemFree(guidString);
            }
            for (int i = 0; i < pTypeAttr->cFuncs; ++i)
            {
                FUNCDESC* pDesc;
                if (FAILED(pTInfo->GetFuncDesc(i, &pDesc)))
                    continue;
                const char* invokeType =
                    (INVOKE_FUNC == pDesc->invkind ? "call" :
                    (INVOKE_PROPERTYGET == pDesc->invkind ? "get" :
                    (INVOKE_PROPERTYPUT == pDesc->invkind ? "put" :
                    (INVOKE_PROPERTYPUTREF == pDesc->invkind ? "putRef" :
                    "???"
                    ))));
                BSTR wName = 0;
                DWORD helpId;
                if (SUCCEEDED(pTInfo->GetDocumentation(pDesc->memid, &wName, 0, &helpId, 0)))
                {
                    LOG4CXX_DEBUG(log, invokeType
                        << " " << (const char*)_bstr_t(wName)
                        << "=" << std::dec << pDesc->memid
                        << "=0x" << std::hex << pDesc->memid
                        << " paramCount " << std::dec << pDesc->cParams
                    );
                    SysFreeString(wName);
                }
                pTInfo->ReleaseFuncDesc(pDesc);
            }
            ++alreadyLogged[pTypeAttr->guid];
        }
        pTInfo->ReleaseTypeAttr(pTypeAttr);
    }
}

HRESULT SetHidden(IDispatch* pObject, LCID lcid)
{
    VARIANT      vArgArray[1];
    VARIANT      vResult;
    VariantInit(&vArgArray[0]);
    VariantInit(&vResult);
    vArgArray[0].vt      = VT_BOOL;
    vArgArray[0].boolVal = FALSE;

    DISPID          dispidNamed   = DISPID_PROPERTYPUT;
    DISPPARAMS      dispParams;
    dispParams.rgvarg             = vArgArray;
    dispParams.rgdispidNamedArgs  = &dispidNamed;
    dispParams.cArgs              = 1;
    dispParams.cNamedArgs         = 1;
    return pObject->Invoke(0x22e, IID_NULL, lcid, DISPATCH_PROPERTYPUT, &dispParams, &vResult, NULL,NULL);
}

IDispatch* GetDispatchObject(IDispatch* pObject, DISPID dispid, WORD wFlags, LCID lcid, const std::string& objectName)
{
    DISPPARAMS   NoArgs = {NULL,NULL,0,0};
    VARIANT      vResult;
    VariantInit(&vResult);
    if (FAILED(pObject->Invoke(dispid, IID_NULL, lcid, wFlags, &NoArgs, &vResult, NULL, NULL)))
        return NULL;
    logMembers(Foundation::GetLogger("OLE." + objectName), vResult.pdispVal, lcid);
    return vResult.pdispVal;
}

struct excelInstance
{
    LCID lcid;
    IDispatch* pXLApp;
    IDispatch* pXLWorkBooks;
    excelInstance() : lcid(GetUserDefaultLCID()), pXLApp(NULL), pXLWorkBooks(NULL)
    {
        if (FAILED(CoInitialize(NULL)))
            throw std::runtime_error("Failed to initialize COM Subsystem");
        const CLSID  CLSID_XLApplication = {0x00024500,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
        const IID    IID_Application     = {0x000208D5,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
        HRESULT      hr                  = CoCreateInstance
            ( CLSID_XLApplication
            , NULL
            , CLSCTX_LOCAL_SERVER
            , IID_Application
            , (void**)&this->pXLApp
            );
        if (SUCCEEDED(hr) && this->pXLApp)
        {
            logMembers(Foundation::GetLogger("OLE.Excel"), pXLApp, this->lcid);
            SetHidden(pXLApp, this->lcid);
            this->pXLWorkBooks = GetDispatchObject(this->pXLApp, 572, DISPATCH_PROPERTYGET, this->lcid, "Workbooks");
        }
    }
    ~excelInstance()
    {
        if (this->pXLWorkBooks)
        {
            this->pXLWorkBooks->Release();
            this->pXLWorkBooks = NULL;
        }
        if (pXLApp)
        {
            DISPPARAMS NoArgs = {NULL,NULL,0,0};
            this->pXLApp->Invoke(0x12e, IID_NULL, this->lcid ,DISPATCH_METHOD ,&NoArgs ,NULL,NULL,NULL); // pXLApp->Quit()
            this->pXLApp->Release();
            this->pXLApp = NULL;
        }
        CoUninitialize();
    }
};

excelInstance& GetInstance()
{
    static excelInstance instance;
    return instance;
}

IDispatch* GetNamedDispatchObject(IDispatch* pObject, DISPID dispid, LCID lcid, const _bstr_t& name, const std::string& objectName)
{
    VARIANT         vArgArray[1];
    VARIANT         vResult;
    DISPPARAMS      DispParams;

    VariantInit(&vResult);
    vArgArray[0].vt                = VT_BSTR,
    vArgArray[0].bstrVal           = SysAllocString(name);
    DispParams.rgvarg              = vArgArray;
    DispParams.rgdispidNamedArgs   = 0;
    DispParams.cArgs               = 1;
    DispParams.cNamedArgs          = 0;
    HRESULT hr = pObject->Invoke
    (
        dispid,
        IID_NULL,
        lcid,
        DISPATCH_PROPERTYGET,
        &DispParams,
        &vResult,
        NULL,
        NULL
    );
    SysFreeString(vArgArray[0].bstrVal);
    if(FAILED(hr))
        return NULL;
    logMembers(Foundation::GetLogger("OLE." + objectName), vResult.pdispVal, lcid);
    return vResult.pdispVal;
}

int GetDispatchCount(IDispatch* pObject, LCID lcid)
{
    DISPPARAMS   NoArgs = {NULL,NULL,0,0};
    _variant_t   vResult;
    VariantInit(&vResult);
    if (FAILED(pObject->Invoke(118, IID_NULL, lcid,DISPATCH_PROPERTYGET, &NoArgs, &vResult, NULL, NULL)))
        return 0;
    else
        return vResult;
}

// Member Get Item <170> (In Index As Variant<0>) As IDispatch
IDispatch* GetDispatchItem(IDispatch* pObject, int item, LCID lcid, const std::string& objectName)
{
    VARIANT         vResult;
    HRESULT         hr;
    VARIANT         vArgArray[1];
    DISPPARAMS      DispParams;
    DISPID          dispidNamed;
    VariantInit(&vResult);
    VariantInit(&vArgArray[0]);
    vArgArray[0].vt                = VT_I4;
    vArgArray[0].lVal              = item;
    DispParams.rgvarg              = vArgArray;
    DispParams.rgdispidNamedArgs   = &dispidNamed;
    DispParams.cArgs               = 1;
    DispParams.cNamedArgs          = 0;
    if (FAILED(pObject->Invoke(0xAA,IID_NULL,lcid,DISPATCH_PROPERTYGET,&DispParams,&vResult,NULL,NULL)))
        return NULL;
    logMembers(Foundation::GetLogger("OLE." + objectName + ".Item"), vResult.pdispVal, lcid);
    return vResult.pdispVal;
}

auto GetDispatchString(IDispatch* pObject, DISPID dispid, LCID lcid) -> std::string
{
    std::string result;
    DISPPARAMS   NoArgs = {NULL,NULL,0,0};
    _variant_t   vResult;
    if (SUCCEEDED(pObject->Invoke(dispid, IID_NULL, lcid, DISPATCH_PROPERTYGET, &NoArgs, &vResult, NULL, NULL)))
        result = (_bstr_t)vResult;
    return result;
}

auto GetDispatchVariant(IDispatch* pObject, DISPID dispid, LCID lcid, VARIANT* vResult) -> bool
{
    DISPPARAMS   NoArgs = {NULL,NULL,0,0};
    return SUCCEEDED(pObject->Invoke(dispid, IID_NULL, lcid, DISPATCH_PROPERTYGET, &NoArgs, vResult, NULL, NULL));
}

} // anonymous namespace

namespace Excel
{

struct Book::Impl
{
    IDispatch* pXLWorkBook;
    IDispatch* pXLWorkSheets;
    Impl()
        : pXLWorkBook(NULL)
        , pXLWorkSheets(NULL)
    {
    }
    ~Impl()
    {
        if (pXLWorkSheets) pXLWorkSheets->Release();
        if (pXLWorkBook)
        {
            // Call WorkBook::Close(SaveChanges=False)
            VARIANT      vArgArray[1];
            VARIANT      vResult;
            VariantInit(&vArgArray[0]);
            VariantInit(&vResult);
            vArgArray[0].vt      = VT_BOOL;
            vArgArray[0].boolVal = FALSE;
            DISPID     dispidNamed;
            DISPPARAMS dispParams;
            dispParams.rgvarg              = vArgArray;
            dispParams.rgdispidNamedArgs   = &dispidNamed;
            dispParams.cArgs               = 1;
            dispParams.cNamedArgs          = 0;
            pXLWorkBook->Invoke(277, IID_NULL, GetInstance().lcid ,DISPATCH_METHOD ,&dispParams ,NULL,NULL,NULL);
            pXLWorkBook->Release();
        }
    }
};

Book::Book()
{
}

Book::~Book()
{
}

auto Book::CanLoad() const -> bool
{
    return !!GetInstance().pXLWorkBooks;
}

// Call WorkBooks::_Open()  >> Gets pXLWorkSheets
auto Book::Load(const fs::path& path) -> bool
{
    m_impl = std::make_shared<Impl>();
    VARIANT         vResult;
    VARIANT         vArgArray[1];

    VariantInit(&vResult);
    vArgArray[0].vt                = VT_BSTR;
    vArgArray[0].bstrVal           = SysAllocString(path.wstring().c_str());
    DISPID     dispidNamed;
    DISPPARAMS dispParams;
    dispParams.rgvarg              = vArgArray;
    dispParams.rgdispidNamedArgs   = &dispidNamed;
    dispParams.cArgs               = 1;
    dispParams.cNamedArgs          = 0;
    HRESULT hr = GetInstance().pXLWorkBooks->Invoke(682, IID_NULL, GetInstance().lcid, DISPATCH_METHOD, &dispParams, &vResult, NULL,NULL);
    SysFreeString(vArgArray[0].bstrVal);
    if (!FAILED(hr))
    {
        m_impl->pXLWorkBook = vResult.pdispVal;
        logMembers(Foundation::GetLogger("OLE.Workbook"), m_impl->pXLWorkBook, GetInstance().lcid);
        m_impl->pXLWorkSheets = GetDispatchObject(m_impl->pXLWorkBook, 494, DISPATCH_PROPERTYGET, GetInstance().lcid, "Worksheets");
    }
    return m_impl->pXLWorkBook && m_impl->pXLWorkSheets;
}

auto Book::IsValid() const -> bool
{
    return m_impl && m_impl->pXLWorkSheets;
}

auto Book::GetSheetCount() const -> int
{
    return IsValid() ? GetDispatchCount(m_impl->pXLWorkSheets, GetInstance().lcid) : 0;
}

struct Sheet::Impl
{
    IDispatch* pXLWorkSheet;
    std::string name;
    Impl(IDispatch* pObj = NULL)
        : pXLWorkSheet(pObj)
    {
    }
    ~Impl()
    {
        if (pXLWorkSheet) pXLWorkSheet->Release();
    }
};

auto Book::GetSheet(int item) const -> Sheet
{
    Sheet result;
    if (result.m_impl->pXLWorkSheet = GetDispatchItem(m_impl->pXLWorkSheets, item, GetInstance().lcid, "Worksheet"))
    {
        result.m_impl->name = GetDispatchString(result.m_impl->pXLWorkSheet, 110, GetInstance().lcid);
    }
    return result;
}

auto Book::GetSheet(const std::regex& selector, int afterItem) const -> SheetPosition
{
    SheetPosition result;
    auto sheetCount = GetSheetCount();
    for (result.second = afterItem + 1; result.second <= sheetCount; ++result.second)
    {
        result.first = GetSheet(result.second);
        if (result.first.IsValid())
        {
            if (std::regex_match(result.first.GetName(), selector))
                break;
        }
    }
    return result;
}

Sheet::Sheet() : m_impl(std::make_shared<Impl>())
{
}

Sheet::~Sheet()
{
}

auto Sheet::operator==(const Sheet& other) const -> bool
{
    return other.m_impl->pXLWorkSheet == m_impl->pXLWorkSheet;
}

bool Sheet::IsValid() const
{
    return !!m_impl->pXLWorkSheet;
}

auto Sheet::GetName() const -> std::string
{
    return m_impl->name;
}

struct Rows::Impl
{
    Sheet clx;
    IDispatch* pXLRows;
    Impl(const Sheet& parent)
        : clx(parent)
        , pXLRows(NULL)
    {
    }
    ~Impl()
    {
        if (pXLRows) pXLRows->Release();
    }
};

auto Sheet::GetRows() const -> Rows
{
    Rows result(*this);
    result.m_impl->pXLRows = GetDispatchObject(m_impl->pXLWorkSheet, 258, DISPATCH_PROPERTYGET, GetInstance().lcid, "Rows");
    return result;
}

auto Sheet::GetRows(const std::regex& selector) const -> RowsPosition
{
    return RowsPosition(GetRows(), 0);
}

struct Sheet::Iterator::Impl
{
    Book clx;
    std::optional<std::regex> selector;
    SheetPosition current;
};

Sheet::Iterator::Iterator() : m_impl(std::make_shared<Impl>())
{
}

Sheet::Iterator::~Iterator()
{
}

// Is this iterator beyond the end or before the start?
    bool
Sheet::Iterator::Off() const
{
    return !m_impl->current.first.m_impl->pXLWorkSheet;
}

// Move to the first sheet in \c book matching \c sheetPattern
    void
Sheet::Iterator::Start(const Book& book, const std::string& sheetPattern)
{
    m_impl->clx = book;
    if (sheetPattern.empty())
        m_impl->selector.reset();
    else
        m_impl->selector = sheetPattern;
    Start();
}

// Move to the first sheet
    void
Sheet::Iterator::Start()
{
    if (m_impl->selector)
        m_impl->current = m_impl->clx.GetSheet(m_impl->selector.value());
    else
    {
        m_impl->current.second = 1;
        m_impl->current.first = m_impl->clx.GetSheet(1);
    }
    m_item = m_impl->current.first;
}

// Move to the next sheet. Precondition: !Off()
    void
Sheet::Iterator::Forth()
{
    ++m_impl->current.second;
    if (m_impl->selector)
        m_impl->current = m_impl->clx.GetSheet(m_impl->selector.value(), m_impl->current.second);
    else
        m_impl->current.first = m_impl->clx.GetSheet(m_impl->current.second);
    m_item = m_impl->current.first;
}

Rows::Rows(const Sheet& parent) : m_impl(std::make_shared<Impl>(parent))
{
}

Rows::~Rows()
{
}

auto Rows::operator==(const Rows& other) const -> bool
{
    return other.m_impl->pXLRows == m_impl->pXLRows;
}

auto Rows::IsValid() const -> bool
{
    return !!m_impl->pXLRows;
}

struct Rows::Iterator::Impl
{
    RowsPosition current;
    Cells cells;
    Impl(const Sheet& sheet)
        : current(sheet.GetRows(), 0)
    {
    }
};

Rows::Iterator::Iterator()
{
}

Rows::Iterator::~Iterator()
{
}

// Is this iterator beyond the end or before the start?
    bool
Rows::Iterator::Off() const
{
    return m_item.empty();
}

// Move to the first row in \c sheet
    void
Rows::Iterator::Start(const Sheet& sheet)
{
    m_impl = std::make_shared<Impl>(sheet);
    Start();
}

// Move to the first row
    void
Rows::Iterator::Start()
{
    m_impl->current.second = 1;
    m_impl->cells = m_impl->current.first.GetCells(1);
    m_item = m_impl->cells.GetData();
}


// Move to the next row. Precondition: !Off()
    void
Rows::Iterator::Forth()
{
    ++m_impl->current.second;
    m_impl->cells = m_impl->current.first.GetCells(m_impl->current.second);
    m_item = m_impl->cells.GetData();
}

struct Cells::Impl
{
    IDispatch* pXLCells;
    CellRow data;
    Impl()
        : pXLCells(NULL)
    {
    }
    ~Impl()
    {
        if (pXLCells) pXLCells->Release();
    }
};

auto Rows::GetCells(int row) const -> Cells
{
    Cells result;
    if (result.m_impl->pXLCells) result.m_impl->pXLCells->Release();
    result.m_impl->pXLCells = GetDispatchItem(m_impl->pXLRows, row, GetInstance().lcid, "Cells");
    return result;
}

Cells::Cells() : m_impl(std::make_shared<Impl>())
{
}

Cells::~Cells()
{
}

auto Cells::operator==(const Cells& other) const -> bool
{
    return other.m_impl->pXLCells == m_impl->pXLCells;
}

auto Cells::IsValid() const -> bool
{
    return !!m_impl->pXLCells;
}

auto Cells::GetData() const -> CellRow
{
    if (m_impl->data.empty())
    {
        auto pItem = GetDispatchItem(m_impl->pXLCells, 1, GetInstance().lcid, "Cells");
        _variant_t itemData;
        if (pItem && GetDispatchVariant(pItem, 6, GetInstance().lcid, &itemData))
        {
            SafeArrayData sa(itemData.parray);
            m_impl->data.resize(sa.DimensionSize(1));
            sa.GetStringVector(m_impl->data.begin(), m_impl->data.end());
        }
        while (!m_impl->data.empty())
        {
            if (!m_impl->data.back().empty())
                break;
            m_impl->data.pop_back();
        }
        if (pItem)
            pItem->Release();
    }
    return m_impl->data;
}

} // namespace Excel
