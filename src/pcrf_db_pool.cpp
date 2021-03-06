#include "app_pcrf.h"
#include "app_pcrf_header.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

/* ��������� ��� �������� �������� � ���� */
struct SDBPoolInfo {
	otl_connect *m_pcoDBConn;
	volatile int m_iIsBusy;
	SDBPoolInfo *m_psoNext;
};

/* ��������� �� ��� ����������� � �� */
static SDBPoolInfo *g_psoDBPoolHead = NULL;
static SDBPoolInfo *g_psoDBPoolTail = NULL;

/* ������� ��� ����������� ������� �� ��������� ��������� ����������� � �� */
static sem_t g_tDBPoolSem;
/* ������� ��� ����������� ������ ���������� ����������� */
static pthread_mutex_t g_tMutex;
static int g_iMutexInitialized = 0;

/* ����������� ������ �� ��������� */
static const char *g_pcszDefCheckReq = "select to_char(sysdate,'ddmmyyyy') from dual";
/* ������ ���� ����������� � �� �� ��������� */
#define DB_POOL_SIZE_DEF 1
#define DB_POOL_WAIT_DEF 1

/* ������� ����������� � �� */
int pcrf_db_pool_connect (otl_connect *p_pcoDBConn);
/* ������� ���������� �� �� */
void pcrf_db_pool_logoff (otl_connect *p_pcoDBConn);
/* ������� �������� ����������������� ����������� � �� */
int pcrf_db_pool_check_conn (otl_connect *p_pcoDBConn);

int pcrf_db_pool_init (void)
{
	int iRetVal = 0;
	/* ���� � ���������������� ����� �� ����� ������ ���� �� ���������� �������� �� ��������� */
	int iPoolSize = g_psoConf->m_iDBPoolSize ? g_psoConf->m_iDBPoolSize : DB_POOL_SIZE_DEF;

	/* ������������� �������� */
	//g_ptDBPoolSem = sem_open (NULL, O_CREAT);
	//if (SEM_FAILED == g_ptDBPoolSem) {
	//	iRetVal = errno;
	//	goto fn_error;
	//}
	/* ������������� ����������� �������� */
	CHECK_POSIX_DO (sem_init (&g_tDBPoolSem, 0, iPoolSize), goto fn_error);

	/* ������������� �������� ������ ���������� ����������� */
	CHECK_POSIX_DO (pthread_mutex_init (&g_tMutex, NULL), goto fn_error);

	g_iMutexInitialized = 1;
	/* ������������� ���� �� */
	SDBPoolInfo *psoTmp;

	for (int iInd = 0; iInd < iPoolSize; ++iInd) {
		psoTmp = new SDBPoolInfo;
		if (NULL ==  g_psoDBPoolTail) {
			g_psoDBPoolTail  = psoTmp;
		}
		/* �� ������ ������ ���������, ��� �� � ������� � ���������� */
		if (NULL == psoTmp) {
			iRetVal = -1500;
			break;
		}
		/* ������������� �������� */
		memset (psoTmp, 0, sizeof (*psoTmp));
		/* ���������� ���� � ������ */
		if (NULL == g_psoDBPoolHead) {
			g_psoDBPoolHead = psoTmp;
		} else {
			psoTmp->m_psoNext = g_psoDBPoolHead;
			g_psoDBPoolHead = psoTmp;
		}
		/* ������� ������ ������ ����������� � �� */
		psoTmp->m_pcoDBConn = new otl_connect;
		/* �����, �� ������ ������, ��������� ��������� */
		if (psoTmp->m_pcoDBConn) {
			CHECK_POSIX_DO (pcrf_db_pool_connect (psoTmp->m_pcoDBConn), goto fn_error);
			/* ���� �������� ������ ����������� */
		}
	}
	/* ���� �� ��������� ��� ������ ������� �� ������� */
	goto fn_return;

	fn_error:
	pcrf_db_pool_fin ();
	iRetVal = -1600;

	fn_return:
	return iRetVal;
}

