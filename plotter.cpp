#include "plotter.h"
#include <thread>
#include <mutex>
#include <stdio.h>

std::ostream & operator << (std::ostream & os, const Motor& aMotor) {
	os << aMotor.iS << "; " << aMotor.iA << "; " << aMotor.iP << "; " << aMotor.iTP << "; " << aMotor.iV << std::endl;
	return os;
}

std::ostream & operator << (std::ostream & os, const Pen& aPen) {	
	nullptr != aPen.iX ? os << aPen.iX->getPosition() : os << "00"; 
	os << ";";
	nullptr != aPen.iY ? os << aPen.iY->getPosition() : os << "00";	
	return os;
}

class Logger {
public:
	static Period iTime ;
public:
	~Logger() {}
	Logger(const std::string& aName) { 
		iLog = std::shared_ptr<std::ofstream>(new std::ofstream(), [=](std::ofstream * os) {os->close();});
		iLog->open(aName + ".log"); 
	}
	std::ostream &stream() { return *iLog; };
	bool isOn() const { return iOn;}
	void setOn(bool aO) { iOn = aO; }

	void dump(const Pen& aPen) {
		if (isOn() && Pen::EToggle::ON == aPen.getToggle())
			*iLog << iTime << ";" << aPen << std::endl;
		else if (isOn() && Pen::EToggle::ON != aPen.getToggle()) {
			*iLog << iTime << "--;--" << std::endl;
			iOn = false;
		}
		else if (!isOn() && Pen::EToggle::ON == aPen.getToggle()) {
			*iLog << iTime << ";" << aPen << std::endl;
			iOn = true;
		}
		else {
		}		
	}
private:
	std::shared_ptr<std::ofstream> iLog;
	bool iOn = true;
};

Period Logger::iTime = .0;

void Pen::dump(Logger & aLog) {	
	aLog.dump(*this);
}

std::mutex  s_mtx;
std::thread s_sim;
std::thread s_logging;
bool running = true;

void Simulation::createMotor(const std::string& aName) {	
	if (iMotors.insert(std::pair<std::string, Motor>(aName, Motor())).second == false) {
		throw (std::out_of_range("a Motor with given name already exists: " + aName));
	}
}

void Simulation::createPen(const std::string& aName) {
	if (iPens.insert(std::pair<std::string, Pen>(aName, Pen())).second == false)
		throw (std::out_of_range("a Pen with given name already exists: " + aName));
	iLogs.insert(std::pair<std::string, Logger>(aName, Logger(aName)));
}

void Simulation::attach(const std::string& aPen, const std::string& aMotor, const EAxis& anAxis) {
	iPens.at(aPen).attach(iMotors.at(aMotor), anAxis);
}

void Simulation::toggle(const std::string& aPen, Pen::EToggle aToggle) {
	std::lock_guard<std::mutex> lck(s_mtx);
	iPens.at(aPen).setToggle(aToggle);
}

void Simulation::start() {
	s_sim = std::thread([=]() {
		while (running) {
			iSimulationTime += iSimulationPeriod;
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(iSimulationPeriod * 1000)));
			doSim();
		}
	});

	s_logging = std::thread([=]() {
		while (running) {
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(iLoggingPeriod * 1000)));
			doLog();			
		} 
	});
}

void Simulation::stop() {	
	running = false;
}

void Simulation::setSimulationPeriod(const Period &aP) { iSimulationPeriod = aP; }
void Simulation::setLoggingPeriod(const Period &aP) { iLoggingPeriod = aP; }

void Simulation::doSim() {
	std::lock_guard<std::mutex> lck(s_mtx);
	
	for (auto& m : iMotors) {		
		const auto& pos = m.second.getPosition();
		const auto& acc = m.second.getAcceleration();
		const auto& vel = m.second.getVelocity();
		const auto& tp  = m.second.getTPosition();

		Velocity vel2 = vel + acc * iSimulationPeriod;
		Position pos2 = pos + vel2 + acc + iSimulationPeriod*iSimulationPeriod / 2; //TODO: can be cached 
		m.second.setVelocity(vel2);
		m.second.setPosition(pos2);
	}
}

void Simulation::doLog() {
	std::lock_guard<std::mutex> lck(s_mtx);
	
	Logger::iTime = iSimulationTime;

	for each (auto& p in iPens) {
		auto& l = iLogs.at(p.first);
		l.dump(p.second);
	}
}

void Simulation::dump(std::ostream& os) {
	os << "-----MOTORS-------" << std::endl;
	for each (auto& m in iMotors)
		os << m.first << ": " << m.second << std::endl;

	os << "-----PENS-------" << std::endl;
	for each (auto& p in iPens)
		os << p.first << ": " << p.second << std::endl;
}

int show_usage(int aCode) {	
	std::cout << "usage: \n\tplotter.exe  cfg_file\n";
	return aCode;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
		return show_usage(-1);

	Simulation sim;

	std::fstream file;
	
	file.open(argv[1]);

	if (file.is_open() && Parser::parse_file(file, sim)) {
	
		sim.dump(std::cout);

		file.close(); 
		sim.start();

		bool nem = true;
		do {
			std::string line;
			std::getline(std::cin, line);
			nem = Parser::parse_line(line, sim);
		} while (nem);
		sim.stop();
	}

	s_sim.join();
	s_logging.join();
    return 0;
}

