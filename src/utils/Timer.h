#pragma once
#include <chrono>
#include <iostream>

namespace narvalengine {
	class Timer {
	public:
		std::chrono::steady_clock::time_point begin, end;
		Timer();
		~Timer();

		void startTimer();
		void endTimer();
		float getMicroseconds();
		void printlnNanoSeconds();
		void printlnSeconds();
		void printlnMinutes();
	};
}