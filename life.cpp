#define STRICT
#define SET 0
#define RESET 1
#define INVERT 2
#define TIMER 0
#define KEYBOARD 1
#define CONTROL_KEY 0x1
#define SHIFT_KEY 0x2
#define ALT_KEY 0x4
//#define PM_PRINTNTURN 20000
#define MIN_TIMER_CONSTANT 2000/55
#include <windows.h>
#include <mem.h>
#include <windowsx.h>
#include <commdlg.h>
#include <string.h>
#include <stdio.h>
#include "life.hpp"
//#define FALSE 0
//#define TRUE 1
typedef struct {
	 UINT long nTurn;
	 int TimerMode;
	 int TimerPeriod;
	 BYTE data[64][256];

	 HWND hwnd;
	 int HorzPosit;
	 int VertPosit;
	 int HorzLen;
	 int VertLen;
	 RECT WinSize;
	 int Line;
	 //int HorzSize;
	 //int VertSize;
	 BOOL titled;
	 BOOL busy;
	 char FieldName[256];
	 BOOL Running;
	 BOOL RunningCycle;
	 BOOL SecondPass;
	 } one_field;
BOOL Register(HINSTANCE);
LRESULT CALLBACK _export MainWndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK _export ChildWndProc(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK FileOpen();
BOOL CALLBACK FileNew();
BOOL CALLBACK FileSave(int);
BOOL CALLBACK FileSaveAs(int);
BOOL CALLBACK FileSaveAll();
void CALLBACK ClearField(int);
UINT CALLBACK FoundFree();
BOOL CALLBACK CreateField(int,char *);
BOOL CALLBACK SavingDataFile(HFILE,UINT,UINT,one_field *);
BOOL CALLBACK DrawLine(HDC,int,int,int,int);
//void CALLBACK UpdateMenu();
void CALLBACK SetRangePos(int,int,int,int,int);
BOOL CALLBACK GetCellState(int,UINT,UINT);
BOOL CALLBACK GetCellChangingState(int,UINT,UINT);
void CALLBACK SetCellState(int,UINT,UINT,int);
void CALLBACK SetCellChangingState(int,UINT,UINT,int);
void CALLBACK UpdateField(int);
void CALLBACK DeleteField(int);
//void CALLBACK ClearUpdateField();
void RunTimer();
void StopTimer(int);
void FirstPass(int);
void SecondPass();
void UpdateMenu(int);
int max(int,int);
int min(int,int);

int const MaxFields=3;
char const szMainClassName[]="Main_Window_Game_Class";
char const szChildClassName[]="Child_Window_Game_Class";
char szMainWindowTitle[]="Жизнь";

int const CELL_SIZE=8;
int const FIELD_SIZE=CELL_SIZE*256+1;
int const CIRCLE_CENTER=CELL_SIZE/2;
int const CIRCLE_RADIUS=CELL_SIZE/4;
int ControlKey;
int Toggle=INVERT;
int CurrentField,ActiveField;
int nOpenedFiles=0,nUntitledFiles=0;
static int cyChar;//????????
static TEXTMETRIC tm;
one_field Field[MaxFields];
OPENFILENAME ofn;
HINSTANCE hInstance;
BOOL OpenOff,SaveOff=TRUE,RunOff;
HCURSOR FirstCursor,SecondCursor;
HWND MainHwnd;
//HWND ChildHwnd1;
HMENU hmenu;
LPARAM uParam;
HICON MainIcon;
#pragma argsused
int PASCAL
WinMain(HINSTANCE hInst,
		  HINSTANCE hPrevInst,
		  LPSTR lpszCmdLine,
		  int nCmdShow) {
  HDC hdc;
  hInstance=hInst;
  MSG msg;
  if (!Register(hInstance))
	 return FALSE;
  hmenu=LoadMenu(hInstance,"GAME_MENU");
  if((MainHwnd=CreateWindow(
		szMainClassName,
		szMainWindowTitle,
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		hmenu,
		hInstance,
		NULL))==0)
	 return FALSE;
  hdc=GetDC(MainHwnd);
  GetTextMetrics(hdc,&tm);
  ReleaseDC(MainHwnd,hdc);
  cyChar=tm.tmHeight+tm.tmExternalLeading;
  while(GetMessage(&msg,0,0,0)) {
	 DispatchMessage(&msg);}
  return msg.wParam;}
BOOL Register(HINSTANCE hInstance) {
  ATOM aWndClass;
  WNDCLASS wc;
  memset(&wc,0,sizeof(wc));
  wc.style=0;
  wc.lpfnWndProc=(WNDPROC) MainWndProc;
  wc.cbClsExtra=0;
  wc.cbWndExtra=0;
  wc.hInstance=hInstance;
  MainIcon=LoadIcon(hInstance,"FIRST_ICON");
  wc.hIcon=MainIcon;
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground=(HBRUSH) (COLOR_WINDOW+1);
  wc.lpszMenuName=NULL; //"GAME_MENU";
  wc.lpszClassName=(LPSTR) szMainClassName;
  aWndClass=RegisterClass(&wc);
  if(aWndClass==0)
	 return FALSE;
  wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
  wc.lpfnWndProc=(WNDPROC) ChildWndProc;
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.lpszMenuName=(LPSTR) NULL;
  wc.lpszClassName=(LPSTR) szChildClassName;
  FirstCursor=LoadCursor(hInstance,"UP_CURSOR");
  SecondCursor=LoadCursor(hInstance,"DOWN_CURSOR");
  wc.hCursor=FirstCursor;
  aWndClass=RegisterClass(&wc);
  if(aWndClass==0)
	 return FALSE;
  return TRUE;}
LRESULT CALLBACK _export
  MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
  static POINT pt;
  RECT rc;
  switch (msg) {
	 case WM_KEYDOWN:{
		GetCursorPos(&pt);
		ScreenToClient(Field[ActiveField].hwnd,&pt);
		switch (wParam) {
		  case VK_SPACE: {
			 if (!RunOff)
				PostMessage(hwnd,WM_COMMAND,CM_OPTIONSCONTINUE,0);
			 return 0;}
		  case VK_RETURN: {
			 PostMessage(Field[ActiveField].hwnd,WM_LBUTTONDOWN,0,(((DWORD)pt.y)<<16)+pt.x);
			 return 0;}
		  case VK_EXECUTE: {
			 if (!RunOff)
				PostMessage(hwnd,WM_COMMAND,CM_OPTIONSCONTINUE,0);
			 return 0;}
		  case VK_SELECT: {
			 PostMessage(Field[ActiveField].hwnd,WM_LBUTTONDOWN,0,(((DWORD)pt.y)<<16)+pt.x);
			 return 0;}
		  case VK_ESCAPE: {
			 if (RunOff)
				PostMessage(hwnd,WM_COMMAND,CM_OPTIONSSTOP,0);
			 return 0;}
		  case VK_RIGHT: {
			 if (Field[ActiveField].Running) {
				PostMessage(Field[ActiveField].hwnd,WM_HSCROLL,SB_LINERIGHT,0);
				return 0;}
			 else
				pt.x++;
			 break;}
		  case VK_LEFT: {
			 if (Field[ActiveField].Running) {
				PostMessage(Field[ActiveField].hwnd,WM_HSCROLL,SB_LINELEFT,0);
				return 0;}
			 else
				pt.x--;
			 break;}
		  case VK_DOWN: {
			 if (Field[ActiveField].Running) {
				PostMessage(Field[ActiveField].hwnd,WM_VSCROLL,SB_LINEDOWN,0);
				return 0;}
			 else
				pt.y++;
			 break;}
		  case VK_UP: {
			 if (Field[ActiveField].Running) {
				PostMessage(Field[ActiveField].hwnd,WM_VSCROLL,SB_LINEUP,0);
				return 0;}
			 else
				pt.y--;
			 break;}
		  case VK_PRIOR: {
			 PostMessage(Field[ActiveField].hwnd,WM_VSCROLL,SB_PAGEUP,0);
			 return 0;}
		  case VK_NEXT: {
			 PostMessage(Field[ActiveField].hwnd,WM_VSCROLL,SB_PAGEDOWN,0);
			 return 0;}
		  case VK_HOME: {
			 PostMessage(Field[ActiveField].hwnd,WM_HSCROLL,SB_PAGELEFT,0);
			 return 0;}
		  case VK_END: {
			 PostMessage(Field[ActiveField].hwnd,WM_HSCROLL,SB_PAGERIGHT,0);
			 return 0;}
		  case VK_HELP: {
			 PostMessage(hwnd,WM_COMMAND,CM_HELPCONTENTS,0);
			 return 0;}
/*		  case VK_F1: {
			 PostMessage(hwnd,WM_COMMAND,CM_HELPCONTENTS,0);
			 return 0;}
		  case VK_F2: {
			 if (!SaveOff)
				PostMessage(hwnd,WM_COMMAND,CM_FILESAVE,0);
			 return 0;}
		  case VK_F3: {
			 if (!OpenOff)
				PostMessage(hwnd,WM_COMMAND,CM_FILEOPEN,0);
			 return 0;}*/
		  case VK_CONTROL: {
			 ControlKey|=CONTROL_KEY;
			 return 0;}
		  case VK_SHIFT: {
			 ControlKey|=SHIFT_KEY;
			 return 0;}
		  case VK_MENU: {
			 ControlKey|=ALT_KEY;
			 return 0;}
/*		  case 'B': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  if (!RunOff)
					 PostMessage(hwnd,WM_COMMAND,CM_OPTIONSBEGIN,0);
				  return 0;}
				default:
				  return 0;}}
		  case 'P': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  PostMessage(hwnd,WM_COMMAND,CM_OPTIONSTIMER_PAUSE,0);
				  return 0;}
				default:
				  return 0;}}
		  case 'C': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  PostMessage(hwnd,WM_COMMAND,CM_WINDOWCASCADE,0);
				  return 0;}
				default:
				  return 0;}}
		  case 'T': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  PostMessage(hwnd,WM_COMMAND,CM_WINDOWTILE,0);
				  return 0;}
				default:
				  return 0;}}
		  case 'L': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  PostMessage(hwnd,WM_COMMAND,CM_WINDOWCLOSE_ALL,0);
				  return 0;}
				default:
				  return 0;}}
		  case 'M': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  PostMessage(hwnd,WM_COMMAND,CM_WINDOWMINIMIZE_ALL,0);
				  return 0;}
				default:
				  return 0;}}
		  case 'R': {
			 switch (ControlKey) {
				case CONTROL_KEY: {
				  PostMessage(hwnd,WM_COMMAND,CM_WINDOWRESTORE_ALL,0);
				  return 0;}
				default:
				  return 0;}}*/
		 default:
			 return DefWindowProc(hwnd,msg,wParam,lParam);}
	  GetClientRect(Field[ActiveField].hwnd,&rc);
	  pt.y=max(min(pt.y,rc.bottom),rc.top);
	  pt.x=max(min(pt.x,rc.right),rc.left);
	  ClientToScreen(Field[ActiveField].hwnd,&pt);
	  SetCursorPos(pt.x,pt.y);}
	 case WM_KEYUP: {
		switch (wParam) {
		  case VK_CONTROL: {
			 ControlKey&=!(CONTROL_KEY);
			 return 0;}
		  case VK_SHIFT: {
			 ControlKey&=!(SHIFT_KEY);
			 return 0;}
		  case VK_MENU: {
			 ControlKey&=!(ALT_KEY);
			 return 0;}
	     case VK_RETURN: {
			 PostMessage(Field[ActiveField].hwnd,WM_LBUTTONUP,0,(((DWORD)pt.y)<<16)+pt.x);
			 return 0;}
			 default:
			 return DefWindowProc(hwnd,msg,wParam,lParam);}}
	 case WM_COMMAND: {
		switch (wParam) {
		  case CM_FILENEW: {
			 FileNew();
			 return 0;}
		  case CM_FILEOPEN: {
			 FileOpen();
			 return 0;}
		  case CM_FILESAVE: {
			 FileSave(ActiveField);
			 return 0;}
		  case CM_FILESAVE_AS: {
			 FileSaveAs(ActiveField);
			 return 0;}
		  case CM_FILESAVE_ALL: {
			 FileSaveAll();
			 return 0;}
		  case CM_FILEEXIT: {
			 DestroyWindow(hwnd);
			 return 0;}
		  case CM_EDITCLEAR: {
			 ClearField(ActiveField);
			 UpdateField(ActiveField);
			 //ClearUpdateField();
			 return 0;}
		  case CM_TOGGLESET: {
			 Toggle=SET;
			 CheckMenuItem(hmenu,CM_TOGGLESET,MF_CHECKED);
			 CheckMenuItem(hmenu,CM_TOGGLERESET,MF_UNCHECKED);
			 CheckMenuItem(hmenu,CM_TOGGLESWITCH,MF_UNCHECKED);
			 return 0;}
		  case CM_TOGGLERESET: {
			 Toggle=RESET;
			 CheckMenuItem(hmenu,CM_TOGGLESET,MF_UNCHECKED);
			 CheckMenuItem(hmenu,CM_TOGGLERESET,MF_CHECKED);
			 CheckMenuItem(hmenu,CM_TOGGLESWITCH,MF_UNCHECKED);
			 return 0;}
		  case CM_TOGGLESWITCH: {
			 Toggle=INVERT;
			 CheckMenuItem(hmenu,CM_TOGGLESET,MF_UNCHECKED);
			 CheckMenuItem(hmenu,CM_TOGGLERESET,MF_UNCHECKED);
			 CheckMenuItem(hmenu,CM_TOGGLESWITCH,MF_CHECKED);
			 return 0;}
		  case CM_OPTIONSBEGIN: {
			 Field[ActiveField].nTurn=0;
			 RunTimer();
			 return 0;}
		  case CM_OPTIONSCONTINUE: {
			 RunTimer();
			 return 0;}
		  case CM_OPTIONSSTOP: {
			 StopTimer(ActiveField);
			 return 0;}
		  case CM_MODETIMING: {
			 Field[ActiveField].TimerMode=TIMER;
			 return 0;}
		  case CM_MODEKEYBOARD: {
			 Field[ActiveField].TimerMode=KEYBOARD;
			 return 0;}
		  default:
			 return DefWindowProc(hwnd,msg,wParam,lParam);}}
	 case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;}
	 default:
		return DefWindowProc(hwnd,msg,wParam,lParam);}}
