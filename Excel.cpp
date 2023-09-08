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
#include <Foundation/Logger.h>
#include "comutil.h"

namespace
{

struct GUIDComparer
{
    bool operator()(const GUID& left, const GUID& right) const
    {
        return memcmp(&left, &right, sizeof(right)) < 0;
    }
};

void logMembers(const Foundation::LoggerPtr& log, IDispatch* pObj, LCID lcid)
{
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
                        << " paramCount " << pDesc->cParams
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

HRESULT SetHidden(IDispatch* pObject);
HRESULT GetXLCell(IDispatch* pXLWorksheet, wchar_t* pszRange, wchar_t* pszCell, size_t iBufferLength);
HRESULT GetCell(IDispatch* pXLSheet, wchar_t* pszRange, VARIANT& pVt);
IDispatch* GetDispatchObject(IDispatch* pObject, DISPID dispid, WORD wFlags, LCID lcid, const std::string& objectName);

IDispatch* StartExcel(LCID* plcid = 0)
{
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
                , (void**)&pXLApp
                );
            if (SUCCEEDED(hr) && pXLApp)
            {
                logMembers(Foundation::GetLogger("OLE.Excel"), pXLApp, this->lcid);
                SetHidden(pXLApp);
                pXLWorkBooks = GetDispatchObject(pXLApp, 572, DISPATCH_PROPERTYGET, this->lcid, "Workbooks");
            }
        }
        ~excelInstance()
        {
            if (pXLWorkBooks)
            {
                pXLWorkBooks->Release();
                pXLWorkBooks = NULL;
            }
            if (pXLApp)
            {
                DISPPARAMS NoArgs = {NULL,NULL,0,0};
                pXLApp->Invoke(0x12e, IID_NULL, LOCALE_USER_DEFAULT ,DISPATCH_METHOD ,&NoArgs ,NULL,NULL,NULL); // pXLApp->Quit() 0x12E
                pXLApp->Release();
                pXLApp = NULL;
            }
            CoUninitialize();
        }
    };
    static excelInstance instance;
    if (plcid)
        *plcid = instance.lcid;
    return instance.pXLWorkBooks;
}

HRESULT SetHidden(IDispatch* pObject)
{
    VARIANT      vArgArray[1];
    VARIANT      vResult;
    VariantInit(&vArgArray[0]);
    VariantInit(&vResult);
    vArgArray[0].vt      = VT_BOOL;
    vArgArray[0].boolVal = FALSE;

    DISPID          dispidNamed   = DISPID_PROPERTYPUT;
    LCID            lcid          = GetUserDefaultLCID();
    DISPPARAMS      dispParams;
    dispParams.rgvarg             = vArgArray;
    dispParams.rgdispidNamedArgs  = &dispidNamed;
    dispParams.cArgs              = 1;
    dispParams.cNamedArgs         = 1;
    HRESULT hr = pObject->Invoke(0x0000022e, IID_NULL, lcid, DISPATCH_PROPERTYPUT, &dispParams, &vResult, NULL,NULL);

    return hr;
}


HRESULT GetXLCell(IDispatch* pXLWorksheet, wchar_t* pszRange, wchar_t* pszCell, size_t iBufferLength)
{
    VARIANT         vArgArray[1];
    VARIANT         vResult;
    DISPPARAMS      DispParams;
    LCID            lcid;

    VariantInit(&vResult);
    lcid                           = GetUserDefaultLCID();
    vArgArray[0].vt                = VT_BSTR,
    vArgArray[0].bstrVal           = SysAllocString(pszRange);
    DispParams.rgvarg              = vArgArray;
    DispParams.rgdispidNamedArgs   = 0;
    DispParams.cArgs               = 1;
    DispParams.cNamedArgs          = 0;
    HRESULT hr = pXLWorksheet->Invoke
    (
        0xC5,
        IID_NULL,
        lcid,
        DISPATCH_PROPERTYGET,
        &DispParams,
        &vResult,
        NULL,
        NULL
    );
    if(FAILED(hr))
        return E_FAIL;
    IDispatch* pXLRange = vResult.pdispVal;

    // Member Get Value <6> () As Variant
    DISPPARAMS      NoArgs         = {NULL,NULL,0,0};
    VariantClear(&vArgArray[0]);
    hr=pXLRange->Invoke
        (
        6,
        IID_NULL,
        lcid,
        DISPATCH_PROPERTYGET,
        &NoArgs,
        &vResult,
        NULL,
        NULL
        );
    if(SUCCEEDED(hr))
    {
        if(vResult.vt==VT_BSTR)
        {
           if(SysStringLen(vResult.bstrVal)<iBufferLength)
           {
              wcscpy(pszCell,vResult.bstrVal);
              VariantClear(&vResult);
              return S_OK;
           }
           else
           {
              VariantClear(&vResult);
              return E_FAIL;
           }
        }
        else
        {
           pszCell[0]=0;
           VariantClear(&vResult);
        }
    }
    pXLRange->Release();

    return E_FAIL;
    }


