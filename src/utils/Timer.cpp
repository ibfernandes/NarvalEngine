#include "utils/Timer.h"


namespace narvalengine {
	Timer::Timer() {
	}

	Timer::~Timer() {
	}

	void Timer::startTimer() {
		begin = std::chrono::steady_clock::now();
	}

	void Timer::endTimer() {
		end = std::chrono::steady_clock::now();
	}

	float Timer::getElapsedSeconds() {
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - begin).count();
	}

	float Timer::getSeconds() {
		return std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	}

	float Timer::getMicroseconds() {
		return std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;
	}

	void Timer::printlnNanoSeconds() {
		std::cout << "Elapsed time (sec): " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0 << std::endl;
	}

	void Timer::printlnSeconds() {
		std::cout << "Elapsed time (sec): " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << std::endl;
	}

	void Timer::printlnMinutes() {
		std::cout << "Elapsed time (min): " << std::chrono::duration_cast<std::chrono::minutes>(end - begin).count() << std::endl;
	}
}