LRESULT CALLBACK _export
  ChildWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
  UINT k,l;
  int i,j,HorzShift,VertShift;
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rcWndRect;
  char buf[11];
  for (i=0;i<MaxFields;i++)
	 if (Field[i].hwnd==hwnd) {
		CurrentField=i;
		break;}
  switch (msg) {
/*	 case PM_PRINTNTURN: {
		GetClientRect(Field[CurrentField].hwnd,&rcWndRect);
		rcWndRect.bottom=cyChar;
		InvalidateRect(Field[CurrentField].hwnd,&rcWndRect,TRUE);
		hdc=GetDC(Field[CurrentField].hwnd);
		ReleaseDC(Field[CurrentField].hwnd,hdc);

		return TRUE;}*/
	 case WM_GETTEXT: {
		 _asm {
			nop}
		 //(const char near*) lParam=www;
		return DefWindowProc(hwnd,msg,wParam,lParam);}
	 case WM_MOUSEACTIVATE: {
		ActiveField=CurrentField;
		UpdateMenu(ActiveField);
		return TRUE;}
	 case WM_LBUTTONDOWN: {
		if (Field[ActiveField].Running)
		  return TRUE;
		VertShift=(HIWORD (lParam)-cyChar+Field[ActiveField].VertPosit)/CELL_SIZE;
		HorzShift=(LOWORD (lParam)+Field[ActiveField].HorzPosit)/CELL_SIZE;
		SetCellState(ActiveField,HorzShift,VertShift,INVERT);
		VertShift=VertShift*CELL_SIZE-Field[ActiveField].VertPosit+1;
		HorzShift=HorzShift*CELL_SIZE-Field[ActiveField].HorzPosit+1;
		GetClientRect(Field[ActiveField].hwnd,&rcWndRect);
		ValidateRect(Field[ActiveField].hwnd,&rcWndRect);
		if (VertShift<1)
		  rcWndRect.top=cyChar;
		else
		  rcWndRect.top=VertShift+cyChar;
		rcWndRect.bottom=VertShift+CELL_SIZE-2+cyChar;
		rcWndRect.left=HorzShift;
		rcWndRect.right=HorzShift+CELL_SIZE-2;
		InvalidateRect(Field[ActiveField].hwnd,&rcWndRect,TRUE);
		SetCursor(SecondCursor);
		UpdateWindow(Field[ActiveField].hwnd);
		return TRUE; }
	 case WM_LBUTTONUP: {
		SetCursor(FirstCursor);
		return TRUE; }
	 case WM_TIMER: {
		switch (wParam) {
		  case 0: {
			 if (!Field[CurrentField].RunningCycle) {
				Field[CurrentField].RunningCycle=TRUE;
				SetTimer(Field[CurrentField].hwnd,1,1,NULL);  }
			 return TRUE;}
		  case 1: {
			 if (!Field[CurrentField].SecondPass) {
				FirstPass(Field[CurrentField].Line);
				Field[CurrentField].Line++;
				  if(Field[CurrentField].Line==16) {
					 Field[CurrentField].Line=0;
					 Field[CurrentField].SecondPass=TRUE;}}
			 else {
				SecondPass();
				Field[CurrentField].SecondPass=FALSE;
				Field[CurrentField].RunningCycle=FALSE;
				if (Field[CurrentField].TimerMode==KEYBOARD) {
				  Field[CurrentField].Running=FALSE;
				  SetCursor(FirstCursor);          }
				/*if (!Field[CurrentField].Running)
				  ShowCursor(TRUE);*/
				KillTimer(Field[CurrentField].hwnd,1);}
			 return TRUE;}}}
	 case WM_PAINT: {
		hdc=BeginPaint(Field[CurrentField].hwnd,&ps);
		TextOut(hdc,0,0,"Ход:",4);
		sprintf(buf,"%d",Field[CurrentField].nTurn);
		TextOut(hdc,32,0,buf,strlen(buf));
		/*GetClientRect(Field[CurrentField].hwnd,&rcWndRect);
		rcWndRect.bottom=cyChar;
		ValidateRect(Field[CurrentField].hwnd,&rcWndRect);//*/
		HorzShift=((Field[CurrentField].HorzPosit+CELL_SIZE-1)/CELL_SIZE)
		  *CELL_SIZE-Field[CurrentField].HorzPosit;
		VertShift=((Field[CurrentField].VertPosit+CELL_SIZE-1)/CELL_SIZE)
		  *CELL_SIZE-Field[CurrentField].VertPosit;
		for (i=HorzShift;i<Field[CurrentField].WinSize.right;i+=CELL_SIZE)
		  DrawLine(hdc,i,cyChar,i,Field[CurrentField].WinSize.bottom+cyChar);
		for (i=cyChar+VertShift;i<Field[CurrentField].WinSize.bottom+cyChar;i+=CELL_SIZE)
		  DrawLine(hdc,0,i,Field[CurrentField].WinSize.right,i);

		HorzShift=((HorzShift+CELL_SIZE-1)%CELL_SIZE)-(CIRCLE_CENTER-1);
		VertShift=((VertShift+CELL_SIZE-1)%CELL_SIZE)-(CIRCLE_CENTER-1);
		k=Field[CurrentField].HorzPosit/CELL_SIZE;
		for (i=HorzShift;i<Field[CurrentField].WinSize.right+CIRCLE_CENTER;i+=CELL_SIZE) {
		  l=Field[CurrentField].VertPosit/CELL_SIZE;
		  for (j=cyChar+VertShift;j<Field[CurrentField].WinSize.bottom+CIRCLE_CENTER+cyChar;j+=CELL_SIZE) {
			 if(GetCellState(CurrentField,k,l))
				Ellipse(hdc,i-CIRCLE_RADIUS,j-CIRCLE_RADIUS,i+CIRCLE_RADIUS,j+CIRCLE_RADIUS);
			 l++;}
		  k++;}

		EndPaint(Field[CurrentField].hwnd,&ps);
		return TRUE;}
	 case WM_SIZE: {
		Field[ActiveField].WinSize.right=LOWORD(lParam);
		if((Field[ActiveField].WinSize.bottom=HIWORD(lParam)-cyChar)<0)
		  Field[ActiveField].WinSize.bottom=0;
		SetRangePos(ActiveField,-1,-1,FIELD_SIZE-Field[ActiveField].WinSize.right,
		  FIELD_SIZE-Field[ActiveField].WinSize.bottom);
		return TRUE;}
	 case WM_VSCROLL: {
		switch (wParam) {
		  case SB_TOP: {
			 Field[ActiveField].VertPosit=0;
			 break;}
		  case SB_LINEUP: {
			 if((Field[ActiveField].VertPosit-=CELL_SIZE)<0)
				Field[ActiveField].VertPosit=0;
			 break;}
		  case SB_PAGEUP: {
			 if((Field[ActiveField].VertPosit-=Field[ActiveField].WinSize.bottom)<0)
				Field[ActiveField].VertPosit=0;
			 break;}
		  case SB_BOTTOM: {
			 Field[ActiveField].VertPosit=Field[ActiveField].VertLen;
			 break;}
		  case SB_LINEDOWN: {
			 if((Field[ActiveField].VertPosit+=CELL_SIZE)>Field[ActiveField].VertLen)
				Field[ActiveField].VertPosit=Field[ActiveField].VertLen;
			 break;}
		  case SB_PAGEDOWN: {
			 if((Field[ActiveField].VertPosit+=Field[ActiveField].WinSize.bottom)>
				Field[ActiveField].VertLen)
				Field[ActiveField].VertPosit=Field[ActiveField].VertLen;
			 break;}
		  case SB_THUMBTRACK: {
			 Field[ActiveField].VertPosit=LOWORD (lParam);
			 break;}
		  case SB_THUMBPOSITION: {
			 Field[ActiveField].VertPosit=LOWORD (lParam);
			 break;}
		  default: return DefWindowProc(hwnd,msg,wParam,lParam);}
		SetScrollPos(Field[ActiveField].hwnd,SB_VERT,Field[ActiveField].VertPosit,TRUE);
		UpdateField(ActiveField);
		return TRUE;}
	 case WM_HSCROLL: {
		switch (wParam) {
		  case SB_LEFT: {
			 Field[ActiveField].HorzPosit=0;
			 break;}
		  case SB_LINELEFT: {
			 if((Field[ActiveField].HorzPosit-=CELL_SIZE)<0)
				Field[ActiveField].HorzPosit=0;
			 break;}
		  case SB_PAGELEFT: {
			 if((Field[ActiveField].HorzPosit-=Field[ActiveField].WinSize.right)<0)
				Field[ActiveField].HorzPosit=0;
			 break;}
		  case SB_RIGHT: {
			 Field[ActiveField].HorzPosit=Field[ActiveField].HorzLen;
			 break;}
		  case SB_LINERIGHT: {
			 if((Field[ActiveField].HorzPosit+=CELL_SIZE)>Field[ActiveField].HorzLen)
				Field[ActiveField].HorzPosit=Field[ActiveField].HorzLen;
			 break;}
		  case SB_PAGERIGHT: {
			 if((Field[ActiveField].HorzPosit+=Field[ActiveField].WinSize.right)>
				Field[ActiveField].HorzLen)
				Field[ActiveField].HorzPosit=Field[ActiveField].HorzLen;
			 break;}
		  case SB_THUMBTRACK: {
			 Field[ActiveField].HorzPosit=LOWORD (lParam);
			 break;}
		  case SB_THUMBPOSITION: {
			 Field[ActiveField].HorzPosit=LOWORD (lParam);
			 break;}
		  default: return DefWindowProc(hwnd,msg,wParam,lParam);}
		SetScrollPos(Field[ActiveField].hwnd,SB_HORZ,Field[ActiveField].HorzPosit,TRUE);
		UpdateField(ActiveField);
		return TRUE;}
	 case WM_DESTROY: {
		nOpenedFiles--;
		KillTimer(Field[ActiveField].hwnd,1);
		if (Field[ActiveField].TimerMode==TIMER)
		  StopTimer(ActiveField);
		DeleteField(ActiveField);
		return TRUE;}}
  return DefWindowProc(hwnd,msg,wParam,lParam);}
