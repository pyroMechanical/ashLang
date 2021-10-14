#pragma once
#include <chrono>
#include <iostream>

class Timer
{
public:
	Timer(std::string name)
		:begin(std::chrono::high_resolution_clock::now()), name(name)
	{}
	
	~Timer()
	{
		auto end = std::chrono::high_resolution_clock::now();
		std::cout <<  name << " took " << ((double)(std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count())) / 1000.0 << "microseconds.\n";
	}
public:
	const std::string name;
private:
	std::chrono::steady_clock::time_point begin;

};