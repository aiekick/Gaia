// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
MIT License

Copyright (c) 2010-2020 Stephane Cuillerdier (aka Aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "VulkanLogger.h"

#define TRACE_MEMORY
#include <Helper/Profiler.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// singleton
ofstream *VulkanLogger::debugLogFile = new ofstream("vulkandebug.log", ios::out);
//wofstream *VulkanLogger::wdebugLogFile = new wofstream("wdebug.log", ios::out);
std::mutex VulkanLogger::VulkanLogger_Mutex;

VulkanLogger::VulkanLogger(void) 
{
	ZoneScoped;

	std::unique_lock<std::mutex> lck(VulkanLogger::VulkanLogger_Mutex, std::defer_lock);
	lck.lock();
	lastTick = ct::GetTicks();
	ConsoleVerbose = false;
	lck.unlock();
}

VulkanLogger::~VulkanLogger(void) 
{
	ZoneScoped;

	Close();
}

void VulkanLogger::LogString(const char* fmt, ...)
{
	ZoneScoped;

	std::unique_lock<std::mutex> lck(VulkanLogger::VulkanLogger_Mutex, std::defer_lock);
	lck.lock();
	va_list args;
	va_start(args, fmt);
	char TempBuffer[3072 + 1];//3072 = 1024 * 3
	int w = vsnprintf(TempBuffer, 3072, fmt, args);
	if (w)
	{
		int64 ticks = ct::GetTicks();
		float time = (ticks - lastTick) / 100.0f;

		char TempBufferBis[3072 + 1];
		w = snprintf(TempBufferBis, 3072, "[%.3fs]%s", time, TempBuffer);
		if (w)
		{
			std::string msg = std::string(TempBufferBis, w);
			m_ConsoleMap["App"][""][""].push_back(msg);

			TracyMessageL(msg.c_str());

			printf("%s\n", msg.c_str());
			if (!debugLogFile->bad())
				*debugLogFile << msg << std::endl;
		}
	}
	va_end(args);
	lck.unlock();
}

void VulkanLogger::LogStringWithFunction_Debug(std::string vFunction, int vLine, const char* fmt, ...)
{
#ifdef _DEBUG
	ZoneScoped;

	std::unique_lock<std::mutex> lck(VulkanLogger::VulkanLogger_Mutex, std::defer_lock);
	lck.lock();
	va_list args;
	va_start(args, fmt);
	char TempBuffer[1024 * 3 + 1];
	int w = vsnprintf(TempBuffer, 1024 * 3, fmt, args);
	if (w)
	{
		int64 ticks = ct::GetTicks();
		float time = (ticks - lastTick) / 100.0f;

		char TempBufferBis[1024 * 3 + 1];
		w = snprintf(TempBufferBis, 1024 * 3, "[%.3fs][%s:%i] => %s", time, vFunction.c_str(), vLine, TempBuffer);
		if (w)
		{
			std::string msg = std::string(TempBufferBis, w);
			m_ConsoleMap["App"][""][""].push_back(msg);

			TracyMessageL(msg.c_str());

			printf("%s\n", msg.c_str());
			if (!debugLogFile->bad())
				*debugLogFile << msg << std::endl;
		}
	}
	va_end(args);
	lck.unlock();
#else
	UNUSED(vFunction);
	UNUSED(vLine);
	UNUSED(fmt);
#endif
}

void VulkanLogger::LogStringWithFunction(std::string vFunction, int vLine, const char* fmt, ...)
{
	ZoneScoped;

	std::unique_lock<std::mutex> lck(VulkanLogger::VulkanLogger_Mutex, std::defer_lock);
	lck.lock();
	va_list args;
	va_start(args, fmt);
	char TempBuffer[1024 * 3 + 1];
	int w = vsnprintf(TempBuffer, 1024 * 3, fmt, args);
	if (w)
	{
		int64 ticks = ct::GetTicks();
		float time = (ticks - lastTick) / 100.0f;
		
		char TempBufferBis[1024 * 3 + 1];
		w = snprintf(TempBufferBis, 1024 * 3, "[%.3fs][%s:%i] => %s", time, vFunction.c_str(), vLine, TempBuffer);
		if (w)
		{
			std::string msg = std::string(TempBufferBis, w);
			m_ConsoleMap["App"][""][""].push_back(msg);

			TracyMessageL(msg.c_str());

			printf("%s\n", msg.c_str());
			if (!debugLogFile->bad())
				*debugLogFile << msg << std::endl;
		}
	}
	va_end(args);
	lck.unlock();
}

void VulkanLogger::Close()
{
	ZoneScoped;

	std::unique_lock<std::mutex> lck(VulkanLogger::VulkanLogger_Mutex, std::defer_lock);
	lck.lock();
	debugLogFile->close();
	lck.unlock();
}
