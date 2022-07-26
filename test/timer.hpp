#include <string>
#include <chrono>

struct Timer
{
	std::string name;
	std::chrono::system_clock::duration start;
	Timer(const std::string& name)
	{
		this->name = name;
		start = std::chrono::system_clock::now().time_since_epoch();
	}

	~Timer()
	{
		auto t = std::chrono::system_clock::now().time_since_epoch() - start;
		std::cout << name << std::chrono::duration_cast<std::chrono::milliseconds>(t).count() << "ms" << std::endl;
	}
};
