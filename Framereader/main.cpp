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

#define DEF_HELP
#define SAVE_IMAGE

const size_t imageSize = 153600;
char frameBuffer[imageSize];
const char* deviceName = "/dev/ttyUSB0";
int deviceFile = -1;
struct termios original_termios, previous_termios;
bool original_termios_defined = false;

enum class States
{
	WAIT_MAGIC1 = 0,
	
	WAIT_MAGIC2 = 1,
	
	WAIT_MAGIC3 = 2,

	WAIT_MAGIC4 = 3,
	
	WAIT_FRAME_SIZE = 4,
	
	WAIT_FRAMEBUFFER = 5
};

//inspierd by Richard Stevens "Advanced Programming in the UNIX Environment" 1st Edition, page 355
int tty_raw(int fd, int min, int time, struct termios &save_termios)
{
    struct termios buf;

    if (tcgetattr(fd, &save_termios) < 0)
        return -1;

    buf = save_termios;

    // 115200 baud
    if (cfsetispeed(&buf, B115200) < 0)
        return -1;

    if (cfsetospeed(&buf, B115200) < 0)
        return -1;

    // 8 data bits, no parity, 1 stop bit, no hardware flow control
    buf.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
    buf.c_cflag |= CS8;

    // Raw input
    buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP |
                     IXON | IXOFF);

    // Raw output
    buf.c_oflag &= ~(OPOST);

    // Raw local mode
    buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Read behavior
    buf.c_cc[VMIN]  = min;
    buf.c_cc[VTIME] = time;

    if (tcsetattr(fd, TCSANOW, &buf) < 0)
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

enum class ImageFormat
{
	RAW = 0,
	JPEG = 1
};

int SaveImageToDisk(const char* buffer, size_t size, const ImageFormat extension = ImageFormat::RAW)
{
	static int imageNumber = 0;

	std::string extensionStr;

	switch (extension)
	{
		case ImageFormat::RAW:
			extensionStr = ".raw";
			break;
		case ImageFormat::JPEG:
			extensionStr = ".jpeg";	
			break;
		default:
			std::cerr << "invalid image format specified.  Exiting..." << std::endl;
			throw std::runtime_error("invalid image format specified.  Exiting...");
			return -1;
	}

	std::string filename = "imageRGB565_" + std::to_string(imageNumber++) + extensionStr;

	int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		std::cerr << "unable to open file for writing: " << filename << std::endl;
		return -1;
	}

	ssize_t bytesWritten = write(fd, buffer, size);
	if (bytesWritten < 0)
	{
		std::cerr << "error writing to file: " << filename << std::endl;
		close(fd);
		return -1;
	}
	else if (static_cast<size_t>(bytesWritten) != size)
	{
		std::cerr << "incomplete write to file: " << filename << std::endl;
		close(fd);
		return -1;
	}

	close(fd);
	std::cout << "image saved to disk as: " << filename << std::endl;
	return 0;
}