BOOL CALLBACK FileOpen(){
  UINT i;
  UINT DefineHeader,LenFile;
  char szFile[256],szFileTitle[256];
  HFILE hf;
  szFile[0]='\0';
  memset(&ofn,0,sizeof(OPENFILENAME));
  ofn.lStructSize=sizeof(OPENFILENAME);
  ofn.hwndOwner=NULL;
  ofn.lpstrFilter="Data Files (*.dat)\0*.dat\0Compressed Data Files (*.cdt)\0*.cdt\0All Files (*.*)\0*.*\0\0";
  ofn.nFilterIndex=1;
  ofn.lpstrFile=szFile;
  ofn.nMaxFile=sizeof(szFile);
  ofn.lpstrFileTitle=szFileTitle;
  ofn.nMaxFileTitle=sizeof(szFileTitle);
  ofn.lpstrInitialDir=NULL;
  ofn.Flags=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
  if (!GetOpenFileName(&ofn)) {
	 return FALSE;}
  hf=_lopen(ofn.lpstrFile,OF_READ);
  i=FoundFree();
  ClearField(i);
  if (_lread(hf,&DefineHeader,2)!=2) {
	 MessageBeep(MB_OK);
	 MessageBox(NULL,"Ошибка чтения с диска","Ошибка",MB_OK|MB_ICONSTOP);
	 return FALSE;}
  if (DefineHeader!=0x44bb) {
	 MessageBeep(MB_ICONHAND);
	 MessageBox(NULL,"Это не файл с данными","Ошибка",MB_OK|MB_ICONSTOP);
	 return FALSE;}
  if (_lread(hf,&LenFile,2)!=2) {
	 MessageBeep(MB_OK);
	 MessageBox(NULL,"Ошибка чтения с диска","Ошибка",MB_OK|MB_ICONSTOP);
	 return FALSE;}
  if (_lread(hf,&Field[i],LenFile)!=(unsigned int) LenFile) {
	 MessageBeep(MB_OK);
	 MessageBox(NULL,"Ошибка чтения с диска","Ошибка",MB_OK|MB_ICONSTOP);
	 return FALSE;}
  _lclose(hf);
  if(!CreateField(i,ofn.lpstrFileTitle))
	 return FALSE;
  Field[i].titled=TRUE;
/*	char string[10];
	char *str1 = "abcdefghi";

	strcpy(string, str1);
	printf("%s\n", string);
	return 0;*/

  strcpy(Field[i].FieldName, ofn.lpstrFile);
  return TRUE;}
