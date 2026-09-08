#include "logger.h"

#include <windows.h>
#include <iostream>

Refactoring::Logger::Logger(std::string base_dir, bool enable_std_logs, bool enable_debug_logs, bool send_logs_through_pipe)
{
	this->base_logpath = base_dir;

	m_pipe_handle = nullptr;
	m_log_file = nullptr;
	m_tz = nullptr;

	this->ChangeDate();

	standard_logs = enable_std_logs;
	debug_logs = enable_debug_logs;
	piped_logs = send_logs_through_pipe;

}

Refactoring::Logger::~Logger()
{
	m_pipe_handle = nullptr;
	m_tz = nullptr;
	this->CloseLogFile();
}

void Refactoring::Logger::Init(HANDLE * ext_pipe)
{
	m_pipe_handle = ext_pipe;

	if (standard_logs)
	{
		this->OpenLogFile();
	}
}

/* possible cases :
 * standard_logs è falso fin dall'inizio. OpenLogFile non viene mai chiamato. siamo sicuri di questo?
 * standard_logs è vero fin dall'inizio. OpenLogFile viene chiamato.
	*/
void Refactoring::Logger::OpenLogFile()
{
	auto base_dir = base_logpath + "\\Logs";

	auto date = std::format("{:%Y-%m-%d}", today);

	auto filepath = base_dir + "\\log_" + date + ".txt";

	if (!CreateDirectory(base_dir.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		std::cout << "Directory could not be created. Error Code: " << GetLastError();
		std::cout << "Logging will be disabled from now on. ";
		standard_logs = false;
		return;
	}

	errno_t err = fopen_s(&m_log_file, filepath.c_str(), "a");

	if (err != 0)
	{
		std::cout << "Log file corrupted or occupied. Error Code: " << err;
		std::cout << "Logging will be disabled from now on. ";
		standard_logs = false;
		return;
	}
}

void Refactoring::Logger::CloseLogFile()
{
	if (m_log_file)
	{
		fclose(m_log_file);
		m_log_file = nullptr;
	}
}

void Refactoring::Logger::ToggleStandardLogs(bool enable)
{
	standard_logs = enable;

	if (standard_logs && !m_log_file)
		this->OpenLogFile();

	if (!standard_logs && m_log_file)
		this->CloseLogFile();
}

void Refactoring::Logger::ChangeDate()
{
	m_tz = std::chrono::current_zone();
	today = GetDate();
}

std::chrono::year_month_day Refactoring::Logger::GetDate()
{
	std::chrono::time_point now{std::chrono::system_clock::now()};
	auto zt = std::chrono::zoned_time{m_tz, now};
	return
		std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(zt.get_local_time())};
}

bool Refactoring::Logger::HasDateChanged()
{
	std::chrono::year_month_day new_day = this->GetDate();

	if (new_day != today)
		return true;
	return false;
}

void Refactoring::Logger::ToggleDebugLogs(bool enable)
{
	debug_logs = enable;
}

void Refactoring::Logger::TogglePipedLogs(bool enable)
{
	piped_logs = enable;
}

void Refactoring::Logger::Message(LogType type, std::string msg)
{
	if (!standard_logs)
	{
		return;
	}

	if (type == LogType::Debug && !debug_logs)
	{
		return;
	}

	if (this->HasDateChanged())
	{
		this->CloseLogFile();
		this->ChangeDate();
		this->OpenLogFile();
	}

	//construct actual message
	{
		auto now = std::chrono::system_clock::now();

		auto zt = std::chrono::zoned_time {m_tz, now};

		auto timestamp = std::format("{:%Y-%m-%d %X}", zt);

		std::stringstream ss;

		ss << timestamp;

		switch (type)
		{
		case LogType::Error: //'e'
			ss << " [ERROR] ";
			break;
		case LogType::Info: //'i'
			ss << " [INFO] ";
			break;
		case LogType::Pipe: //'p'
			ss << " [PIPE] ";
			break;
		case LogType::Debug: //'d'
			ss << " [DEBUG] ";
			break;
		case LogType::Warning: //'w'
			ss << " [WARNING] ";
			break;
		case LogType::Testing: //'t'
			ss << " [TESTING] ";
			break;
		case LogType::Companion: //'c'
			ss << " [COMPANION] ";
			break;
		default:
			ss << " [UNKNOWN] ";
			break;
		}

		ss << msg;

		fprintf(m_log_file, " %s\n", ss.str().c_str());

		if (piped_logs && m_pipe_handle && *m_pipe_handle != INVALID_HANDLE_VALUE)
		{
			this->SendToPipe(ss.str());
		}
	}
}

void Refactoring::Logger::SendToPipe(const std::string &logMessage)
{
	DWORD bytesWritten;
	DWORD logMessageSize = static_cast<DWORD>(logMessage.size());
	WriteFile(*m_pipe_handle, logMessage.c_str(), logMessageSize, &bytesWritten, NULL);
}