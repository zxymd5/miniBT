#include "MyBitTorrent.h"
#include "TorrentTask.h"

#include <signal.h>

CTorrentTask clTask;

void SigHandler(int nSigno)
{
	clTask.Shutdown();
	exit(0);
}

void SigSetup()
{
	signal(SIGPIPE, SIG_IGN);

	struct sigaction stAction;
	stAction.sa_handler = SigHandler;
	sigemptyset(&stAction.sa_mask);
	stAction.sa_flags = 0;

	sigaction(SIGINT, &stAction, NULL);
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("Please specify the torrent file and the file save path!\n");
		printf("For example: ./miniBT /a/b/c/a.torrent /a/b/c\n");
		return 0;
	}

	SigSetup();

	clTask.LoadTorrentFile(argv[1]);
	clTask.SetDstPath(argv[2]);

	clTask.Startup();
	long long llCurrTick = GetTickCount();
	while (!clTask.GetTaskStorage()->Finished())
	{
		if (GetTickCount() - llCurrTick >= 1000)
		{
			//打印下载速度和完成情况
			char szBuff[300];
			sprintf(szBuff,
					"Download speed: %.2fKB/S, Upload speed: %.2fKB/S,  Download: %.2fMB, Upload: %.2fMB\n",
					clTask.GetDownloadSpeed() / 1024.00, clTask.GetUploadSpeed() / 1024.00,
					clTask.GetDownloadCount() / 1024.00 / 1024.00, clTask.GetUploadCount() / 1024.00 / 1024.00);
			printf(szBuff);
		}
	}

    printf("Download finished\n");
	clTask.Shutdown();
	return 0;
}
