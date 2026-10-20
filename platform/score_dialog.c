/* Reconstructed from 40e1f0/40e940 and40ead0/40f410. See findings. */
#include "score_dialog.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
static int32_t sub32(int32_t a,int32_t b){return (int32_t)((uint32_t)a-(uint32_t)b);}
static int32_t rating(int w,int l,int32_t p){
 int32_t n=(int32_t)(((uint32_t)(w-l)*10u+(uint32_t)p)*10u);
 return w == -100 ? 0 : (n>0?n:0)/(w+100);
}
static int32_t ratio(int w,int32_t p){return w>0?(int32_t)((uint32_t)p*100u)/w:0;}
static int ci(const char *a,const char *b){
 for(;;++a,++b){unsigned x=(unsigned char)*a,y=(unsigned char)*b;
  if(x>='A'&&x<='Z')x+=32;
  if(y>='A'&&y<='Z')y+=32;
  if(x!=y)return x<y?-1:1;
  if(!x)return 0;
 }
}
/* Native416930 qsort/416a90 shortsort, specialized to40d940 squad key.
 * Preserve unstable tie permutations: the first spelling labels each group. */
static void score_swap(SniperPlayerScore *a,SniperPlayerScore *b){
 SniperPlayerScore tmp=*a;*a=*b;*b=tmp;
}
static void squad_key_sort(SniperPlayerScore *p,int lo,int hi){
 while(lo<hi){
  if(hi-lo+1<=8){
   while(hi>lo){int largest=lo;
    for(int i=lo+1;i<=hi;++i)if(ci(p[i].squad,p[largest].squad)>0)largest=i;
    score_swap(p+largest,p+hi);--hi;
   }
   return;
  }
  score_swap(p+lo+(hi-lo+1)/2,p+lo);
  int left=lo,right=hi+1;
  for(;;){
   do{++left;}while(left<=hi&&ci(p[left].squad,p[lo].squad)<=0);
   do{--right;}while(right>lo&&ci(p[right].squad,p[lo].squad)>=0);
   if(right<left)break;
   score_swap(p+left,p+right);
  }
  score_swap(p+lo,p+right);
  /* Smaller side first bounds recursion even for all-equal records. */
  if(right-lo-1<hi-left){squad_key_sort(p,lo,right-1);lo=left;}
  else{squad_key_sort(p,left,hi);hi=right-1;}
 }
}
int sniper87_build_squad_scores(SniperPlayerScore *p,int count,SniperSquadScore *out,int capacity){
 if(count<0||capacity<0||(count&&!p))return -1;
 if(count>1)squad_key_sort(p,0,count-1);
 int required=0;
 for(int i=0;i<count;){int end=i+1;while(end<count&&!ci(p[i].squad,p[end].squad))++end;
  if(p[i].squad[0]&&end-i>1)++required;
  i=end;
 }
 if(required>capacity||(required&&!out))return -1;
 int n=0;
 for(int i=0;i<count;){int end=i+1;while(end<count&&!ci(p[i].squad,p[end].squad))++end;
  if(p[i].squad[0]&&end-i>1){SniperSquadScore *s=out+n++;memset(s,0,sizeof(*s));
   snprintf(s->name,sizeof(s->name),"%.31s",p[i].squad);
   s->members=end-i;
   for(int j=i;j<end;++j){s->wins=(int16_t)((uint16_t)s->wins+(uint16_t)p[j].wins);
    s->losses=(int16_t)((uint16_t)s->losses+(uint16_t)p[j].losses);
    s->points=(int32_t)((uint32_t)s->points+(uint32_t)p[j].points);
   }
  }
  i=end;
 }
 return n;
}
static int32_t divide(int32_t a,int32_t b){return !b||(a==INT32_MIN&&b==-1)?0:a/b;}
int sniper87_player_score_compare(const SniperPlayerScore *a,const SniperPlayerScore *b,int c){
 if(!a||!b)return 0;
 switch(c){case 0:return ci(a->name,b->name);case 1:{int n=ci(a->squad,b->squad);return !a->squad[0]||!b->squad[0]?-n:n;}
 case 2:return b->wins-a->wins;case 3:return b->losses-a->losses;
 case 4:return sub32(rating(b->wins,b->losses,b->points),rating(a->wins,a->losses,a->points));
 case 5:return sub32(b->points,a->points);case 6:return sub32(ratio(b->wins,b->points),ratio(a->wins,a->points));default:return 0;}
}
int sniper87_squad_score_compare(const SniperSquadScore *a,const SniperSquadScore *b,int c){
 if(!a||!b)return 0;
 switch(c){case 0:return ci(a->name,b->name);case 1:return sub32(b->members,a->members);
 case 2:return b->wins-a->wins;case 3:return b->losses-a->losses;
 case 4:return sub32(rating(b->wins,b->losses,b->points),rating(a->wins,a->losses,a->points));
 case 5:return sub32(b->points,a->points);
 case 6:return sub32(divide(b->wins,b->members),divide(a->wins,a->members));
 case 7:return sub32(divide(b->losses,b->members),divide(a->losses,a->members));
 case 8:return sub32(divide(b->points,b->members),divide(a->points,a->members));
 case 9:return sub32(ratio(b->wins,b->points),ratio(a->wins,a->points));default:return 0;}
}
static void num(char *s,int32_t n){snprintf(s,64,"%d",(int)n);}
static void average(char *s,int w,int32_t p){if(w>0)snprintf(s,64,"%.1f",(double)p/w);else strcpy(s,"0.0");}
int sniper87_player_score_cells(const SniperPlayerScore *s,char c[10][64]){
 if(!s||s->wins==-100)return 0;
 memset(c,0,10*64);
 snprintf(c[0],64,"%.32s",s->name);snprintf(c[1],64,"%.32s",s->squad);
 num(c[2],s->wins);num(c[3],s->losses);num(c[4],rating(s->wins,s->losses,s->points));num(c[5],s->points);average(c[6],s->wins,s->points);return 1;
}
int sniper87_squad_score_cells(const SniperSquadScore *s,char c[10][64]){
 if(!s||s->wins==-100||!s->members||(s->points==INT32_MIN&&s->members==-1))return 0;
 memset(c,0,10*64);
 snprintf(c[0],64,"%.32s",s->name);num(c[1],s->members);num(c[2],s->wins);num(c[3],s->losses);
 num(c[4],rating(s->wins,s->losses,s->points));num(c[5],s->points);
 num(c[6],divide(s->wins,s->members));num(c[7],divide(s->losses,s->members));num(c[8],divide(s->points,s->members));average(c[9],s->wins,s->points);return 1;
}
void sniper87_score_widths(int squad,int w,int scroll,uint16_t c[10]){
 int n=w-scroll-6;memset(c,0,10*sizeof(*c));
 if(squad){c[0]=(uint16_t)(n/5);c[1]=(uint16_t)(n/14);c[2]=(uint16_t)(n/12);for(int i=3;i<9;++i)c[i]=(uint16_t)(n/11);c[9]=(uint16_t)(n-n/5-n/14-n/12-6*(n/11));}
 else{c[0]=c[1]=(uint16_t)(n/4);for(int i=2;i<6;++i)c[i]=(uint16_t)(n/11);c[6]=(uint16_t)(n-2*(n/4)-4*(n/11));}
}
#ifdef _WIN32
#include <commctrl.h>
typedef struct {int squad,count;const void *rows;} ScoreContext;
static int CALLBACK player_sort(LPARAM a,LPARAM b,LPARAM c){return sniper87_player_score_compare((const void*)a,(const void*)b,(int)c);}
static int CALLBACK squad_sort(LPARAM a,LPARAM b,LPARAM c){return sniper87_squad_score_compare((const void*)a,(const void*)b,(int)c);}
static INT_PTR CALLBACK score_proc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 ScoreContext *s=(ScoreContext*)GetWindowLongPtrA(h,DWLP_USER);
 if(msg==WM_INITDIALOG){s=(ScoreContext*)lp;SetWindowLongPtrA(h,DWLP_USER,lp);}
 if(!s)return FALSE;
 const char *section=s->squad?"SquadScores":"Scores";
 HWND list=GetDlgItem(h,1010);int cols=s->squad?10:7;
 switch(msg){
 case WM_INITDIALOG:{
  static const char *player[]={"Player Name","Squad","Win","Lose","Rating","Points","Pnt/Win"};
  static const char *squad[]={"Squad Name","#","Win","Lose","Rating","Points","Win/#","Lose/#","Pnts/#","Pnt/Win"};
  for(int i=0;i<cols;++i){LVCOLUMNA col;memset(&col,0,sizeof(col));col.mask=15;col.pszText=(char*)(s->squad?squad[i]:player[i]);
   col.cx=s->squad?(i==0?70:i<6?40:50):(i<2?100:i<6?50:60);col.iSubItem=s->squad?0:(i<2?0:i==2?1:2);SendMessageA(list,LVM_INSERTCOLUMNA,i,(LPARAM)&col);}
  SendMessageA(list,LVM_SETITEMCOUNT,s->count,0);
  for(int i=0;i<s->count;++i){char cells[10][64];const void *row=s->squad?(const void*)((const SniperSquadScore*)s->rows+i):(const void*)((const SniperPlayerScore*)s->rows+i);
   int valid=s->squad?sniper87_squad_score_cells(row,cells):sniper87_player_score_cells(row,cells);if(!valid)continue;
   LVITEMA item;memset(&item,0,sizeof(item));item.mask=LVIF_TEXT|LVIF_PARAM;item.iItem=i;item.pszText=cells[0];item.lParam=(LPARAM)row;
   int index=(int)SendMessageA(list,LVM_INSERTITEMA,0,(LPARAM)&item);
   for(int j=1;j<cols;++j){item.iSubItem=j;item.pszText=cells[j];SendMessageA(list,LVM_SETITEMTEXTA,index,(LPARAM)&item);}
  }
  int x=(int)GetPrivateProfileIntA(section,"X",-1,"sniper.ini"),y=(int)GetPrivateProfileIntA(section,"Y",-1,"sniper.ini");
  int w=(int)GetPrivateProfileIntA(section,"Width",150,"sniper.ini"),height=(int)GetPrivateProfileIntA(section,"Height",100,"sniper.ini");
  if(x!=-1)MoveWindow(h,x,y,w,height,FALSE);
  return TRUE;
 }
 case WM_SIZE:{uint16_t widths[10];MoveWindow(list,0,0,LOWORD(lp),HIWORD(lp),TRUE);sniper87_score_widths(s->squad,LOWORD(lp),GetSystemMetrics(SM_CXVSCROLL),widths);for(int i=0;i<cols;++i)SendMessageA(list,LVM_SETCOLUMNWIDTH,i,widths[i]);return TRUE;}
 case WM_NOTIFY:{NM_LISTVIEW *n=(NM_LISTVIEW*)lp;if(n&&n->hdr.code==LVN_COLUMNCLICK)SendMessageA(n->hdr.hwndFrom,LVM_SORTITEMS,n->iSubItem,(LPARAM)(s->squad?squad_sort:player_sort));return TRUE;}
 case WM_COMMAND:if(LOWORD(wp)==1||LOWORD(wp)==2)PostMessageA(h,WM_CLOSE,0,0);return TRUE;
 case WM_CLOSE:{RECT r;char b[64];GetWindowRect(h,&r);const char *keys[]={"X","Y","Width","Height"};int vals[]={r.left,r.top,r.right-r.left,r.bottom-r.top};for(int i=0;i<4;++i){num(b,vals[i]);WritePrivateProfileStringA(section,keys[i],b,"sniper.ini");}EndDialog(h,wp);return TRUE;}
 default:return FALSE;}
}
static INT_PTR scores(HWND h,HINSTANCE inst,const void *rows,int count,int squad){
 if(count<0||(count&&!rows)){SetLastError(ERROR_INVALID_PARAMETER);return -1;}
 INITCOMMONCONTROLSEX cc={sizeof(cc),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&cc);
 ScoreContext s={squad,count,rows};return DialogBoxParamA(inst,MAKEINTRESOURCEA(squad?175:144),h,score_proc,(LPARAM)&s);
}
INT_PTR sniper87_player_scores_dialog(HWND h,HINSTANCE i,const SniperPlayerScore *r,int n){return scores(h,i,r,n,0);}
INT_PTR sniper87_squad_scores_dialog(HWND h,HINSTANCE i,const SniperSquadScore *r,int n){return scores(h,i,r,n,1);}
#endif
