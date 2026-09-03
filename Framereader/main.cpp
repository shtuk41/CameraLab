#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

const size_t imageSize = 153600;
const size_t dataframeSize = imageSize + 8;
const char* deviceName = "/dev/ttyUSB0";

enum class States
{
	WAIT_MAGIC1 = 0,
	
	WAIT_MAGIC2 = 1,
	
	WAIT_MAGIC3 = 2,
	
	WAIT_FRAME_SIZE = 3,
	
	WAIT_FRAMEBUFFER = 4
};

//Richard Stevens "Advanced Programming in the UNIX Environment" 1st Edition, page 355
int tty_raw(int fd, int min, int time, struct termios &save_termios)
{
	struct termios buf;
	
	if (tcgetattr(fd, &save_termios) < 0)
		return -1;
	
	buf = save_termios;
	
	//echo off, canonical mode off, extended input processing off, signal chars off
	buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	//no SIGINT on BREAK, CR-to-NL off, input parity check off, don't strip 8th bit on input, output flow control off
	buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	
	//clear size bits, parity checking off
	buf.c_cflag &= ~(CSIZE | PARENB);

	//set 8 bits/char
	buf.c_cflag |= CS8;
	
	//output processing off
	buf.c_oflag &= ~(OPOST);
	
	buf.c_cc[VMIN] = min;
	buf.c_cc[VTIME] = time;
	
	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
		return -1;
	
	return 0;
}

int main()
{
	States states = States::WAIT_MAGIC1;
	struct termios original_termios;

	try
	{
		int deviceFile;
	 
		if((deviceFile = open(deviceName, O_RDONLY, O_NOCTTY)) < 0)
			throw std::runtime_error("unable to open device file. Exiting...");
		
		int isAtty  = isatty(deviceFile);
		
		if (isAtty == 0)
		{
			std::string error_message = "the device " + std::string(deviceName) + " is not a terminal.  Exiting...";
			throw std::runtime_error(error_message.c_str());
		}

		if (tty_raw(deviceFile, 1, 0, original_termios) < 0)
			throw std::runtime_error("unable to set terminal to raw mode.  Exiting...");

		while (true)
			switch(states)
			{
				case States::WAIT_MAGIC1:

					states == States::WAIT_MAGIC2;
					break;
				case States::WAIT_MAGIC2:
					
					states = States::WAIT_MAGIC3;
					break;
				case States::WAIT_MAGIC3:
					
					states = States::WAIT_FRAME_SIZE;
					break;
				case States::WAIT_FRAME_SIZE:
					
					states = States::WAIT_FRAMEBUFFER;
					break;
				case States::WAIT_FRAMEBUFFER:
					
					states = States::WAIT_MAGIC1;
					break;
				default:
					throw std::runtime_error("invalid state.  Exiting...");
			}
		}



	}
	catch (std::exception &e)
	{
		std::cout << "exception: " << e .what() << std::endl;
	}
}

