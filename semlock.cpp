#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <errno.h>
#include "semlock.h"
#include "BoostLog.h"

///////////////////////////////////////////////////////////////////////////////
// ＤＥＦＩＮＥ
///////////////////////////////////////////////////////////////////////////////
//#define  DEBUG
//#define  DUMP
#define  PERMS    0666//8進数で666
#define SEMKEYF   "/etc/hosts"
//#define SEMKEY    0x01230000

////////////////////////////////////////////////////////////////////////////////
// グローバル変数
///////////////////////////////////////////////////////////////////////////////
static  struct  sembuf	op_lock[2] = {
			{0, 0, 0},//このtransitionの時はカウント値は0になる
			{0, 1, SEM_UNDO}//このtransitionが実行されると1カウントアップされてカウント値は1になり、ロック状態になる
			//SEM_UNDO:この状態の時にプロセスが死んだらリセットする
};

static  struct  sembuf	op_unlock[1] = {
			{0, -1, (IPC_NOWAIT | SEM_UNDO)}
};

int    semid[10]  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};//セマフォの値(semid)を格納する配列(セマフォ配列)
//key_tはSystem V IPC キーを表す型。数値はint
key_t  semkey[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};//セマフォの値(semid)を生成する為に必要なkeyを格納する配列


///////////////////////////////////////////////////////////////////////////////
// sem_lock() :
///////////////////////////////////////////////////////////////////////////////
int sem_lock(int lockid,					// ロックＩＤ(どの観点でセマフォを適用するかを一意に定めたもの)	
			int ipcflg)						// 作成モード	
{
	int		rcode ;							// リターンコード
//----------------------------------------------------------------------------

	// ロックＩＤの範囲チェック	
	if (  lockid < LOCK_ID_TYPE01  ||
		  lockid > LOCK_ID_MAX      ) {
#ifdef DEBUG
		//printf("invalid lockid \n");
		write_log(4, "invalid lockid \n");
#endif
		return(LOCK_RC_PARM);				// パラメタ不正
	}

//ftok - パス名とプロジェクト識別子を System V IPC キーに変換する  

#if 0
	if (semkey[lockid] < 0 )  {
		semkey[lockid] = ftok(SEMKEYF,	
							  lockid);
		//printf("%d\n", semkey[lockid]);
		write_log(4, "%d\n", semkey[lockid]);
		if ( semkey[lockid] == -1 )  {
#ifdef  DEBUG
			//perror("ftok error");
			write_log(4, "ftok error");
#endif
			return(LOCK_RC_SYSER) ;			// システムエラー
		}
	}
#else
	//SEMKEYをsetting.confから読み取り
	FILE *fp = fopen("setting.conf", "r");
	char line[256];//読み取り領域
	char* buff;//tab区切り用領域

	if(fp == NULL){
		//printf("Open setting.conf File Failed!\n");
		write_log(4, "Open setting.conf File Failed!\n");
		return -1;
	}else{
		fgets(line, 256, fp);
		buff = strtok(line, "\t");

		if ( buff == NULL )
		{
			//printf("Nothing in setting.conf File!\n");
			write_log(4, "Nothing in setting.conf File!\n");
			return -1;
		}

		//SEMKEYというフィールド名になっているかどうか
		int nRet = strcmp(buff, "SEMKEY");
		if(nRet != 0){
			//printf("Field Name isn't SEMKEY!\n");
			write_log(4, "Field Name isn't SEMKEY!\n");
			return -1;
		}
		buff = strtok(NULL, "\t");

		if ( buff == NULL )
		{
			//printf("Nothing in setting.conf File!\n");
			write_log(4, "Nothing in setting.conf File!\n");
			return -1;
		}

		/*チェック項目
		buff[0]が0かどうか
		buff[1]がxかどうか
		buff[2]以降が0~fのいずれかかどうか
		numがint Max未満かどうか
		*/
		//printf("SEMKEY:%s\n", buff);
		write_log(2, "SEMKEY:%s\n", buff);

		//hex文字列を数値に変換
		std::string s = (std::string)(buff+2);//buff[2]の位置
		//printf("s=[%s]\n", s.c_str());
		write_log(2, "s=[%s]\n", s.c_str());
		int num = std::stoi(s, 0, 16);//C++11 only
		//printf("SEMKEY = %x\n", num);
		write_log(2, "SEMKEY = %x\n", num);
		/*
		std::stringstream str;
		std::string s1 = (std::string)buff;
		str << s1;
		int SEMKEY;
		str >> std::hex >> SEMKEY;
		semkey[lockid] = SEMKEY + lockid;
		*/
	}

#endif

	if (semid[lockid] < 0 )  {
		//semget関数にkey値を渡すことでsemidが作れる
		semid[lockid] = semget(semkey[lockid],
								1,
								ipcflg | PERMS);
		if ( semid[lockid] == -1 )  {
#ifdef DEBUG
			//perror("semget") ;
			write_log(4, "semget");	
#endif
			if ( errno == EINVAL ) {		// セマフォ数がシステムの値をオーバ 
				return(LOCK_RC_OVER) ;		//  IDがとれない
			}
			else if (ipcflg == IPC_CREAT &&	// ALLOC指定でIDなし
					 errno  == ENOENT ) {
				return(LOCK_RC_NOID) ;		//  ID未作成	
			}
			else {							// その他
				return(LOCK_RC_SYSER) ;		//   システムエラー
			}
		}
	}

	//semop:セマフォ・オペレーション
	rcode = semop(semid[lockid],
				&op_lock[0],
				2);

	if ( rcode < 0 )  {
#ifdef DEBUG
		//perror("semop");
		write_log(4, "semop");
#endif
		if ( errno == ENOSPC ) {
				return(LOCK_RC_NOMEM) ;		//   メモリ不足
		}
		else {								// その他
			return(LOCK_RC_SYSER);			//   システムエラー
		}
	}

	return(0);
}

