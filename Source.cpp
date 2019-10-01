#include <windows.h>
#include <stdio.h>

#define READERSCOUNT 40
#define WRITERSCOUNT 20
#define DELAYTIME 40
#define CYCLESCOUNT 40

HANDLE ghEvent;
HANDLE ghReadersThreads[READERSCOUNT];
HANDLE ghWritersThreads[WRITERSCOUNT];
HANDLE ghMutexReader;
HANDLE ghMutexWriter;
HANDLE ghSemaphore;
int NR = 0;
int NW = 0;

void CreateEventsAndThreadsAndMutex(void);
void CloseEventsAndMutex(void);
DWORD WINAPI Writers(LPVOID lpParam);
DWORD WINAPI Readers(LPVOID lpParam);

using namespace std;

int main()
{
	DWORD dwWaitResultReaders;
	DWORD dwWaitResultWriters;
	CreateEventsAndThreadsAndMutex();



	dwWaitResultReaders = WaitForMultipleObjects(READERSCOUNT, ghReadersThreads, TRUE, INFINITE);
	dwWaitResultWriters = WaitForMultipleObjects(WRITERSCOUNT, ghWritersThreads, TRUE, INFINITE);


	switch (dwWaitResultWriters)
	{
	case WAIT_OBJECT_0:
		printf("Writers threads ended...\n");
		break;
	default:
		printf("WaitForMultipleObjects Writers failed (%d)\n", GetLastError());
		return 1;
	}

	switch (dwWaitResultReaders)
	{
	case WAIT_OBJECT_0:
		printf("All threads ended, cleaning up for application exit...\n");
		break;
	default:
		printf("WaitForMultipleObjects Readers failed (%d)\n", GetLastError());
		return 1;
	}


	CloseEventsAndMutex();
	system("pause");
	return 0;
}

void CreateEventsAndThreadsAndMutex(void)
{
	DWORD dwThreadID;

	ghMutexReader = CreateMutex(NULL, FALSE, NULL);

	if (ghMutexReader == NULL)
	{
		printf("CreateMutexReader error: %d\n", GetLastError());
		return;
	}

	ghMutexWriter = CreateMutex(NULL, FALSE, NULL);

	if (ghMutexWriter == NULL)
	{
		printf("CreateMutexWriter error: %d\n", GetLastError());
		return;
	}

	ghSemaphore = CreateSemaphore(NULL, 1, 1, NULL);

	if (ghSemaphore == NULL)
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		return;
	}


	ghEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("WriteEvent"));

	if (ghEvent == NULL)
	{
		printf("CreateEvent failed (%d)\n", GetLastError());
		return;
	}

	for (int i = 0; i < READERSCOUNT; i++)
	{
		ghReadersThreads[i] = CreateThread(NULL, 0, Readers, NULL, 0, &dwThreadID);
		if (ghReadersThreads[i] == NULL)
		{
			printf("CreateReaderThread failed (%d)\n", GetLastError());
			return;
		}
	}

	for (int j = 0; j < WRITERSCOUNT; j++)
	{
		ghWritersThreads[j] = CreateThread(NULL, 0, Writers, NULL, 0, &dwThreadID);
		if (ghWritersThreads[j] == NULL)
		{
			printf("CreateWriterThread failed (%d)\n", GetLastError());
			return;
		}
	}
}

void CloseEventsAndMutex(void)
{
	CloseHandle(ghEvent);
	CloseHandle(ghMutexReader);
	CloseHandle(ghMutexWriter);
	CloseHandle(ghSemaphore);
}

DWORD WINAPI Writers(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	DWORD dwWaitResult;
	DWORD dwWaitResult2;

	for (int i = 0; i < CYCLESCOUNT; i++)
	{

		dwWaitResult = WaitForSingleObject(ghMutexWriter, INFINITE);
		if (!NW++)	ResetEvent(ghEvent);
		ReleaseMutex(ghMutexWriter);

		dwWaitResult2 = WaitForSingleObject(ghSemaphore, INFINITE);

		switch (dwWaitResult2)
		{
		case WAIT_OBJECT_0:
			printf("Writer %d writing...\n", GetCurrentThreadId());
			break;
		default:
			printf("Wait error (%d)...\n", GetCurrentThreadId());
			return 0;
		}

		printf("Writer %d exiting\n", GetCurrentThreadId());

		ReleaseSemaphore(ghSemaphore, 1, NULL);

		dwWaitResult = WaitForSingleObject(ghMutexWriter, INFINITE);
		if (!--NW)SetEvent(ghEvent);
		ReleaseMutex(ghMutexWriter);

		Sleep(DELAYTIME);

	}

	return 1;
}

DWORD WINAPI Readers(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);
	DWORD dwWaitResult;
	DWORD dwWaitResult2;

	for (int i = 0; i < CYCLESCOUNT; i++)
	{
		dwWaitResult = WaitForSingleObject(ghEvent, INFINITE);

		dwWaitResult2 = WaitForSingleObject(ghMutexReader, INFINITE);
		if (!NR++) dwWaitResult = WaitForSingleObject(ghSemaphore, INFINITE);
		ReleaseMutex(ghMutexReader);

		switch (dwWaitResult2)
		{
		case WAIT_OBJECT_0:
			printf("Reader %d reading from buffer\n", GetCurrentThreadId());
			break;
		default:
			printf("Wait error (%d)\n", GetLastError());
			return 0;
		}

		printf("Reader %d exiting\n", GetCurrentThreadId());

		dwWaitResult2 = WaitForSingleObject(ghMutexReader, INFINITE);
		if (!(--NR)) ReleaseSemaphore(ghSemaphore, 1, NULL);
		ReleaseMutex(ghMutexReader);

		Sleep(DELAYTIME);

	}
	return 1;
}