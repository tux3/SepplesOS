#include <lib/sepples.h>
#include <lib/string.h>

#define cmdis(str) !strcmp(inbuf,str)

int main()
{
	print("Sepples shell\n");
	
	for(;;)
	{
		char inbuf[512];
		
		print(">");
		nget(inbuf, 512);
		
		if (cmdis("exit\n"))
			return 0;
		else if (cmdis("help\n"))
		{
			print("Enter an absolute path to a binary, or one of the following commands:\n");
			print("exit: Exit the shell\n");
			print("help: Show this help\n");
		}
		else
		{
			int len = strlen(inbuf);
			if (len<=1)
				continue;
			if (inbuf[len-1] == '\n')
				inbuf[len-1]='\0';
			int pid = run(inbuf);
			if (!pid)
			{
				print("File or command not found: ");
				print(inbuf);
				print("\n");
			}
			else
			{
				/// TODO: Wait for the process to die
			}
		}
	}
	
	return 0;
}