HRESULT GetCell(IDispatch* pXLSheet, wchar_t* pszRange, VARIANT& pVt)
{
    DISPPARAMS      NoArgs         = {NULL,NULL,0,0};
    IDispatch*      pXLRange       = NULL;
    VARIANT         vArgArray[1];
    VARIANT         vResult;
    DISPPARAMS      DispParams;
    HRESULT         hr;
    LCID            lcid;

    VariantInit(&vResult);
    vArgArray[0].vt                = VT_BSTR,
    vArgArray[0].bstrVal           = SysAllocString(pszRange);
    DispParams.rgvarg              = vArgArray;
    DispParams.rgdispidNamedArgs   = 0;
    DispParams.cArgs               = 1;
    DispParams.cNamedArgs          = 0;
    lcid                           = GetUserDefaultLCID();
    hr=pXLSheet->Invoke(0xC5,IID_NULL,lcid,DISPATCH_PROPERTYGET,&DispParams,&vResult,NULL,NULL);
    if(FAILED(hr))
        return hr;
    pXLRange=vResult.pdispVal;

    //Member Get Value <6> () As Variant
    VariantClear(&vArgArray[0]);
    VariantClear(&pVt);
    hr=pXLRange->Invoke(6,IID_NULL,lcid,DISPATCH_PROPERTYGET,&NoArgs,&pVt,NULL,NULL);
    pXLRange->Release();

    return hr;
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


IDispatch* GetNamedDispatchObject(IDispatch* pObject, DISPID dispid, LCID lcid, const _bstr_t& name, const std::string& objectName)
{
    VARIANT         vArgArray[1];
    VARIANT         vResult;
    DISPPARAMS      DispParams;

    VariantInit(&vResult);
    vArgArray[0].vt                = VT_BSTR,
    vArgArray[0].bstrVal           = name;
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
    logMembers(Foundation::GetLogger("OLE." + objectName), vResult.pdispVal, lcid);
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

} // anonymous namespace

namespace Excel
{

struct Book::Impl
{
    LCID lcid;
    IDispatch* pXLWorkBooks;
    IDispatch* pXLWorkBook;
    IDispatch* pXLWorkSheets;
    Impl()
        : pXLWorkBooks(StartExcel(&lcid))
        , pXLWorkBook(NULL)
        , pXLWorkSheets(NULL)
    {
    }
};

Book::Book() : m_impl(std::make_shared<Impl>())
{
}

Book::~Book()
{
}

auto Book::CanLoad() const -> bool
{
    return !!m_impl->pXLWorkBooks;
}

auto Book::IsValid() const -> bool
{
    return !!m_impl->pXLWorkBook;
}

// Call WorkBooks::Open() - 682  >> Gets pXLWorkBook
auto Book::Load(const fs::path& path) -> bool
{
    if (m_impl->pXLWorkBook)
    {
        m_impl->pXLWorkBook->Release();
        m_impl->pXLWorkBook = NULL;
    }
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
    HRESULT hr = m_impl->pXLWorkBooks->Invoke(682, IID_NULL, m_impl->lcid, DISPATCH_METHOD, &dispParams, &vResult, NULL,NULL);
    SysFreeString(vArgArray[0].bstrVal);
    if (!FAILED(hr))
    {
        m_impl->pXLWorkBook = vResult.pdispVal;
        logMembers(Foundation::GetLogger("OLE.Workbook"), m_impl->pXLWorkBook, m_impl->lcid);
        m_impl->pXLWorkSheets = GetDispatchObject(m_impl->pXLWorkBook, 494, DISPATCH_PROPERTYGET, m_impl->lcid, "Worksheets");
    }
    return m_impl->pXLWorkBook && m_impl->pXLWorkSheets;
}

auto Book::GetSheetCount() const -> int
{
    return GetDispatchCount(m_impl->pXLWorkSheets, m_impl->lcid);
}

struct Sheet::Impl
{
    IDispatch* pXLWorkSheet;
    Foundation::LoggerPtr log = Foundation::GetLogger("Worksheet");
    Impl(IDispatch* pObj = NULL)
        : pXLWorkSheet(pObj)
    {
    }
    auto GetName(LCID lcid) const -> std::string
    {
        std::string result;
        if (this->pXLWorkSheet)
        {
            result = GetDispatchString(this->pXLWorkSheet, 110, lcid);
            LOG4CXX_DEBUG(this->log, result);
        }
        return result;
    }
};

auto Book::GetSheet(int item) const -> Sheet
{
    Sheet result;
    result.m_impl->pXLWorkSheet = GetDispatchItem(m_impl->pXLWorkSheets, item, m_impl->lcid, "Worksheet");
    return result;
}

auto Book::GetSheet(const std::regex& selector, int afterItem) const -> SheetPosition
{
    SheetPosition result;
    auto sheetCount = GetSheetCount();
    for (result.second = afterItem + 1; result.second <= sheetCount; ++result.second)
    {
        if (result.first.m_impl->pXLWorkSheet)
            result.first.m_impl->pXLWorkSheet->Release();
        result.first = GetSheet(result.second);
        if (result.first.m_impl->pXLWorkSheet)
        {
            if (std::regex_match(result.first.m_impl->GetName(m_impl->lcid), selector))
                break;
        }
    }
    if (sheetCount < result.second && result.first.m_impl->pXLWorkSheet)
    {
        result.first.m_impl->pXLWorkSheet->Release();
        result.first.m_impl->pXLWorkSheet = NULL;
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

struct Range::Impl
{
    Sheet clx;
    _bstr_t range;
    LCID lcid;
    IDispatch* pXLRange;
    Impl(const Sheet& parent, const std::string& cellRange)
        : clx(parent)
        , range(cellRange.c_str())
        , pXLRange(NULL)
    {
        StartExcel(&lcid);
    }
};

Range::Range(const Sheet& parent, const std::string& cellRange) : m_impl(std::make_shared<Impl>(parent, cellRange))
{
}

Range::~Range()
{
}

auto Range::operator==(const Range& other) const -> bool
{
    return other.m_impl->pXLRange == m_impl->pXLRange;
}

auto Range::IsValid() const -> bool
{
    return !!m_impl->pXLRange;
}

auto Sheet::GetRange(const std::string& cellRange) const -> Range
{
    Range result(*this, cellRange);
    result.m_impl->pXLRange = GetNamedDispatchObject(m_impl->pXLWorkSheet, 0xc5, result.m_impl->lcid, result.m_impl->range, "Range");
    return result;
}

auto Sheet::GetRange(const std::string& cellRange, const std::regex& selector) const -> RangePosition
{
    return RangePosition(GetRange(cellRange), 0);
}

struct Sheet::Iterator::Impl
{
    Book clx;
    std::optional<std::regex> selector;
    SheetPosition current;
    Foundation::LoggerPtr log = Foundation::GetLogger("SheetIterator");
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

struct Range::Iterator::Impl
{
    RangePosition current;
    Impl(const Sheet& sheet, const std::string& cellRange)
        : current(sheet.GetRange(cellRange), 0)
    {}
};

Range::Iterator::Iterator()
{
}

Range::Iterator::~Iterator()
{
}

// Is this iterator beyond the end or before the start?
    bool
Range::Iterator::Off() const
{
    return !m_impl || !m_impl->current.first.IsValid();
}

// Move to the first \c cellRange in \c sheet
    void
Range::Iterator::Start(const Sheet& sheet, const std::string& cellRange)
{
    m_impl = std::make_shared<Impl>(sheet, cellRange);
    Start();
}

// Move to the first row
    void
Range::Iterator::Start()
{
    m_impl.reset();
}

// Move to the next row. Precondition: !Off()
    void
Range::Iterator::Forth()
{
}

} // namespace Excel