BOOL CALLBACK FileNew() {
  int i;
  i=FoundFree();
  ClearField(i);
  if(!CreateField(i,"UNTITLED.DAT"))
	 return FALSE;
  strcpy(Field[i].FieldName, "UNTITLED.DAT");
  Field[i].nTurn=0;
  Field[i].TimerMode=TIMER;
  Field[i].TimerPeriod=MIN_TIMER_CONSTANT;
  Field[i].titled=FALSE;
  return TRUE;}
BOOL CALLBACK FileSave(int f) {
  HFILE hf;
  if (!Field[f].titled)
	 return FileSaveAs(f);
  hf=_lcreat(Field[f].FieldName,0);
  if(!SavingDataFile(hf,0x44bb,16392,&Field[f]))
	 return FALSE;
  return TRUE;}
BOOL CALLBACK FileSaveAs(int f) {
  char szFile[256],szFileTitle[256];
  HFILE hf;
  szFile[0]='\0';
  memset(&ofn,0,sizeof(OPENFILENAME));
  ofn.lStructSize=sizeof(OPENFILENAME);
  ofn.hwndOwner=NULL;
  ofn.lpstrFilter="Data Files (*.dat)\0*.dat\0Compressed Data Files (*.cdt)\0*.cdt\0\0";
  ofn.nFilterIndex=1;
  ofn.lpstrFile=szFile;
  ofn.nMaxFile=sizeof(szFile);
  ofn.lpstrFileTitle=szFileTitle;
  ofn.nMaxFileTitle=sizeof(szFileTitle);
  ofn.lpstrInitialDir=NULL;
  ofn.Flags=OFN_HIDEREADONLY;
  if (!GetSaveFileName(&ofn))
	 return FALSE;
  hf=_lcreat(ofn.lpstrFile,0);
  if(!SavingDataFile(hf,0x44bb,16392,&Field[f])) {
	 MessageBeep(MB_OK);
	 MessageBox(NULL,"Ошибка записи на диск","Ошибка",MB_OK|MB_ICONSTOP);
	 return FALSE;}
  Field[f].titled=TRUE;
  strcpy(Field[f].FieldName, ofn.lpstrFile);
  return TRUE;}
