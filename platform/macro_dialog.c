/* Original40faa0..410062; see macro-dialog-findings.txt. */
#include "macro_dialog.h"
#include <commdlg.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
typedef struct {char (*macros)[82];HWND owner;HINSTANCE instance;int recording;} MacroDialog;
static const int text_ids[8]={1020,1021,1022,1026,1024,1025,1023,1027};
static void finish_recording(HWND dialog,MacroDialog *state) {
    char command[64];
    SetDlgItemTextA(dialog,1028,"Record");SetDlgItemTextA(dialog,1030,"Record");
    KillTimer(dialog,1);mciSendStringA("stop capture",NULL,0,NULL);
    snprintf(command,sizeof command,"save capture record%d.wav",state->recording-1);
    mciSendStringA(command,NULL,0,NULL);mciSendStringA("close capture",NULL,0,NULL);state->recording=0;
}
static void import_wave(MacroDialog *state,int slot) {
    char path[256]="";OPENFILENAMEA ofn;memset(&ofn,0,sizeof ofn);
    ofn.lStructSize=sizeof ofn;ofn.hwndOwner=state->owner;ofn.hInstance=state->instance;
    ofn.lpstrFilter="WAV Files (*.wav)\0*.WAV\0\0";ofn.lpstrFile=path;ofn.nMaxFile=sizeof path;
    ofn.lpstrTitle="Import Wave File";ofn.Flags=0x8180c;
    if(!GetOpenFileNameA(&ofn))return;
    FILE *input=fopen(path,"rb");if(!input)return;
    if(fseek(input,0,SEEK_END)){fclose(input);return;}
    long size=ftell(input);rewind(input);
    if(size>32768){fclose(input);MessageBoxA(state->owner,"File too big to import, must be under 32k.  Recommend 11k/8bit/mono","Error",MB_ICONINFORMATION);return;}
    if(size<0){fclose(input);return;}
    FILE *output=fopen(slot?"record1.wav":"record0.wav","wb");
    if(output){char block[512];size_t n;while((n=fread(block,1,sizeof block,input))!=0)if(fwrite(block,1,n,output)!=n)break;fclose(output);}
    fclose(input);
}
static INT_PTR CALLBACK macro_proc(HWND dialog,UINT message,WPARAM wp,LPARAM lp) {
    MacroDialog *state=(MacroDialog *)GetWindowLongPtrA(dialog,DWLP_USER);
    switch(message) {
    case WM_INITDIALOG:
        state=(MacroDialog *)lp;SetWindowLongPtrA(dialog,DWLP_USER,(LONG_PTR)state);state->recording=0;
        for(int i=0;i<8;++i)SetWindowTextA(GetDlgItem(dialog,text_ids[i]),state->macros[i]);
        return TRUE;
    case WM_CLOSE:EndDialog(dialog,(INT_PTR)wp);return TRUE;
    case WM_TIMER:if(state->recording)finish_recording(dialog,state);return TRUE;
    case WM_COMMAND:
        /* While recording every command only stops/saves, including OK/Cancel. */
        if(state->recording){finish_recording(dialog,state);return TRUE;}
        switch(LOWORD(wp)) {
        case IDOK:
            for(int i=0;i<8;++i)GetWindowTextA(GetDlgItem(dialog,text_ids[i]),state->macros[i],82);
            PostMessageA(dialog,WM_CLOSE,1,0);break;
        case IDCANCEL:PostMessageA(dialog,WM_CLOSE,0,0);break;
        case 1028:case 1030:
            state->recording=LOWORD(wp)==1028?1:2;SetTimer(dialog,1,3000,NULL);
            mciSendStringA("open new type waveaudio alias capture",NULL,0,NULL);
            mciSendStringA("record capture",NULL,0,NULL);SetDlgItemTextA(dialog,LOWORD(wp),"Stop");break;
        case 1029:case 1031:
            SetCursor(LoadCursor(NULL,IDC_WAIT));PlaySoundA(LOWORD(wp)==1029?"record0.wav":"record1.wav",NULL,SND_FILENAME);
            SetCursor(LoadCursor(NULL,IDC_ARROW));break;
        case 1032:case 1033:import_wave(state,LOWORD(wp)==1032?0:1);break;
        }
        return TRUE;
    default:return FALSE;
    }
}
INT_PTR sniper87_macro_dialog(HWND owner,HINSTANCE instance,char macros[8][82]) {
    if(!macros)return -1;
    MacroDialog state={macros,owner,instance,0};
    return DialogBoxParamA(instance,MAKEINTRESOURCEA(152),owner,macro_proc,(LPARAM)&state);
}
