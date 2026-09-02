#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

const size_t imageSize = 153600;
const size_t dataframeSize = imageSize + 8;
const char* deviceName = "/dev/ttyUSB0";

class circular_buffer
{
private:
	char *buffer;
	size_t bufferSize;
	size_t pointer;
	
public:
	circular_buffer(size_t size) : bufferSize(size), pointer(0)
	{
		buffer = new char[size];
	}
	
	void push(std::vector<char> &data)
	{
		//make enough room to store numberOfBytes
	}
	
	void check()
	{
		//save data
	}
};

int main()
{
	circular_buffer circus(dataframeSize * 10);
	
	std::ifstream infile(deviceName, std::ios::binary);
	
	if (!infile.is_open())
	{
		std::cerr << "unable to open file: " << deviceName << std::endl;
		return 0;
	}
	
	while(true)
	{
		infile.seekg(0, std::ios::end);
		std::streamsize bytesAvailable = infile.tellg();
	
		infile.seekg(0, std::ios::beg);
		
		if (bytesAvailable <= 0)
		{
			circus.check();
			continue;
		}
			
		std::vector<char> buffer(bytesAvailable);
		
		if(infile.read(buffer.data(), bytesAvailable))
		{
			circus.push(buffer);
		}
	}
	
	return 0;
}

