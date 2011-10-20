#include "stdafx.h"
#include "MDIFrameWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMainMDIFrame, CXTPMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainMDIFrame)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
    ON_UPDATE_COMMAND_UI(XTP_ID_RIBBONCONTROLTAB, OnUpdateRibbonTab)
    ON_COMMAND(XTP_ID_CUSTOMIZE, OnCustomize)
    ON_COMMAND(XTP_ID_RIBBONCUSTOMIZE, OnCustomizeQuickAccess)
END_MESSAGE_MAP()

CMainMDIFrame::CMainMDIFrame()
{
}

CMainMDIFrame::~CMainMDIFrame()
{
}

UINT CMainMDIFrame::GetFrameID() const
{
    ASSERT(m_frameNode.IsNotNull());
    return m_frameNode->GetUInt32(L"id");
}

BOOL CMainMDIFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CXTPMDIFrameWnd::PreCreateWindow(cs))
        return FALSE;

    cs.lpszClass = _T("XTPMainFrame");
    CXTPDrawHelpers::RegisterWndClass(AfxGetInstanceHandle(), cs.lpszClass, 
        CS_DBLCLKS, AfxGetApp()->LoadIcon(GetFrameID()));

    return TRUE;
}

BOOL CMainMDIFrame::LoadFrame(const Cx_ConfigSection& root)
{
    m_frameNode = root.GetSection(L"mainframe");
    m_ribbonNode = root.GetSection(L"ribbon");

    BOOL ret = CXTPMDIFrameWnd::LoadFrame(GetFrameID());

    m_frameNode.Release();
    m_ribbonNode.Release();

    return ret;
}

int CMainMDIFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CXTPMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!CreateStatusBar())
        return -1;

    if (!InitCommandBars())
        return -1;

    CXTPCommandBars* pCommandBars = GetCommandBars();

    m_wndStatusBar.SetCommandBars(pCommandBars);
    m_wndStatusBar.SetDrawDisabledText(FALSE);
    m_wndStatusBar.SetCommandBars(pCommandBars);
    m_wndStatusBar.GetStatusBarCtrl().SetMinHeight(22);
    m_wndStatusBar.GetPane(0)->SetMargins(8, 1, 2, 1);

    CXTPToolTipContext* pToolTipContext = GetCommandBars()->GetToolTipContext();
    pToolTipContext->SetStyle(xtpToolTipResource);
    pToolTipContext->ShowTitleAndDescription();
    pToolTipContext->SetMargin(CRect(2, 2, 2, 2));
    pToolTipContext->SetMaxTipWidth(180);
    pToolTipContext->SetFont(pCommandBars->GetPaintManager()->GetIconFont());

    pCommandBars->GetCommandBarsOptions()->ShowKeyboardCues(xtpKeyboardCuesShowWindowsDefault);
    pCommandBars->GetCommandBarsOptions()->bToolBarAccelTips = TRUE;

    pCommandBars->GetShortcutManager()->SetAccelerators(GetFrameID());

    if (InitRibbonTheme() && (!LoadRibbonIcons() || !CreateRibbonBar()))
    {
        TRACE0("Failed to create ribbon\n");
        return -1;
    }

    m_wndClient.Attach(this, FALSE);
    m_wndClient.GetToolTipContext()->SetStyle(xtpToolTipResource);

    LoadCommandBars(_T("CommandBars"));

    return 0;
}

BOOL CMainMDIFrame::InitRibbonTheme()
{
    CXTPCommandBars* pCommandBars = GetCommandBars();
    CXTPOffice2007Theme* pOfficeTheme = (CXTPOffice2007Theme*)(pCommandBars->GetPaintManager());

    HMODULE hThemeDll = LoadLibrary(m_frameNode->GetString(L"themeDll").c_str());
    if (hThemeDll != NULL)
    {
        pOfficeTheme->SetImages(XTPResourceImages());
        pOfficeTheme->SetImageHandle(hThemeDll, m_frameNode->GetString(L"themeName").c_str());

        XTPPaintManager()->SetTheme(xtpThemeRibbon);
        pCommandBars->SetTheme(xtpThemeRibbon);
    }

    return hThemeDll != NULL;
}

BOOL CMainMDIFrame::CreateStatusBar()
{
    static UINT indicators[] =
    {
        ID_SEPARATOR,
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_SCRL,
    };

    return m_wndStatusBar.Create(this)
        && m_wndStatusBar.SetIndicators(indicators, _countof(indicators));
}

BOOL CMainMDIFrame::LoadRibbonIcons()
{
    CXTPImageManager* pImageManager = GetCommandBars()->GetImageManager();

    for (int ibar = 0; ; ibar++)
    {
        Cx_ConfigSection barNode(m_frameNode.GetSectionByIndex(L"toolbars/bar", ibar));
        UINT barID = barNode->GetUInt32(L"id");
        if (0 == barID)
            break;

        std::vector<UINT> ids;

        for (int ibtn = 0; ; ibtn++)
        {
            Cx_ConfigSection btnNode(barNode.GetSectionByIndex(L"button", ibtn));
            UINT id = btnNode->GetUInt32(L"id");
            if (0 == id)
                break;
            ids.push_back(id);
        }

        if (ids.empty())
        {
            pImageManager->SetIcons(barID);
        }
        else
        {
            long cy = barNode->GetBool(L"large", false) ? 32 : 16;
            pImageManager->SetIcons(barID, &ids.front(), x3::GetSize(ids), CSize(cy, cy));
        }
    }

    return TRUE;
}

