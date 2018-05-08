#include "Log.hpp"

#include <iostream>
#include <cstdarg>

namespace Log {

const char* LevelTable[] = {
	"Trace",    ///< Extremely detailed and repetitive debugging information that is likely to
				///  pollute logs.
	"Debug",    ///< Less detailed debugging information.
	"Info",     ///< Status information from important points during execution.
	"Warning",  ///< Minor or potential problems found during execution of a task.
	"Error",    ///< Major problems found during execution of a task that prevent it from being
			 	///  completed.
	"Critical", ///< Major problems during execution that threathen the stability of the entire
				///  application.

	NULL ///< Total number of logging levels
};

const char* ClassTable[] = {
	"Arm",
	"ArmDispatcher",
	"ArmGdbStub",
	"Loader",
	"Kernel",
	"HLE",
	NULL
};

void LogMessage(Class log_class, Level log_level, const char* filename, unsigned int line_nr,
                const char* function, const char* format, ...) {
    char buffer[4096];

    va_list args;
    va_start(args, format);
	vsnprintf (buffer, sizeof(buffer), format, args); 
    va_end(args);

	std::cout << "[" << LevelTable[(int)log_level] << "] " << ClassTable[(int)log_class] << ": " << buffer << std::endl;
	std::cout << "    File: " << filename << std::endl;
	std::cout << "    Line: " << line_nr << std::endl;
	if(log_level == Level::Critical)
		abort();
}

}