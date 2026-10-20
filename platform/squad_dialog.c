/* Original callback40e100..40e1e8,resource174. */
#include "squad_dialog.h"
static INT_PTR CALLBACK squad_proc(HWND dialog,UINT message,WPARAM wp,LPARAM lp) {
    Sniper87SquadCredentials *s=(Sniper87SquadCredentials *)GetWindowLongPtrA(dialog,DWLP_USER);
    switch(message){
    case WM_INITDIALOG:SetWindowLongPtrA(dialog,DWLP_USER,lp);SetWindowTextA(GetDlgItem(dialog,1036),"");SetWindowTextA(GetDlgItem(dialog,1037),"");return TRUE;
    case WM_CLOSE:EndDialog(dialog,(INT_PTR)wp);return TRUE;
    case WM_COMMAND:
        if(LOWORD(wp)==IDOK){GetWindowTextA(GetDlgItem(dialog,1036),s->name,32);s->name[31]=0;GetWindowTextA(GetDlgItem(dialog,1037),s->password,32);s->password[31]=0;PostMessageA(dialog,WM_CLOSE,1,0);}
        else if(LOWORD(wp)==IDCANCEL)PostMessageA(dialog,WM_CLOSE,0,0);
        return TRUE;
    default:return FALSE;
    }
}
INT_PTR sniper87_squad_dialog(HWND owner,HINSTANCE instance,Sniper87SquadCredentials *s) {
    if(!s)return -1;
    return DialogBoxParamA(instance,MAKEINTRESOURCEA(174),owner,squad_proc,(LPARAM)s);
}
