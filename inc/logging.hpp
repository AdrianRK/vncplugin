/*
 * =====================================================================================
 *
 *       Filename:  logging.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  14/05/2020 10:24:44
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef VNC_CLIENT_LOGGING_HEADER_
#define VNC_CLIENT_LOGGING_HEADER_

#include <iostream>
#include <fstream>

template <typename T>
void log(std::ostream &stream, const T & obj)
{
	stream << obj;
}

template <typename T, typename... agcs>
void log (std::ostream &stream, const T& obj, const agcs&... arg)
{
	stream << obj;
	log(stream, arg...);
}
#ifndef DISABLE_LOG

#ifdef LOG_TO_FILE

static std::ofstream logger("logfile.txt", std::ios::app);

#define printLog(...) log(logger,"Message::Fille [", __FILE__, "] fuction[", __FUNCTION__, "] line[", __LINE__, "] message:[", __VA_ARGS__, "]\n")
#define printError(...) log(logger,"Error::Fille [", __FILE__, "] fuction[", __FUNCTION__, "] line[", __LINE__, "] message:[", __VA_ARGS__, "]\n")
#define printWarning(...) log(logger,"Warning::Fille [", __FILE__, "] fuction[", __FUNCTION__, "] line[", __LINE__, "] message:[", __VA_ARGS__, "]\n")

#else
static std::ostream& logger = std::cout;
#define printLog(...) log(logger,"Message::Fille [", __FILE__, "] fuction[", __FUNCTION__, "] line[", __LINE__, "] message:[", __VA_ARGS__, "]\n")
#define printError(...) log(logger,"Error::Fille [", __FILE__, "] fuction[", __FUNCTION__, "] line[", __LINE__, "] message:[", __VA_ARGS__, "]\n")
#define printWarning(...) log(logger,"Warning::Fille [", __FILE__, "] fuction[", __FUNCTION__, "] line[", __LINE__, "] message:[", __VA_ARGS__, "]\n")
#endif
#else
#define printLog(...)
#define printError(...)
#define printWarning(...)
#endif

#endif // VNC_CLIENT_LOGGING_HEADER_