int main(int argc, char* argv[])
{

	if (argc > 1)
	{
		if (!strcmp(argv[1], "/dev/ttyUSB0") || !strcmp(argv[1], "/dev/ttyUSB1") || !strcmp(argv[1], "/dev/ttyACM0"))
		{
			deviceName = argv[1];
		}
		else
		{
			std::cerr << "invalid device name specified.  Exiting..." << std::endl;
			return -1;	
		}
	}

#ifdef DEF_HELP
	std::cout << "DEF_HELP is defined." << std::endl;	
#endif 	

	States state = States::WAIT_MAGIC1;
	States prevState = States::WAIT_MAGIC1;
	char frameSize[4];
	size_t totalBytes = 0;
	int numberReads = 0;
	int frameSizeInt;


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

		char c;

#ifdef DEF_HELP
		std::cout << "Entering main loop..." << std::endl;
#endif

		while (true)
		{
			switch(state)
			{
				case States::WAIT_MAGIC1:


#ifdef DEF_HELP_
		std::cout << "Line number: " << __LINE__ << " - States::WAIT_MAGIC1" << std::endl;
#endif				
					
					read(deviceFile, &c, 1);

#ifdef DEF_HELP_
    std::cout << "Line number: " << __LINE__
              << " - read byte: 0x"
              << std::hex << (static_cast<unsigned int>(
                     static_cast<unsigned char>(c)))
              << std::dec << std::endl;
#endif

					if (c == 's')
					{
						prevState = state;
						state = States::WAIT_MAGIC2;
					}

					break;
				case States::WAIT_MAGIC2:
#ifdef DEF_HELP
		std::cout << "Line number: " << __LINE__ << " - States::WAIT_MAGIC2" << std::endl;
#endif						
					read(deviceFile, &c, 1);

					if (c == 't')
					{
						prevState = state;
						state = States::WAIT_MAGIC3;
					}
					else  
					{
						prevState = state;
						state = States::WAIT_MAGIC1;
					}

					break;
				case States::WAIT_MAGIC3:
#ifdef DEF_HELP
		std::cout << "Line number: " << __LINE__ << " - States::WAIT_MAGIC3" << std::endl;
#endif						
					read(deviceFile, &c, 1);

					if (c == 'a')
					{
						prevState = state;
						state = States::WAIT_MAGIC4;
					}
					else
					{
						prevState = state;
						state = States::WAIT_MAGIC1;
					}

					break;
				case States::WAIT_MAGIC4:
#ifdef DEF_HELP
		std::cout << "Line number: " << __LINE__ << " - States::WAIT_MAGIC4" << std::endl;
#endif						
					read(deviceFile, &c, 1);

					if (c == 'r')
					{
						prevState = state;
						state = States::WAIT_FRAME_SIZE;
					}
					else
					{
						prevState = state;
						state = States::WAIT_MAGIC1;
					}

					break;	
				case States::WAIT_FRAME_SIZE:

#ifdef DEF_HELP
					std::cout << "Line number: " << __LINE__ << " - States::WAIT_FRAME_SIZE" << std::endl;
#endif	
					if (read(deviceFile, frameSize, 4) != 4)
						throw std::runtime_error("unable to read frame size.  Exiting...");
				

					memcpy(&frameSizeInt, frameSize, 4);

					std::cout << "frame size: " << frameSizeInt << std::endl;

					//if (frameSizeInt != imageSize)
					//	throw std::runtime_error("frame size is not equal to expected image size.  Exiting...");
					
					prevState = state;
					state = States::WAIT_FRAMEBUFFER;

					break;
				case States::WAIT_FRAMEBUFFER:
#ifdef DEF_HELP
					std::cout << "Line number: " << __LINE__ << " - States::WAIT_FRAMEBUFFER" << std::endl;
#endif
					totalBytes = 0;
					numberReads = 0;
					while (totalBytes < size_t(frameSizeInt))
					{
						ssize_t bytesRead = read(deviceFile, frameBuffer + totalBytes, imageSize - totalBytes);
						if (bytesRead < 0)
							throw std::runtime_error("error reading frame buffer.  Exiting...");
						
#ifdef DEF_HELP_
					std::cout << "Line number: " << __LINE__ << " framebuffer bytes read: " << bytesRead << '\n';
#endif
						numberReads++;	
						totalBytes += bytesRead;
					}

#ifdef DEF_HELP
					std::cout << "Line number: " << __LINE__ << " framebuffer bytes read: " << totalBytes << std::endl;
					std::cout << "Line number: " << __LINE__ << " number of reads: " << numberReads << std::endl;
#endif

					std::cout << "frame buffer read successfully." << std::endl;

#ifdef SAVE_IMAGE
					if (SaveImageToDisk(frameBuffer, imageSize, ImageFormat::RAW) < 0)
						throw std::runtime_error("unable to save image to disk.  Exiting...");
#endif 					

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