BOOL CALLBACK FileSaveAll() {
  int i;
  for (i=0;i<MaxFields;i++)
	 if(Field[i].busy==TRUE)
		FileSave(i);
  return TRUE;}

void CALLBACK ClearField(int i) {
  memset(&Field[i],0,sizeof(one_field));
/*  int j,k;

  for (j=0;j<64;j++)
	 for (k=0;k<256;k++)
		Field[i].data[j][k]=0;//*/
		}
UINT CALLBACK FoundFree() {
  int i;
  for (i=0;i<MaxFields;i++)
	 if (!Field[i].busy){
		return (i);}
  return (-1);}
BOOL CALLBACK CreateField(int i,char *FileName){
  RECT rcWndRect;
  if((Field[i].hwnd=CreateWindow(
		szChildClassName,
		FileName,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_HSCROLL|
		WS_VSCROLL|WS_OVERLAPPEDWINDOW|WS_MAXIMIZE,
		0,
		0,
		200,
		200,
		MainHwnd,
		0,
		hInstance,
		NULL))==0)
	 return FALSE;
  GetClientRect(Field[i].hwnd,&rcWndRect);
  Field[i].HorzPosit=(FIELD_SIZE-(rcWndRect.right-rcWndRect.left))/2;
  Field[i].VertPosit=(FIELD_SIZE-(rcWndRect.bottom-rcWndRect.top))/2;
  SetRangePos(i,Field[i].HorzPosit,Field[i].VertPosit,
	 FIELD_SIZE-(rcWndRect.right-rcWndRect.left),
	 FIELD_SIZE-(rcWndRect.bottom-rcWndRect.top));
  nOpenedFiles++;
  UpdateMenu(i);
  Field[i].busy=TRUE;
  ActiveField=i;
  CurrentField=i;
  Field[i].SecondPass=FALSE;
  Field[CurrentField].Line=0;
//  PostMessage(Field[i].hwnd,PM_PRINTNTURN,0,0);
  ShowCursor(TRUE);

  UpdateField(i);
  return TRUE;}
