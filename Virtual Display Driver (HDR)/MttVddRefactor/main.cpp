#include <iostream>
#include <format>

int main()
{
	bool debug = false;
	bool logs = true;
	std::cout << std::format("DEBUG={:s} LOG={:s}", debug, logs);

	return 0;
}