#ifndef   UNICODE
    #define   UNICODE
#endif
#ifndef   _UNICODE
    #define   _UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include "Excel.h"

namespace
{
HRESULT SetHidden(IDispatch* pObject);
HRESULT GetXLCell(IDispatch* pXLWorksheet, wchar_t* pszRange, wchar_t* pszCell, size_t iBufferLength);
HRESULT GetCell(IDispatch* pXLSheet, wchar_t* pszRange, VARIANT& pVt);
IDispatch* SelectWorkSheet(IDispatch* pXLWorksheets, LCID& lcid, wchar_t* pszSheet);
IDispatch* GetDispatchObject(IDispatch* pCallerObject, DISPID dispid, WORD wFlags, LCID lcid);
IDispatch* XLOpenWorkSheet(IDispatch* pXLWorkBook, wchar_t* pSheet);

IDispatch* StartExcel(LCID* plcid = 0)
{
    struct excelInstance
    {
        LCID lcid;
        IDispatch* pXLApp;
        IDispatch* pXLWorkBooks;
        excelInstance() : lcid(GetUserDefaultLCID()), pXLApp(NULL), pXLWorkBooks(NULL)
        {
            CoInitialize(NULL); // Start COM Subsystem
            const CLSID  CLSID_XLApplication = {0x00024500,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
            const IID    IID_Application     = {0x000208D5,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
            IDispatch*   pXLApp              = NULL;
            HRESULT      hr                  = CoCreateInstance
                ( CLSID_XLApplication
                , NULL
                , CLSCTX_LOCAL_SERVER
                , IID_Application
                , (void**)&pXLApp
                );
            if (SUCCEEDED(hr) && pXLApp)
            {
                SetHidden(pXLApp);
                pXLWorkBooks = GetDispatchObject(pXLApp, 572, DISPATCH_PROPERTYGET, this->lcid);
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
    VARIANT         vArgArray[1];                    
    VARIANT         vResult;                         
    VariantInit(&vArgArray[0]);                      
    VariantInit(&vResult);
    vArgArray[0].vt               = VT_BOOL;         
    vArgArray[0].boolVal          = FALSE;                 

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


IDispatch* SelectWorkSheet(IDispatch* pXLWorksheets, LCID& lcid, wchar_t* pszSheet)
{
 VARIANT         vResult;
 HRESULT         hr;
 VARIANT         vArgArray[1];
 DISPPARAMS      DispParams;
 DISPID          dispidNamed;
 IDispatch*      pXLWorksheet   = NULL;

 // Member Get Item <170> (In Index As Variant<0>) As IDispatch  >> Gets pXLWorksheet
 // [id(0x000000aa), propget, helpcontext(0x000100aa)] IDispatch* Item([in] VARIANT Index);
 VariantInit(&vResult);
 vArgArray[0].vt                = VT_BSTR;
 vArgArray[0].bstrVal           = SysAllocString(pszSheet);
 DispParams.rgvarg              = vArgArray;
 DispParams.rgdispidNamedArgs   = &dispidNamed;
 DispParams.cArgs               = 1;
 DispParams.cNamedArgs          = 0;
 hr=pXLWorksheets->Invoke(0xAA,IID_NULL,lcid,DISPATCH_PROPERTYGET,&DispParams,&vResult,NULL,NULL);
 if(FAILED(hr))
    return NULL;
 pXLWorksheet=vResult.pdispVal;
 SysFreeString(vArgArray[0].bstrVal);

 // Worksheet::Select()
 VariantInit(&vResult);
 VARIANT varReplace;
 varReplace.vt                  = VT_BOOL;
 varReplace.boolVal             = VARIANT_TRUE;
 dispidNamed                    = 0;
 DispParams.rgvarg              = &varReplace;
 DispParams.rgdispidNamedArgs   = &dispidNamed;
 DispParams.cArgs               = 1;
 DispParams.cNamedArgs          = 1;
 hr=pXLWorksheet->Invoke(0xEB,IID_NULL,lcid,DISPATCH_METHOD,&DispParams,&vResult,NULL,NULL);

 return pXLWorksheet;
}


IDispatch* GetDispatchObject(IDispatch* pCallerObject, DISPID dispid, WORD wFlags, LCID lcid)
{
 DISPPARAMS   NoArgs     = {NULL,NULL,0,0};
 VARIANT      vResult;
 HRESULT      hr;

 VariantInit(&vResult);
 hr=pCallerObject->Invoke(dispid,IID_NULL,lcid,wFlags,&NoArgs,&vResult,NULL,NULL);
 if(FAILED(hr))
    return NULL;
 else
    return vResult.pdispVal;
}

IDispatch* XLOpenWorkSheet(IDispatch* pXLWorkBook, wchar_t* pSheet)
{
 IDispatch*   pXLWorkSheets       = NULL;
 IDispatch*   pXLWorkSheet        = NULL;
 LCID         lcid;
 
 lcid=GetUserDefaultLCID();
 pXLWorkSheets=GetDispatchObject(pXLWorkBook,494,DISPATCH_PROPERTYGET,lcid); 
 pXLWorkSheet=SelectWorkSheet(pXLWorkSheets,lcid,pSheet);
 pXLWorkSheets->Release();

 return pXLWorkSheet; 
}

} // anonymous namespace

namespace Excel
{
    
struct Book::Impl
{
    LCID lcid;
    IDispatch* pXLWorkBooks;
    IDispatch* pXLWorkBook;
    Impl()
        : pXLWorkBooks(StartExcel(&lcid))
        , pXLWorkBook(NULL)
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
        m_impl->pXLWorkBook = vResult.pdispVal;
    return !!m_impl->pXLWorkBook;
}

struct Sheet::Impl
{
    IDispatch* pXLWorkBook;
    IDispatch* pXLWorkSheet;
    Impl()
        : pXLWorkBook(NULL)
        , pXLWorkSheet(NULL)
    {
    }
};

Sheet::Sheet() : m_impl(std::make_shared<Impl>())
{
}

Sheet::~Sheet()
{
}

auto Sheet::operator==(const Sheet& other) const -> bool
{
    return other.m_impl->pXLWorkBook == m_impl->pXLWorkBook && other.m_impl->pXLWorkSheet == m_impl->pXLWorkSheet;
}

struct SheetIterator::Impl
{
    Book clx;
    std::string pattern;
};

SheetIterator::SheetIterator() : m_impl(std::make_shared<Impl>())
{
}

SheetIterator::~SheetIterator()
{
}

// Is this iterator beyond the end or before the start?
    bool
SheetIterator::Off() const
{
    return true;
}

// Move to the first sheet in \c book matching \c sheetPattern
    void
SheetIterator::Start(const Book& book, const std::string& sheetPattern)
{
    m_impl->clx = book;
    m_impl->pattern = sheetPattern;
    Start();
}

// Move to the first sheet
    void
SheetIterator::Start()
{
}

// Move to the next sheet. Precondition: !Off()
    void
SheetIterator::Forth()
{
}

struct RowIterator::Impl
{
    Sheet clx;
    std::string range;
};

RowIterator::RowIterator() : m_impl(std::make_shared<Impl>())
{
}

RowIterator::~RowIterator()
{
}

// Is this iterator beyond the end or before the start?
    bool
RowIterator::Off() const
{
    return true;
}

// Move to the first \c cellRange in \c sheet
    void
RowIterator::Start(const Sheet& sheet, const std::string& cellRange)
{
    m_impl->clx = sheet;
    m_impl->range = cellRange;
    Start();
}

// Move to the first sheet
    void
RowIterator::Start()
{
}

// Move to the next sheet. Precondition: !Off()
    void
RowIterator::Forth()
{
}

} // namespace Excel
