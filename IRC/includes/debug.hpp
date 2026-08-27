#ifndef DEBUG_HPP
# define DEBUG_HPP

#define RESET "\033[0m"
#define WHITE "\033[37m"
#define YELLOW "\033[33m"
# include <iostream>
# include <string>
# include <sstream>

#ifndef DEBUG
# define DEBUG 0
#endif

#define DBG(msg) if (DEBUG) std::cerr << YELLOW << "[DEBUG]" << WHITE << msg << RESET << std::endl

#endif
