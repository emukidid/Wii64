package com.wii64.texpacker.util;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class Log {
	private static Logger	logger	= null;

	public static void info(String s) {
		if (logger == null) {
			logger = LogManager.getLogger();
		}
		logger.info(s);
	}
}
