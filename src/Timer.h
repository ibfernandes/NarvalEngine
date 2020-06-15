#pragma once
#include <chrono>
#include <iostream>

class Timer
{
public:
	std::chrono::steady_clock::time_point begin, end;
	Timer();
	~Timer();

	void startTimer() {
		begin = std::chrono::steady_clock::now();
	}

	void endTimer() {
		end = std::chrono::steady_clock::now();
	}

	float getMicroseconds() {
		return std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;
	}

	void printlnNanoSeconds() {
		std::cout << "Elapsed time (sec): " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0 << std::endl;
	}

	void printlnSeconds() {
		std::cout << "Elapsed time (sec): " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()<< std::endl;
	}

	void printlnMinutes() {
		std::cout << "Elapsed time (min): " << std::chrono::duration_cast<std::chrono::minutes>(end - begin).count() << std::endl;
	}
};

