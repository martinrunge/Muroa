/*
 * CApp.h
 *
 *  Created on: 23 Oct 2011
 *      Author: martin
 */

#ifndef CAPP_H_
#define CAPP_H_

#include <mutex>
#include <log4cplus/logger.h>

#include "Exceptions.h"
#include "CSettings.h"

namespace muroa {

class CApp {
private:
	CApp(int argc, char** argv) throw(configEx);

public:
	static CApp* getInstPtr(int argc = 0, char** argv = NULL) throw(configEx);
	virtual ~CApp();

	static CSettings& settings();
	static log4cplus::Logger& logger();

	CSettings& getSettingsRef();
	log4cplus::Logger& getLoggerRef();

	void serviceChanged();
	int daemonize();

private:
	static CApp* m_inst_ptr;
	static std::mutex m_mutex;

	CSettings m_settings;
	log4cplus::Logger m_logger;

	void initLog();

};

} /* namespace muroa */
#endif /* CAPP_H_ */
