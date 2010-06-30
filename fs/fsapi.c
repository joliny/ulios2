/*	fsapi.c for ulios file system
	作者：孙亮
	功能：文件系统接口，响应应用程序的请求，执行服务
	最后修改日期：2010-04-24
*/

#include "fs.h"

extern long InitFS();
extern void InitPart();
extern void CloseFS();
extern long GetExec(PROCRES_DESC *pres, const char *path, DWORD pid, DWORD *exec);
extern long ReadPage(PROCRES_DESC *pres, void *buf, DWORD siz, DWORD seek);
extern long ProcExit(PROCRES_DESC *pres);
extern long EnumPart(PROCRES_DESC *pres, DWORD *pid);
extern long GetPart(PROCRES_DESC *pres, DWORD pid, PART_INFO *pi);
extern long creat(PROCRES_DESC *pres, const char *path, DWORD *fhi);
extern long open(PROCRES_DESC *pres, const char *path, BOOL isWrite, DWORD *fhi);
extern long close(PROCRES_DESC *pres, DWORD fhi);
extern long read(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz);
extern long write(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz);
extern long seek(PROCRES_DESC *pres, DWORD fhi, SQWORD seek, DWORD from);
extern long SetSize(PROCRES_DESC *pres, DWORD fhi, QWORD siz);
extern long OpenDir(PROCRES_DESC *pres, const char *path, DWORD *fhi);
extern long ReadDir(PROCRES_DESC *pres, DWORD fhi, FILE_INFO *fi);
extern long ChDir(PROCRES_DESC *pres, const char *path);
extern long MkDir(PROCRES_DESC *pres, const char *path);
extern long remove(PROCRES_DESC *pres, const char *path);
extern long rename(PROCRES_DESC *pres, const char *path, const char *name);
extern long GetAttr(PROCRES_DESC *pres, const char *path, FILE_INFO *fi);
extern long SetAttr(PROCRES_DESC *pres, const char *path, DWORD attr);
extern long SetTime(PROCRES_DESC *pres, const char *path, DWORD time, DWORD cma);

#define ATTR_ID	0
#define SIZE_ID	1
#define ADDR_ID	2
#define API_ID	3
#define PTID_ID	MSG_DATA_LEN

static const char *CheckPathSize(const char *path, DWORD siz)
{
	if (siz > MAX_PATH)
		return NULL;
	do
	{
		if (*path == 0)
			return path;
		path++;
	} while (--siz);
	return NULL;
}

void ApiGetExec(DWORD *argv)
{
	const char *path;
	DWORD exec[8];

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = GetExec(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[API_ID + 1], exec);
	KUnmapProcAddr((void*)path, argv);
	if (argv[0] == NO_ERROR)
	{
		exec[0] |= MSG_ATTR_USER;
		KSendMsg(*((THREAD_ID*)&argv[PTID_ID]), exec, 0);
	}
}

void ApiReadPage(DWORD *argv)
{
	void *buf;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	if (!(argv[ATTR_ID] & 1))
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	buf = (void*)argv[ADDR_ID];
	argv[0] = ReadPage(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], buf, argv[SIZE_ID], argv[API_ID + 1]);
	KUnmapProcAddr(buf, argv);
}

void ApiProcExit(DWORD *argv)
{
	PROCRES_DESC *CurPres;

	CurPres = pret[((THREAD_ID*)&argv[PTID_ID])->ProcID];
	ProcExit(CurPres);
	free(CurPres, sizeof(PROCRES_DESC));
	pret[((THREAD_ID*)&argv[PTID_ID])->ProcID] = NULL;
}