void pcrf_db_pool_fin (void)
{
	/* ����������� �������, ������� ��������� */
	sem_close (&g_tDBPoolSem);
	/* ����������� ��� ����������� � �� */
	SDBPoolInfo *psoTmp;
	psoTmp = g_psoDBPoolHead;
	while (psoTmp) {
		psoTmp = g_psoDBPoolHead->m_psoNext;
		if (g_psoDBPoolHead->m_pcoDBConn) {
			if (g_psoDBPoolHead->m_iIsBusy) {
				g_psoDBPoolHead->m_pcoDBConn->cancel ();
			}
			if (g_psoDBPoolHead->m_pcoDBConn->connected) {
				pcrf_db_pool_logoff (g_psoDBPoolHead->m_pcoDBConn);
			}
			delete g_psoDBPoolHead->m_pcoDBConn;
			g_psoDBPoolHead->m_pcoDBConn = NULL;
		}
		delete g_psoDBPoolHead;
		g_psoDBPoolHead = psoTmp;
	}
	g_psoDBPoolTail = NULL;
	/* ����������� �������, ������� ��������� */
	if (g_iMutexInitialized) {
		pthread_mutex_destroy (&g_tMutex);
		g_iMutexInitialized = 0;
	}
}

int pcrf_db_pool_get (void **p_ppcoDBConn)
{
	int iRetVal = 0;

	/* ������������� �������� */
	*p_ppcoDBConn = NULL;

	timeval soCurTime;
	timespec soWaitTime;

	/* ����������� ������� ����� */
	gettimeofday (&soCurTime, NULL);
	/* ������ ����� ���������� �������� �������� */
	soWaitTime.tv_sec = soCurTime.tv_sec + (g_psoConf->m_iDBPoolWait ? g_psoConf->m_iDBPoolWait : DB_POOL_WAIT_DEF);
	soWaitTime.tv_nsec = soCurTime.tv_usec * 1000;

	/* ���� ����� ����������� ������� ��� ������� ������� */
	CHECK_POSIX (sem_timedwait (&g_tDBPoolSem, &soWaitTime));

	/* �������� ����� ���������� ����������� */
	/* ��������� ������ � ������� ���� ��� ����������� ������ */
	/* ��� �������� ���������� �� �� ��������� �����, ����� ������ �������� �� ��������� ��������� �������� �������� */
	CHECK_POSIX (pthread_mutex_timedlock (&g_tMutex, &soWaitTime));

	SDBPoolInfo *psoTmp = g_psoDBPoolHead;
	/* ������� ���� ��� ������� � ������ ���� �� ������ �� ����� */
	while (psoTmp) {
		/* ���� ����������� ������ ���� ������ */
		if (psoTmp->m_iIsBusy) {
			psoTmp = psoTmp->m_psoNext;
		} else {
			/* � ��������� ������ ��������� ����� */
			break;
		}
	}
	/* �� ������ ������, �������� ��������� */
	if (psoTmp) {
		/* �������� ����������� ��� ������� */
		psoTmp->m_iIsBusy = 1;
		*p_ppcoDBConn = psoTmp->m_pcoDBConn;
	} else {
		iRetVal = -2222;
	}
	CHECK_POSIX (pthread_mutex_unlock (&g_tMutex));

	return iRetVal;
}

int pcrf_db_pool_rel (void *p_pcoDBConn)
{
	int iRetVal = 0;
	SDBPoolInfo *psoTmp = g_psoDBPoolHead;

	/* ������������� ���������� �� ������� ���� */
	iRetVal = pthread_mutex_lock (&g_tMutex);
	/* ���� �������� ������ �������� �������� */
	if (iRetVal) {
		return iRetVal;
	}
	/* ������� ���� ������ */
	while (psoTmp) {
		/* ���� ����� ������� ����������� */
		if (psoTmp->m_pcoDBConn == p_pcoDBConn) {
			break;
		} else {
			psoTmp = psoTmp->m_psoNext;
		}
	}
	/* �� ������ ������ ��������� ��������� */
	if (psoTmp) {
		/* ����� ����������� ��� ��������� */
		psoTmp->m_iIsBusy = 0;
		/* ����������� ������� */
		sem_post (&g_tDBPoolSem);
	} else {
		/* ������ ���� �� ������ */
		iRetVal = -2000;
	}
	/* ������� ���������� ������� ���� */
	iRetVal = pthread_mutex_unlock (&g_tMutex);
	/* ���� ������������� �������� ����������� �������� */
	if (iRetVal) {
		/* ��� �����, ��� ������ � ������ ������? */
	}

	return iRetVal;
}

