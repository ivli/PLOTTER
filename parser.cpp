#include "plotter.h"


enum ECommand { EStart, EStop, ESet, ECreate, EAttach, EInvalid };
enum EFlags { ECfg = 0x01, ESim = 0x10 };

static const struct { char* TEXT; ECommand CMD; int FLAGS; } COMMANDS[] = {
	{ "start",   EStart,   ECfg },
	{ "stop",    EStop,    ESim },
	{ "set",     ESet,     ECfg | ESim },
	{ "create",  ECreate,  ECfg },
	{ "attach",  EAttach,  ECfg },
	{ "",        EInvalid, ECfg | ESim } };

inline std::string trim(const std::string &s);

inline bool isValidName(const std::string &s);
inline bool isValidDecimal(const std::string &s);

bool Parser::internal_parse_line(std::string &aLine, Simulation& aSim, bool aCfg) {
		std::string line = trim(aLine);
		int i = 0;
		
		for (; i < _countof(COMMANDS); ++i) {
			if (std::string::npos != line.find(COMMANDS[i].TEXT)) {
				break;
			}
		}

		/**/
		if (COMMANDS[i].FLAGS & ESim && !aCfg || COMMANDS[i].FLAGS & ECfg && aCfg) {
		
		}
		else 
			return false;
         //cut command away
		line = trim(line.substr(strlen(COMMANDS[i].TEXT), std::string::npos));

		switch (COMMANDS[i].CMD) {
			case EStart: aSim.start(); break;
			case EStop: aSim.stop(); return false; break;
			case ESet: 
				if (std::string::npos != line.find("motor")) {
					//TODO:
				}
				else if (std::string::npos != line.find("pen")) {
					line = trim(line.substr(_countof("pen"), std::string::npos));
					//TODO:
					std::size_t p1 = line.find_last_of(' ');
					
					if (std::string::npos != p1) {
						std::string name = line.substr(0, p1);
						std::string s1 = line.substr(p1, std::string::npos);
						
						if (std::string::npos != s1.find("on"))
							aSim.toggle(name, Pen::EToggle::ON);
						else if (std::string::npos != s1.find("off"))
							aSim.toggle(name, Pen::EToggle::OFF);
						else
							return false;
						return true;
					}

				}
				else if (std::string::npos != line.find("sim")) {
					//TODO:
				}
				else if (std::string::npos != line.find("log")) {
					//TODO:
				}
				else return false;
				break;
			case ECreate: 			
				if (std::string::npos != line.find("motor")) {
					aSim.createMotor(trim(line.substr(_countof("motor"), std::string::npos)));
				}
				else if (std::string::npos != line.find("pen")) {				
					aSim.createPen(trim(line.substr(_countof("pen"), std::string::npos)));
				}
				else 
					return false;
				break;

			case EAttach: {
				size_t p1 = line.find("with ");
				size_t p2 = line.find("of ");
				if (std::string::npos != p1 && std::string::npos != p1) {
					std::string motor = trim(line.substr(0, p1));
					std::string axis = trim(line.substr(motor.size() + strlen("with "), p2 - (motor.size() + strlen("with "))));
					std::string pen = trim(line.substr(p2 + 2, std::string::npos));
					aSim.attach(pen, motor, std::string::npos != axis.find("x") ? EAxis::X : Y);
				}

			} break;

			default: return false; break;
			}
		return true;
	}

bool Parser::parse_file(std::istream & aIs, Simulation& aSim) {
		std::string line;
		int i = 0;

		while (getline(aIs, line)) {
			try {
				if (false == internal_parse_line(line, aSim))
					return false;
			}
			catch (std::out_of_range & ex) {
				std::cout << "error: " << ex.what() << std::endl;
				return false;
			}
		}
		return true;
	}

bool Parser::parse_line(std::string & aCmd, Simulation& aSim) {
	try {
		return internal_parse_line(aCmd, aSim, false);
	}
	catch (std::out_of_range & ex) {
		std::cout << "error: " << ex.what() << std::endl;
	}
	return true; //suppress an error
}

inline std::string trim(const std::string &s) {
	auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) {return std::isspace(c); });
	auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) {return std::isspace(c); }).base();
	return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}
	
inline bool isValidName(const std::string &s) {
	return true; //TODO: implement 
}

inline bool isValidDecimal(const std::string &s) {
	return true; //TODO: implement 
}