void ApiEnumPart(DWORD *argv)
{
	if ((argv[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP)
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	argv[0] = MSG_ATTR_USER;
	argv[1] = EnumPart(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], &argv[API_ID + 1]);
	argv[2] = argv[API_ID + 1];
	KSendMsg(*((THREAD_ID*)&argv[PTID_ID]), argv, 0);
}

void ApiGetPart(DWORD *argv)
{
	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	if (!(argv[ATTR_ID] & 1) || argv[SIZE_ID] < sizeof(PART_INFO) + 8)
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	argv[0] = GetPart(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1], (PART_INFO*)argv[ADDR_ID]);
	KUnmapProcAddr((void*)argv[ADDR_ID], argv);
}

void ApiCreat(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = creat(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, &argv[1]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiOpen(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = open(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[API_ID + 1], &argv[1]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiClose(DWORD *argv)
{
	if ((argv[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP)
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	argv[0] = MSG_ATTR_USER;
	argv[1] = close(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1]);
	KSendMsg(*((THREAD_ID*)&argv[PTID_ID]), argv, 0);
}

void ApiRead(DWORD *argv)
{
	void *buf;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	if (!(argv[ATTR_ID] & 1))
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	buf = (void*)argv[ADDR_ID];
	argv[0] = read(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1], buf, &argv[SIZE_ID]);
	KUnmapProcAddr(buf, argv);
}

void ApiWrite(DWORD *argv)
{
	void *buf;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	buf = (void*)argv[ADDR_ID];
	argv[0] = write(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1], buf, &argv[SIZE_ID]);
	KUnmapProcAddr(buf, argv);
}

void ApiSeek(DWORD *argv)
{
	if ((argv[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP)
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	argv[0] = MSG_ATTR_USER;
	argv[1] = seek(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1], *((SQWORD*)&argv[API_ID + 2]), argv[API_ID + 4]);
	KSendMsg(*((THREAD_ID*)&argv[PTID_ID]), argv, 0);
}

void ApiSetSize(DWORD *argv)
{
	if ((argv[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP)
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	argv[0] = MSG_ATTR_USER;
	argv[1] = SetSize(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1], *((QWORD*)&argv[API_ID + 2]));
	KSendMsg(*((THREAD_ID*)&argv[PTID_ID]), argv, 0);
}

void ApiOpenDir(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = OpenDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, &argv[1]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiReadDir(DWORD *argv)
{
	FILE_INFO *buf;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	if (!(argv[ATTR_ID] & 1) || argv[SIZE_ID] < sizeof(FILE_INFO))
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	buf = (FILE_INFO*)argv[ADDR_ID];
	argv[0] = ReadDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[API_ID + 1], buf);
	KUnmapProcAddr((void*)buf, argv);
}

void ApiChDir(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = ChDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path);
	KUnmapProcAddr((void*)path, argv);
}

void ApiMkDir(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = MkDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path);
	KUnmapProcAddr((void*)path, argv);
}

void ApiRemove(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = remove(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path);
	KUnmapProcAddr((void*)path, argv);
}

void ApiReName(DWORD *argv)
{
	const char *path, *name;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if ((name = CheckPathSize(path, argv[SIZE_ID])) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else if (name++, CheckPathSize(name, argv[SIZE_ID] - (name - path)) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = rename(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, name);
	KUnmapProcAddr((void*)path, argv);
}

void ApiGetAttr(DWORD *argv)
{
	char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	if (!(argv[ATTR_ID] & 1) || argv[SIZE_ID] < sizeof(FILE_INFO))
	{
		argv[0] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[ADDR_ID], argv);
		return;
	}
	path = (char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = GetAttr(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, (FILE_INFO*)path);
	KUnmapProcAddr(path, argv);
}

void ApiSetAttr(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = SetAttr(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[API_ID + 1]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiSetTime(DWORD *argv)
{
	const char *path;

	if ((argv[ATTR_ID] & 0xFFFF0000) != MSG_ATTR_MAP)
		return;
	path = (const char*)argv[ADDR_ID];
	if (CheckPathSize(path, argv[SIZE_ID]) == NULL)
		argv[0] = FS_ERR_ARGS_TOOLONG;
	else
		argv[0] = SetTime(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[API_ID + 1], argv[API_ID + 2]);
	KUnmapProcAddr((void*)path, argv);
}

/*系统调用表*/
void (*ApiTable[])(DWORD *argv) = {
	ApiGetExec, ApiReadPage, ApiProcExit, ApiEnumPart, ApiGetPart,
	ApiCreat, ApiOpen, ApiClose, ApiRead, ApiWrite, ApiSeek, ApiSetSize, ApiOpenDir,
	ApiReadDir, ApiChDir, ApiMkDir, ApiRemove, ApiReName, ApiGetAttr, ApiSetAttr, ApiSetTime,
};

/*高速缓冲保存线程*/
void CacheProc(DWORD interval)
{
	for (;;)
	{
		KSleep(interval);
		SaveCache();
	}
}

void ApiProc(DWORD *argv)
{
	PROCRES_DESC *CurPres;

	if ((CurPres = pret[((THREAD_ID*)&argv[PTID_ID])->ProcID]) == NULL)
	{
		if ((CurPres = (PROCRES_DESC*)malloc(sizeof(PROCRES_DESC))) == NULL)
		{
			if ((argv[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP)
			{
				argv[0] = FS_ERR_HAVENO_MEMORY;
				KUnmapProcAddr((void*)argv[ADDR_ID], argv);
			}
			else if (argv[ATTR_ID] == MSG_ATTR_USER)
			{
				argv[0] = MSG_ATTR_USER;
				argv[1] = FS_ERR_HAVENO_MEMORY;
				KSendMsg(*((THREAD_ID*)&argv[PTID_ID]), argv, 0);
			}
			free(argv, sizeof(DWORD) * (MSG_DATA_LEN + 1));
			KExitThread(NO_ERROR);
		}
		memset32(CurPres, 0, sizeof(PROCRES_DESC) / sizeof(DWORD));
		pret[((THREAD_ID*)&argv[PTID_ID])->ProcID] = CurPres;
	}
	ApiTable[argv[API_ID]](argv);
	free(argv, sizeof(DWORD) * (MSG_DATA_LEN + 1));
	KExitThread(NO_ERROR);
}

int main()
{
	THREAD_ID CachePtid;
	long res;

	if ((res = InitFS()) != NO_ERROR)
		return res;
	InitPart();
	KCreateThread((void(*)(void*))CacheProc, 0x40000, (void*)PROC_INTERVAL, &CachePtid);	/*启动后台定时缓冲保存线程*/
	for (;;)
	{
		DWORD data[MSG_DATA_LEN + 1];
		THREAD_ID ptid;

		if ((res = KRecvMsg((THREAD_ID*)&data[PTID_ID], data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (data[ATTR_ID] == MSG_ATTR_PROCEXIT)
			data[API_ID] = FS_API_PROCEXIT;
		if (((data[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP || data[ATTR_ID] == MSG_ATTR_USER || data[ATTR_ID] == MSG_ATTR_PROCEXIT) && data[API_ID] < sizeof(ApiTable) / sizeof(void*))
		{
			DWORD *buf;

			if ((buf = (DWORD*)malloc(sizeof(DWORD) * (MSG_DATA_LEN + 1))) == NULL)
			{
				if ((data[ATTR_ID] & 0xFFFF0000) == MSG_ATTR_MAP)
				{
					data[0] = FS_ERR_HAVENO_MEMORY;
					KUnmapProcAddr((void*)data[ADDR_ID], data);
				}
				else if (data[ATTR_ID] == MSG_ATTR_USER)
				{
					data[0] = MSG_ATTR_USER;
					data[1] = FS_ERR_HAVENO_MEMORY;
					KSendMsg(*((THREAD_ID*)&data[PTID_ID]), data, 0);
				}
				continue;
			}
			memcpy32(buf, data, MSG_DATA_LEN + 1);
			KCreateThread((void(*)(void*))ApiProc, 0x40000, buf, &ptid);
		}
	}
	CloseFS();
	return NO_ERROR;
}