///////////////////////////////////////////////////////////////////////////////
// sem_unlock :
///////////////////////////////////////////////////////////////////////////////
int sem_unlock(int lockid)
{
	int		rcode ;							// リターンコード
//----------------------------------------------------------------------------

	// ロックＩＤの範囲チェック	
	if ( lockid < 0  ||  lockid > 9 )  {
#ifdef DEBUG
		//printf("invalid lockid \n");
		write_log(4, "invalid lockid \n");
#endif
		return(LOCK_RC_PARM);
	}


	rcode = semop(semid[lockid],
					&op_unlock[0],
					 1);
	if ( rcode < 0 )  {
#ifdef DEBUG
		//perror("semop unlock error");
		write_log(4, "semop unlock error");
#endif
		if ( errno == ENOSPC ) {
			return(LOCK_RC_NOMEM);
		}
		else {
			return(LOCK_RC_SYSER);
		}
	}

	return(0);
}

///////////////////////////////////////////////////////////////////////////////
// sem_dellock
///////////////////////////////////////////////////////////////////////////////
int sem_dellock(int lockid)
{
	int		rcode;							// リターンコード
//----------------------------------------------------------------------------

	// ロックＩＤの範囲チェック	
	if ( lockid < 0  ||  lockid > 9 )  {
#ifdef DEBUG
		//printf("invalid lockid \n");
		write_log(4, "invalid lockid \n");
#endif
		return(1);
	}

	rcode = semctl( semid[lockid],
					lockid,
					IPC_RMID,
					0);

	if ( rcode == -1 ) {
#ifdef DEBUG
		//perror("semctl");
		write_log(4, "semctl");
#endif
		return(-1);
	}

	return(0);
}

int semUnLock(int macro) {
	int nRetS = sem_unlock(macro);
	if (nRetS != 0) {
		//printf("sem_unlock error. nRet=%d\n", nRetS);
		write_log(4, "sem_unlock error. nRet=%d\n", nRetS);
		return -1;
	}
	//printf("Unlocked\n");
	write_log(2, "Unlocked\n");
	return 0;
}