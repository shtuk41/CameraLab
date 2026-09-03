#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

const size_t imageSize = 153600;
const size_t dataframeSize = imageSize + 8;
const char* deviceName = "/dev/ttyUSB0";
int deviceFile = -1;
struct termios original_termios, previous_termios;
bool original_termios_defined = false;

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

int tty_reset(int fd, struct termios &save_termios)
{
	if (!original_termios_defined)
		return 0;

	if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
		return -1;
	
	return 0;
}

void tty_atexit(void)
{
	if (original_termios_defined)
		tty_reset(deviceFile, original_termios);
}

static void sig_catch(int signo)
{
	std::cout << "caught signal: " << signo << std::endl;
	
	tty_reset(deviceFile, original_termios);
	exit(0);
}

int main()
{
	States state = States::WAIT_MAGIC1;
	States prevState = States::WAIT_MAGIC1;
	char frameSize[4];
	char frameBuffer[imageSize];

	if (signal(SIGINT, sig_catch) == SIG_ERR)
	{
		std::cerr << "signal(SIGINT) error.  Exiting..." << std::endl;
		return -1;
	}
	if (signal(SIGQUIT, sig_catch) == SIG_ERR)
	{
		std::cerr << "signal(SIGQUIT) error.  Exiting..." << std::endl;
		return -1;
	}	
	if (signal(SIGTERM, sig_catch) == SIG_ERR)
	{
		std::cerr << "signal(SIGTERM) error.  Exiting..." << std::endl;
		return -1;
	}

	try
	{
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
		else
		{
			std::cout << "terminal set to raw mode." << std::endl;
			original_termios_defined = true;
		}

		while (true)
		{
			switch(state)
			{
				case States::WAIT_MAGIC1:
					
					char c;
					read(deviceFile, &c, 1);

					if (c == 'n')
					{
						prevState = state;
						state == States::WAIT_MAGIC2;
					}

					break;
				case States::WAIT_MAGIC2:
					
					char c;
					read(deviceFile, &c, 1);

					if (c == 'a')
					{
						prevState = state;
						state == States::WAIT_MAGIC3;
					}
					else if (c == 'n')
					{
						prevState = state;
						state == States::WAIT_MAGIC2;
					}
					else
					{
						prevState = state;
						state == States::WAIT_MAGIC1;
					}

					break;
				case States::WAIT_MAGIC3:
					
					char c;
					read(deviceFile, &c, 1);

					if (c == 'd')
					{
						prevState = state;
						state == States::WAIT_FRAME_SIZE;
						if (tty_raw(deviceFile, 4, 0, previous_termios) < 0)
							throw std::runtime_error("unable to set terminal to raw mode.  Exiting...");

					}
					else if (c == 'n')
					{
						prevState = state;
						state == States::WAIT_MAGIC2;
					}
					else if (c == 'a')
					{
						prevState = state;
						state == States::WAIT_MAGIC3;
					}
					else
					{
						prevState = state;
						state == States::WAIT_MAGIC1;
					}

					break;
				case States::WAIT_FRAME_SIZE:
					
					if (read(deviceFile, frameSize, 4) != 4)
						throw std::runtime_error("unable to read frame size.  Exiting...");
				
					int frameSizeInt;
					memcpy(&frameSizeInt, frameSize, 4);

					std::cout << "frame size: " << frameSizeInt << std::endl;

					if (frameSizeInt != imageSize)
						throw std::runtime_error("frame size is not equal to expected image size.  Exiting...");
					
					prevState = state;
					state == States::WAIT_FRAMEBUFFER;

					if (tty_raw(deviceFile, frameSizeInt, 0, previous_termios) < 0)
							throw std::runtime_error("unable to set terminal to raw mode.  Exiting...");	

					state = States::WAIT_FRAMEBUFFER;
					break;
				case States::WAIT_FRAMEBUFFER:
					
					if (read(deviceFile, frameBuffer, imageSize) != imageSize)
						throw std::runtime_error("unable to read frame buffer.  Exiting...");	

					std::cout << "frame buffer read successfully." << std::endl;

					state = States::WAIT_MAGIC1;
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

	atexit(tty_atexit);
	return 0;
}

