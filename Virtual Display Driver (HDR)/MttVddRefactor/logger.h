#pragma once
#include <chrono>
#include <string>
#include <windows.h>

namespace Refactoring
{

enum class LogType : __int8
{
	None		= 0,
	Error		= 1,
	Info		= 2,
	Pipe		= 3,
	Debug		= 4,
	Warning		= 5,
	Testing		= 6,
	Companion	= 7,
};

class Logger
{
  public:
	Logger(std::string base_dir, bool enable_std_logs, bool enable_debug_logs, bool send_logs_through_pipe);
	~Logger();

	void SendToPipe(const std::string &logMessage);

	void Init(HANDLE *ext_pipe);

	void Message(LogType type, std::string msg);

	void ToggleStandardLogs(bool enable);
	void ToggleDebugLogs(bool enable);
	void TogglePipedLogs(bool enable);

  protected:
  private:
	void OpenLogFile();
	void CloseLogFile();
	void ChangeDate();

	bool HasDateChanged();
	std::chrono::year_month_day GetDate();

	std::chrono::year_month_day today;
	const std::chrono::time_zone * m_tz;
	std::string base_logpath;

	bool standard_logs;
	bool debug_logs;
	bool piped_logs;

	FILE *m_log_file;
	HANDLE *m_pipe_handle;
};
} // namespace Refactoring