BOOL CALLBACK SavingDataFile(HFILE hf,UINT DefineHeader,UINT LenFile,one_field *Buffer) {
  if(!_lwrite(hf,&DefineHeader,2))
	 return FALSE;
  if(!_lwrite(hf,&LenFile,2))
	 return FALSE;
  if(!_lwrite(hf,Buffer,LenFile))
	 return FALSE;
  return TRUE;}
/*void CALLBACK UpdateMenu() {
  UINT function;
  if(nOpenedFiles==MaxFields)
	 function=MF_GRAYED;
  else
	 function=MF_ENABLED;
  (void) EnableMenuItem(hmenu,CM_FILENEW,function|MF_BYCOMMAND);
  (void) EnableMenuItem(hmenu,CM_FILEOPEN,function|MF_BYCOMMAND);
  if(nOpenedFiles<1)
	 function=MF_GRAYED;
  else
	 function=MF_ENABLED;
  (void) EnableMenuItem(hmenu,CM_FILESAVE,function|MF_BYCOMMAND);
  (void) EnableMenuItem(hmenu,CM_FILESAVE_AS,function|MF_BYCOMMAND);
  (void) EnableMenuItem(hmenu,CM_FILESAVE_ALL,function|MF_BYCOMMAND);
  DrawMenuBar(MainHwnd);
  return;}*/
BOOL CALLBACK DrawLine(HDC hdc,int x1,int y1,int x2,int y2){
  POINT pt;
  MoveToEx(hdc,x1,y1,&pt);
  return LineTo(hdc,x2,y2);}
