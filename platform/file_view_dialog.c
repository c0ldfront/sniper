/* Original40f810..40fa9c. */
#include "file_view_dialog.h"
#include <stdio.h>
#include <stdlib.h>
static const char *const keys[4]={"X","Y","Width","Height"};
static INT_PTR CALLBACK file_proc(HWND dialog,UINT message,WPARAM wp,LPARAM lp) {
    switch(message){
    case WM_INITDIALOG:{
        FILE *f=fopen((const char *)lp,"rb");if(f){char *data=malloc(0x8000);if(data){size_t n=fread(data,1,0x7f00,f);data[n]=0;SetDlgItemTextA(dialog,1018,data);free(data);}fclose(f);}
        int v[4];for(int i=0;i<4;++i)v[i]=(int)GetPrivateProfileIntA("ViewFile",keys[i],i<2?-1:i==2?150:100,"sniper.ini");
        if(v[0]!=-1)MoveWindow(dialog,v[0],v[1],v[2],v[3],FALSE);
        SendMessageA(GetDlgItem(dialog,1018),EM_SETSEL,0,0);return TRUE;}
    case WM_SIZE:MoveWindow(GetDlgItem(dialog,1018),0,0,LOWORD(lp),HIWORD(lp),TRUE);return TRUE;
    case WM_CLOSE:{
        RECT r;GetWindowRect(dialog,&r);int v[4]={r.left,r.top,r.right-r.left,r.bottom-r.top};
        for(int i=0;i<4;++i){char text[32];snprintf(text,sizeof text,"%d",v[i]);WritePrivateProfileStringA("ViewFile",keys[i],text,"sniper.ini");}
        EndDialog(dialog,(INT_PTR)wp);return TRUE;}
    case WM_COMMAND:
        if(LOWORD(wp)==IDOK || LOWORD(wp)==IDCANCEL)PostMessageA(dialog,WM_CLOSE,0,0);
        else if(LOWORD(wp)==1018)SendMessageA(GetDlgItem(dialog,1018),EM_SETSEL,0,0);
        return TRUE;
    default:return FALSE;
    }
}
INT_PTR sniper87_file_view_dialog(HWND owner,HINSTANCE instance,const char *path) {
    if(!path)return -1;
    return DialogBoxParamA(instance,MAKEINTRESOURCEA(150),owner,file_proc,(LPARAM)path);
}