int pcrf_db_pool_restore (void *p_pcoDBConn)
{
	int iFnRes;
	otl_connect *pcoDBConn = (otl_connect *) p_pcoDBConn;

	/* ��� ������ ����� ������� �������� */
	/* ����� ������ �� ��������� � �� */
	if (! pcoDBConn->connected) {
		iFnRes = pcrf_db_pool_connect (pcoDBConn);
		/* ����������� ������������� */
		if (0 == iFnRes) {
			return 1;
		} else {
			/* ����������� ������������ �� ������� */
			return -1;
		}
	}
	/* ��������� ����������� �� ����������������� */
	iFnRes = pcrf_db_pool_check_conn (pcoDBConn);
	/* ���� ����������� ���������������� */
	if (iFnRes) {
		pcrf_db_pool_logoff (pcoDBConn);
		iFnRes = pcrf_db_pool_connect (pcoDBConn);
		/* ����������� ������������� */
		if (0 == iFnRes) {
			return 1;
		} else {
			/* ����������� ������������ �� ������� */
			return -1;
		}
	} else {
		/* ���� ����������� �������������� � �� ������� �������������� */
		return 0;
	}
}

int pcrf_db_pool_connect (otl_connect *p_pcoDBConn)
{
	int iRetVal = 0;

	try {
		char mcConnString[0x1000];
		int iStrLen;

		iStrLen = snprintf (mcConnString, sizeof (mcConnString) - 1, "%s/%s@%s", g_psoConf->m_pszDBUser, g_psoConf->m_pszDBPswd, g_psoConf->m_pszDBServer);
		if (0 >= iStrLen) {
			return -10;
		}
		if (sizeof (mcConnString) - 1 <= iStrLen) {
			return -20;
		}
		mcConnString[iStrLen] = '\0';
		p_pcoDBConn->rlogon (mcConnString, 0, NULL, NULL);
	} catch (otl_exception &coExcept) {
		iRetVal = coExcept.code;
		printf ("pcrf_db_pool_connect: error: code: '%d'; description: '%s';\n", coExcept.code, coExcept.msg);
	}

	return iRetVal;
}

void pcrf_db_pool_logoff (otl_connect *p_pcoDBConn)
{
	if (p_pcoDBConn) {
		if (p_pcoDBConn->connected) {
			p_pcoDBConn->logoff ();
		}
	}
}

int pcrf_db_pool_check_conn (otl_connect *p_pcoDBConn)
{
	int iRetVal = 0;

	try {
		const char *pcszCheckReq;

		/* ���� ����������� ������ ����� � ������������ � �� �������� */
		if (g_psoConf->m_pszDBDummyReq && g_psoConf->m_pszDBDummyReq[0]) {
			pcszCheckReq = g_psoConf->m_pszDBDummyReq;
		} else {
			/* � ��������� ������ ���������� ������ �� ��������� */
			pcszCheckReq = g_pcszDefCheckReq;
		}

		otl_stream coStream (1, pcszCheckReq, *p_pcoDBConn);
		char mcResult[32];
		coStream >> mcResult;
	} catch (otl_exception &coExcept) {
		iRetVal = coExcept.code;
		printf ("pcrf_db_pool_check_conn: error: code: '%d'; description: '%s';\n", coExcept.code, coExcept.msg);
	}

	return iRetVal;
}