void CALLBACK SetRangePos(int i,int xPos,int yPos,int xRange,int yRange) {
  SetScrollRange(Field[i].hwnd,SB_HORZ,0,xRange,FALSE);
  Field[i].HorzLen=xRange;
  SetScrollRange(Field[i].hwnd,SB_VERT,0,yRange,FALSE);
  Field[i].VertLen=yRange;
  if(xPos!=-1) {
	 Field[i].HorzPosit=xPos;
	 SetScrollPos(Field[i].hwnd,SB_HORZ,xPos,TRUE);}
  else
	 if(Field[i].HorzPosit>xRange) {
		Field[i].HorzPosit=xRange;
		SetScrollPos(Field[i].hwnd,SB_HORZ,xRange,TRUE);
//		UpdateWindow(Field[i].hwnd);}
		UpdateField(i);  }
	 else
		SetScrollPos(Field[i].hwnd,SB_HORZ,Field[i].HorzPosit,TRUE);
  if(yPos!=-1) {
	 Field[i].VertPosit=yPos;
	 SetScrollPos(Field[i].hwnd,SB_VERT,yPos,TRUE);}
  else
	 if(Field[i].VertPosit>yRange) {
		Field[i].VertPosit=yRange;
		SetScrollPos(Field[i].hwnd,SB_VERT,yRange,TRUE);
//		UpdateWindow(Field[CurrentField].hwnd);}
		UpdateField(i);   }
	 else
		SetScrollPos(Field[i].hwnd,SB_VERT,Field[i].VertPosit,TRUE);}
BOOL CALLBACK GetCellState(int i,UINT x,UINT y) {
  BYTE a;
  a=Field[i].data[x>>2][y];
  return ((a>>((~x&3)<<1))&1);}
BOOL CALLBACK GetCellChangingState(int i,UINT x,UINT y) {
  BYTE a;
  a=Field[i].data[x>>2][y];
  return ((a>>(((~x&3)<<1)+1))&1);}
void CALLBACK SetCellState(int i,UINT x,UINT y,int Mode) {
  switch (Mode) {
	 case SET: {
		Field[i].data[x>>2][y]|=1<<((~x&3)<<1);
		break;}
	 case RESET: {
		Field[i].data[x>>2][y]&=~(1<<((~x&3)<<1));
		break;}
	 case INVERT: {
		Field[i].data[x>>2][y]^=1<<((~x&3)<<1);
		break;}}}
void CALLBACK SetCellChangingState(int i,UINT x,UINT y,int Mode) {
  switch (Mode) {
	 case SET: {
		Field[i].data[x>>2][y]|=1<<(((~x&3)<<1)+1);
		break;}
	 case RESET: {
		Field[i].data[x>>2][y]&=~(1<<(((~x&3)<<1)+1));
		break;}}}
void CALLBACK UpdateField(int i) {
  RECT rcWndRect;
  GetClientRect(Field[i].hwnd,&rcWndRect);
  InvalidateRect(Field[i].hwnd,&rcWndRect,TRUE);}
/*void CALLBACK ClearUpdateField() {
  RECT rcWndRect;
  ClearField(CurrentField);
  UpdateWindow(Field[CurrentField].hwnd);}*/
void CALLBACK DeleteField(int i) {
  Field[i].busy=FALSE;
  for (i=0;i<MaxFields;i++)
	 if(Field[i].busy==TRUE) {
		ActiveField=i;
		CurrentField=i;
		break;}}
void RunTimer() {
  Field[ActiveField].Running=TRUE;
  ShowCursor(FALSE);
  UpdateMenu(ActiveField);
  switch (Field[ActiveField].TimerMode) {
	 /*case NO: {
		PostMessage(Field[CurrentField].hwnd,WM_TIMER,0,0);
		EnableMenuItem(hmenu,CM_OPTIONSSTOP,MF_ENABLED|MF_BYCOMMAND);
		EnableMenuItem(hmenu,CM_OPTIONSCONTINUE,MF_GRAYED|MF_BYCOMMAND);
		EnableMenuItem(hmenu,CM_OPTIONSBEGIN,MF_GRAYED|MF_BYCOMMAND);
		break;}*/
	 case KEYBOARD: {
		PostMessage(Field[ActiveField].hwnd,WM_TIMER,0,0);
		break;}
	 case TIMER: {
		SetTimer(Field[ActiveField].hwnd,0,Field[ActiveField].TimerPeriod,NULL);
		/*EnableMenuItem(hmenu,CM_OPTIONSSTOP,MF_ENABLED|MF_BYCOMMAND);
		EnableMenuItem(hmenu,CM_OPTIONSCONTINUE,MF_GRAYED|MF_BYCOMMAND);
		EnableMenuItem(hmenu,CM_OPTIONSBEGIN,MF_GRAYED|MF_BYCOMMAND);*/
		break;}}}
void StopTimer(int f) {
  Field[f].Running=FALSE;
  UpdateMenu(f);
  KillTimer(Field[f].hwnd,0);}
void FirstPass(int a) {
  int i,j,k,l,m;
  for (j=a*16;j<(a+1)*16;j++)
	 for (i=0;i<256;i++) {
		m=0;
		k=i-1;
		l=j-1;
		if (k!=-1&&l!=-1&&GetCellState(CurrentField,k,l))
		  m++;
		k++;
		if (l!=-1&&GetCellState(CurrentField,k,l))
		  m++;
		k++;
		if (l!=-1&&k!=256&&GetCellState(CurrentField,k,l))
		  m++;
		l++;
		if (k!=256&&GetCellState(CurrentField,k,l))
		  m++;
		l++;
		if (k!=256&&l!=256&&GetCellState(CurrentField,k,l))
		  m++;
		k--;
		if (l!=256&&GetCellState(CurrentField,k,l))
		  m++;
		k--;
		if (k!=-1&&l!=256&&GetCellState(CurrentField,k,l))
		  m++;
		l--;
		if (k!=-1&&GetCellState(CurrentField,k,l))
		  m++;
		if ((m==3&&(!GetCellState(CurrentField,i,j)))||
			 (GetCellState(CurrentField,i,j)&&(m<2||m>3)))
		  SetCellChangingState(CurrentField,i,j,SET);}}