BOOL CMainMDIFrame::CreateRibbonBar()
{
    CXTPRibbonBar* pRibbonBar = (CXTPRibbonBar*)GetCommandBars()->Add(_T("The Ribbon"), 
        xtpBarTop, RUNTIME_CLASS(CXTPRibbonBar));
    if (!pRibbonBar)
    {
        return FALSE;
    }

    pRibbonBar->EnableDocking(0);

    CMenu menu;
    menu.Attach(::GetMenu(m_hWnd));
    SetMenu(NULL);

    CXTPControlPopup* pControlFile = (CXTPControlPopup*)pRibbonBar->AddSystemButton(0);
    pControlFile->SetCommandBar(menu.GetSubMenu(0));

    SetSystemButtonStyle(menu);
    CreateRibbonTabs(pRibbonBar);

    CXTPControl* pControlAbout = pRibbonBar->GetControls()->Add(xtpControlButton, ID_APP_ABOUT);
    pControlAbout->SetFlags(xtpFlagRightAlign);

    pRibbonBar->GetQuickAccessControls()->Add(xtpControlButton, ID_FILE_SAVE);
    pRibbonBar->GetQuickAccessControls()->Add(xtpControlButton, ID_EDIT_UNDO);
    pRibbonBar->GetQuickAccessControls()->Add(xtpControlButton, ID_FILE_PRINT);
    pRibbonBar->GetQuickAccessControls()->CreateOriginalControls();

    pRibbonBar->EnableFrameTheme();

    return TRUE;
}

void CMainMDIFrame::CreateRibbonTabs(CXTPRibbonBar* pRibbonBar)
{
    for (int iTab = 0; ; iTab++)
    {
        Cx_ConfigSection tabNode(m_ribbonNode.GetSectionByIndex(L"tab", iTab));
        std::wstring caption(tabNode->GetString(L"caption"));
        if (caption.empty())
            break;

        CXTPRibbonTab* pTab = pRibbonBar->AddTab(caption.c_str());
    }
}

void CMainMDIFrame::OnUpdateRibbonTab(CCmdUI* pCmdUI)
{
    CXTPRibbonControlTab* pControl = DYNAMIC_DOWNCAST(CXTPRibbonControlTab, CXTPControl::FromUI(pCmdUI));
    if (!pControl)
        return;

    //CXTPRibbonTab* pTab = pControl->FindTab(ID_TAB_EDIT);
    //pTab->SetVisible(MDIGetActive() != NULL);
}

void CMainMDIFrame::SetSystemButtonStyle(const CMenu& menu)
{
    CXTPRibbonBar* pRibbonBar = DYNAMIC_DOWNCAST(CXTPRibbonBar, GetCommandBars()->GetMenuBar());
    std::wstring themeName(m_frameNode->GetString(L"themeName"));

    if (themeName.find(L"WINDOWS7") != std::wstring::npos
        || themeName.find(L"OFFICE2010") != std::wstring::npos)
    {
        CString caption;
        menu.GetMenuString(0, caption, MF_BYPOSITION);
        pRibbonBar->GetSystemButton()->SetCaption(caption);
        pRibbonBar->GetSystemButton()->SetStyle(xtpButtonCaption);
    }
}

void CMainMDIFrame::OnSetPreviewMode(BOOL bPreview, CPrintPreviewState* pState)
{
    GetCommandBars()->OnSetPreviewMode(bPreview);
    m_wndClient.ShowWorkspace(!bPreview);

    CXTPMDIFrameWnd::OnSetPreviewMode(bPreview, pState);
}

void CMainMDIFrame::OnClose() 
{   
    /*  CXTPPropExchangeXMLNode px(FALSE, 0, _T("Settings"));

    if (px.OnBeforeExchange()) 
    {
    CXTPPropExchangeSection pxCommandBars(px.GetSection(_T("CommandBars")));
    XTP_COMMANDBARS_PROPEXCHANGE_PARAM param; 
    param.bSerializeControls = TRUE; 
    param.bSaveOriginalControls = FALSE;
    param.bSerializeOptions = TRUE;
    GetCommandBars()->DoPropExchange(&pxCommandBars, &param);

    px.SaveToFile(_T("C:\\save.xml"));
    }*/

    SaveCommandBars(_T("CommandBars"));

    CXTPMDIFrameWnd::OnClose();
}

void CMainMDIFrame::ShowCustomizeDialog(int nSelectedPage)
{
    CXTPCustomizeSheet cs(GetCommandBars());
    UINT resid = GetFrameID();

    CXTPRibbonCustomizeQuickAccessPage pageQuickAccess(&cs);
    cs.AddPage(&pageQuickAccess);
    pageQuickAccess.AddCategories(resid);

    CXTPCustomizeKeyboardPage pageKeyboard(&cs);
    cs.AddPage(&pageKeyboard);
    pageKeyboard.AddCategories(resid);

    CXTPCustomizeOptionsPage pageOptions(&cs);
    cs.AddPage(&pageOptions);

    CXTPCustomizeCommandsPage* pCommands = cs.GetCommandsPage();
    pCommands->AddCategories(resid);

    cs.SetActivePage(nSelectedPage);

    cs.DoModal();
}

void CMainMDIFrame::OnCustomize()
{
    ShowCustomizeDialog(0);
}

void CMainMDIFrame::OnCustomizeQuickAccess()
{
    ShowCustomizeDialog(2);
}