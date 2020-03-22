#ifndef __SEMLOCK__
#define __SEMLOCK__

#include <sys/ipc.h>


int sem_lock(int lockid, int ipcflg);	// ロック
int sem_unlock(int lockid);				// アンロック
int sem_dellock(int lockid);			// 削除
int semUnLock(int macro);//アンロックのラッパー関数

// LOCKID(どの観点でセマフォを適用するかによってTYPEを分ける。ex:マルチ入室制限用、ECサイトチケット販売用)
#define  LOCK_ID_TYPE01		0
#define  LOCK_ID_TYPE02		1
#define  LOCK_ID_MAX		10

// Return Code
#define LOCK_RC_PARM -1
#define LOCK_RC_SYSER -2
#define LOCK_RC_OVER -3
#define LOCK_RC_NOMEM -4
#define LOCK_RC_NOID -5

#endif