void SecondPass() {
  int i,j,k,l,m,n,o=0,HorzShift,VertShift;
  RECT rcWndRect;
  k=Field[CurrentField].HorzPosit/CELL_SIZE;
  l=Field[CurrentField].VertPosit/CELL_SIZE;
  m=(Field[CurrentField].HorzPosit+Field[CurrentField].WinSize.right)/CELL_SIZE;
  n=(Field[CurrentField].VertPosit+Field[CurrentField].WinSize.bottom)/CELL_SIZE;
  HorzShift=k*CELL_SIZE-Field[CurrentField].HorzPosit;
  VertShift=l*CELL_SIZE-Field[CurrentField].VertPosit;
  GetClientRect(Field[CurrentField].hwnd,&rcWndRect);
  ValidateRect(Field[CurrentField].hwnd,&rcWndRect);
  for (i=0;i<256;i++)
	 for (j=0;j<256;j++)
		if (GetCellChangingState(CurrentField,i,j)) {
		  SetCellChangingState(CurrentField,i,j,RESET);
		  SetCellState(CurrentField,i,j,INVERT);
		  if (i>=k&&i<=m&&j>=l&&j<=n) {
			 rcWndRect.top=cyChar+VertShift+(j-l)*CELL_SIZE;
			 rcWndRect.bottom=cyChar+VertShift+(j-l)*CELL_SIZE+(CELL_SIZE-1);
			 rcWndRect.left=HorzShift+(i-k)*CELL_SIZE;
			 rcWndRect.right=HorzShift+(i-k)*CELL_SIZE+(CELL_SIZE-1);
			 InvalidateRect(Field[CurrentField].hwnd,&rcWndRect,TRUE);
			 o++;}}
  GetClientRect(Field[CurrentField].hwnd,&rcWndRect);
  if (o>40)
	 InvalidateRect(Field[CurrentField].hwnd,&rcWndRect,TRUE);
  else {
	 rcWndRect.bottom=cyChar;
	 InvalidateRect(Field[CurrentField].hwnd,&rcWndRect,TRUE);}
  Field[CurrentField].nTurn++;
 // PostMessage(Field[CurrentField].hwnd,WM_PAINT,0,0);}
	}
void UpdateMenu(int i) {
  UINT function;

  if(nOpenedFiles==MaxFields)
	 OpenOff=TRUE;
  else
	 OpenOff=FALSE;
  if(nOpenedFiles<1||Field[i].Running)
	 SaveOff=TRUE;
  else
	 SaveOff=FALSE;
  if (Field[i].Running)
	 RunOff=TRUE;
  else
	 RunOff=FALSE;

  if(OpenOff)
	 function=MF_GRAYED;
  else
	 function=MF_ENABLED;
  EnableMenuItem(hmenu,CM_FILENEW,function|MF_BYCOMMAND);
  EnableMenuItem(hmenu,CM_FILEOPEN,function|MF_BYCOMMAND);

  if(SaveOff)
	 function=MF_GRAYED;
  else
	 function=MF_ENABLED;
  EnableMenuItem(hmenu,CM_FILESAVE,function|MF_BYCOMMAND);
  EnableMenuItem(hmenu,CM_FILESAVE_AS,function|MF_BYCOMMAND);
  EnableMenuItem(hmenu,CM_FILESAVE_ALL,function|MF_BYCOMMAND);
  EnableMenuItem(hmenu,1,function|MF_BYPOSITION);
  EnableMenuItem(hmenu,2,function|MF_BYPOSITION);
  EnableMenuItem(hmenu,3,function|MF_BYPOSITION);

  if (Field[i].TimerMode==TIMER) {
	 CheckMenuItem(hmenu,CM_MODETIMING,MF_CHECKED);
	 CheckMenuItem(hmenu,CM_MODEKEYBOARD,MF_UNCHECKED); }
  else {
	 CheckMenuItem(hmenu,CM_MODETIMING,MF_UNCHECKED);
	 CheckMenuItem(hmenu,CM_MODEKEYBOARD,MF_CHECKED); }
  if (Field[i].Running) {
	 EnableMenuItem(hmenu,CM_OPTIONSSTOP,MF_ENABLED|MF_BYCOMMAND);
	 EnableMenuItem(hmenu,CM_OPTIONSCONTINUE,MF_GRAYED|MF_BYCOMMAND);
	 EnableMenuItem(hmenu,CM_OPTIONSBEGIN,MF_GRAYED|MF_BYCOMMAND);}
  else {
	 EnableMenuItem(hmenu,CM_OPTIONSSTOP,MF_GRAYED|MF_BYCOMMAND);
	 EnableMenuItem(hmenu,CM_OPTIONSCONTINUE,MF_ENABLED|MF_BYCOMMAND);
	 EnableMenuItem(hmenu,CM_OPTIONSBEGIN,MF_ENABLED|MF_BYCOMMAND);}
  DrawMenuBar(MainHwnd);}
int max(int a,int b){
  return ((a>b)?a:b);}
int min(int a,int b){
  return ((a<b)?a:b);}

